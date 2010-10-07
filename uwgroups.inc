<?php

/***************************************************************
 * UW Groups Module                                            *
 *                                                             *
 * Created by the University of Washington Information School. *
 * Be sure to read README and LICENSE.                         *
 ***************************************************************/

/**
 * @file
 * uwgroups shared module functions.
 */

define('UWGWS_ENABLE_AC', 'UWGWS_ENABLE_AC');
define('UWGWS_ENABLE_AC_DEFAULT', FALSE);
define('UWGWS_AC_GROUP', 'UWGWS_AC_GROUP');
define('UWGWS_AC_GROUP_DEFAULT', '');
define('UWGWS_PERM', 'administer UW GWS authorization');

module_load_include('inc', 'lts', 'lts.api');

/**
 * Gets or sets a UWGWS module variable.
 */
function _uwgroups_var($var, $val = NULL) {
  switch ($var) {
    case UWGWS_ENABLE_AC:
      if ($val)
        variable_set(UWGWS_ENABLE_AC, $val === TRUE);
      else
        return variable_get(UWGWS_ENABLE_AC, UWGWS_ENABLE_AC_DEFAULT);
    case UWGWS_AC_GROUP:
      if ($val)
        variable_set(UWGWS_AC_GROUP, strval($val));
      else
        return variable_get(UWGWS_AC_GROUP, UWGWS_AC_GROUP_DEFAULT);
  }
  
  return NULL;
}

/**
 * Logs an error in the Drupal log.
 */
function _uwgroups_error($msg, $var = NULL) {
  if ($var)
    watchdog('UWGWS', $msg, array('%var' => $var,), WATCHDOG_ERROR);
  else
    watchdog('UWGWS', $msg, array(), WATCHDOG_ERROR);
}

/**
 * Removes all groups that this module manages.
 *
 * @return
 */
function _uwgroups_remove_managed($keepPerms = FALSE) {
  // get the old user groups that the module was managing
  $result = db_query('SELECT name FROM {uwgroups}');
  
  while ($obj = db_fetch_object($result)) {
    // delete the role, associated permissions, and any references from users
  	$r2 = db_query("SELECT rid FROM {role} WHERE name = '%s'", $obj->name);
  	
    if ($row = db_fetch_object($r2)) {
      db_query('DELETE FROM {users_roles} WHERE rid = %d', $row->rid);
      db_query('DELETE FROM {permission} WHERE rid = %d', $row->rid);
    }
    
    db_query("DELETE FROM {role} WHERE name = '%s'", $obj->name);
  }
  
  // delete all thre groups
  db_query('DELETE FROM {uwgroups}');
}

/**
 * Refreshes the managed groups for this module.
 *
 * @return
 */
function _uwgroups_refresh_managed() {
  $groups = lts_command('groups all');
  $success = TRUE;
  
  if (!$groups || empty($groups)) {
  	_uwgroups_error('Could not fetch all groups from LTS.');
  	return FALSE;
  }
  
  // -- delete the groups that we no longer need --
  $result = db_query("SELECT name FROM {uwgroups}");
  
  while ($obj = db_fetch_object($result)) {
    if (in_array($obj->name, $groups))
      continue;
    
    // delete the role & permissions
    $result = db_query("SELECT rid FROM {role} WHERE name = '%s'", $obj->name);
    
    if ($row = db_fetch_object($result)) {
      db_query("DELETE FROM {users_roles} WHERE rid = %d", $row->rid);
      db_query("DELETE FROM {permission} WHERE rid = %d", $row->rid);
      db_query("DELETE FROM {role} WHERE rid = %d", $row->rid);
    }
    
    db_query("DELETE FROM {uwgroups} WHERE name = '%s'", $obj->name);
  }
  
  // -- insert new groups --
  
  foreach ($groups as $group) {
    if (db_result(db_query("SELECT COUNT(*) FROM {uwgroups} WHERE name = '%s'", $group)) === '0') {
      db_query("INSERT INTO {uwgroups} (name) VALUES ('%s')", $group);
      
      if (db_error()) {
      	_uwgroups_error('Could not insert the group "%var" into the database.', $group);
      	$success = FALSE;
      	break;
      }
    }
    
    if (db_result(db_query("SELECT COUNT(*) FROM {role} WHERE name = '%s'", $group)) === '0') {
      db_query("INSERT INTO {role} (name) VALUES ('%s')", $group);
      
      if (db_error()) {
      	_uwgroups_error('Could not insert the group "%var" into the Drupal roles.', $group);
      	$success = FALSE;
      	break;
      }
    }
  }
  
  if (!$success)
    _uwgroups_error('Could not refresh all groups.');
  
  return $success;
}

/**
 * Deny a user with of all roles (that the module is managing).
 *
 * @param $user
 *   A user object.
 *
 * @return
 */
function _uwgroups_deny_roles($user) {
  db_query("DELETE FROM {users_roles} WHERE uid = %d AND rid IN (SELECT rid FROM {role} WHERE name IN (SELECT name FROM {uwgroups}))", $user->uid);
}

/**
 * Grant a user with a role.
 *
 * @param $user
 *   A user object.
 * @param $rolename
 *   A name of the role.
 *
 * @return
 */
function _uwgroups_grant_role($user, $rolename) {
  $result = db_query("SELECT rid FROM {role} WHERE name = '%s'", $rolename);
  
  // check to see if we can find the group
  if ($row = db_fetch_object($result)) {
    $result = db_query('SELECT uid, rid FROM {users_roles} WHERE uid = %d AND rid = %d', $user->uid, $row->rid);
    
    // check to see if we need to add the role to the user
    if (!db_fetch_object($result))
      db_query('INSERT INTO {users_roles} (uid, rid) VALUES (%d, %d)', $user->uid, $row->rid);
  }
}

/**
 * Removes ALL roles for a user then uses the GWS
 * to fetch the user's groups then one-by-one adds them as roles.
 *
 * @param $user
 *   A user object.
 *
 * @return
 */
function _uwgroups_map_groups($user) {
  if (!$user || !$user->name)
    return;
  
  $groups = lts_command('groups user ' . $user->name);
  
  // ensure we got groups
  if (!$groups || empty($groups)) {
    _uwgroups_error('Could not get groups for user "%var".', $user->name);
    return;
  }
  
  // deny all roles to the user (we add them back one by one
  // based off of the UW GWS)
  _uwgroups_deny_roles($user);
  
  foreach ($groups as $group)
  	_uwgroups_grant_role($user, $group);
}

/**
 * Remaps all of the groups (via '_uwgroups_map_groups') for all the
 * currently logged in users.
 *
 * @return
 */
function _uwgroups_remap_logged_in_users() {
  $result = db_query('SELECT uid FROM {sessions}');
  
  while ($row = db_fetch_object($result)) {
    $user = user_load($row->uid);
    _uwgroups_map_groups($user);
  }
}

/**
 * Remaps all of the groups (via '_uwgroups_map_groups') for all the
 * Drupal users.
 *
 * @return
 */
function _uwgroups_remap_all() {
  $result = db_query('SELECT uid FROM {users}');
  
  while ($row = db_fetch_object($result)) {
    $user = user_load($row->uid);
    _uwgroups_map_groups($user);
  }
}
