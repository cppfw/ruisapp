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

#include "probe_server.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <nitki/loop_thread.hpp>
#include <opros/wait_set.hpp>
#include <setka/init_guard.hpp>
#include <setka/tcp_server_socket.hpp>
#include <setka/tcp_socket.hpp>
#include <tml/parser.hpp>
#include <tml/tree.hpp>
#include <utki/debug.hpp>
#include <utki/flags.hpp>
#include <utki/span.hpp>
#include <utki/string.hpp>
#include <utki/unique_ref.hpp>

#include "application.hpp"

using namespace ruisapp;

// =========================================================================
// tml_run_parser
// =========================================================================

/**
 * @brief Incrementally parses the received tml data chunks.
 *
 * This class owns the tml::parser and the parser listener. The listener builds
 * the tml tree on the fly as the data is being parsed (see how the parser is
 * used in tml::read()). As soon as a top-level block (i.e. a `run{}` block) is
 * fully parsed it is moved into the ready_top_level_nodes member, which the
 * caller (the connection thread) checks after feeding a chunk of data.
 */
class tml_run_parser : public tml::listener
{
	// stack of forests used to build the tree on the fly
	std::vector<tml::forest> stack;

	// the tml tree (forest) under construction; when a top-level block is
	// completed it holds exactly that completed block
	tml::forest cur_forest;

	// the tml parser used to incrementally parse the received data chunks
	tml::parser parser;

public:
	// the completed top-level nodes; the caller (the connection thread) checks
	// this member after feeding a chunk of data to the parser and pops the
	// nodes one by one to process them; the parser does not know about the
	// owner, it just accumulates the completed top-level nodes here
	std::deque<tml::tree> ready_top_level_nodes;

	void on_string_parsed(std::string_view str, const tml::extra_info&) override;
	void on_children_parse_started(tml::location) override;
	void on_children_parse_finished(tml::location loc) override;

	// feed a chunk of received data to the parser
	void feed(utki::span<const uint8_t> data)
	{
		this->parser.parse_data_chunk(data, *this);
	}

	// resets the parser and the tree building state to the initial state
	void reset()
	{
		this->parser.reset();
		this->cur_forest.clear();
		this->stack.clear();
		this->ready_top_level_nodes.clear();
	}
};

// =========================================================================
// tml_run_parser (tml::listener implementation)
// =========================================================================

void tml_run_parser::on_string_parsed(std::string_view str, const tml::extra_info&)
{
	this->cur_forest.emplace_back(str);
}

void tml_run_parser::on_children_parse_started(tml::location)
{
	this->stack.push_back(std::move(this->cur_forest));
}

void tml_run_parser::on_children_parse_finished(tml::location loc)
{
	if (this->stack.empty()) {
		throw std::invalid_argument(utki::cat(
			"probe_server: malformed tml, unopened curly brace encountered at line ", //
			loc.line,
			":",
			loc.offset
		));
	}

	this->stack.back().back().children = std::move(this->cur_forest);
	this->cur_forest = std::move(this->stack.back());
	this->stack.pop_back();

	if (this->stack.empty()) {
		// a top-level block has been fully parsed, move it to the ready list and
		// clear the tree under construction so that the parser is in a clean
		// state for the next block
		this->ready_top_level_nodes.push_back(std::move(this->cur_forest.back()));
		this->cur_forest.clear();
	}
}

// =========================================================================
// probe_server::connection_thread
// =========================================================================

namespace {
// creates an `error{malformed_request{...}}` tml forest with the given message
tml::forest make_error_response(const std::string& message)
{
	return tml::forest{tml::tree(
		tml::leaf("error"), //
		tml::forest{tml::tree(
			tml::leaf("malformed_request"), //
			tml::forest{tml::leaf(message)}
		)}
	)};
}
} // namespace

