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
#include <QHash>
#include <QScopedArrayPointer>
#include "engine.hh"
#include "error.hh"
#include "objects.hh"
#include "schedule_object.hh"
#include "free_object.hh"
#include "logging/logger.hh"
#include "globals.hh"
#include "xodtemplate.hh"
#include "add_object.hh"

using namespace com::centreon::engine;
using namespace com::centreon::engine::logging;

/**
 *  Parse and return object options.
 *
 *  @param[in] opt         The option to parse.
 *  @param[in] pattern     The option list.
 *  @param[in] default_opt The default option.
 *
 *  @return The options list.
 */
static QHash<char, bool> get_options(std::string const* opt,
                                     QString const& pattern,
                                     char const* default_opt) {
  QHash<char, bool> res;
  QString _opt(opt ? opt->c_str() : default_opt);
  _opt.toLower().trimmed();
  if (_opt.contains(QRegExp("[^" + pattern + "na, ]", Qt::CaseInsensitive)))
    return (res);

  for (QString::const_iterator it = pattern.begin(), end = pattern.end(); it != end; ++it)
    if (_opt == "n")
      res[it->toAscii()] = false;
    else if (_opt == "a")
      res[it->toAscii()] = true;
    else
      res[it->toAscii()] = (_opt.indexOf(*it) != -1);
  return (res);
}

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
  if (::add_command(cmd.name.c_str(), cmd.commandLine.c_str()) == NULL)
    throw (engine_error() << "command '" << cmd.name << "' create failed.");
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
  if (group == NULL)
    throw (engine_error() << "contactgroup '" << cntctgrp.name << "' create failed.");

  // add all contacts into the contactgroup.
  if (_add_contacts_to_object(cntctgrp.members, &group->members) == false) {
    free_contactgroup(group);
    throw (engine_error() << "contactgroup '" << cntctgrp.name << "' invalid member.");
  }

  // add the content of other contactgroups into this contactgroup.
  for (std::vector<std::string>::const_iterator it = cntctgrp.contactgroupMembers.begin(),
	 end = cntctgrp.contactgroupMembers.end();
       it != end;
       ++it) {
    // check if the contactgroup exist.
    contactgroup* fgroup = find_contactgroup(it->c_str());
    if (fgroup == NULL) {
      free_contactgroup(group);
      throw (engine_error() << "contactgroup '" << cntctgrp.name << "' invalid group member.");
    }

    contactsmember* cntctmembers = fgroup->members;
    while (cntctmembers) {
      contactsmember* member = add_contact_to_contactgroup(group, cntctmembers->contact_name);
      member->contact_ptr = cntctmembers->contact_ptr;
      cntctmembers = cntctmembers->next;
    }
  }

  for (contactsmember const* cm = group->members; cm != NULL; cm = cm->next)
    add_object_to_objectlist(&cm->contact_ptr->contactgroups_ptr, group);
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
  if (group == NULL)
    throw (engine_error() << "hostgroup '" << hstgrp.name << "' create failed.");

  // add all host into the hostgroup.
  if (hstgrp.members.empty() || _add_hosts_to_object(hstgrp.members, &group->members) == false) {
    free_hostgroup(group);
    throw (engine_error() << "hostgroup '" << hstgrp.name << "' invalid member.");
  }

  // add the content of other hostgroups into this hostgroup.
  for (std::vector<std::string>::const_iterator it = hstgrp.hostgroupMembers.begin(),
	 end = hstgrp.hostgroupMembers.end();
       it != end;
       ++it) {
    // check if the hostgroup exist.
    hostgroup* fgroup = find_hostgroup(it->c_str());
    if (fgroup == NULL) {
      free_hostgroup(group);
      throw (engine_error() << "hostgroup '" << hstgrp.name << "' invalid group member.");
    }

    hostsmember* hstmembers = fgroup->members;
    while (hstmembers) {
      hostsmember* member = add_host_to_hostgroup(group, hstmembers->host_name);
      member->host_ptr = hstmembers->host_ptr;
      hstmembers = hstmembers->next;
    }
  }

  for (hostsmember const* hm = group->members; hm != NULL; hm = hm->next)
    add_object_to_objectlist(&hm->host_ptr->hostgroups_ptr, group);
}

