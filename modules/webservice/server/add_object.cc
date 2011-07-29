/*
** Copyright 2011 Merethis
**
** This file is part of Centreon Engine.
**
** Centreon Engine is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License version 2
** as published by the Free Software Foundation.
**
** Centreon Engine is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Centreon Engine. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include <QRegExp>
#include <QScopedArrayPointer>
#include "engine.hh"
#include "error.hh"
#include "objects.hh"
#include "free_object.hh"
#include "add_object.hh"

// TODO: add check host dependency for child.

using namespace com::centreon::engine;

/**
 *  Add somme custom variable to a generic object with custom variables member list.
 *
 *  @param[in]  custom_vars    The custom variables to insert.
 *  @param[out] list_customvar The object custom variables.
 *
 *  @return True if insert sucessfuly, false otherwise.
 */
static bool _add_custom_variables_to_object(std::vector<std::string> const& custom_vars,
					    customvariablesmember** list_customvar) {
  if (list_customvar == NULL)
    return (false);

  for (std::vector<std::string>::const_iterator it = custom_vars.begin(),
	 end = custom_vars.end();
       it != end;
       ++it) {
    // split string into custom var name (key) and the custom var value (value).
    size_t pos = it->find_first_of('=');
    std::string key(*it, 0, pos);
    std::string value(*it, pos + 1);

    // trim the key.
    key.erase(0, key.find_first_not_of(' '));
    key.erase(key.find_last_not_of(' ') + 1);

    // add a new custom var into object.
    if (pos == std::string::npos || key.empty() || value.empty() || key[0] != '_'
	|| add_custom_variable_to_object(list_customvar, key.c_str(), value.c_str()) == NULL)
      return (false);
  }
  return (true);
}

/**
 *  Add somme contacts to a generic object with contacts member list.
 *
 *  @param[in]  contacts     The contacts to insert.
 *  @param[out] list_contact The object contact.
 *
 *  @return True if insert sucessfuly, false otherwise.
 */
static bool _add_contacts_to_object(std::vector<std::string> const& contacts,
				    contactsmember** list_contact) {
  if (list_contact == NULL)
    return (false);

  for (std::vector<std::string>::const_iterator it = contacts.begin(),
	 end = contacts.end();
       it != end;
       ++it) {
    // check if the contact exist..
    contact* cntct = find_contact(it->c_str());
    if (cntct == NULL)
      return (false);

    // create contactsmember and add it into the contact list.
    contactsmember* member = add_contact_to_object(list_contact, it->c_str());
    if (member == NULL)
      return (false);

    // add contact to the contactsmember.
    member->contact_ptr = cntct;
  }
  return (true);
}

/**
 *  Add somme hosts to a generic object with hosts member list.
 *
 *  @param[in]  hosts     The hosts to insert.
 *  @param[out] list_host The object host.
 *
 *  @return True if insert sucessfuly, false otherwise.
 */
static bool _add_hosts_to_object(std::vector<std::string> const& hosts,
				 hostsmember** list_host) {
  if (list_host == NULL)
    return (false);

  for (std::vector<std::string>::const_iterator it = hosts.begin(),
	 end = hosts.end();
       it != end;
       ++it) {
    // check if the host exist.
    host* hst = find_host(it->c_str());
    if (hst == NULL)
      return (false);

    // create a new hostsmember and add it into the host list.
    hostsmember* member = new hostsmember;
    memset(member, 0, sizeof(*member));

    member->host_name = my_strdup(it->c_str());
    member->next = *list_host;
    *list_host = member;

    // add host to the hostsmember.
    member->host_ptr = hst;
  }
  return (true);
}

/**
 *  Add somme contactgroups to a generic object with contactgroups member list.
 *
 *  @param[in]  contactgroups     The contactgroups to insert.
 *  @param[out] list_contactgroup The object contactgroup.
 *
 *  @return True if insert sucessfuly, false otherwise.
 */
static bool _add_contactgroups_to_object(std::vector<std::string> const& contactgroups,
					 contactgroupsmember** list_contactgroup) {
  if (list_contactgroup == NULL)
    return (false);

  for (std::vector<std::string>::const_iterator it = contactgroups.begin(),
	 end = contactgroups.end();
       it != end;
       ++it) {
    // check if the contactgroup exist.
    contactgroup* group = find_contactgroup(it->c_str());
    if (group == NULL)
      return (false);

    // create a new contactgroupsmember and add it into the contactgroup list.
    contactgroupsmember* member = new contactgroupsmember;
    memset(member, 0, sizeof(*member));

    member->group_name = my_strdup(it->c_str());
    member->next = *list_contactgroup;
    *list_contactgroup = member;

    // add contactgroup to the contactgroupsmember.
    member->group_ptr = group;
  }
  return (true);
}