/**
 * @brief The probe server worker thread.
 *
 * This hidden thread does all the work of the probe server: it listens on the
 * TCP server socket for incoming connections, manages the single accepted
 * connection, parses the received tml data chunks and sends back the
 * responses. It works as a state machine with the following states:
 * - LISTENING: the server socket is added to the wait set and the server waits
 *   for a new incoming connection. As soon as a connection is accepted the
 *   server switches to the RECEIVING state and stops listening.
 * - RECEIVING: the server manages the single accepted connection. It receives
 *   data from the connection, buffers and incrementally parses it as tml using
 *   the tml_run_parser. As soon as a `run{}` block is fully received the server
 *   stores the received commands in the owning probe_server, pokes the
 *   application's main loop so that the probe_server::exec() method can be
 *   called to execute them, and switches to the WAITING state.
 * - WAITING: the server is waiting for the responses for the current batch of
 *   commands (or for an error response) to be ready. It keeps reading from the
 *   connection but discards the data, its only purpose here is to detect the
 *   connection being closed. As soon as the responses (or the error) are
 *   handed over to the thread (via the push() method) the server switches to
 *   the SENDING state.
 * - SENDING: the server sends the responses (wrapped into a `response{...}`
 *   tml document) back to the connection. As soon as all the data is sent the
 *   server switches back to the RECEIVING state.
 *
 * If the connection is broken or closed by the peer, the server clears the
 * current receive buffer (resets the tml parser and the send buffer) and goes
 * back to the LISTENING state, listening on the TCP server socket again.
 */
class probe_server::connection_thread : public nitki::loop_thread
{
	// the setka socket library initialization guard, it must be kept alive for
	// the whole lifetime of the sockets
	std::shared_ptr<utki::destructable> setka_init_guard = setka::get_init_guard_reference();

	// the TCP server socket used to listen for new connections
	setka::tcp_server_socket accept_socket;

	// the currently accepted connection socket, it is empty in LISTENING state
	setka::tcp_socket connection_socket;

	// the tml parser used to incrementally parse the received tml data chunks
	tml_run_parser run_parser;

	// the data buffered for sending to the connection
	std::string send_buffer;
	// the number of bytes of send_buffer already sent
	std::size_t send_offset = 0;

	enum class state {
		listening, // waiting for a new incoming connection
		receiving, // reading from the connection and parsing the data as tml
		// waiting for the responses (or an error) for the current batch of
		// commands to be handed over to the thread by the main loop; no data is
		// read from the connection in this state, the connection being closed is
		// detected only when the current batch of commands is executed and the
		// response is about to be sent
		waiting_commands_result, // waiting for the command execution result
		sending, // sending the response back to the connection
	};
	state cur_state = state::listening;

	// reference to the owning probe_server
	probe_server& owner;

	// handlers for the server thread main loop events
	void handle_accept_socket_event();
	void handle_connection_socket_event(const opros::event_info& ev);

	// handles a chunk of data received from the connection
	void on_data_received(utki::span<const uint8_t> data);

	// called (from on_data_received) when a top-level tml block
	// (i.e. a `run{}` block) is fully received
	void on_top_level_node_parsed(tml::tree node);

	// sets the data to be sent to the connection; the data is sent when the
	// connection socket becomes writable; passing an empty string clears the
	// buffer
	void set_sending_buffer(std::string data);

	// switches the connection socket to either write-only or read-only
	// readiness
	void set_conn_write_mode(bool write);

	// called when the connection is broken or closed by the peer, it clears
	// the buffers and goes back to the LISTENING state
	void on_disconnected();

	// indicates whether the server is currently managing a connection
	bool is_connected() const
	{
		return this->cur_state != state::listening;
	}

public:
	connection_thread(uint16_t port, probe_server& owner);
	~connection_thread();

	connection_thread(const connection_thread&) = delete;
	connection_thread& operator=(const connection_thread&) = delete;

	connection_thread(connection_thread&&) = delete;
	connection_thread& operator=(connection_thread&&) = delete;

