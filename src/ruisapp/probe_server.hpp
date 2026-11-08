/*
ruisapp - ruis GUI adaptation layer

Copyright (C) 2016-2025  Ivan Gagis <igagis@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/* ================ LICENSE END ================ */

#pragma once

#include <cstddef>
#include <mutex>

#include <tml/tree.hpp>
#include <utki/unique_ref.hpp>

namespace ruisapp {

/**
 * @brief A TCP probe server.
 *
 * This class represents a TCP server which handles a single connection at a
 * time. When constructed, it starts listening on the given TCP port and waits
 * for a single incoming connection.
 *
 * The server receives data from the connection and incrementally parses it as
 * tml. The expected protocol format is a sequence of
 * `run{command1{parameters} command2 ...}` blocks, where parameters are
 * optional and at least one command is required. As soon as a `run{}` block is
 * fully received the server stores the received commands (the children of the
 * `run` node) and pokes the application's main loop so that the
 * probe_server::exec() method can be called to execute them.
 *
 * The probe_server::exec() method is meant to be called from the
 * application's main loop (i.e. from ruisapp::application::on_loop()). It
 * executes the stored commands one at a time: the special `loop` command
 * makes exec() return immediately (after appending a `loop{ok}` response) so
 * that it is called again on the next main loop cycle and continues with the
 * remaining commands, while every other command is forwarded to
 * ruisapp::application::probe(). Once all the commands have been executed the
 * collected responses are sent back to the connection wrapped into a
 * `response{...}` tml document and the server goes back to reading from the
 * connection.
 *
 * When the connection is broken or closed by the peer, the server clears the
 * current receive buffer and goes back to waiting for a new connection.
 */
class probe_server
{
private:
	// the worker thread which does all the server work, the class is defined
	// in probe_server.cpp
	class connection_thread;

	// the commands of the last fully received `run{}` block (i.e. the children
	// of the `run` node) which are to be executed by exec(); an empty list
	// means that there is no batch of commands to execute
	tml::forest exec_list;
	// the index of the command in exec_list which is yet to be executed
	std::size_t exec_position = 0;
	// the responses collected by exec() so far
	tml::forest exec_responses;
	// protects exec_list, exec_position and exec_responses; they are accessed
	// by the server thread and by the caller of exec()
	mutable std::mutex exec_list_mutex;

	// the worker thread; it is declared last so that it is constructed (and
	// started) only after all the state members above have been initialized
	utki::unique_ref<connection_thread> conn_thread;

	// called by the server thread when a new batch of commands has been
	// received; it stores the commands and pokes the application's main loop;
	// it is thread-safe (it can be called from the server thread);
	// it asserts if a batch of commands is already being executed
	void push_commands(tml::forest commands);

public:
	/**
	 * @brief Constructs the probe server.
	 * Creates the TCP server socket listening on the given port and starts the
	 * server thread which listens for incoming connections.
	 * @param port - the TCP port number to listen on.
	 */
	explicit probe_server(uint16_t port);
	~probe_server();

	probe_server(const probe_server&) = delete;
	probe_server& operator=(const probe_server&) = delete;

	probe_server(probe_server&&) = delete;
	probe_server& operator=(probe_server&&) = delete;

	/**
	 * @brief Executes the stored probe commands.
	 * This method is meant to be called from the application's main loop (i.e.
	 * from ruisapp::application::on_loop()). It executes the stored commands
	 * (the children of the last received `run{}` block) one at a time. The
	 * special `loop` command makes the method return immediately (after
	 * appending a `loop{ok}` response) so that it is called again on the next
	 * main loop cycle and continues with the remaining commands, while every
	 * other command is forwarded to ruisapp::application::probe() and its
	 * response is collected. If there are no stored commands the method does
	 * nothing and returns immediately. Once all the commands have been
	 * executed the collected responses are handed over to the server thread to
	 * be sent back to the connection.
	 */
	void exec();
};
} // namespace ruisapp
