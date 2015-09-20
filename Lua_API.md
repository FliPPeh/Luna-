Luna++ native interface
========================

The native interface is implemented in C++ and automatically provided to Lua
scripts.

It contains the bare essentials and is thus not necessarily fun to work with
directly, so large parts of it are wrapped in the Luna corelib (documented
further below), which is itself implemented in Lua and provides some sugar for
the native interface.

Lua standard library modifications
----------------------------------

### os library extension

* `os.time_ms() -> number`

    Returns the current time in milliseconds.

* `os.setenv(var: string, val: string) -> nil` *(POSIX only)*

    Sets the environment variable `var` to `val`.

* `os.unsetenv(var: string) -> nil` *(POSIX only)*

    Unsets the environment variable `var`.


### string library extensions

* `string.rfc1459lower(self: string) -> string`

   Return the ASCII-lowercased corespondence of `self`.

   Example:

        string.rfc1459lower("Hello, \\ [World]") == "hello, | {world}"

* `string.rfc1459upper(self: string) -> string`

   Return the ASCII-uppercased corespondence of `self`.

   Example:

        string.rfc1459upper("Hello, | {World}") == "HELLO \\ [WORLD]"

### Regular expressions

While Lua patterns are as powerful as they are simple, there are some cases
where proper regular expressions may be desired. The following functions export
a subset of C++11's `std::thread` namespace for matching, finding, and replacing
with regular expressions in various dialects.

The dialect can be one of the following:
* `"ecma"`     *(EMCAScript standard regular expressions, default)*
* `"basic"`    *(Basic POSIX regular expressions)*
* `"extended"` *(Extended POSIX regular expressions)*
* `"awk"`      *(awk regular expressions)*
* `"grep"`     *(grep regular expressions)*
* `"egrep"`    *(egrep regular expressions)*


The functions available are:

* `string.regex_match(self: string, pat: string, dialect: string?)`

    Attempts to match the given pattern `pat` against all of `self`.

    If the pattern matches, returns the start of the match, the end of the match
    and all captured groups.

* `string.regex_find(self: string, pat: string, dialect: string?)`

    Attempts to match the given pattern `pat` against part of `self`.

    If the pattern matches, returns the start of the match, the end of the match
    and all captured groups.

* `string.regex_replace(self: string,
                        pat: string,
                        rep: string,
                        dialect: string?)`

    Replaces the **first** occurrence matching `pat` in `self` with `rep`.

    `rep` can refer to captured groups by means of the "`\1`", `"\2"`¸ ...
    escape sequences, which will be replaced by the respective capture.

    Returns the replaced string.

    Example:

        string.regex_replace("banana", "((\\w+)ana)", "\\1rama")
        > "bananarama"

* `string.regex_replace_all(self: string,
                            pat: string,
                            rep: string,
                            dialect: string?)`

    Replaces **all** occurrences matching `pat` in `self` with `rep`.

    `rep` can refer to captured groups by means of the "`\1`", `"\2"`¸ ...
    escape sequences, which will be replaced by the respective capture.

    Returns the replaced string.

    Example:

        string.regex_replace_all("123456789", "(\\d)(\\d)(\\d)", "\\3\\2\\1")
        > "321654987"

Luna++ additions
-----------------

### Logging library

Provides access to the logging system of Luna++.

* `log.log(level: string, message: message) -> nil`

    Logs a message with the given level.

    Level can be any of, in order of increasing severity:
    - "debug" *(debugging output, may be filtered out depending on settings)*
    - "info"  *(general information / output)*
    - "warn"  *(general warnings)*
    - "error" *(errors)*
    - "wtf"   *(here be dragons)*


### luna library

The luna library provides the actual interface through which Lua scripts
can interact with the core. It contains functions for querying or modifying
information about the environment (channels, extensions, users, ...) and
communicating with the server.


#### Basic queries

* `luna.environment.server_supports() -> table`

    Returns a table of the reportedly supported server features and
    capabilities.

    Returned table's schema:

        { PREFIX = "ov(@+)",
          CHANMODES = "eIbq,k,flj,CFLPQTcgimnprstz",
          ... }