/**
 *  Add a new servicegroup into the engine.
 *
 *  @param[in] svcgrp The struct with all information to create new servicegroup.
 */
void modules::add_servicegroup(ns1__serviceGroupType const& svcgrp) {
  // check if service have host name and service description.
  if (svcgrp.members.size() % 2)
    throw (engine_error() << "servicegroup '" << svcgrp.name << "' invalid members.");

  char const* notes = (svcgrp.notes ? svcgrp.notes->c_str() : NULL);
  char const* notes_url = (svcgrp.notesUrl ? svcgrp.notesUrl->c_str() : NULL);
  char const* action_url = (svcgrp.actionUrl ? svcgrp.actionUrl->c_str() : NULL);

  // create a new service group.
  servicegroup* group = ::add_servicegroup(svcgrp.name.c_str(),
					   svcgrp.alias.c_str(),
					   notes,
					   notes_url,
					   action_url);
  if (group == NULL)
    throw (engine_error() << "servicegroup '" << svcgrp.name << "' create failed.");

  // add all services into the servicegroup.
  for (std::vector<std::string>::const_iterator it = svcgrp.members.begin(),
  	 end = svcgrp.members.end();
       it != end;
       ++it) {
    std::string host_name = *it; // the first iterator is the host name.
    std::string service_description = *(++it); // the second iterator it the service description.

    // check if the service exist.
    service* svc = find_service(host_name.c_str(), service_description.c_str());
    if (svc == NULL) {
      free_servicegroup(group);
      throw (engine_error() << "servicegroup '" << svcgrp.name << "' invalid member.");
    }

    // create a new servicegroupsmember and add it into the servicegroup list.
    servicesmember* member = add_service_to_servicegroup(group,
							 host_name.c_str(),
							 service_description.c_str());
    // add service to the servicesmember.
    member->service_ptr = svc;
  }

  // add the content of other servicegroups into this servicegroup.
  for (std::vector<std::string>::const_iterator it = svcgrp.servicegroupMembers.begin(),
	 end = svcgrp.servicegroupMembers.end();
       it != end;
       ++it) {
    // check if the servicegroup exist.
    servicegroup* fgroup = find_servicegroup(it->c_str());
    if (fgroup == NULL) {
      free_servicegroup(group);
      throw (engine_error() << "servicegroup '" << svcgrp.name
	     << "' invalid group member '" << *it << "'.");
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

  for (servicesmember const* sm = group->members; sm != NULL; sm = sm->next)
    add_object_to_objectlist(&sm->service_ptr->servicegroups_ptr, group);
}

/**
 *  Add a new host into the engine.
 *
 *  @param[in] hst The struct with all information to create new host.
 */
void modules::add_host(ns1__hostType const& hst) {
  // check all arguments and set default option for optional options.
  if (hst.contacts.empty() == true && hst.contactGroups.empty() == true)
    throw (engine_error() << "host '" << hst.name
           << "' no contact or no contact groups are defined.");

  QHash<char, bool> notif_opt = get_options(hst.notificationOptions, "durfs", "n");
  if (notif_opt.empty())
    throw (engine_error() << "host '" << hst.name << "' invalid notification options.");

  QHash<char, bool> flap_detection_opt = get_options(hst.flapDetectionOptions, "odu", "n");
  if (flap_detection_opt.empty())
    throw (engine_error() << "host '" << hst.name << "' invalid flap detection options.");

  QHash<char, bool> stalk_opt = get_options(hst.stalkingOptions, "odu", "n");
  if (stalk_opt.empty())
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

  command* cmd_event_handler = NULL;
  if (hst.eventHandler != NULL
      && (cmd_event_handler = find_command(hst.eventHandler->c_str())) == NULL)
      throw (engine_error() << "host '" << hst.name << "' invalid event handler.");

  command* cmd_check_command = NULL;
  if (hst.checkCommand != NULL
      && (cmd_check_command = find_command(hst.checkCommand->c_str())) == NULL)
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

  // XXX: add check host dependency for child.

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
			     notif_opt['r'],
                             notif_opt['d'],
                             notif_opt['u'],
                             notif_opt['f'],
                             notif_opt['s'],
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
                             flap_detection_opt['o'],
                             flap_detection_opt['d'],
                             flap_detection_opt['u'],
                             stalk_opt['o'],
                             stalk_opt['d'],
                             stalk_opt['u'],
			     process_perfdata,
			     true, // XXX: no documentation for
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
			     -1,     // XXX: 2d_coords not used in centreon.
			     -1,
			     false,
			     0,     // XXX: 3d_coords not used in centreon.
			     0,
			     0,
			     false,
			     true,
			     retain_status_information,
			     retain_nonstatus_information,
			     obsess_over_host);
  if (new_hst == NULL)
    throw (engine_error() << "host '" << hst.name << "' create failed.");

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

  // add into scheduler.
  schedule_host(new_hst);

  // host services are update by add service.
}

/**
 *  Add a new service into the engine.
 *
 *  @param[in] svc The struct with all information to create new service.
 */
void modules::add_service(ns1__serviceType const& svc) {
  // check all arguments and set default option for optional options.
  if (svc.contacts.empty() == true && svc.contactGroups.empty() == true)
    throw (engine_error() << "service '" << svc.hostName << "', "
           << svc.serviceDescription << "' no contact or no contact groups are defined.");

  QHash<char, bool> notif_opt = get_options(svc.notificationOptions, "wucrfs", "n");
  if (notif_opt.empty())
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid notification options.");

  QHash<char, bool> stalk_opt = get_options(svc.stalkingOptions, "owuc", "n");
  if (stalk_opt.empty())
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' invalid stalking options.");

  QHash<char, bool> flap_detection_opt = get_options(svc.flapDetectionOptions, "owuc", "n");
  if (flap_detection_opt.empty())
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

  command* cmd_event_handler = NULL;
  if (svc.eventHandler != NULL) {
    std::string cmd_name(*svc.eventHandler, 0, svc.eventHandler->find('!'));
    if ((cmd_event_handler = find_command(cmd_name.c_str())) == NULL)
      throw (engine_error() << "service '" << svc.hostName << ", "
             << svc.serviceDescription << "' invalid event handler.");
  }

  std::string cmd_name(svc.checkCommand, 0, svc.checkCommand.find('!'));
  command* cmd_check_command = find_command(cmd_name.c_str());
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
				   notif_opt['r'],
				   notif_opt['u'],
				   notif_opt['w'],
				   notif_opt['c'],
				   notif_opt['f'],
				   notif_opt['s'],
				   notifications_enabled,
				   is_volatile,
				   event_handler,
				   event_handler_enabled,
				   svc.checkCommand.c_str(),
				   active_checks_enabled,
				   flap_detection_enabled,
				   low_flap_threshold,
				   high_flap_threshold,
				   flap_detection_opt['o'],
				   flap_detection_opt['u'],
				   flap_detection_opt['w'],
				   flap_detection_opt['c'],
				   stalk_opt['o'],
				   stalk_opt['u'],
				   stalk_opt['w'],
				   stalk_opt['c'],
				   process_perfdata,
				   true,
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
  if (new_svc == NULL)
    throw (engine_error() << "service '" << svc.hostName << ", "
	   << svc.serviceDescription << "' create failed.");

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

  ++hst->total_services;
  hst->total_service_check_interval += static_cast<unsigned long>(new_svc->check_interval);

  schedule_service(new_svc);
}

/**
 *  Add a new contact into the engine.
 *
 *  @param[in] cntct The struct with all information to create new contact.
 */
void modules::add_contact(ns1__contactType const& cntct) {
  // check all arguments and set default option for optional options.
  QHash<char, bool> service_opt = get_options(&cntct.serviceNotificationOptions, "rcwufs", "n");
  if (service_opt.empty())
    throw (engine_error() << "contact '" << cntct.name
           << "' invalid service notification options.");

  QHash<char, bool> host_opt = get_options(&cntct.hostNotificationOptions, "rdufs", "n");
  if (host_opt.empty())
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
				     (cntct.address.empty() ? NULL : &(*address)),
				     cntct.serviceNotificationPeriod.c_str(),
				     cntct.hostNotificationPeriod.c_str(),
                                     service_opt['r'],
                                     service_opt['c'],
                                     service_opt['w'],
                                     service_opt['u'],
                                     service_opt['f'],
                                     service_opt['s'],
                                     host_opt['r'],
                                     host_opt['d'],
                                     host_opt['u'],
                                     host_opt['f'],
                                     host_opt['s'],
				     cntct.hostNotificationsEnabled,
				     cntct.serviceNotificationsEnabled,
				     can_submit_commands,
				     retain_status_information,
				     retain_nonstatus_information);
  if (new_cntct == NULL)
    throw (engine_error() << "contact '" << cntct.name << "' create failed.");

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

/**
 *  Add a new host dependency into the engine.
 *
 *  @param[in] hostdependency The struct with all information to create new host dependency.
 */
void modules::add_hostdependency(ns1__hostDependencyType const& hstdependency) {
  // XXX: hostgroups_name is not implemented yet.
  if (!hstdependency.hostgroupsName.empty())
    throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName
           << ", " << hstdependency.hostName
           << "' hostgroups name is not implemented yet.");

  // XXX: dependency_hostgroups_name is not implemented yet.
  if (!hstdependency.dependentHostgroupsName.empty())
    throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName
           << ", " << hstdependency.hostName
           << "' dependent host name is not implemented yet.");

  // check all arguments and set default option for optional options.
  if (!hstdependency.executionFailureCriteria && !hstdependency.notificationFailureCriteria) {
    throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName
           << ", " << hstdependency.hostName << "' have no notification failure criteria and "
           << "no execution failure criteria define.");
  }

  QHash<char, bool> execution_opt = get_options(hstdependency.executionFailureCriteria, "odup", "n");
  if (execution_opt.empty())
    throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName
	   << ", " << hstdependency.hostName << "' invalid execution failure criteria.");

  QHash<char, bool> notif_opt = get_options(hstdependency.notificationFailureCriteria, "odup", "n");
  if (notif_opt.empty())
    throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName
	   << ", " << hstdependency.hostName << "' notification failure criteria.");

  host* dependent_host_ptr = find_host(hstdependency.dependentHostName.c_str());
  if (dependent_host_ptr == NULL)
    throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName << ", "
	   << hstdependency.hostName << "' invalid dependent host name.");

  host* master_host_ptr = find_host(hstdependency.hostName.c_str());
  if (master_host_ptr == NULL)
    throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName << ", "
	   << hstdependency.hostName << "' invalid host name.");

  char const* dependency_period = NULL;
  timeperiod* dependency_period_ptr = NULL;

  if (hstdependency.dependencyPeriod != NULL) {
    dependency_period = hstdependency.dependencyPeriod->c_str();
    if ((dependency_period_ptr = find_timeperiod(dependency_period)) == NULL)
      throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName
             << ", " << hstdependency.hostName << "' invalid dependency period.");
  }

  bool inherits_parent = (hstdependency.inheritsParent ? *hstdependency.inheritsParent : true);

  if (hstdependency.executionFailureCriteria != NULL) {
    hostdependency* new_hstdependency =
      add_host_dependency(hstdependency.dependentHostName.c_str(),
                          hstdependency.hostName.c_str(),
                          EXECUTION_DEPENDENCY,
                          inherits_parent,
                          execution_opt['o'],
                          execution_opt['d'],
                          execution_opt['u'],
                          execution_opt['p'],
                          dependency_period);
    if (new_hstdependency == NULL)
      throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName
             << ", " << hstdependency.hostName << "' create failed.");

    new_hstdependency->master_host_ptr = master_host_ptr;
    new_hstdependency->dependent_host_ptr = dependent_host_ptr;
    new_hstdependency->dependency_period_ptr = dependency_period_ptr;
  }

  if (hstdependency.notificationFailureCriteria != NULL) {
    hostdependency* new_hstdependency =
      add_host_dependency(hstdependency.dependentHostName.c_str(),
                          hstdependency.hostName.c_str(),
                          NOTIFICATION_DEPENDENCY,
                          inherits_parent,
                          notif_opt['o'],
                          notif_opt['d'],
                          notif_opt['u'],
                          notif_opt['p'],
                          dependency_period);
    if (new_hstdependency == NULL)
      throw (engine_error() << "hostdependency '" << hstdependency.dependentHostName
             << ", " << hstdependency.hostName << "' create failed.");

    new_hstdependency->master_host_ptr = master_host_ptr;
    new_hstdependency->dependent_host_ptr = dependent_host_ptr;
    new_hstdependency->dependency_period_ptr = dependency_period_ptr;
  }
}

