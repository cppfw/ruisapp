# Application probe server

## Purpose

`ruisapp::probe_server` is a **TCP server** that lets you inspect the state
of a running application and drive it **from another process** — without
attaching a debugger. It is the transport front-end of the application-level
probe (`ruisapp::application::probe()`), the same mechanism that
`ruis::gui::probe()` and `ruis::widget::probe()` build on.

It is intended for **debugging** and **automated testing**:

- **Debugging** — connect with any TCP client (e.g. `telnet`, `nc`, a small
  script) and send probe commands to the running application: ask how many
  windows there are, forward a widget command into a window, request the
  application to quit, and read back a structured answer.
- **Testing** — because the response is a deterministic, structured value
  (an `tml` document, not free-form text), a test harness can open a socket,
  send a command, parse the `response{...}` and assert on it precisely.

## How it works

A `probe_server` owns a worker thread (a `nitki::loop_thread`) that does all
the socket work. It **handles a single connection at a time**:

1. On construction it binds a TCP server socket to the given port and starts
   listening.
2. When a peer connects, it stops listening and reads from the connection,
   incrementally parsing the incoming data as `tml`.
3. As soon as a top-level `run{...}` block is fully received it hands the
   commands over to the owning object and **pokes** the application's main
   loop (`ruisapp::inst().poke()`) so that `probe_server::exec()` is run.
4. `probe_server::exec()` is called from the application's main loop (see
   below). It executes the stored commands one at a time and collects their
   responses.
5. Once the whole batch is executed, the collected responses are wrapped into
   a `response{...}` `tml` document and sent back to the peer.
6. When the connection is closed or broken the server resets and goes back to
   listening for a new connection.

The division of labour is important: **the worker thread only ever reads and
sends; it never executes the probe commands.** Commands are always executed on
the application's main loop thread, by `probe_server::exec()`, which is what
makes it safe to query and mutate the application's (single-threaded) state.

## The wire protocol

The protocol is a sequence of top-level `tml` blocks. The client sends blocks
of the form:

```
run { <command1> [ { <parameters> } ] <command2> ... }
```

- `run` is the **only** allowed top-level block.
- Its children are the **commands** to execute, in order.
- Each command is a `tml` node: its **value** is the command name and its
  **children** (if any) are the parameters. Parameters are optional; at least
  one command is required.

### Executing commands

`probe_server::exec()` runs the commands one at a time:

- The **special `loop` command** makes `exec()` return immediately (after
  recording a `loop { ok }` response) so that it is called again on the *next*
  main-loop cycle and continues with the remaining commands. Use it to yield
  to the application so that a frame is rendered between commands.
- **Every other command** is forwarded to `ruisapp::application::probe()`. Its
  response is collected and wrapped in a node named after the command.

The commands that `ruisapp::application::probe()` understands (the
application-level probe) are:

| command      | form                            | meaning                                                        |
| ------------ | ------------------------------- | -------------------------------------------------------------- |
| `get_windows`| —                              | number of application windows (`windows_count { N }`)          |
| `for_window` | `index { N } <command>`         | forward `<command>` to the `N`th window's gui probe (0-based)  |
| `quit`       | —                              | request the application to exit (returns an `ok` node)         |

An unknown command yields an `error { unknown_command }` node. For the full
set of widget/gui commands that can be forwarded through `for_window`, see the
*ruis* probe documentation.

### The response

Once all the commands of a `run{...}` batch have been executed, the responses
are sent back as a single `response{...}` block, one child per command:

```
response {
	<command1-name> { <response of command1> }
	<command2-name> { <response of command2> }
	...
}
```

For example, sending `run { get_windows loop quit }` and reading until the
`response{...}` is complete would give (values abbreviated):

```
response {
	get_windows {
		windows_count { 1 }
	}
	loop { ok }
	quit { ok }
}
```

### Errors

A structurally well-formed `run{...}` with an invalid payload produces a
`response{...}` containing an `error` node instead of a result:

- `run { }` (no commands) → `error { malformed_request { no commands given } }`
- a top-level block that is not `run` → `error { malformed_request { expected 'run' but got: <name> as top level node } }`