* `luna.runtime_info() -> number, number`

    Query information about current uptime.

    Returns, in order:

    1. time of program start, as UNIX timestamp (UTC)
    2. time of connections, as UNIX timestamp (UTC)

* `luna.user_info() -> string, string`

    Query information about this client's identity.

    Returns, in order:

    1. own nickname
    2. own username

* `luna.server_info() -> string, string, string`

    Query information about the current server.

    Returns, in order:

    1. the server's hostname
    2. the server's IP address
    3. the server's port number

* `luna.traffic_info() -> number, number, number, number`

    Query traffic information.

    Returns, in order:

    1. total number of bytes sent since program start
    2. total number of bytes sent this session
    3. total number of bytes received since program start
    4. total number of bytes received this session


#### Channel list

* `luna.channels.find(name: string) -> luna.channel?`

    Try to locate a known channel by name. Returns the found channel on
    success, or nil on failure.

* `luna.channels.list() -> table`

    Return a list of all known channels as `luna.channel` objects, indexed by
    their names.


#### User list and user management

* `luna.users.list() -> table`

    Return a list of all registered users as `luna.user` objects, index by
    their IDs.

* `luna.users.save() -> nil`

    Force saving the userlist.

* `luna.users.reload() -> nil`

    Force reloading the userlist.

* `luna.users.create(id: string,
                     hostmask: string,
                     flags: string,
                     title: string) -> luna.user`

    Create a new user identified by `id`, matching the mask `hostmask`,
    with the given flags and title and return the object representing it.

    Raises an error if a user with that ID already exists.

* `luna.users.remove(id: string) -> nil`

    Remove the user identified by `id`.

    Raises an error if no such user exists.


#### Extension list and extension management

* `luna.extensions.self: luna.ext`

    Static property referring to the script from which it was accessed (or
    in other words: `luna.extensions.self:is_self()` will always be `true`)

* `luna.extensions.list() -> table`

    Returns a list of currently loaded extensions of type `luna.ext`, indexed
    by their IDs.

* `luna.extensions.load(id: string) -> luna.ext`

    Load the extension identified by `id` and return the object representing it.

    Raises an error if an extension with that ID is already loaded.
    Returns the loaded extension.

    *Can only load Lua scripts as of the time of writing.*


* `luna.extensions.unload(id: string) -> nil`

    Unload the extension identified by ID "id".

    Raises an error if a script is trying to unload itself (in other words,
    if `luna.extensions.list()[id]:is_self() == true`), or if no such
    extension is loaded.


#### Server interaction

* `luna.send_message(cmd: string, args...: string...) -> nil`

    Send a raw message to the IRC server.


#### Shared variables

Luna++ maintains a simple hashtable of variables mapped to strings, called
shared variables, which are preserved across restarts.

Extensions can use these shared variables to save configuration or as a means of
very primitive communication. A few variables are set by default and reset
with every restart (notably the `luna.*` namespace), but by default these
variables will not be touched by the runtime.

No particular format is enforced or expected for both the name and value,
although the recommendation for variable names is a hierarchy (joined with `.`)
e.g. `some_script.some_var` for variables used by the script `some_script`.

* `luna.shared.save()`

  Save shared variables from memory to file.

* `luna.shared.reload()`

  Re-load shared variable file into memory, discarding changes in memory.

* `luna.shared.get(var: string) -> string`

  Fetch the shared variable "var".

* `luna.shared.set(var: string, val: string) -> nil`

  Set the variable "var" to "val".

* `luna.shared.clear(var: string) -> nil`

  Unset the variable "var".

* `luna.shared.list() -> table`

  Return the current list of shared variables as a table from key -> value.


## Types

### luna.channel

Represents a known (or joined) IRC channel with its various properties.

#### Methods

* `name() -> string`

    Returns the channel's name (e.g. `"#example"`).

* `created() -> number`

    Returns the channel's creation time as UNIX timestamp. (UTC)

* `topic() -> string, string, number`

    Returns the channel's topic and information about it.

    Returned values, in order:

    1. the topic
    2. the user who set the topic
    3. topic set date as UNIX timestamp (UTC)