	/**
	 * @brief Hands a batch of responses over to the thread to be sent back to
	 * the connection. The responses are wrapped into a `response{...}` tml
	 * document and queued for sending via the thread's message queue.
	 */
	void push(tml::forest responses);

	/**
	 * @brief Called on each main loop iteration (inherited from
	 * nitki::loop_thread). Dispatches the socket events triggered during the
	 * previous wait on the wait set according to the current state.
	 */
	std::optional<uint32_t> on_loop() override;

	/**
	 * @brief Called once the main loop has exited (inherited from
	 * nitki::loop_thread). Removes the current socket from the wait set so
	 * that the wait set is empty at the moment it is destroyed.
	 */
	void on_quit() override;
};

probe_server::connection_thread::connection_thread(uint16_t port, probe_server& owner) :
	nitki::loop_thread(2),
	accept_socket(port),
	owner(owner)
{
	// note, that the (inherited) message queue flag is already added to the
	// wait set by the nitki::loop_thread base class constructor; it is
	// identified by nullptr user_data
	this->wait_set.add(this->accept_socket, opros::ready::read, &this->accept_socket);

	this->start();
}

probe_server::connection_thread::~connection_thread()
{
	this->quit();
	this->join();
}

void probe_server::connection_thread::push(tml::forest responses)
{
	// the state must be "waiting" (a batch of commands has been received and is
	// being executed); change the state to "sending" here (on the calling
	// thread) so that the server thread does not try to read from the
	// connection while the response is being prepared
	utki::assert(this->cur_state == state::waiting_commands_result, SL);
	this->cur_state = state::sending;

	// wrap the responses into a `response{...}` tml document
	const tml::tree response_tree(tml::leaf("response"), std::move(responses));

	// post the response to the thread's message queue; the lambda runs on the
	// thread and sets up the send buffer for the response
	this->push_back([this, response_tree]() {
		// there should be no data in the send buffer while a new response is
		// being prepared
		utki::assert(this->send_buffer.empty(), SL);

		if (this->is_connected()) {
			this->set_sending_buffer(tml::to_string(response_tree));
			this->set_conn_write_mode(true);
		}
	});
}

std::optional<uint32_t> probe_server::connection_thread::on_loop()
{
	try {
		for (auto& t : this->wait_set.get_triggered()) {
			// the (inherited) message queue is identified by nullptr user_data,
			// its events are handled by the base class
			if (t.user_data == nullptr) {
				continue;
			}

			if (t.user_data == &this->accept_socket) {
				this->handle_accept_socket_event();
			} else if (t.user_data == &this->connection_socket) {
				this->handle_connection_socket_event(t);
			}
		}
	} catch (std::exception& e) {
		utki::log_debug([&](auto& o) {
			o << "probe_server: std::exception caught while handling a socket event: " << e.what() << std::endl;
		});

		// on any error while managing the connection go back to the LISTENING
		// state; for the accept socket event the disconnect handling does not
		// apply because there is no connection to disconnect
		if (this->is_connected()) {
			this->on_disconnected();
		}
	}

	// wait for the next event indefinitely
	return std::nullopt;
}

void probe_server::connection_thread::on_quit()
{
	// remove the current socket from the wait set so that the wait set is empty
	// at the moment it is destroyed; the (inherited) message queue is removed by
	// the nitki::loop_thread destructor
	if (this->is_connected()) {
		this->wait_set.remove(this->connection_socket);
	} else {
		this->wait_set.remove(this->accept_socket);
	}
}

void probe_server::connection_thread::handle_accept_socket_event()
{
	utki::assert(this->cur_state == state::listening, SL);

	auto sock = this->accept_socket.accept();

	if (sock.is_empty()) {
		// no pending connection (spurious wakeup), stay in the LISTENING state
		return;
	}

	utki::log_debug([](auto& o) {
		o << "probe_server: connection accepted" << std::endl;
	});

	// transition to the RECEIVING state: stop listening and manage the
	// connection

	// the connection socket must be empty before we assign the new one to it
	utki::assert(this->connection_socket.is_empty(), SL);

	// reset the receive buffer and other per-connection state
	this->run_parser.reset();
	this->set_sending_buffer({});

	this->connection_socket = std::move(sock);

	// no longer listen on the server socket, wait for the connection socket
	// instead
	this->wait_set.remove(this->accept_socket);
	this->wait_set.add(this->connection_socket, opros::ready::read, &this->connection_socket);

	this->cur_state = state::receiving;
}