/**
 *  Add somme commands to a generic object with commands member list.
 *
 *  @param[in]  commands     The commands to insert.
 *  @param[out] list_command The object command.
 *
 *  @return True if insert sucessfuly, false otherwise.
 */
static bool _add_commands_to_object(std::vector<std::string> const& commands,
				    commandsmember** list_command) {
  if (list_command == NULL)
    return (false);

  for (std::vector<std::string>::const_iterator it = commands.begin(),
	 end = commands.end();
       it != end;
       ++it) {
    // check if the command exist.
    command* cmd = find_command(it->c_str());
    if (cmd == NULL)
      return (false);

    // create a new commandsmember and add it into the command list.
    commandsmember* member = new commandsmember;
    memset(member, 0, sizeof(*member));

    member->cmd = my_strdup(it->c_str());
    member->next = *list_command;
    *list_command = member;

    // add command to the commandsmember.
    member->command_ptr = cmd;
  }
  return (true);
}

/**
 *  Add somme objects to a generic object with objects member list.
 *
 *  @param[in]  objects     The objects to insert.
 *  @param[out] list_object The object object.
 *  @param[in]  find_object The function to find object with it's name.
 *
 *  @return True if insert sucessfuly, false otherwise.
 */
static bool _add_object_to_objectlist(std::vector<std::string> const& objects,
				      objectlist** list_object,
				      void* (find_object)(char const*)) {
  if (list_object == NULL || find_object == NULL)
    return (false);

  for (std::vector<std::string>::const_iterator it = objects.begin(),
	 end = objects.end();
       it != end;
       ++it) {
    void* obj = find_object(it->c_str());
    if (add_object_to_objectlist(list_object, obj) != OK)
      return (false);
  }
  return (true);
}

/**
 *  Add a new command into the engine.
 *
 *  @param[in] cmd The struct with all information to create new command.
 */
void modules::add_command(ns1__commandType const& cmd) {
  if (::add_command(cmd.name.c_str(), cmd.commandLine.c_str()) != OK)
    throw (engine_error() << "command '" << cmd.name << "' already exist.");
}

/**
 *  Add a new contactgroup into the engine.
 *
 *  @param[in] cntctgrp The struct with all information to create new contactgroup.
 */
void modules::add_contactgroup(ns1__contactGroupType const& cntctgrp) {
  // create a new contactgroup.
  contactgroup* group = ::add_contactgroup(cntctgrp.name.c_str(),
					   cntctgrp.alias.c_str());
  if (group != OK)
    throw (engine_error() << "contactgroup '" << cntctgrp.name << "' already exist.");

  // add all contacts into the contactgroup.
  if (_add_contacts_to_object(cntctgrp.contacts, &group->members) == false) {
    free_contactgroup(group);
    throw (engine_error() << "contactgroup '" << cntctgrp.name << "' invalid contact.");
  }

  // add the content of other contactgroups into this contactgroup.
  for (std::vector<std::string>::const_iterator it = cntctgrp.contactgroups.begin(),
	 end = cntctgrp.contactgroups.end();
       it != end;
       ++it) {
    // check if the contactgroup exist.
    contactgroup* fgroup = find_contactgroup(it->c_str());
    if (fgroup == NULL) {
      free_contactgroup(group);
      throw (engine_error() << "contactgroup '" << cntctgrp.name << "' invalid contactgroup.");
    }

    contactsmember* cntctmembers = fgroup->members;
    while (cntctmembers) {
      contactsmember* member = add_contact_to_contactgroup(group, cntctmembers->contact_name);
      member->contact_ptr = cntctmembers->contact_ptr;
      cntctmembers = cntctmembers->next;
    }
  }
}

/**
 *  Add a new hostgroup into the engine.
 *
 *  @param[in] hstgrp The struct with all information to create new hostgroup.
 */
void modules::add_hostgroup(ns1__hostGroupType const& hstgrp) {
  char const* notes = (hstgrp.notes ? hstgrp.notes->c_str() : NULL);
  char const* notes_url = (hstgrp.notesUrl ? hstgrp.notesUrl->c_str() : NULL);
  char const* action_url = (hstgrp.actionUrl ? hstgrp.actionUrl->c_str() : NULL);

  // create a new hostgroup.
  hostgroup* group = ::add_hostgroup(hstgrp.name.c_str(),
				     hstgrp.alias.c_str(),
				     notes,
				     notes_url,
				     action_url);
  if (group != OK)
    throw (engine_error() << "hostgroup '" << hstgrp.name << "' already exist.");

  // add all host into the hostgroup.
  if (_add_hosts_to_object(hstgrp.hosts, &group->members) == false) {
    free_hostgroup(group);
    throw (engine_error() << "hostgroup '" << hstgrp.name << "' invalid host.");
  }

  // add the content of other hostgroups into this hostgroup.
  for (std::vector<std::string>::const_iterator it = hstgrp.hostgroups.begin(),
	 end = hstgrp.hostgroups.end();
       it != end;
       ++it) {
    // check if the hostgroup exist.
    hostgroup* fgroup = find_hostgroup(it->c_str());
    if (fgroup == NULL) {
      free_hostgroup(group);
      throw (engine_error() << "hostgroup '" << hstgrp.name << "' invalid hstgroup.");
    }

    hostsmember* hstmembers = fgroup->members;
    while (hstmembers) {
      hostsmember* member = add_host_to_hostgroup(group, hstmembers->host_name);
      member->host_ptr = hstmembers->host_ptr;
      hstmembers = hstmembers->next;
    }
  }
}