A *malformed* `tml` stream (e.g. an unclosed brace) is not answered with an
error document: the connection is simply reset and the server goes back to
waiting for a new connection.

## Wiring it up

A `probe_server` must be:

1. **Constructed with the TCP port** to listen on — this binds the socket and
   starts the worker thread. The port argument is **optional**: omit it to
   listen on the default port **`12555`** (`ruisapp::default_probe_port`).
2. **Driven from the application's main loop** — `exec()` must be called once
   per main-loop iteration. The natural place is the `on_loop()` override of
   your `ruisapp::application` subclass.

The `probe_server` should be **owned by the application** (a member of your
`application` subclass): it coordinates with the application singleton
(`ruisapp::inst()`) to execute commands and to poke the main loop, so it must
live for as long as the application does.

```cpp
#include <ruisapp/application.hpp>
#include <ruisapp/probe_server.hpp>

class my_app : public ruisapp::application
{
	ruisapp::probe_server probe_server; // member, owned by the application

public:
	my_app() :
		application(/* parameters */),
		probe_server(9527) // the TCP port to listen on
	{}

	// called once per main loop iteration
	void on_loop() override
	{
		this->probe_server.exec();
	}
};
```

While the application runs, connect to the port and send a command:

```
$ nc localhost 9527
run { get_windows }
response {
	get_windows {
		windows_count { 1 }
	}
}
```

### Default port

The port is the *only* constructor argument and it is **optional**. When you
construct the server without giving it a port it listens on the default port
**`12555`** (the constant `ruisapp::default_probe_port`):

```cpp
ruisapp::probe_server probe_server; // listens on 12555
```

So in the example above you could just write `probe_server()` instead of
`probe_server(9527)` and connect to `localhost 12555`.

## Debug builds only

The probe server is a **debugging/testing facility**: it opens a network port
and executes arbitrary probe commands against a live application. In a
**release** build you typically do not want it present at all — no listening
socket, no attack surface, no runtime cost.

Because all the usage sites are just (a) a `probe_server` member and (b) the
`probe_server.exec()` call in `on_loop()`, the whole thing can be **compiled
out with a preprocessor guard**. The project defines the `DEBUG` macro in debug
builds (see `utki/debug.hpp`, where `_DEBUG` implies `DEBUG`), so `#ifdef DEBUG`
is the natural condition:

```cpp
#include <ruisapp/application.hpp>

// only pull in the probe server header in a debug build
#ifdef DEBUG
#	include <ruisapp/probe_server.hpp>
#endif

class my_app : public ruisapp::application
{
#ifdef DEBUG
	ruisapp::probe_server probe_server; // debug-only member
#endif

public:
	my_app() :
		application(/* parameters */)
#ifdef DEBUG
		, probe_server(9527)
#endif
	{}

	void on_loop() override
	{
#ifdef DEBUG
		this->probe_server.exec(); // debug-only
#endif
		// ... the rest of the application's on_loop() body ...
	}
};
```

`DEBUG` is only defined in debug builds, so in a release build the `probe_server`
member, its construction, and the `exec()` call are all removed by the
preprocessor — the binary carries no probe server at all.

> **Note on ordering.** Keep the guarded initializer in the same order as the
> guarded member declaration (both after the base-class initializer) so that the
> in-class member-initializer order matches the declaration order. The base
> class (`application`) is always constructed before any member, which is the
> ordering `probe_server` expects: it only calls into `ruisapp::inst()` at
> runtime, well after the application is fully constructed.

As an alternative to guarding the `exec()` call by hand, you can leave the call
in place unconditionally and wrap it in `utki::run_debug()`, which invokes its
argument only in a debug build and is a no-op otherwise:

```cpp
void on_loop() override
{
#ifdef DEBUG
	utki::run_debug([&]() { this->probe_server.exec(); });
#endif
}
```

In practice the explicit `#ifdef DEBUG` around both the member and the `exec()`
call (the first example) is the cleaner choice, because it removes the
`probe_server` object itself from the release binary rather than merely
disabling its use.