void probe_server::connection_thread::handle_connection_socket_event(const opros::event_info& ev)
{
	utki::assert(this->is_connected(), SL);

	// check for error condition
	if (ev.flags.get(opros::ready::error)) {
		utki::log_debug([](auto& o) {
			o << "probe_server: socket error, disconnecting" << std::endl;
		});
		this->on_disconnected();
		return;
	}

	// data is only read from the connection in the RECEIVING state; in the
	// WAITING state no data is read, the connection being closed is detected
	// only when the current batch of commands is executed and the response is
	// about to be sent
	if (ev.flags.get(opros::ready::read) && this->cur_state == state::receiving) {

		// the maximum number of bytes to receive in a single receive() call
		constexpr size_t receive_buffer_size = 0x1000; // 4kb

		// no need to initialize the receive buffer, the received data will
		// overwrite its contents
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
		std::array<uint8_t, receive_buffer_size> buf;

		bool first_read = true;
		while (size_t n = this->connection_socket.receive(utki::make_span(buf))) {
			first_read = false;
			this->on_data_received(utki::make_span(buf.data(), n));
		}

		if (first_read) {
			// zero bytes received right after the socket became ready to read
			// means that the connection was closed by the peer
			utki::log_debug([](auto& o) {
				o << "probe_server: connection closed by peer" << std::endl;
			});
			this->on_disconnected();
			return;
		}
	}

	// handle writing to the connection
	if (ev.flags.get(opros::ready::write)) {
		utki::assert(this->cur_state == state::sending, SL);
		utki::assert(!this->send_buffer.empty(), SL);

		// the send buffer is a std::string (i.e. a sequence of chars), but the
		// socket send() expects a span of const unsigned char, so reinterpret
		// the buffer data as a sequence of unsigned char
		const auto data = utki::to_uint8_t(utki::make_span(this->send_buffer)).subspan(this->send_offset);

		const size_t sent = this->connection_socket.send(data);

		if (sent == 0) {
			// the socket is not ready to accept more data,
			// wait for the next write readiness event
			return;
		}

		this->send_offset += sent;

		if (this->send_offset >= this->send_buffer.size()) {
			utki::assert(this->send_offset <= this->send_buffer.size(), SL);

			// all the buffered data has been sent, clear the buffer
			this->set_sending_buffer({});

			// no more data to send, switch back to reading and parsing the
			// received data
			this->set_conn_write_mode(false);
			this->cur_state = state::receiving;
		}
	}
}

void probe_server::connection_thread::on_data_received(utki::span<const uint8_t> data)
{
	this->run_parser.feed(data);

	// check if any top-level nodes are ready and process them one by one
	while (!this->run_parser.ready_top_level_nodes.empty()) {
		auto node = std::move(this->run_parser.ready_top_level_nodes.front());
		this->on_top_level_node_parsed(std::move(node));
		this->run_parser.ready_top_level_nodes.pop_front();
	}
}

void probe_server::connection_thread::on_top_level_node_parsed(tml::tree node)
{
	// in any case, stop parsing new data until the responses (or the error)
	// are sent back to the connection; the parser state does not need to be
	// reset here because the completed node has been moved out of the tree
	// under construction when it was added to the ready list
	this->cur_state = state::waiting_commands_result;

	if (node.value == "run") {
		if (!node.children.empty()) {
			// store the received commands (the children of the `run` node) in
			// the owning probe_server and poke the application's main loop so
			// that the probe_server::exec() method can be called to execute them
			this->owner.push_commands(std::move(node.children));
		} else {
			// a `run{}` block must contain at least one command
			this->push(make_error_response("no commands given"));
		}
	} else {
		// the top level node must be a `run` node
		this->push(make_error_response(utki::cat("expected 'run' but got: ", node.value, " as top level node")));
	}
}