/**
 *  Add a new servicegroup into the engine.
 *
 *  @param[in] svcgrp The struct with all information to create new servicegroup.
 */
void modules::add_servicegroup(ns1__serviceGroupType const& svcgrp) {
  // check if service have host name and service description.
  if (svcgrp.services.size() % 2)
    throw (engine_error() << "servicegroup '" << svcgrp.name << "' invalid services.");

  char const* notes = (svcgrp.notes ? svcgrp.notes->c_str() : NULL);
  char const* notes_url = (svcgrp.notesUrl ? svcgrp.notesUrl->c_str() : NULL);
  char const* action_url = (svcgrp.actionUrl ? svcgrp.actionUrl->c_str() : NULL);

  // create a new service group.
  servicegroup* group = ::add_servicegroup(svcgrp.name.c_str(),
					   svcgrp.alias.c_str(),
					   notes,
					   notes_url,
					   action_url);
  if (group != OK)
    throw (engine_error() << "servicegroup '" << svcgrp.name << "' already exist.");

  // add all services into the servicegroup.
  for (std::vector<std::string>::const_iterator it = svcgrp.services.begin(),
  	 end = svcgrp.services.end();
       it != end;
       ++it) {
    std::string host_name = *it; // the first iterator is the host name.
    std::string service_description = *(++it); // the second iterator it the service description.

    // check if the service exist.
    service* svc = find_service(host_name.c_str(), service_description.c_str());
    if (svc == NULL) {
      free_servicegroup(group);
      throw (engine_error() << "servicegroup '" << svcgrp.name << "' invalid service.");
    }

    // create a new servicegroupsmember and add it into the servicegroup list.
    servicesmember* member = add_service_to_servicegroup(group,
							 host_name.c_str(),
							 service_description.c_str());
    // add service to the servicesmember.
    member->service_ptr = svc;
  }

  // add the content of other servicegroups into this servicegroup.
  for (std::vector<std::string>::const_iterator it = svcgrp.servicegroups.begin(),
	 end = svcgrp.servicegroups.end();
       it != end;
       ++it) {
    // check if the servicegroup exist.
    servicegroup* fgroup = find_servicegroup(it->c_str());
    if (fgroup == NULL) {
      free_servicegroup(group);
      throw (engine_error() << "servicegroup '" << svcgrp.name
	     << "' invalid svcgroup '" << *it << "'.");
    }

    servicesmember* svcmembers = fgroup->members;
    while (svcmembers) {
      servicesmember* member = add_service_to_servicegroup(group,
							   svcmembers->host_name,
							   svcmembers->service_description);
      member->service_ptr = svcmembers->service_ptr;
      svcmembers = svcmembers->next;
    }
  }
}

/**
 *  Add a new host into the engine.
 *
 *  @param[in] hst The struct with all information to create new host.
 */
