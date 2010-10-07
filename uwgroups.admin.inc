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
  
  $form['grouping']['uwpubcookie'] = array(
    '#type' => 'fieldset',
    '#title' => t('Access Control'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE
  );
  
  if (module_exists('uwpubcookie')) {
    $form['grouping']['uwpubcookie']['enable_ac'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable AC'),
      '#default_value' => _uwgroups_var(UWGWS_ENABLE_AC),
      '#description' => t('Enable UW Pubcookie access control.'),
    );
    
    $form['grouping']['uwpubcookie']['group'] = array(
      '#type' => 'textfield',
      '#title' => t('AC Group'),
      '#default_value' => _uwgroups_var(UWGWS_AC_GROUP),
      '#description' => t('The group a user must belong to in order to be allowed to log in to the site.'),
      '#size' => 30,
      '#maxlength' => 50,
    );
    
    $form['grouping']['uwpubcookie']['save'] = array(
      '#type' => 'submit',
      '#value' => t('Save Configuration'),
    );
  }
  else {
    $form['grouping']['uwpubcookie']['info'] = array(
      '#value' => '<p>' . t('You must enable the UW Pubcookie module for extra configuration settings.') . '</p>',
    );
  }
  
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
    _uwgroups_var(UWGWS_ENABLE_AC, $form_state['values']['grouping']['uwpubcookie']['enable_ac']);
    _uwgroups_var(UWGWS_ENABLE_AC, $form_state['values']['grouping']['uwpubcookie']['group']);
    drupal_set_message(t('Configuration saved.'));
  }
}

/**
 * Page callback for the UWGWS administration page.
 */
function _uwgroups_admin_page() {
  return drupal_get_form('_uwgroups_admin_form');
}
