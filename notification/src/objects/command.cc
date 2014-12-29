/*
** Copyright 2011-2013 Merethis
**
** This file is part of Centreon Broker.
**
** Centreon Broker is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License version 2
** as published by the Free Software Foundation.
**
** Centreon Broker is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Centreon Broker. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include "com/centreon/broker/notification/utilities/qhash_func.hh"
#include <QRegExp>
#include <QStringList>
#include "com/centreon/broker/logging/logging.hh"
#include "com/centreon/broker/notification/objects/string.hh"
#include "com/centreon/broker/notification/objects/command.hh"
#include "com/centreon/broker/notification/state.hh"
#include "com/centreon/broker/notification/macro_generator.hh"

using namespace com::centreon::broker::notification;
using namespace com::centreon::broker::notification::objects;

const QRegExp command::_macro_regex("\\$(\\w+)\\$");

// Forward declaration.
static void single_pass_replace(
              std::string &str,
              macro_generator::macro_container const& macros);

/**
 *  Constructor from a base command string.
 *
 *  @param[in] base_command  The command from which to construct this object.
 */
command::command(std::string const& base_command) :
  _base_command(base_command) {}

/**
 *  Copy constructor.
 *
 *  @param[in] obj  The object to copy.
 */
command::command(command const& obj) {
  command::operator=(obj);
}

/**
 *  Assignment operator.
 *
 *  @param[in] obj  The object to copy.
 *
 *  @return         A reference to this object.
 */
command& command::operator=(command const& obj) {
  if (this != &obj) {
    _name = obj._name;
    _base_command = obj._base_command;
  }
  return (*this);
}

/**
 *  Get the name of this command.
 *
 *  @return  The name of this command.
 */
std::string const& command::get_name() const throw() {
  return (_name);
}

/**
 *  Set the name of this command.
 *
 *  @param[in] name  The new name of this command.
 */
void command::set_name(std::string const& name) {
  _name = name;
}

/**
 *  Resolve this command.
 *
 *  @return  A string containing the resolved command.
 */
std::string command::resolve(
                       contact::ptr const& cnt,
                       node::ptr const& n,
                       node_cache const& cache,
                       state const& st,
                       action const& act) {
  // Match all the macros with the wonderful magic of RegExp.
  QString base_command = QString::fromStdString(_base_command);
  macro_generator::macro_container macros;
  int index = 0;
  while ((index = _macro_regex.indexIn(base_command, index)) != -1) {
    macros.insert(_macro_regex.cap(1).toStdString(), "");
    index += _macro_regex.matchedLength();
  }
  if (macros.empty())
    return (_base_command);

  logging::debug(logging::medium)
    << "notification: found " << macros.size() << " macros";

  // Generate each macro.
  try {
    macro_generator generator;
    generator.generate(macros, n->get_node_id(), *cnt, st, cache, act);
  }
  catch (std::exception const& e) {
    logging::error(logging::medium)
      << "notification: could not resolve some macro in command '"
      << _name << "': " << e.what();
  }

  // Replace the macros by their values.
  std::string resolved_command = _base_command;
  single_pass_replace(resolved_command, macros);

  return (resolved_command);
}

/**
 *  @brief Replace all the macros of the string in a single pass.
 *
 *  Marginally faster, and don't mangle contents and values.
 *
 *  @param[in] str     The string.
 *  @param[in] macros  The macros to replace.
 */
static void single_pass_replace(
              std::string& str,
              macro_generator::macro_container const& macros) {
  std::vector<std::pair<std::string, std::string> > macro_list;
  for (macro_generator::macro_container::iterator it(macros.begin()),
                                                  end(macros.end());
       it != end;
       ++it) {
    std::string key("$");
    key.append(it.key());
    key.append("$");
    macro_list.push_back(std::make_pair(key, *it));
  }

  for (std::vector<std::pair<std::string, std::string> >::const_iterator
         it(macro_list.begin()),
         end(macro_list.end());
       it != end;
       ++it) {
    size_t tmp(0);
    while ((tmp = str.find(it->first, tmp)) != std::string::npos) {
      str.replace(tmp, it->first.size(), it->second);
      tmp += it->second.size();
    }
  }
}