void modules::add_host(ns1__hostType const& hst) {
  // check all arguments and set default option for optional options.
  QString notification_options(hst.notificationOptions ? hst.notificationOptions->c_str() : "a");
  notification_options.toLower().trimmed();
  if (notification_options.contains(QRegExp("[^durfsna, ]", Qt::CaseInsensitive)))
    throw (engine_error() << "host '" << hst.name << "' invalid notification options.");

  QString flap_detection_options(hst.flapDetectionOptions ? hst.flapDetectionOptions->c_str() : "a");
  flap_detection_options.toLower().trimmed();
  if (flap_detection_options.contains(QRegExp("[^oduna, ]", Qt::CaseInsensitive)))
    throw (engine_error() << "host '" << hst.name << "' invalid flap detection options.");

  QString stalking_options(hst.stalkingOptions ? hst.stalkingOptions->c_str() : "a");
  stalking_options.toLower().trimmed();
  if (stalking_options.contains(QRegExp("[^oduna, ]", Qt::CaseInsensitive)))
    throw (engine_error() << "host '" << hst.name << "' invalid stalking options.");

  int initial_state = 0;
  QString initial_state_options(hst.initialState ? hst.initialState->c_str() : "o");
  initial_state_options.toLower().trimmed();
  if (initial_state_options == "o" || initial_state_options == "up")
    initial_state = 0;
  else if (initial_state_options == "d" || initial_state_options == "down")
    initial_state = 1;
  else if (initial_state_options == "u" || initial_state_options == "unreachable")
    initial_state = 2;
  else
    throw (engine_error() << "host '" << hst.name << "' invalid initial state.");

  timeperiod* check_period = find_timeperiod(hst.checkPeriod.c_str());
  if (check_period == NULL)
    throw (engine_error() << "host '" << hst.name << "' invalid check period.");

  timeperiod* notification_period = find_timeperiod(hst.notificationPeriod.c_str());
  if (notification_period == NULL)
    throw (engine_error() << "host '" << hst.name << "' invalid notification period.");

  command* cmd_event_handler = find_command(hst.eventHandler->c_str());
  if (cmd_event_handler == NULL)
    throw (engine_error() << "host '" << hst.name << "' invalid event handler.");

  command* cmd_check_command = find_command(hst.checkCommand->c_str());
  if (cmd_check_command == NULL)
    throw (engine_error() << "host '" << hst.name << "' invalid check command.");

  char const* display_name = (hst.displayName ? hst.displayName->c_str() : NULL);
  char const* check_command = (hst.checkCommand ? hst.checkCommand->c_str() : NULL);
  char const* event_handler = (hst.eventHandler ? hst.eventHandler->c_str() : NULL);
  char const* notes = (hst.notes ? hst.notes->c_str() : NULL);
  char const* notes_url = (hst.notesUrl ? hst.notesUrl->c_str() : NULL);
  char const* action_url = (hst.actionUrl ? hst.actionUrl->c_str() : NULL);
  char const* icon_image = (hst.iconImage ? hst.iconImage->c_str() : NULL);
  char const* icon_image_alt = (hst.iconImageAlt ? hst.iconImageAlt->c_str() : NULL);
  char const* vrml_image = (hst.vrmlImage ? hst.vrmlImage->c_str() : NULL);
  char const* statusmap_image = (hst.statusmapImage ? hst.statusmapImage->c_str() : NULL);

  unsigned int check_interval = (hst.checkInterval ? *hst.checkInterval : 5.0);
  unsigned int retry_interval = (hst.retryInterval ? *hst.retryInterval : 1.0);
  bool active_checks_enabled = (hst.activeChecksEnabled ? *hst.activeChecksEnabled : true);
  bool passive_checks_enabled = (hst.passiveChecksEnabled ? *hst.passiveChecksEnabled : true);
  bool obsess_over_host = (hst.obsessOverHost ? *hst.obsessOverHost : true);
  bool event_handler_enabled = (hst.eventHandlerEnabled ? *hst.eventHandlerEnabled : true);
  bool flap_detection_enabled = (hst.flapDetectionEnabled ? *hst.flapDetectionEnabled : true);
  bool notifications_enabled = (hst.notificationsEnabled ? *hst.notificationsEnabled : true);
  bool process_perfdata = (hst.processPerfData ? *hst.processPerfData : true);
  bool retain_status_information = (hst.retainStatusInformation
				    ? *hst.retainStatusInformation : true);
  bool retain_nonstatus_information = (hst.retainNonstatusInformation
				       ? *hst.retainNonstatusInformation : true);

  int first_notification_delay = (hst.firstNotificationDelay ? * hst.firstNotificationDelay : 60);

  double low_flap_threshold = (hst.lowFlapThreshold ? *hst.lowFlapThreshold / 100 : 0.0);
  double high_flap_threshold = (hst.highFlapThreshold ? *hst.highFlapThreshold / 100 : 0.0);

  bool check_freshness = (hst.checkFreshness ? *hst.checkFreshness : false);
  int freshness_threshold = (hst.freshnessThreshold ? *hst.freshnessThreshold : false);

  bool notify_up = (notification_options.indexOf('r') != -1);
  bool notify_down = (notification_options.indexOf('d') != -1);
  bool notify_unreachable = (notification_options.indexOf('u') != -1);
  bool notify_flapping = (notification_options.indexOf('f') != -1);
  bool notify_downtime = (notification_options.indexOf('s') != -1);
  if (notification_options == "a") {
    notify_up = true;
    notify_down = true;
    notify_unreachable = true;
    notify_flapping = true;
    notify_downtime = true;
  }

  bool flap_detection_on_up = flap_detection_options.indexOf('o');
  bool flap_detection_on_down = flap_detection_options.indexOf('d');
  bool flap_detection_on_unreachable = flap_detection_options.indexOf('u');
  if (flap_detection_options == "a") {
    flap_detection_on_up = true;
    flap_detection_on_down = true;
    flap_detection_on_unreachable = true;
  }

  bool stalk_on_up = stalking_options.indexOf('o');
  bool stalk_on_down = stalking_options.indexOf('d');
  bool stalk_on_unreachable = stalking_options.indexOf('u');
  if (stalking_options == "a") {
    stalk_on_up = true;
    stalk_on_down = true;
    stalk_on_unreachable = true;
  }

  // create a new host.
  host* new_hst = ::add_host(hst.name.c_str(),
			     display_name,
			     hst.alias.c_str(),
			     hst.address.c_str(),
			     hst.checkPeriod.c_str(),
			     initial_state,
			     check_interval,
			     retry_interval,
			     hst.maxCheckAttempts,
			     notify_up,
			     notify_down,
			     notify_unreachable,
			     notify_flapping,
			     notify_downtime,
			     hst.notificationInterval,
			     first_notification_delay,
			     hst.notificationPeriod.c_str(),
			     notifications_enabled,
			     check_command,
			     active_checks_enabled,
			     passive_checks_enabled,
			     event_handler,
			     event_handler_enabled,
			     flap_detection_enabled,
			     low_flap_threshold,
			     high_flap_threshold,
			     flap_detection_on_up,
			     flap_detection_on_down,
			     flap_detection_on_unreachable,
			     stalk_on_up,
			     stalk_on_down,
			     stalk_on_unreachable,
			     process_perfdata,
			     false, // XXX: no documentation for
			     NULL,  // failure_prediction_options in nagios 3.
			     check_freshness,
			     freshness_threshold,
			     notes,
			     notes_url,
			     action_url,
			     icon_image,
			     icon_image_alt,
			     vrml_image,
			     statusmap_image,
			     0,     // XXX: 2d_coords not used in centreon.
			     0,
			     false,
			     0,     // XXX: 3d_coords not used in centreon.
			     0,
			     0,
			     false,
			     true,
			     retain_status_information,
			     retain_nonstatus_information,
			     obsess_over_host);

  // add host parents.
  if (_add_hosts_to_object(hst.parents, &new_hst->parent_hosts) == false) {
    free_host(new_hst);
    throw (engine_error() << "host '" << hst.name << "' invalid parent.");
  }

  // add host childs.
  for (hostsmember* member = new_hst->parent_hosts; member != NULL; member = member->next)
    add_child_link_to_host(member->host_ptr, new_hst);

  if (_add_custom_variables_to_object(hst.customVariables,
				      &new_hst->custom_variables) == false) {
    free_host(new_hst);
    throw (engine_error() << "host '" << hst.name << "' invalid custom variable.");
  }

  if (_add_contacts_to_object(hst.contacts, &new_hst->contacts) == false) {
    free_host(new_hst);
    throw (engine_error() << "host '" << hst.name << "' invalid contact.");
  }

  if (_add_contactgroups_to_object(hst.contactGroups, &new_hst->contact_groups) == false) {
    free_host(new_hst);
    throw (engine_error() << "host '" << hst.name << "' invalid contact groups.");
  }

  if (_add_object_to_objectlist(hst.hostgroups,
				&new_hst->hostgroups_ptr,
				(void* (*)(char const*))&find_hostgroup) == false) {
    free_host(new_hst);
    throw (engine_error() << "host '" << hst.name << "' invalid hostgroup.");
  }

  // update timeperiod pointer.
  new_hst->check_period_ptr = check_period;
  new_hst->notification_period_ptr = notification_period;

  // update command pointer.
  new_hst->event_handler_ptr = cmd_event_handler;
  new_hst->check_command_ptr = cmd_check_command;

  // update initial state.
  new_hst->initial_state = initial_state;

  // XXX: host services are update by add service.
}

