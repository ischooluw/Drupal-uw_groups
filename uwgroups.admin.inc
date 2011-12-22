<?php

/***************************************************************
 * UW Groups Module                                            *
 *                                                             *
 * Created by the University of Washington Information School. *
 * Be sure to read README and LICENSE.                         *
 ***************************************************************/

/**
 * @file
 * Provides the UW GWS module administration interface.
 */

/**
 * Gets the configuration form.
 */
function _uwgroups_admin_form($form_state) {
  $form = array();
  
  $form['grouping'] = array(
    '#type' => 'fieldset',
    '#title' => t('UW GWS Module Settings'),
    '#collapsible' => FALSE,
    '#tree' => TRUE,
  );
  
  $form['grouping']['reminder'] = array(
    '#value' => '<p>' . t('NOTE: LTS caches responses from the UW GWS. If certain groups are not getting pulled try clearing the LTS cache.') . '</p>',
  );
  
  $form['grouping']['groups'] = array(
  	'#type' => 'fieldset',
  	'#title' => t('Groups'),
  	'#collapsible' => false,
  	'#tree' => true
  );
  
  $form['grouping']['groups']['active_groups'] = array(
  	'#type' => 'textarea',
  	'#title' => 'Active Groups',
  	'#default_value' => _uwgroups_var(UWGWS_ACTIVE_GROUPS),
  	'#description' => t('List one group per line (note: removing items from this list is the same as deleting the role.)')
  );
  
  $form['grouping']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Configuration'),
  );
  
  
  $form['grouping']['sep'] = array( '#value' => '<br/>', );
  
  $form['grouping']['reload'] = array(
    '#type' => 'submit',
    '#value' => t('Reload Groups'),
  );
  
  $form['grouping']['remap'] = array(
    '#type' => 'submit',
    '#value' => t('Remap All Users'),
  );
  return $form;
}

/**
 * Handles the submission of the form described in '_uwgroups_admin_form'.
 */
function _uwgroups_admin_form_submit($form, &$form_state) {
  if (!user_access(UWGWS_PERM)) {
    drupal_set_message(t('You are unauthorized to make UW GWS Auth changes.'), 'error');
    _uwgroups_error('Unauthorized user attempted to make changes.');
    return;
  }
  
  $op = $form_state['values']['op'];
  
  if ($op == t('Reload Groups')) {
    if (_uwgroups_refresh_managed())
      drupal_set_message(t('Groups have been refreshed.'));
    else
      drupal_set_message(t('Groups could not be refreshed.'), 'error');
  }
  else if ($op == t('Remap All Users')) {
    _uwgroups_remap_all();
    drupal_set_message(t('User memberships have been re-mapped.'));
  }
  else if ($op == t('Save Configuration')) {
    _uwgroups_var(UWGWS_ACTIVE_GROUPS, $form_state['values']['grouping']['groups']['active_groups']);
    drupal_set_message(t('Configuration saved.'));
  }
}

/**
 * Page callback for the UWGWS administration page.
 */
function _uwgroups_admin_page() {
  return drupal_get_form('_uwgroups_admin_form');
}

