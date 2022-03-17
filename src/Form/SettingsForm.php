<?php

/**
 * @file
 * Contains Drupal\uw_groups\Form\SettingsForm.
 */

namespace Drupal\uw_groups\Form;

use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\user\Entity\Role;

/**
 * Class SettingsForm.
 *
 * @package Drupal\uw_groups\Form
 */
class SettingsForm extends ConfigFormBase
{

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames()
  {
    return [
      'uw_groups.settings',
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId()
  {
    return 'uw_groups_settings_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state)
  {
    $config = $this->config('uw_groups.settings');

    $form['details'] = array(
      '#title' => 'Information',
      '#type' => 'details',
      '#open' => true,
      '#collapsible' => false,
      '#description' => t('The UW Groups listed below will be added as roles within Drupal. Paired with NetID login, NetID users will automatically be added/removed from groups they in which they are or are not members.'),
    );
    $form['active_groups'] = array(
      '#type' => 'textarea',
      '#title' => $this->t('Active Groups'),
      '#description' => t('List one group per line (note: removing items from this list is the same as deleting the role.)'),
      '#default_value' => $this->config('uw_groups.settings')->get('active_groups'),
    );

    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state)
  {
    parent::validateForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state)
  {
    // Get the current groups
    $old_groups = explode("\n", $this->config('uw_groups.settings')->get('active_groups'));

    // Save the new groups
    $this->config('uw_groups.settings')
      ->set('active_groups', $form_state->getValue('active_groups'))
      ->save();

    // Array of new Groups
    $groups = explode("\n", $form_state->getValue('active_groups'));

    // Array of current roles
    $roleObjects = Role::loadMultiple();
    $roles = array();
    foreach ($roleObjects as $key => $roleObject) {
      $roles[$key] = $key;
    }

    // Sync groups to roles
    foreach ($groups as $group) {
      if (trim($group) == '') {
        continue;
      }
      $found = false;
      foreach ($roles as $key => $role) {
        if (trim($group) == $role) {
          unset($roles[$key]);
          $found = true;
          break;
        }
      }
      foreach ($old_groups as $key => $old_group) {
        if (trim($old_group) == trim($group)) {
          unset($old_groups[$key]);
        }
      }

      if (!$found) {;
        $role = \Drupal\user\Entity\Role::create(array('id' => trim($group), 'label' => trim($group)));
        $role->save();
      }
    }

    foreach ($old_groups as $old_group) {
      foreach ($roleObjects as $name => $roleObject) {
        if (trim($name) == trim($old_group)) {
          $roleObject->delete();
        }
      }
    }

    parent::submitForm($form, $form_state);
  }
}