/**
 *  Add a new host escalation into the engine.
 *
 *  @param[in] hostescalation The struct with all information to create new host escalation.
 */
void modules::add_hostescalation(ns1__hostEscalationType const& hstescalation) {
  // XXX: hostgroups_name is not implemented yet.
  if (!hstescalation.hostgroupsName.empty())
    throw (engine_error() << "hostescalation '" << hstescalation.hostName
           << "' hostgroups name is not implemented yet.");

  // check all arguments and set default option for optional options.
  if (hstescalation.contacts.empty() == true && hstescalation.contactGroups.empty() == true)
    throw (engine_error() << "hostescalation '" << hstescalation.hostName
           << "' no contact or no contact groups are defined.");

  QHash<char, bool> escalation_opt = get_options(hstescalation.escalationOptions, "dur" , "n");
  if (escalation_opt.empty())
    throw (engine_error() << "hostescalation '" << hstescalation.hostName
           << "' invalid escalation options.");

  host* hst = find_host(hstescalation.hostName.c_str());
  if (hst == NULL)
    throw (engine_error() << "hostescalation '" << hstescalation.hostName << "' invalid host.");

  char const* escalation_period = NULL;
  timeperiod* escalation_period_ptr = NULL;
  if (hstescalation.escalationPeriod != NULL) {
    escalation_period = hstescalation.escalationPeriod->c_str();
    if ((escalation_period_ptr = find_timeperiod(escalation_period)) == NULL)
      throw (engine_error() << "hostescalation '" << hstescalation.hostName
             << "' invalid check period.");
  }

  hostescalation* new_hstescalation =
    ::add_hostescalation(hstescalation.hostName.c_str(),
                         hstescalation.firstNotification,
                         hstescalation.lastNotification,
                         hstescalation.notificationInterval,
                         escalation_period,
                         escalation_opt['d'],
                         escalation_opt['u'],
                         escalation_opt['r']);
  if (new_hstescalation == NULL)
    throw (engine_error() << "hostescalation '" << hstescalation.hostName << "' invalid host name.");

  if (_add_contactgroups_to_object(hstescalation.contactGroups,
                                   &new_hstescalation->contact_groups) == false) {
    free_hostescalation(new_hstescalation);
    throw (engine_error() << "hostescalation '" << hstescalation.hostName
           << "' invalid contactgroups.");
  }

  if (_add_contacts_to_object(hstescalation.contacts, &new_hstescalation->contacts) == false) {
    free_hostescalation(new_hstescalation);
    throw (engine_error() << "hostescalation '" << hstescalation.hostName << "' invalid contacts.");
  }

  new_hstescalation->host_ptr = hst;
  new_hstescalation->escalation_period_ptr = escalation_period_ptr;
}