* `users() -> table`

    Returns a list of channel users of type `luna.channel.user`, indexed by
    their prefixes.

    Returned table's schema example:

        { "nick1!user@host" = <luna.channel.user: nick1>,
          "nick2!user@host" = <luna.channel.user: nick2>,
          ... }

* `find_user(user: string) -> luna.channel.user?`

    Try locating a user in a channel (by nickname or prefix).

    Returns the located user on success, or nil on failure.

* `modes() -> table`

    Returns a table of the channel's modes.

    Returned table's schema example for a channel with modes "+ntk key" and
    a banlist:

        { t = "",
          n = "",
          k = "key",
          b = {"n1!u1@h1", "n2!u2@h2", ...}}


### luna.channel.user

Represents a known user in a known channel.

#### Methods

* `user_info() -> string, string, string`

    Returns the user's identity information.

    Returned values, in order:

    1. nickname
    2. username
    3. hostname

* `match_reguser() -> luna.user?`

    Attempt to match the user against the list of known users.

    Returns an object of type `luna.user` on success, or nil if no user
    could be matched.

* `modes() -> string`

    Return the modes of the user in the associated channel as a string.

    Example return value for a user both voiced and opped: `"ov"`.

* `channel() -> luna.channel`

    Return the associated channel for this user.


### luna.unknown\_user

Represents an unknown user (not associated with a particular channel).

#### Methods

* `user_info() -> string, string, string`

    Returns the user's identity information.

    Returned values, in order:

    1. nickname
    2. username
    3. hostname

* `match_reguser() -> luna.user?`

    Attempt to match the user against the list of known users.

    Returns an object of type `luna.user` on success, or nil if no user
    could be matched.


### luna.user

Represents a known user from the userlist ("users.txt", by default).

#### Methods

* `id() -> string`

     Returns the user's ID.

* `set_id(new_id: string) -> nil`

     Sets a new user ID.

* `hostmask() -> string`

     Returns the user's hostmask (ECMA regular expression syntax).

* `set_hostmask(new_mask: string) -> nil`

     Sets a new hostmask.

* `flags() -> string`

     Returns the user's flags (e.g. `"foa"`).

* `set_flags(new_flags: string) -> nil`

     Sets new flags.

* `title() -> string`

     Returns the user's title.

* `set_title(new_title: string) -> nil`

     Sets a new user title.


### luna.ext

Represents a loaded Luna++ extension module (such as a Lua script).

#### Methods

* `id() -> string`

     Returns the extension's identification (Lua module for Lua scripts).

* `name() -> string`

     Returns the arbitrary name chosen by the extension (may not be unique).

* `description() -> string`

     Returns the extension's description.

* `version() -> string`

     Returns the extension's version.

* `is_self() -> boolean`

     If the represented extension module is both a Lua script, and the same
     script as the one this method was called from, returns true, in all other
     cases false.



Luna corelib
=============
The luna corelib is automatically imported into all scripts and provides
more comfortable wrappers and helpers around the more minimal native exports.

Lua standard library modifications
-----------------------------------

### string library extensions

* `string.split(self: string, sep: string) -> list of string`

    Split the string on a separator (excluding blank parts).

    Example:

        string.split("Hello  World", " ")
        > {"Hello", "World"}

* `string.fcolour(self: string, front: number, back: number?) -> string`

* `string.fbold(self: string) -> string`

* `string.funderline(self: string) -> string`

* `string.freverse(self: string) -> string`

    Surround the given string with the corresponding de-facto standard
    formatting codes and return it.


* `string.stripformat(self: string) -> string`

    Remove formatting codes from the string and return it.

* `string.irctoansi(self: string) -> string`

    Maps (imperfectly) any contained formatting codes to ANSI escape sequences.

* `string.literalpattern(self: string) -> string`

    Return a pattern that matches this string literally, by escaping every
    non-alphanumeric character.

