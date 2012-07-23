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

define('UWGWS_ACTIVE_GROUPS', 'UWGWS_ACTIVE_GROUUPS');
define('UWGWS_ACTIVE_GROUPS_DEFAULT', '');
define('UWGWS_ENABLE_AC', 'UWGWS_ENABLE_AC');
define('UWGWS_ENABLE_AC_DEFAULT', FALSE);
define('UWGWS_AC_GROUP', 'UWGWS_AC_GROUP');
define('UWGWS_AC_GROUP_DEFAULT', '');
define('UWGWS_PERM', 'administer UW GWS');

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
	case UWGWS_ACTIVE_GROUPS:
		if ($val)
			variable_set(UWGWS_ACTIVE_GROUPS, strval($val));
		else
			return variable_get(UWGWS_ACTIVE_GROUPS, UWGWS_ACTIVE_GROUPS_DEFAULT);
	}
	return NULL;
}

// Debugging function
if(!function_exists('Pre')){
	function Pre($string){ echo '<pre>'.print_r($string, true).'</pre>'; }
}

// Fetch and format the list of active groups (set in drupal config)
function _uwgroups_active_groups() {
	$groups = explode("\n", variable_get(UWGWS_ACTIVE_GROUPS, UWGWS_ACTIVE_GROUPS_DEFAULT));
	foreach($groups as $group){
		$return[trim(str_replace(array("\r", "\r\n", "\n"), '', $group))] = trim(str_replace(array("\r", "\r\n", "\n"), '', $group));

	}
	return $return;
}


/**
 * Logs an error in the Drupal log.
 */
function _uwgroups_error($msg, $var = NULL) {
	if ($var)
		watchdog('UWGWS', $msg, array('%var' => $var, ), WATCHDOG_ERROR);
	else
		watchdog('UWGWS', $msg, array(), WATCHDOG_ERROR);
}


/**
 * Removes all groups that this module manages. (called during uninstall?) TODO
 *
 * @return
 */
function _uwgroups_remove_managed($keepPerms = FALSE) {
	// get the old user groups that the module was managing
	$result = db_query('SELECT name FROM {uwgroups}')->fetchAll();

	foreach ($result as $name) {
		// delete the role, associated permissions, and any references from users
		$r2 = db_query("SELECT rid FROM {role} WHERE name = :name", array(':name' => $name));

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
 * Refreshes the managed groups for this module. (called on cron, and config option)
 *
 * @return
 */
function _uwgroups_refresh_managed() {
	// Get a list of all active groups (set in config of module).
	$active_groups = _uwgroups_active_groups();
	
	// Fetch a list of all of our groups from LTS
	$groups = lts_command('groups all');
	$success = TRUE;
	
	// If there aren't any groups, something is wrong, error.
	if (!$groups || empty($groups)) {
		_uwgroups_error('Could not fetch all groups from LTS.');
		return FALSE;
	}

	// -- delete the groups that we no longer need --
	
	// Get a list of current groups managed by this module
	$current_groups = db_query("SELECT name FROM {uwgroups}")->fetchAll();
	
	// Step through list of current groups and remove ones that are no longer exist, or have been removed from active group list (config)
	foreach($current_groups as $group){
		if (in_array($group->name, $groups) && isset($active_groups[$group->name]))
			continue;

		// delete the role & permissions
		$row_id = current(db_query("SELECT rid FROM {role} WHERE name = :name", array(':name' => $group->name))->fetchCol());
		
		if ($row_id != '' && $row_id > 0) {
			//db_query("DELETE FROM {users_roles} WHERE rid = :rid", array('rid' => $row_id));
			//db_query("DELETE FROM {permission} WHERE rid = :rid", array('rid' => $row_id));
			//db_query("DELETE FROM {role} WHERE rid = :rid", array('rid' => $row_id));
			user_role_delete((int)$row_id);
		}

		db_query("DELETE FROM {uwgroups} WHERE name = :name", array('name' => $group->name));
	}

	// -- insert new groups --
	foreach ($groups as $group) {
		if (isset($active_groups[$group]) && db_query("SELECT COUNT(*) FROM {uwgroups} WHERE name = :name", array(':name' => $group))->fetchObject()->{'COUNT(*)'} == 0) {
			$query = null;
			$query = db_insert('uwgroups')->fields(array('name'));
			$query->values(array('name' => $group));
			if (!$query->execute()) {
				_uwgroups_error('Could not insert the group "%var" into the database.', $group);
				$success = FALSE;
				//break;
			}
		}
		
		if (isset($active_groups[$group]) && db_query("SELECT COUNT(*) FROM {role} WHERE name = :name", array(':name' => $group))->fetchObject()->{'COUNT(*)'} == 0) {		
			$role = new stdClass();
			$role->name = $group;
			if(!user_role_save($role)) {
				_uwgroups_error('Could not insert the group "%var" into the Drupal roles.', $group);
				$success = FALSE;
				//break;
			}
		}
	}

	if (!$success){
		_uwgroups_error('Could not refresh all groups.');
	}

	return $success;
}


/**
 * Deny a user of all roles (that the module is managing).
 *
 * @param $user
 *   A user object.
 *
 * @return
 */
function _uwgroups_deny_roles($uid) {
	db_query("DELETE FROM {users_roles} WHERE uid = :uid AND rid IN (SELECT rid FROM {role} WHERE name IN (SELECT name FROM {uwgroups}))", array('uid' => $uid));
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
function _uwgroups_grant_role($uid, $rolename) {
	$rid = current(db_query("SELECT rid FROM {role} WHERE name = :name", array(':name' => $rolename))->fetchCol());
	
	// check to see if we can find the group
	if ($rid !== false) {
		$result = db_query('SELECT uid, rid FROM {users_roles} WHERE uid = :uid AND rid = :rid', array(':uid' => $uid, ':rid' => $rid))->fetchAll();

		// check to see if we need to add the role to the user
		if ($result !== false && sizeof($result) == 0){
			$query = db_insert('users_roles')->fields(array('uid','rid'));
			$query->values(array('uid' => $uid,'rid' => $rid));
			if ($query->execute()) {
	   			_uwgroups_error('Could not insert role for user "%var".', $user->name);
			}
		}
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
	if (!is_object($user) || trim($user->name) == '')
		return;

	$users_groups = lts_command('groups user ' . $user->name);

	// ensure we got groups
	if (!$users_groups || empty($users_groups)) {
	    _uwgroups_error('Could not get groups for user "%var".', $user->name);
		return;
	}

	// deny all roles to the user (we add them back one by one
	// based off of the UW GWS)
	_uwgroups_deny_roles($user->uid);
	
	$active_groups = _uwgroups_active_groups();

	foreach ($users_groups as $group){
		if(isset($active_groups[$group]))
			_uwgroups_grant_role($user->uid, $group);
	}
}


/**
 * Remaps all of the groups (via '_uwgroups_map_groups') for all the
 * currently logged in users.
 *
 * @return
 */
function _uwgroups_remap_logged_in_users() {
	$result = db_query('SELECT uid FROM {sessions}')->fetchCol();

	foreach ($result as $uid) {
		$user = user_load($uid);
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
	$result = db_query('SELECT uid FROM {users}')->fetchCol();

	foreach ($result as $uid) {
		$user = user_load($uid);
		_uwgroups_map_groups($user);
	}
}