void probe_server::connection_thread::set_sending_buffer(std::string data)
{
	this->send_buffer = std::move(data);
	this->send_offset = 0;
}

void probe_server::connection_thread::set_conn_write_mode(bool write)
{
	// switch the connection socket to either write-only or read-only readiness
	const utki::flags<opros::ready> flags = write ? opros::ready::write : opros::ready::read;
	this->wait_set.change(this->connection_socket, flags, &this->connection_socket);
}

void probe_server::connection_thread::on_disconnected()
{
	// no longer manage the connection socket, remove it from the wait set
	this->wait_set.remove(this->connection_socket);

	// close the connection socket by moving it into a temporary object which
	// is destroyed (and the socket closed) at the end of this scope; the
	// move-assignment operator cannot be used because it asserts that the
	// target socket is empty, and close() cannot be called directly because it
	// is protected in setka::socket
	{
		setka::tcp_socket tmp = std::move(this->connection_socket);
	}

	// clear the current receive buffer (reset the tml parser) and the send
	// buffer
	this->run_parser.reset();
	this->set_sending_buffer({});

	// start listening on the TCP server socket again
	this->wait_set.add(this->accept_socket, opros::ready::read, &this->accept_socket);
	this->cur_state = state::listening;
}

// =========================================================================
// probe_server
// =========================================================================

probe_server::probe_server(uint16_t port) :
	conn_thread(utki::make_unique<connection_thread>(port, *this))
{}

probe_server::~probe_server() = default;

void probe_server::push_commands(tml::forest commands)
{
	{
		const std::lock_guard lock(this->exec_list_mutex);

		// a new batch must not arrive while the previous one is still being
		// executed; receiving is suspended while a batch is in progress, so
		// this should never happen
		utki::assert(this->exec_list.empty());

		this->exec_list = std::move(commands);
		this->exec_position = 0;
		this->exec_responses.clear();
	}

	// poke the application's main loop so that the exec() method is called
	// promptly to execute the stored commands
	ruisapp::inst().poke();
}

void probe_server::exec()
{
	// lock the mutex for the whole duration of the method; because the server
	// thread suspends receiving while a batch is being executed, there is no
	// contention on this mutex
	const std::lock_guard lock(this->exec_list_mutex);

	// there is no batch of commands to execute
	if (this->exec_list.empty()) {
		return;
	}

	// process the stored commands one at a time: the `loop` command makes this
	// method return (after appending a `loop{ok}` response) so that it is
	// called again on the next main loop cycle, while every other command is
	// forwarded to application::probe()
	for (; this->exec_position != this->exec_list.size();) {
		const auto& cmd = this->exec_list[this->exec_position];
		++this->exec_position;

		if (cmd.value == "loop") {
			// the `loop` command yields to the application's main loop so that a
			// frame is rendered and the remaining commands are executed on the
			// next cycle; poke the main loop so that it is woken up from the wait
			// set wait (which would otherwise block until a timeout) and exec() is
			// called again on the next cycle
			this->exec_responses.emplace_back("loop", tml::forest{tml::leaf("ok")});
			inst().poke();
			return;
		}

		// forward the command to the application and collect its response
		// wrapped into the command name
		const tml::forest resp = inst().probe(cmd);
		this->exec_responses.emplace_back(tml::leaf(cmd.value), resp);
	}

	// all the commands have been executed (or there was no batch to execute),
	// hand the collected responses over to the server thread to be sent back
	// to the connection and reset the batch state
	this->conn_thread.get().push(std::move(this->exec_responses));
	this->exec_list.clear();
	this->exec_position = 0;
}