* `string.template(self: string,
                   replacements: table,
                   unk: string?,
                   flags: string?) -> string`

    Given a mapping of keys to values, format a string using interpolations, e.g.

        local str = "Hello ${SUBJECT}"
        str:template{SUBJECT = "World"}
        > Hello World

    Keys can refer to keys inside the given table `replacements`, to
    environment variables (`env:HOME`), or to shared Luna variables
    (`var:luna.version`).

    Replacements can also take the form of `${lua:expr}`, where `expr` is any
    valid Lua expression that will be evaluated, e.g. `${lua: return 1 + 4}`
    will yield `"5"`.

    Replacements can be escaped with `$`, i.e. `$${VAR}` = `${VAR}`.

    If a variable can not be located, it is replaced with the value of `unk`
    unless a special replacement is specified inline: `${VAR/<VAR not found>`

    The `flags` parameter controls which kinds of replacements are enabled,
    where `"e"` = environment, `"v"` = shared variables, `"l"` = Lua
    expressions.
    By default it allows all replacements, i.e. `"evl"`.

    Due to the information that can be queried with this function, and the
    support for evaluating arbitrary expressions, it should naturally not be
    run on untrusted input unless the "enable" field is sufficiently
    restricting.

* `string.capitalize(self: string) -> string`

    Capitalize the given string (i.e. `"hello world" -> "Hello world"`)

* `string.titlecase(self: string) -> string`

    Title-case the given string (i.e. `"hello world" -> "Hello World"`)

* `string.trim(self: string) -> string`

    Strip leading and trailing whitespace.