/**
 *  Add a new service dependency into the engine.
 *
 *  @param[in] servicedependency The struct with all
 *  information to create new service dependency.
 */
void modules::add_servicedependency(ns1__serviceDependencyType const& svcdependency) {
  // XXX: hostgroups_name is not implemented yet.
  if (!svcdependency.hostgroupsName.empty())
    throw (engine_error() << "hostdependency '" << svcdependency.dependentHostName
           << ", " << svcdependency.dependentServiceDescription
	   << ", " << svcdependency.hostName
           << ", " << svcdependency.serviceDescription
           << "' hostgroups name is not implemented yet.");

  // XXX: dependency_hostgroups_name is not implemented yet.
  if (!svcdependency.dependentHostgroupsName.empty())
    throw (engine_error() << "hostdependency '" << svcdependency.dependentHostName
           << ", " << svcdependency.dependentServiceDescription
	   << ", " << svcdependency.hostName
           << ", " << svcdependency.serviceDescription
           << "' dependent host name is not implemented yet.");

  // check all arguments and set default option for optional options.
  if (!svcdependency.executionFailureCriteria
      && !svcdependency.notificationFailureCriteria)
    throw (engine_error() << "servicedependency '" << svcdependency.dependentHostName
           << ", " << svcdependency.dependentServiceDescription
	   << ", " << svcdependency.hostName
           << ", " << svcdependency.serviceDescription
           << "' have no notification failure criteria and "
           << "no execution failure criteria define.");

  QHash<char, bool> execution_opt = get_options(svcdependency.executionFailureCriteria, "owucp", "n");
  if (execution_opt.empty())
    throw (engine_error() << "servicedependency '" << svcdependency.dependentHostName
           << ", " << svcdependency.dependentServiceDescription
	   << ", " << svcdependency.hostName
           << ", " << svcdependency.serviceDescription
           << "' execution failure criteria.");

  QHash<char, bool> notif_opt = get_options(svcdependency.notificationFailureCriteria, "owucp", "n");
  if (notif_opt.empty())
    throw (engine_error() << "servicedependency '" << svcdependency.dependentHostName
           << ", " << svcdependency.dependentServiceDescription
	   << ", " << svcdependency.hostName
           << ", " << svcdependency.serviceDescription
           << "' notification failure criteria.");


  service* dependent_service_ptr = find_service(svcdependency.dependentHostName.c_str(),
                                                svcdependency.dependentServiceDescription.c_str());
  if (dependent_service_ptr == NULL)
    throw (engine_error() << "servicedependency '" << svcdependency.dependentHostName
           << ", " << svcdependency.dependentServiceDescription
	   << ", " << svcdependency.hostName
           << ", " << svcdependency.serviceDescription
           << "' invalid dependent host name.");

  service* master_service_ptr = find_service(svcdependency.hostName.c_str(),
                                             svcdependency.serviceDescription.c_str());
  if (master_service_ptr == NULL)
    throw (engine_error() << "servicedependency '" << svcdependency.dependentHostName
           << ", " << svcdependency.dependentServiceDescription
	   << ", " << svcdependency.hostName
           << ", " << svcdependency.serviceDescription
	   << "' invalid host name.");

  char const* dependency_period = NULL;
  timeperiod* dependency_period_ptr = NULL;

  if (svcdependency.dependencyPeriod != NULL) {
    dependency_period = svcdependency.dependencyPeriod->c_str();
    if ((dependency_period_ptr = find_timeperiod(dependency_period)) == NULL)
      throw (engine_error() << "servicedependency '" << svcdependency.dependentHostName
             << ", " << svcdependency.dependentServiceDescription
             << ", " << svcdependency.hostName
             << ", " << svcdependency.serviceDescription
             << "' invalid dependency period.");
  }

  bool inherits_parent = (svcdependency.inheritsParent
                          ? *svcdependency.inheritsParent : true);

  if (svcdependency.executionFailureCriteria != NULL) {
    servicedependency* new_svcdependency =
      add_service_dependency(svcdependency.dependentHostName.c_str(),
                             svcdependency.dependentServiceDescription.c_str(),
                             svcdependency.hostName.c_str(),
                             svcdependency.serviceDescription.c_str(),
                             EXECUTION_DEPENDENCY,
                             inherits_parent,
                             execution_opt['o'],
                             execution_opt['w'],
                             execution_opt['u'],
                             execution_opt['c'],
                             execution_opt['p'],
                             dependency_period);
    if (new_svcdependency == NULL)
      throw (engine_error() << "servicedependency '" << svcdependency.dependentHostName
             << ", " << svcdependency.dependentServiceDescription
             << ", " << svcdependency.hostName
             << ", " << svcdependency.serviceDescription
             << "' create failed.");

    new_svcdependency->master_service_ptr = master_service_ptr;
    new_svcdependency->dependent_service_ptr = dependent_service_ptr;
    new_svcdependency->dependency_period_ptr = dependency_period_ptr;
  }

  if (svcdependency.notificationFailureCriteria != NULL) {
    servicedependency* new_svcdependency =
      add_service_dependency(svcdependency.dependentHostName.c_str(),
                             svcdependency.dependentServiceDescription.c_str(),
                             svcdependency.hostName.c_str(),
                             svcdependency.serviceDescription.c_str(),
                             NOTIFICATION_DEPENDENCY,
                             inherits_parent,
                             notif_opt['o'],
                             notif_opt['w'],
                             notif_opt['u'],
                             notif_opt['c'],
                             notif_opt['p'],
                             dependency_period);
    if (new_svcdependency == NULL)
      throw (engine_error() << "servicedependency '" << svcdependency.dependentHostName
             << ", " << svcdependency.dependentServiceDescription
             << ", " << svcdependency.hostName
             << ", " << svcdependency.serviceDescription
             << "' create failed.");

    new_svcdependency->master_service_ptr = master_service_ptr;
    new_svcdependency->dependent_service_ptr = dependent_service_ptr;
    new_svcdependency->dependency_period_ptr = dependency_period_ptr;
  }
}