/**
 *  Add a new service into the engine.
 *
 *  @param[in] svc The struct with all information to create new service.
 */
void modules::add_service(ns1__serviceType const& svc) {
  // check all arguments and set default option for optional options.
  QString notification_options(svc.notificationOptions ? svc.notificationOptions->c_str() : "a");
  notification_options.toLower().trimmed();
  if (notification_options.contains(QRegExp("[^wucrfsna, ]", Qt::CaseInsensitive)))
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid notification options.");

  QString stalking_options(svc.stalkingOptions ? svc.stalkingOptions->c_str() : "a");
  stalking_options.toLower().trimmed();
  if (stalking_options.contains(QRegExp("[^owucna, ]", Qt::CaseInsensitive)))
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid stalking options.");

  QString flap_detection_options(svc.flapDetectionOptions ? svc.flapDetectionOptions->c_str() : "a");
  flap_detection_options.toLower().trimmed();
  if (flap_detection_options.contains(QRegExp("[^owucna, ]", Qt::CaseInsensitive)))
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid flap detection options.");

  int initial_state = STATE_OK;
  QString initial_state_options(svc.initialState ? svc.initialState->c_str() : "o");
  initial_state_options.toLower().trimmed();
  if (initial_state_options == "o" || initial_state_options == "ok")
    initial_state = STATE_OK;
  else if (initial_state_options == "w" || initial_state_options == "warning")
    initial_state = STATE_WARNING;
  else if (initial_state_options == "u" || initial_state_options == "unknown")
    initial_state = STATE_UNKNOWN;
  else if (initial_state_options == "c" || initial_state_options == "critical")
    initial_state = STATE_CRITICAL;
  else
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid initial state.");

  timeperiod* check_period = find_timeperiod(svc.checkPeriod.c_str());
  if (check_period == NULL)
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid check period.");

  timeperiod* notification_period = find_timeperiod(svc.notificationPeriod.c_str());
  if (notification_period == NULL)
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid notification period.");

  command* cmd_event_handler = find_command(svc.eventHandler->c_str());
  if (cmd_event_handler == NULL)
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid event handler.");

  command* cmd_check_command = find_command(svc.checkCommand.c_str());
  if (cmd_check_command == NULL)
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid check command.");

  host* hst = find_host(svc.hostName.c_str());
  if (hst == NULL)
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid host name.");

  char const* display_name = (svc.displayName ? svc.displayName->c_str() : NULL);
  char const* event_handler = (svc.eventHandler ? svc.eventHandler->c_str() : NULL);
  char const* notes = (svc.notes ? svc.notes->c_str() : NULL);
  char const* notes_url = (svc.notesUrl ? svc.notesUrl->c_str() : NULL);
  char const* action_url = (svc.actionUrl ? svc.actionUrl->c_str() : NULL);
  char const* icon_image = (svc.iconImage ? svc.iconImage->c_str() : NULL);
  char const* icon_image_alt = (svc.iconImageAlt ? svc.iconImageAlt->c_str() : NULL);

  bool notify_recovery = (notification_options.indexOf('r') != -1);
  bool notify_unknown = (notification_options.indexOf('u') != -1);
  bool notify_warning = (notification_options.indexOf('w') != -1);
  bool notify_critical = (notification_options.indexOf('c') != -1);
  bool notify_flapping = (notification_options.indexOf('f') != -1);
  bool notify_downtime = (notification_options.indexOf('s') != -1);
  if (notification_options == "a") {
    notify_recovery = true;
    notify_unknown = true;
    notify_warning = true;
    notify_critical = true;
    notify_flapping = true;
    notify_downtime = true;
  }

  bool flap_detection_on_ok = (flap_detection_options.indexOf('o') != -1);
  bool flap_detection_on_unknown = (flap_detection_options.indexOf('u') != -1);
  bool flap_detection_on_warning = (flap_detection_options.indexOf('w') != -1);
  bool flap_detection_on_critical = (flap_detection_options.indexOf('c') != -1);
  if (flap_detection_options == "a") {
    flap_detection_on_ok = true;
    flap_detection_on_unknown = true;
    flap_detection_on_warning = true;
    flap_detection_on_critical = true;
  }

  bool stalk_on_ok = (stalking_options.indexOf('o') != -1);
  bool stalk_on_unknown = (stalking_options.indexOf('u') != -1);
  bool stalk_on_warning = (stalking_options.indexOf('w') != -1);
  bool stalk_on_critical = (stalking_options.indexOf('c') != -1);
  if (stalking_options == "a") {
    stalk_on_ok = true;
    stalk_on_unknown = true;
    stalk_on_warning = true;
    stalk_on_critical = true;
  }

  bool active_checks_enabled = (svc.activeChecksEnabled ? *svc.activeChecksEnabled : true);
  bool passive_checks_enabled = (svc.passiveChecksEnabled ? *svc.passiveChecksEnabled : true);
  bool obsess_over_service = (svc.obsessOverService ? *svc.obsessOverService : true);
  bool event_handler_enabled = (svc.eventHandlerEnabled ? *svc.eventHandlerEnabled : true);
  bool flap_detection_enabled = (svc.flapDetectionEnabled ? *svc.flapDetectionEnabled : true);
  bool notifications_enabled = (svc.notificationsEnabled ? *svc.notificationsEnabled : true);
  bool process_perfdata = (svc.processPerfData ? *svc.processPerfData : true);
  bool retain_status_information = (svc.retainStatusInformation
				    ? *svc.retainStatusInformation : true);
  bool retain_nonstatus_information = (svc.retainNonstatusInformation
				       ? *svc.retainNonstatusInformation : true);

  bool is_volatile = (svc.isVolatile ? *svc.isVolatile : true);

  int first_notification_delay = (svc.firstNotificationDelay ? * svc.firstNotificationDelay : 60);

  double low_flap_threshold = (svc.lowFlapThreshold ? *svc.lowFlapThreshold / 100 : 0.0);
  double high_flap_threshold = (svc.highFlapThreshold ? *svc.highFlapThreshold / 100 : 0.0);

  bool check_freshness = (svc.checkFreshness ? *svc.checkFreshness : false);
  int freshness_threshold = (svc.freshnessThreshold ? *svc.freshnessThreshold : false);

  // create a new service.
  service* new_svc = ::add_service(svc.hostName.c_str(),
				   svc.serviceDescription.c_str(),
				   display_name,
				   svc.checkPeriod.c_str(),
				   initial_state,
				   svc.maxCheckAttempts,
				   true, // XXX: no documentation for parallelize in nagios 3.
				   passive_checks_enabled,
				   svc.checkInterval,
				   svc.retryInterval,
				   svc.notificationInterval,
				   first_notification_delay,
				   svc.notificationPeriod.c_str(),
				   notify_recovery,
				   notify_unknown,
				   notify_warning,
				   notify_critical,
				   notify_flapping,
				   notify_downtime,
				   notifications_enabled,
				   is_volatile,
				   event_handler,
				   event_handler_enabled,
				   svc.checkCommand.c_str(),
				   active_checks_enabled,
				   flap_detection_enabled,
				   low_flap_threshold,
				   high_flap_threshold,
				   flap_detection_on_ok,
				   flap_detection_on_warning,
				   flap_detection_on_unknown,
				   flap_detection_on_critical,
				   stalk_on_ok,
				   stalk_on_warning,
				   stalk_on_unknown,
				   stalk_on_critical,
				   process_perfdata,
				   false,
				   NULL,
				   check_freshness,
				   freshness_threshold,
				   notes,
				   notes_url,
				   action_url,
				   icon_image,
				   icon_image_alt,
				   retain_status_information,
				   retain_nonstatus_information,
				   obsess_over_service);

  if (_add_custom_variables_to_object(svc.customVariables,
				      &new_svc->custom_variables) == false) {
    free_service(new_svc);
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid custom variable.");
  }

  if (_add_contacts_to_object(svc.contacts, &new_svc->contacts) == false) {
    free_service(new_svc);
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid contact.");
  }

  if (_add_contactgroups_to_object(svc.contactGroups, &new_svc->contact_groups) == false) {
    free_service(new_svc);
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid contact groups.");
  }

  if (_add_object_to_objectlist(svc.servicegroups,
				&new_svc->servicegroups_ptr,
				(void* (*)(char const*))&find_servicegroup) == false) {
    free_service(new_svc);
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid hostgroup.");
  }

  // update timeperiod pointer.
  new_svc->check_period_ptr = check_period;
  new_svc->notification_period_ptr = notification_period;

  // update command pointer.
  new_svc->event_handler_ptr = cmd_event_handler;
  new_svc->check_command_ptr = cmd_check_command;

  // update host pointer.
  new_svc->host_ptr = hst;

  // update initial state.
  new_svc->initial_state = initial_state;

  // update host services.
  add_service_link_to_host(hst, new_svc);
}