* `string.shlex(self: string) -> list of string`

    Split a string similarly to how a command line shell would.

    Example:

        string.shlex("Hello World")
        > {"Hello", "World"}

        string.shlex("One\\ Two\\ Three Four \"Five Six\")
        > {"One Two Three", "Four", "Five Six"}

* `string.unichar(...) -> string`

    Like `string.char`, but values outside the ASCII range [0, 127] will be
    encoded using UTF-8.

Luna++ additions
-----------------

### Logging

* `log.debug(...) -> nil`

* `log.info(...) -> nil`

* `log.warn(...) -> nil`

* `log.err(...) -> nil`

* `log.wtf(...) -> nil`

    Helper functions around `log.log()` with the coresponding log level.
    All supplied arguments are joined with `"\t"` (emulating the builtin
    `print()`).

* `print(...) -> nil`

    Overridden default `print()` function, presenting the same interface, but
    using `log.info()` internally.


### Server interaction

* `luna.privmsg(target: string, msg: string)`

* `luna.notice(target: string, msg: string)`

    Send a message or a notice to the given target.

* `luna.request_ctcp(target: string, ctcp: string, arg: string?)`

* `luna.respond_ctcp(target: string, ctcp: string, arg: string?)`

    Request or respond to CTCPs.

* `luna.action(target: string, msg: string)`

    Send a /me message.

* `luna.join(channel: string, key: string?)`

* `luna.part(channel: string, reason: string?)`

    Join or leave a channel.


### Information queries

* `luna.started() -> number`

* `luna.connected() -> number`

    Return the time of program start or server connection. (UNIX timestamp, UTC)


* `luna.own_nick() -> string`

* `luna.own_user() -> string`

    Return the own nick- or username.


* `luna.server_host() -> string`

* `luna.server_addr() -> string`

* `luna.server_port() -> number`

    Return the server hostname, IP address or port number.


* `luna.bytes_sent_total() -> number`

* `luna.bytes_sent() -> number`

* `luna.bytes_received_total() -> number`

* `luna.bytes_received() -> number`

    Return the number of bytes sent or received since program or session start.


### Shared variables

The Luna core library provides a "magic" wrapper around the native shared
variable functions, accessible via the `luna.shared` table, which also hides
the native functions.

    -- fetching variables
    luna.shared["luna.compiler"]
    > "GCC 5.1.0"

    -- storing variables
    luna.shared["testscript.somevar"] = 42  -- automatically converted to string
    luna.shared["testscript.somevar"]
    > "42"  -- note the string type

    -- clearing variables
    luna.shared["testscript.somevar"] = nil


Additionally, the following functions are defined with the wrapper:

* `luna.save_shared() -> nil`

    Force immediate saving of the shared variables.

* `luna.reload_shared() -> nil`

    Force immediate reloading of the shared variables.


### Utility functions

* `luna.util.mask(prefix: string, style: string?, type: number?) -> string`

    Generate a mask of various types based on the nick-, user-, and hostname of
    this user.

    The mask style can be any of:

    * `"irc"`:  generate a hostmask, suitable for IRC bans (default)
    * `"ecma"`: generate an ECMA regular expression, suitable for the userlist
    * `"lua"`:  generate a Lua pattern, suitable for string.find, ...

    The mask type can be any number between 1 and 10 (default 4) and controls
    the placement of wildcards in the generated pattern.

    Examples:

        local p = "nick!user@host"

        luna.util.mask(p, "ecma")
        > "(.+?)!(.?)user@(.+?)"

        luna.util.mask(p, "lua", 1)
        > "(.-)!user@host"

        luna.util.mask(p, "irc", 2)
        > "*!*user@host"

* `luna.util.collect_modechanges(modes: table) -> list of string`

    Minimize a list of planned mode changes.

    Example:

        luna.util.collect_modechanges{
            {"+o", "op1"},
            {"+v", "voice1"},
            {"+o", "op2"},
            {"+b", "ban1"},
            "-t",
            "-c",
            {"+k key"}}

        > { "+vookb-tc", "voice1", "op2", "op1", "key", "ban1"}

* `luna.util.memorize(fn: function, time: number) -> function`

    Return a function that memorizes its last result for `time` seconds (may be
    fractional).


* `map(t: table, fn: function) -> table`

    Map a function over a sequential table. For every element, the function `fn`
    is called with its value, which will be replaced with the returned value.

    Returns the resulting table.

* `map_kv(t: table, fn: function) -> table`

    Map a function over an associative table. For every element, the function
    `fn` is called with the its key and value. It is expected to return a new
    key (which may be the same as the old key) and a new value (which may also
    be the same as the old), which will be inserted into the returned table.

    Returns the resulting table.

Types
------
The Luna corelib also adds some more methods to the exported native types.

### luna.unknown\_user / luna.channel.user

* `nick() -> string`

* `user() -> string`

* `host() -> string`

    Return the nick-, user-, or hostname of the user.

* `mask(style: string?, type: number?) -> string`

    Helper function around `luna.util.mask`. (see above)

* `is_me() -> bool`

    Returns true if the user object refers to this client.

* `privmsg(msg: string) -> nil`

* `notice(msg: string) -> nil`

    Send a message or a notice to this user.

* `action(msg: string) -> nil`

    Send a /me message to this user.

* `request_ctcp(ctcp: string, arg: string?) -> nil`

* `respond_ctcp(ctcp: string, arg: string?) -> nil`

    Send a CTCP request or response to this user.


* `respond(response: string) -> nil`

    Send a response to this user, commonly used for command responses.

    If the user has an associated channel, the response will be sent there, by
    default in the format of "nick: response". Otherwise, `respond()` will act
    the same as `privmsg()`.


### luna.channel

* `privmsg(msg: string, lvl: string?) -> nil`

* `notice(msg: string, lvl: string? -> nil`

    Send a message or a notice to this channel. If level is given, restrict who
    can receive the message (e.g. `"@"` for ops only).

* `action(msg: string, lvl: string?) -> nil`

    Send a /me message to this channel. If level is given, restrict who
    can receive the message (e.g. `"@"` for ops only).

* `request_ctcp(ctcp: string, arg: string?, level: string?) -> nil`

* `respond_ctcp(ctcp: string, arg: string?, level: string?) -> nil`

    Respond to or request a CTCP from this channel. If level is given, restrict
    who can receive the message (e.g. `"@"` for ops only).

* `set_trigger(trigger: string?) -> nil`

    Set the command trigger for this channel, or reset to default if "trigger"
    is nil.

* `trigger() -> string`

    Return the current command trigger for this channel.


Scripts
========
Scripts are loaded from the "scripts/" subdirectory. A script can be a single
file (Lua module) or a directory (Lua package). When trying to load the script
"example", the following paths are tried in order:

1. scripts/**example**.lua
2. scripts/**example**/init.lua

Whichever file is found first will be evaluated and must return a table that
satisfies some basic requirements:

1. an `info` table, with at least the following fields:
  - `name`: `string`
  - `description`: `string`
  - `version`: `string`

If the `info` table was found, the script will have been loaded successfully.

If the returned module table contains an initializer function (`script_load`),
it will then be called.
Corespondingly, when the script is being unloaded, the `script_unload` function
will be called, if exists.

Example for a very basic no-op script (scripts/noop.lua):

    local noop = {
        info = {
            name = "noop",
            description = "does nothing",
            version = "anything, really"
        }
    }

    function noop.script_load()
    end

    function noop.script_unload()
    end

    return noop



Signals
========
Lua scripts are notified of events by means of signals they can subscribe to.
(see "Signal handling" below)

**Caveat**: due to the uncertainties involved in IRC networks, types encountered
in practice, in rare cases, may not be the same as documented below.

For example, a `"channel_message"` signal's first argument may be a
`luna.unknown_user` rather than a `luna.channel.user`, in case the
message came from outside the channel from an unknown user.
Another example would be a channel mode being set by an IRC service
such as ChanServ, which may not have a presence in the channel.

In general, due to `luna.channel.user` and `luna.unknown_user`
presenting the same interface and methods that more often than not do
the correct thing, this isn't a big problem, but should be kept
in mind.

If need be, a bound channel user can be distinguished from an
unbound user by testing for the existance of the `channel()` method.

Implemented signals
--------------------

### connect
Emitted when successfully connected to, and logged into, the IRC server (once
RPL\_WELCOME (numeric 001) is received).

No arguments.


### disconnect
Emitted when connection to the IRC server is lost in any way (QUIT, ERROR,
dead connections).

No arguments.


### tick
Emitted every 125 milliseconds (by default).

No arguments.


### raw
Emitted for every received message. No special treatment or handling is done, so
CTCPs will also be in raw format, all arguments referring to channels or users
will stay raw as well, and won't refer to objects representing the entities.

Arguments:

1. `message_prefix`: `string`
2. `message_command`: `string`
3. `message_arguments`: `list of string`

Examples:

    :nick!user@host KICK #channel victim :And stay out
    > raw("nick!user@host", "KICK", {"#channel", "victim", "And stay out"})

    :irc.example.org 001 yournick :Welcome
    > raw("irc.example.org", "001", {"yournick", "Welcome"})


### invite
Emitted when the client is invited to a channel.

Arguments:

1. `inviter`: `luna.unkown_user`
2. `channel`: `string`

### channel\_user\_sync
Emitted when the client has completely synchronized the channel's userlist.
(i.e. every user's nickname, username, hostname and channel modes are now known)

Arguments:

1. `channel`: `luna.channel`

### channel\_ban\_sync
Emitted when the client has completely synchronized the channel's banlist.

Arguments:

1. `channel`: `luna.channel`

### channel\_user\_join
Emitted when a user joins a channel.

Arguments:

1. `user`: `luna.channel.user`
2. `channel`: `luna.channel`


### channel\_user\_part
Emitted when a user leaves a channel.

Arguments:

1. `user`: `luna.channel.user`
2. `channel`: `luna.channel`
3. `reason`: `string`


### user\_quit
Emitted when a user quits IRC.

Arguments:

1. `user`: `luna.unknown_user`
2. `reason`: `string`


### nick\_change
Emitted when a user changes their nick.

Arguments:

1. `user`: `luna.unknown_user`
2. `new_nick`: `string`


### channel\_user\_kick
Emitted when a user is kicked from a channel.

Arguments:

1. `kicker`: `luna.channel.user`
2. `channel`: `luna.channel`
3. `kicked`: `luna.channel.user`
4. `reason`: `string`


### channel\_topic\_change
Emitted when a channel's topic changes.

Arguments:

1. `changer`: `luna.channel.user`
2. `channel`: `luna.channel`
3. `new_topic`: `string`


### channel\_message / channel\_notice / channel\_action
Emitted when a message, notice, or /me message is received in a channel.

Arguments:

1. `source`: `luna.channel.user`
2. `target`: `luna.channel`
3. `message`: `string`
4. `message_level`: `string?`

If non-`nil`, `message_level` indicates the "level" the message was sent to,
e.g. `"@"` if the message was sent to `"@#channel"` (i.e. channel ops only).


### message / notice / action
Emitted when a message, notice, or /me message is received in private.

Arguments:

1. `source`: `luna.unknown_user`
2. `message`: `string`


### channel\_ctcp\_request / channel\_ctcp\_response
Emitted when a CTCP request or response is received in a channel.

Arguments:

1. `source`: `luna.channel.user`
2. `target`: `luna.channel`
3. `ctcp`: `string`
4. `arg`: `string`
5. `level`: `string?`


### ctcp\_request / ctcp\_response
Emitted when a CTCP request or response is received in private.

Arguments:

1. `source`: `luna.unknown_user`
3. `ctcp`: `string`
4. `arg`: `string`


### channel\_mode
Emitted when a channel mode is set.

Arguments:

1. `source`: `luna.channel.user`
2. `target`: `luna.channel`
3. `mode`: `string`
4. `arg`: `string`


Signal handling
----------------
Signals can be subscribed to via a variety of functions.


### Low level signal handling

* `luna.add_signal_handler(signal: string,
                           id: string,
                           handler: function) -> string`

* `luna.add_signal_handler(signal: string, handler: function) -> string`

    Subscribe to the signal `signal`.

    If `id` is given, the handler will be registered with that ID, otherwise a
    new ID will be generated.

    Returns the handler ID.

    Example: print incoming channel messages to stdout.

        luna.add_signal_handler("channel_message", function(who, where, what)
            print(string.format("(%s) <%s> %s", where:name(), who:nick(), what)
        end)

        > "__unique_0_123"  -- example ID


* `luna.remove_signal_handler(id: string) -> nil`

    Remove the signal handler identified by `id`.

    Raises an error if no such signal handler is registered.

* `luna.remove_current_handler() -> nil`

    Remove the handler from which it was called.

* `luna.current_signal_handler() -> string`

    Returns the ID of the currently running handler.


Higher level signal handling
-----------------------------
* `luna.add_command(command: string, fn: function) -> nil`

* `luna.add_command(command: string, argtype: string, fn: function) -> nil`

    Register a command handler for the `channel_message` signal that
    automatically watches for triggers and sets the correct
    `luna.channel.user.respond()` formatting depending on the trigger used.

    By default, it will watch for whatever trigger is set for the channel in
    which a message was received (i.e. `!command <args...>` with the default
    trigger), or a "hilight" trigger (i.e. `clientnick: command <args...>`).

    `argtype` can be one of:
     * `"*w"`: take command arguments as a list of strings (default if `nil`)
     * `"*l"`: don't process the argument list and just take the remainder of
               the line.

* `luna.remove_command(command: string) -> nil`

    Remove the command handler for "command".

* `luna.add_message_watcher(pattern: string, fn: function) -> string`

    Register a message watcher. If a `channel_message` signal is fired and
    the message matches the supplied pattern, calls `fn` with any captures made.

    Example: watching for HTTP urls.

        luna.add_message_watcher("(https?://[^%s]+)",
            function(who, where, what, match_start, match_end, url)
                print("Found URL: " .. url .. "!")
            end)

    Returns the handler's ID.


* `luna.add_command_watcher(command: string, fn: function) -> string`

    Register a command watcher. Watches for raw events and calls `fn` each time
    an IRC message matching `command` arrives.

    Example: querying a user's channel list.

        luna.add_command_watcher("319", function(prefix, cmd, args)
            -- Do something with args
        end)

        luna.send_message("WHOIS", "user")

    Returns the handler's ID.


* `luna.add_timeout(seconds: number, fn: function) -> function, function`

    Register a timeout that gets executed after `seconds` seconds (which may
    be fractional).

    Returns (in order) a reset function and a cancel function. If the reset
    function is called, the countdown will reset to zero. If the cancel
    function is called, the timeout will be cancelled.


* `luna.add_timer(seconds: number, fn: function) -> string`

    Register a timer with a period of `seconds` seconds (which may be
    fractional).

    Returns the handler's ID.

