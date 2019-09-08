/*
 * Copyright 2011 - 2019 Centreon (https://www.centreon.com/)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For more information : contact@centreon.com
 *
 */

#include <gtest/gtest.h>
#include <cstring>
#include "com/centreon/broker/config/applier/init.hh"
#include "com/centreon/broker/io/events.hh"
#include "com/centreon/broker/io/raw.hh"
#include "com/centreon/broker/multiplexing/engine.hh"
#include "com/centreon/broker/multiplexing/muxer.hh"
#include "com/centreon/broker/multiplexing/publisher.hh"
#include "com/centreon/broker/multiplexing/subscriber.hh"

using namespace com::centreon::broker;

#define MSG1 "0123456789abcdef"
#define MSG2 "foo bar baz qux"

/**
 *  We should be able to read from publisher.
 */
TEST(Publisher, Write) {
  // Initialization.
  config::applier::init();

  int retval{0};
  {
    // Publisher.
    multiplexing::publisher p;

    // Subscriber.
    std::unordered_set<uint32_t> filters;
    filters.insert(io::raw::static_type());
    multiplexing::subscriber s("core_multiplexing_publisher_write", "");
    s.get_muxer().set_read_filters(filters);
    s.get_muxer().set_write_filters(filters);

    // Publish event.
    {
      std::shared_ptr<io::raw> raw(new io::raw);
      raw->append(MSG1);
      p.write(std::static_pointer_cast<io::data>(raw));
    }

    // Launch multiplexing.
    multiplexing::engine::instance().start();

    // Publish another event.
    {
      std::shared_ptr<io::raw> raw(new io::raw);
      raw->append(MSG2);
      p.write(std::static_pointer_cast<io::data>(raw));
    }

    // Check data.
    char const* messages[] = {MSG1, MSG2, nullptr};
    for (unsigned int i = 0; messages[i]; ++i) {
      std::shared_ptr<io::data> data;
      s.get_muxer().read(data, 0);
      if (!data || data->type() != io::raw::static_type())
        retval |= 1;
      else {
        std::shared_ptr<io::raw> raw(std::static_pointer_cast<io::raw>(data));
        retval |= strncmp(raw->const_data(), messages[i], strlen(messages[i]));
      }
    }
  }
  // Cleanup.
  config::applier::deinit();

  // Return.
  ASSERT_EQ(retval, 0);
}