/**
 *  Add a new contact into the engine.
 *
 *  @param[in] cntct The struct with all information to create new contact.
 */
void modules::add_contact(ns1__contactType const& cntct) {
  // check all arguments and set default option for optional options.
  QString service_options(cntct.serviceNotificationOptions.c_str());
  service_options.toLower().trimmed();
  if (service_options.contains(QRegExp("[^rcwufsna, ]", Qt::CaseInsensitive)))
    throw (engine_error() << "contact '" << cntct.name
	   << "' invalid service notification options.");

  QString host_options(cntct.hostNotificationOptions.c_str());
  host_options.toLower().trimmed();
  if (host_options.contains(QRegExp("[^rdufsna, ]", Qt::CaseInsensitive)))
    throw (engine_error() << "contact '" << cntct.name
	   << "' invalid host notification options.");

  timeperiod* host_notification_period = find_timeperiod(cntct.hostNotificationPeriod.c_str());
  if (host_notification_period == NULL)
    throw (engine_error() << "contact '" << cntct.name
	   << "' invalid host notification period '" << cntct.hostNotificationPeriod << "'.");

  timeperiod* service_notification_period = find_timeperiod(cntct.serviceNotificationPeriod.c_str());
  if (service_notification_period == NULL)
    throw (engine_error() << "contact '" << cntct.name
	   << "' invalid service notification period '" << cntct.serviceNotificationPeriod << "'.");

  char const* email = (cntct.email ? cntct.email->c_str() : NULL);
  char const* pager = (cntct.pager ? cntct.pager->c_str() : NULL);
  char const* alias = (cntct.alias ? cntct.alias->c_str() : NULL);
  bool can_submit_commands = (cntct.canSubmitCommands
			      ? *cntct.canSubmitCommands : true);
  bool retain_status_information = (cntct.retainStatusInformation
				    ? *cntct.retainStatusInformation : true);
  bool retain_nonstatus_information = (cntct.retainNonstatusInformation
				       ? *cntct.retainNonstatusInformation : true);

  bool notify_service_ok = (service_options.indexOf('r') != -1);
  bool notify_service_critical = (service_options.indexOf('c') != -1);
  bool notify_service_warning = (service_options.indexOf('w') != -1);
  bool notify_service_unknown = (service_options.indexOf('u') != -1);
  bool notify_service_flapping = (service_options.indexOf('f') != -1);
  bool notify_service_downtime = (service_options.indexOf('s') != -1);
  if (service_options == "n") {
    notify_service_ok = false;
    notify_service_critical = false;
    notify_service_warning = false;
    notify_service_unknown = false;
    notify_service_flapping = false;
    notify_service_downtime = false;
  }
  else if (service_options == "a" || service_options == "") {
    notify_service_ok = true;
    notify_service_critical = true;
    notify_service_warning = true;
    notify_service_unknown = true;
    notify_service_flapping = true;
    notify_service_downtime = true;
  }

  bool notify_host_up = (host_options.indexOf('r') != -1);
  bool notify_host_down = (host_options.indexOf('d') != -1);
  bool notify_host_unreachable = (host_options.indexOf('u') != -1);
  bool notify_host_flapping = (host_options.indexOf('f') != -1);
  bool notify_host_downtime = (host_options.indexOf('s') != -1);
  if (host_options == "n") {
    notify_host_up = false;
    notify_host_down = false;
    notify_host_unreachable = false;
    notify_host_flapping = false;
    notify_host_downtime = false;
  }
  else if (host_options == "a" || host_options == "") {
    notify_host_up = true;
    notify_host_down = true;
    notify_host_unreachable = true;
    notify_host_flapping = true;
    notify_host_downtime = true;
  }

  // create a address array.
  QScopedArrayPointer<char const*> address(new char const*[MAX_CONTACT_ADDRESSES]);
  memset(&(*address), 0, sizeof(*address) * MAX_CONTACT_ADDRESSES);
  for (unsigned int i = 0, end = cntct.address.size(); i < end; ++i)
    address[i] = cntct.address[i].c_str();

  // create a new contact.
  contact* new_cntct = ::add_contact(cntct.name.c_str(),
				     alias,
				     email,
				     pager,
				     &(*address),
				     cntct.serviceNotificationPeriod.c_str(),
				     cntct.hostNotificationPeriod.c_str(),
				     notify_service_ok,
				     notify_service_critical,
				     notify_service_warning,
				     notify_service_unknown,
				     notify_service_flapping,
				     notify_service_downtime,
				     notify_host_up,
				     notify_host_down,
				     notify_host_unreachable,
				     notify_host_flapping,
				     notify_host_downtime,
				     cntct.hostNotificationsEnabled,
				     cntct.serviceNotificationsEnabled,
				     can_submit_commands,
				     retain_status_information,
				     retain_nonstatus_information);

  if (_add_object_to_objectlist(cntct.contactgroups,
				&new_cntct->contactgroups_ptr,
				(void* (*)(char const*))&find_contactgroup) == false) {
    free_contact(new_cntct);
    throw (engine_error() << "contact '" << cntct.name << "' invalid contactgroup.");
  }

  if (_add_commands_to_object(cntct.hostNotificationCommands,
			      &new_cntct->host_notification_commands) == false) {
    free_contact(new_cntct);
    throw (engine_error() << "contact '" << cntct.name << "' invalid host notification commands.");
  }

  if (_add_commands_to_object(cntct.serviceNotificationCommands,
			      &new_cntct->service_notification_commands) == false) {
    free_contact(new_cntct);
    throw (engine_error() << "contact '" << cntct.name << "' invalid service notification commands.");
  }

  if (_add_custom_variables_to_object(cntct.customVariables,
				      &new_cntct->custom_variables) == false) {
    free_contact(new_cntct);
    throw (engine_error() << "contact '" << cntct.name << "' invalid custom variable.");
  }

  // update timeperiod pointer.
  new_cntct->host_notification_period_ptr = host_notification_period;
  new_cntct->service_notification_period_ptr = service_notification_period;
}