/**
 *  Add a new service escalation into the engine.
 *
 *  @param[in] serviceescalation The struct with all
 *  information to create new service escalation.
 */
void modules::add_serviceescalation(ns1__serviceEscalationType const& svcescalation) {
  // XXX: servicegroups_name is not implemented yet.
  if (!svcescalation.hostgroupsName.empty())
    throw (engine_error() << "serviceescalation '" << svcescalation.hostName << ", "
           << svcescalation.serviceDescription << "' hostgroups name is not implemented yet.");

  // check all arguments and set default option for optional options.
  if (svcescalation.contacts.empty() == true && svcescalation.contactGroups.empty() == true)
    throw (engine_error() << "serviceescalation '" << svcescalation.hostName
           << "' no contact or no contact groups are defined.");

  QHash<char, bool> escalation_opt = get_options(svcescalation.escalationOptions, "wucr" , "n");
  if (escalation_opt.empty())
    throw (engine_error() << "serviceescalation '" << svcescalation.hostName << ", "
           << svcescalation.serviceDescription << "' invalid escalation options.");

  service* svc = find_service(svcescalation.hostName.c_str(),
                              svcescalation.serviceDescription.c_str());
  if (svc == NULL)
    throw (engine_error() << "serviceescalation '" << svcescalation.hostName << ", "
           << svcescalation.serviceDescription << "' invalid service.");

  char const* escalation_period = NULL;
  timeperiod* escalation_period_ptr = NULL;
  if (svcescalation.escalationPeriod != NULL) {
    escalation_period = svcescalation.escalationPeriod->c_str();
    if ((escalation_period_ptr = find_timeperiod(escalation_period)) == NULL)
      throw (engine_error() << "serviceescalation '" << svcescalation.hostName << ", "
             << svcescalation.serviceDescription << "' invalid check period.");
  }

  serviceescalation* new_svcescalation =
    ::add_serviceescalation(svcescalation.hostName.c_str(),
                            svcescalation.serviceDescription.c_str(),
                            svcescalation.firstNotification,
                            svcescalation.lastNotification,
                            svcescalation.notificationInterval,
                            escalation_period,
                            escalation_opt['w'],
                            escalation_opt['u'],
                            escalation_opt['c'],
                            escalation_opt['r']);
  if (new_svcescalation == NULL)
    throw (engine_error() << "serviceescalation '" << svcescalation.hostName << ", "
           << svcescalation.serviceDescription << "' create failed.");

  if (_add_contactgroups_to_object(svcescalation.contactGroups,
                                   &new_svcescalation->contact_groups) == false) {
    free_serviceescalation(new_svcescalation);
    throw (engine_error() << "serviceescalation '" << svcescalation.hostName  << ", "
           << svcescalation.serviceDescription << "' invalid contactgroups.");
  }

  if (_add_contacts_to_object(svcescalation.contacts, &new_svcescalation->contacts) == false) {
    free_serviceescalation(new_svcescalation);
    throw (engine_error() << "serviceescalation '" << svcescalation.hostName << ", "
           << svcescalation.serviceDescription << "' invalid contacts.");
  }

  new_svcescalation->service_ptr = svc;
  new_svcescalation->escalation_period_ptr = escalation_period_ptr;
}

