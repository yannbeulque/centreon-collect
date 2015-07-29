/*
** Copyright 2015 Merethis
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

#include <QCoreApplication>
#include <sstream>
#include "com/centreon/broker/exceptions/msg.hh"
#include "com/centreon/broker/extcmd/command_client.hh"
#include "com/centreon/broker/extcmd/command_listener.hh"
#include "com/centreon/broker/extcmd/command_request.hh"
#include "com/centreon/broker/extcmd/command_result.hh"
#include "com/centreon/broker/io/exceptions/shutdown.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::extcmd;

/**
 *  Constructor.
 *
 *  @param[in]     socket    Client socket.
 *  @param[in,out] listener  Command listener.
 */
command_client::command_client(
                  QLocalSocket* socket,
                  command_listener* listener)
  : _listener(listener), _socket(socket) {}

/**
 *  Destructor.
 */
command_client::~command_client() {}

/**
 *  Read from command client.
 *
 *  @param[out] d         Read event.
 *  @param[in]  deadline  Deadline in time.
 *
 *  @return Respect io::stream's read() return value.
 */
bool command_client::read(
                       misc::shared_ptr<io::data>& d,
                       time_t deadline) {
  // Read commands from socket.
  d.clear();
  size_t delimiter(_buffer.find_first_of('\n'));
  while (delimiter == std::string::npos) {
    if (_socket->waitForReadyRead(0)) {
      char buffer[1000];
      int rb(_socket->read(buffer, sizeof(buffer)));
      if (rb == 0)
        throw (io::exceptions::shutdown(true, true)
               << "command: client disconnected");
      else if (rb < 0)
        throw (exceptions::msg() << "command: error on client socket: "
               << _socket->errorString());
      _buffer.append(buffer, rb);
    }
    if ((deadline == (time_t)-1) || (time(NULL) < deadline))
      QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);
    else
      break ;
  }

  // External command received.
  if (delimiter != std::string::npos) {
    std::string cmd(_buffer.substr(0, delimiter));
    _buffer.erase(0, delimiter + 1);

    // Process command.
    misc::shared_ptr<command_request>
      req(new command_request);
    command_result res;
    try {
      // Parse command.
      req->parse(cmd);

      // Process command immediately if it queries
      // another command status.
      static char const* status_cmd("COMMAND_STATUS;");
      if (req->cmd.startsWith(status_cmd)) {
        unsigned int cmd_id(req->cmd.mid(strlen(status_cmd)).toUInt());
        res = _listener->command_status(io::data::broker_id, cmd_id);
      }
      // Store command in result listener and let
      // it be processed by multiplexing engine.
      else {
        d = req;
        _listener->write(req);
        res = _listener->command_status(io::data::broker_id, req->id);
      }
    }
    catch (std::exception const& e) {
      // At this point, command request was not written to the command
      // listener, so it not necessary to write command result either.
      res.destination_id = req->source_id;
      res.id = req->id;
      res.code = -1;
      res.msg = e.what();
    }

    // Write result string to client.
    std::string result_str;
    {
      std::ostringstream oss;
      oss << std::dec << res.id << " " << std::hex << std::showbase
          << res.code << " " << res.msg.toStdString() << "\n";
      result_str = oss.str();
    }
    int pos(0), remaining(result_str.size());
    while (remaining > 0) {
      int wb(_socket->write(result_str.data() + pos, remaining));
      if (wb < 0)
        throw (exceptions::msg()
               << "could not write command result to client: "
               << _socket->errorString());
      pos += wb;
      remaining -= wb;
    }

    return (true);
  }
  // No data so we timed out.
  else
    return (false);
}

/**
 *  Write to command client.
 *
 *  @param[in] d  Unused.
 *
 *  @return This method will throw.
 */
unsigned int command_client::write(
                               misc::shared_ptr<io::data> const& d) {
  (void)d;
  throw (io::exceptions::shutdown(false, true)
         << "command: cannot write event to command client");
  return (1);
}
