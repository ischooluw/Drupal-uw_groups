<?php
namespace Drupal\uw_groups;

use Drupal\lts\Person;

class NetIDGroups{
	
	private $Person;

	public function __construct(){
		$this->Person = new Person;
	}
	
	public function getCurrentUserGroups(){
		return $this->Person->GetGroups(\Drupal::currentUser()->getAccountName());
	}
	
	public function getGroupsByNetID($netid){
		return $this->Person->GetGroups($netid);
	}
	
	public function getGroupMembers($group){
		return $this->Person->GetGroupMembers($group);
	}
	
	public function getActiveGroups(){
		if(sizeof(trim(\Drupal::config('uw_groups.settings')->get('active_groups'))) == 0){
			return array();
		}
		
		$active_groups = explode("\n", \Drupal::config('uw_groups.settings')->get('active_groups'));
		foreach($active_groups as $key => $active_group){
			$active_groups[$key] = trim($active_group);
		}
		
		return $active_groups;
	}
	
	public function isNetIDInAnyActiveGroup($netid){
		$active_groups = $this->getActiveGroups();
		$groups = $this->getGroupsByNetID($netid);
		
		foreach($groups as $group){
			foreach($active_groups as $active_group){
				if(trim($group) == trim($active_group) && trim($group) != ''){
					return true;
				}
			}
		}
		
		return false;
	}
}