/**
 *  Add a new timeperiod into the engine.
 *
 *  @param[in] tperiod The struct with all
 *  information to create new timeperiod.
 */
void modules::add_timeperiod(ns1__timeperiodType const& tperiod) {
  if (find_timeperiod(tperiod.name.c_str()) != NULL)
    throw (engine_error() << "timeperiod '" << tperiod.name << "' timeperiod already exist.");

  xodtemplate_timeperiod* tmpl_tperiod = new xodtemplate_timeperiod();
  memset(tmpl_tperiod, 0, sizeof(*tmpl_tperiod));

  tmpl_tperiod->timeperiod_name = my_strdup(tperiod.name.c_str());
  tmpl_tperiod->alias = my_strdup(tperiod.alias.c_str());
  tmpl_tperiod->register_object = true;
  for (std::vector<std::string>::const_iterator it = tperiod.range.begin(),
         end = tperiod.range.end();
       it != end;
       ++it) {
    std::string base(*it);

    // trim.
    base.erase(0, base.find_first_not_of(' '));
    base.erase(base.find_last_not_of(' ') + 1);

    size_t pos = base.find_first_of(' ');
    std::string key(base, 0, pos);
    std::string value(base, pos + 1);

    // trim.
    key.erase(key.find_last_not_of(' ') + 1);
    value.erase(0, value.find_first_not_of(' '));

    if (xodtemplate_parse_timeperiod_directive(tmpl_tperiod,
                                               key.c_str(),
                                               value.c_str()) == ERROR) {
      throw (engine_error() << "timeperiod '" << tperiod.name << "' invalid exception.");
    }
  }

  std::string exclude;
  for (std::vector<std::string>::const_iterator it = tperiod.exclude.begin(),
         end = tperiod.exclude.end();
       it != end;
       ++it) {
    exclude += *it;
    if (it + 1 != end)
      exclude += ", ";
  }

  int res = xodtemplate_register_timeperiod(tmpl_tperiod);
  xodtemplate_free_timeperiod(tmpl_tperiod);
  if (res != OK)
    throw (engine_error() << "timeperiod '" << tperiod.name << "' create failed.");
}
