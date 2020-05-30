/*
** Copyright 2013 Centreon
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** For more information : contact@centreon.com
*/

#include "com/centreon/broker/instance_broadcast.hh"

using namespace com::centreon::broker;

/**************************************
 *                                     *
 *           Public Methods            *
 *                                     *
 **************************************/

/**
 *  Default constructor.
 */
instance_broadcast::instance_broadcast()
    : io::data(instance_broadcast::static_type()),
      broker_id(0),
      enabled(true),
      poller_id(0) {}

instance_broadcast::instance_broadcast(uint32_t broker_id,
                                       uint32_t poller_id,
                                       std::string const& poller_name,
                                       bool enabled)
    : io::data(instance_broadcast::static_type()),
      broker_id(broker_id),
      enabled(enabled),
      poller_id(poller_id),
      poller_name(poller_name) {}

/**
 *  Copy constructor.
 *
 *  @param[in] other  Object to copy.
 */
instance_broadcast::instance_broadcast(instance_broadcast const& other)
    : io::data(other),
      broker_id{other.broker_id},
      broker_name{other.broker_name},
      enabled{other.enabled},
      poller_id{other.poller_id},
      poller_name{other.poller_name} {}

/**
 *  Destructor.
 */
instance_broadcast::~instance_broadcast() {}

/**
 *  Assignment operator.
 *
 *  @param[in] other  Object to copy.
 *
 *  @return This object.
 */
instance_broadcast& instance_broadcast::operator=(
    instance_broadcast const& other) {
  if (this != &other) {
    io::data::operator=(other);
    broker_id = other.broker_id;
    broker_name = other.broker_name;
    enabled = other.enabled;
    poller_id = other.poller_id;
    poller_name = other.poller_name;
  }
  return *this;
}

/**
 *  Register the event mapping in the engine.
 */
void instance_broadcast::load() {
  io::events& e(io::events::instance());
  e.register_event(
      io::events::internal, io::events::de_instance_broadcast,
      io::event_info("instance_broadcast", &instance_broadcast::operations,
                     instance_broadcast::entries));
}

/**************************************
 *                                     *
 *           Static Objects            *
 *                                     *
 **************************************/

// Mapping.
mapping::entry const instance_broadcast::entries[] = {
    mapping::entry(&instance_broadcast::broker_id,
                   "broker_id",
                   mapping::entry::invalid_on_zero),
    mapping::entry(&instance_broadcast::broker_name, "broker_name"),
    mapping::entry(&instance_broadcast::enabled, "enabled"),
    mapping::entry(&instance_broadcast::poller_id,
                   "poller_id",
                   mapping::entry::invalid_on_zero),
    mapping::entry(&instance_broadcast::poller_name, "poller_name"),
    mapping::entry()};

// Operations.
static io::data* new_instance_broadcast() {
  return new instance_broadcast;
}
io::event_info::event_operations const instance_broadcast::operations = {
    &new_instance_broadcast};
