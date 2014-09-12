/*
 * Copyright 2014 Lukas Niederbremer
 *
 * This file is part of libircclient.
 *
 * libircclient is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * libircclient is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libircclient.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef CORE_H
#define CORE_H

#include "macros.h"
#include "util.h"

#include <string>
#include <vector>
#include <iosfwd>

namespace irc {

//! List of defined numerics and textual commands as per the RFC.
enum command : int {
    RPL_WELCOME             =   1,
    RPL_YOURHOST            =   2,
    RPL_CREATED             =   3,
    RPL_MYINFO              =   4,
    RPL_ISUPPORT            =   5,
    RPL_REDIR               =  10,
    RPL_MAP                 =  15,
    RPL_MAPMORE             =  16,
    RPL_MAPEND              =  17,
    RPL_YOURID              =  20,
    RPL_TRACELINK           = 200,
    RPL_TRACECONNECTING     = 201,
    RPL_TRACEHANDSHAKE      = 202,
    RPL_TRACEUNKNOWN        = 203,
    RPL_TRACEOPERATOR       = 204,
    RPL_TRACEUSER           = 205,
    RPL_TRACESERVER         = 206,
    RPL_TRACENEWTYPE        = 208,
    RPL_TRACECLASS          = 209,
    RPL_STATSLINKINFO       = 211,
    RPL_STATSCOMMANDS       = 212,
    RPL_STATSCLINE          = 213,
    RPL_STATSNLINE          = 214,
    RPL_STATSILINE          = 215,
    RPL_STATSKLINE          = 216,
    RPL_STATSQLINE          = 217,
    RPL_STATSYLINE          = 218,
    RPL_ENDOFSTATS          = 219,
    RPL_STATSPLINE          = 220,
    RPL_UMODEIS             = 221,
    RPL_STATSFLINE          = 224,
    RPL_STATSDLINE          = 225,
    RPL_STATSALINE          = 226,
    RPL_SERVLIST            = 234,
    RPL_SERVLISTEND         = 235,
    RPL_STATSLLINE          = 241,
    RPL_STATSUPTIME         = 242,
    RPL_STATSOLINE          = 243,
    RPL_STATSHLINE          = 244,
    RPL_STATSSLINE          = 245,
    RPL_STATSXLINE          = 247,
    RPL_STATSULINE          = 248,
    RPL_STATSDEBUG          = 249,
    RPL_STATSCONN           = 250,
    RPL_LUSERCLIENT         = 251,
    RPL_LUSEROP             = 252,
    RPL_LUSERUNKNOWN        = 253,
    RPL_LUSERCHANNELS       = 254,
    RPL_LUSERME             = 255,
    RPL_ADMINME             = 256,
    RPL_ADMINLOC1           = 257,
    RPL_ADMINLOC2           = 258,
    RPL_ADMINEMAIL          = 259,
    RPL_TRACELOG            = 261,
    RPL_ENDOFTRACE          = 262,
    RPL_LOAD2HI             = 263,
    RPL_LOCALUSERS          = 265,
    RPL_GLOBALUSERS         = 266,
    RPL_VCHANEXIST          = 276,
    RPL_VCHANLIST           = 277,
    RPL_VCHANHELP           = 278,
    RPL_ACCEPTLIST          = 281,
    RPL_ENDOFACCEPT         = 282,
    RPL_NONE                = 300,
    RPL_AWAY                = 301,
    RPL_USERHOST            = 302,
    RPL_ISON                = 303,
    RPL_TEXT                = 304,
    RPL_UNAWAY              = 305,
    RPL_NOWAWAY             = 306,
    RPL_USERIP              = 307,
    RPL_WHOISUSER           = 311,
    RPL_WHOISSERVER         = 312,
    RPL_WHOISOPERATOR       = 313,
    RPL_WHOWASUSER          = 314,
    RPL_ENDOFWHOWAS         = 369,
    RPL_WHOISCHANOP         = 316,
    RPL_WHOISIDLE           = 317,
    RPL_ENDOFWHOIS          = 318,
    RPL_WHOISCHANNELS       = 319,
    RPL_LISTSTART           = 321,
    RPL_LIST                = 322,
    RPL_LISTEND             = 323,
    RPL_CHANNELMODEIS       = 324,
    RPL_CREATIONTIME        = 329,
    RPL_NOTOPIC             = 331,
    RPL_TOPIC               = 332,
    RPL_TOPICWHOTIME        = 333,
    RPL_WHOISACTUALLY       = 338,
    RPL_INVITING            = 341,
    RPL_INVITELIST          = 346,
    RPL_ENDOFINVITELIST     = 347,
    RPL_EXCEPTLIST          = 348,
    RPL_ENDOFEXCEPTLIST     = 349,
    RPL_VERSION             = 351,
    RPL_WHOREPLY            = 352,
    RPL_ENDOFWHO            = 315,
    RPL_NAMREPLY            = 353,
    RPL_ENDOFNAMES          = 366,
    RPL_KILLDONE            = 361,
    RPL_CLOSING             = 362,
    RPL_CLOSEEND            = 363,
    RPL_LINKS               = 364,
    RPL_ENDOFLINKS          = 365,
    RPL_BANLIST             = 367,
    RPL_ENDOFBANLIST        = 368,
    RPL_INFO                = 371,
    RPL_MOTD                = 372,
    RPL_INFOSTART           = 373,
    RPL_ENDOFINFO           = 374,
    RPL_MOTDSTART           = 375,
    RPL_ENDOFMOTD           = 376,
    RPL_YOUREOPER           = 381,
    RPL_REHASHING           = 382,
    RPL_MYPORTIS            = 384,
    RPL_NOTOPERANYMORE      = 385,
    RPL_RSACHALLENGE        = 386,
    RPL_TIME                = 391,
    RPL_USERSSTART          = 392,
    RPL_USERS               = 393,
    RPL_ENDOFUSERS          = 394,
    RPL_NOUSERS             = 395,
    ERR_NOSUCHNICK          = 401,
    ERR_NOSUCHSERVER        = 402,
    ERR_NOSUCHCHANNEL       = 403,
    ERR_CANNOTSENDTOCHAN    = 404,
    ERR_TOOMANYCHANNELS     = 405,
    ERR_WASNOSUCHNICK       = 406,
    ERR_TOOMANYTARGETS      = 407,
    ERR_NOORIGIN            = 409,
    ERR_NORECIPIENT         = 411,
    ERR_NOTEXTTOSEND        = 412,
    ERR_NOTOPLEVEL          = 413,
    ERR_WILDTOPLEVEL        = 414,
    ERR_UNKNOWNCOMMAND      = 421,
    ERR_NOMOTD              = 422,
    ERR_NOADMININFO         = 423,
    ERR_FILEERROR           = 424,
    ERR_NONICKNAMEGIVEN     = 431,
    ERR_ERRONEUSNICKNAME    = 432,
    ERR_NICKNAMEINUSE       = 433,
    ERR_NICKCOLLISION       = 436,
    ERR_UNAVAILRESOURCE     = 437,
    ERR_NICKTOOFAST         = 438,
    ERR_USERNOTINCHANNEL    = 441,
    ERR_NOTONCHANNEL        = 442,
    ERR_USERONCHANNEL       = 443,
    ERR_NOLOGIN             = 444,
    ERR_SUMMONDISABLED      = 445,
    ERR_USERSDISABLED       = 446,
    ERR_NOTREGISTERED       = 451,
    ERR_ACCEPTFULL          = 456,
    ERR_ACCEPTEXIST         = 457,
    ERR_ACCEPTNOT           = 458,
    ERR_NEEDMOREPARAMS      = 461,
    ERR_ALREADYREGISTRED    = 462,
    ERR_NOPERMFORHOST       = 463,
    ERR_PASSWDMISMATCH      = 464,
    ERR_YOUREBANNEDCREEP    = 465,
    ERR_YOUWILLBEBANNED     = 466,
    ERR_KEYSET              = 467,
    ERR_CHANNELISFULL       = 471,
    ERR_UNKNOWNMODE         = 472,
    ERR_INVITEONLYCHAN      = 473,
    ERR_BANNEDFROMCHAN      = 474,
    ERR_BADCHANNELKEY       = 475,
    ERR_BADCHANMASK         = 476,
    ERR_MODELESS            = 477,
    ERR_BANLISTFULL         = 478,
    ERR_BADCHANNAME         = 479,
    ERR_NOPRIVILEGES        = 481,
    ERR_CHANOPRIVSNEEDED    = 482,
    ERR_CANTKILLSERVER      = 483,
    ERR_RESTRICTED          = 484,
    ERR_BANNEDNICK          = 485,
    ERR_NOOPERHOST          = 491,
    ERR_UMODEUNKNOWNFLAG    = 501,
    ERR_USERSDONTMATCH      = 502,
    ERR_GHOSTEDCLIENT       = 503,
    ERR_USERNOTONSERV       = 504,
    ERR_VCHANDISABLED       = 506,
    ERR_ALREADYONVCHAN      = 507,
    ERR_WRONGPONG           = 513,
    ERR_LONGMASK            = 518,
    ERR_HELPNOTFOUND        = 524,
    RPL_MODLIST             = 702,
    RPL_ENDOFMODLIST        = 703,
    RPL_HELPSTART           = 704,
    RPL_HELPTXT             = 705,
    RPL_ENDOFHELP           = 706,
    RPL_KNOCK               = 710,
    RPL_KNOCKDLVR           = 711,
    ERR_TOOMANYKNOCK        = 712,
    ERR_CHANOPEN            = 713,
    ERR_KNOCKONCHAN         = 714,
    ERR_KNOCKDISABLED       = 715,
    ERR_LAST_ERR_MSG        = 999,

    // Most commands we could possibly receive as a client plus their format
    // according to RFC 2812.
    //
    // 3.1 (Connection Registration)
    PASS,    // <password>
    NICK,    // <nickname>
    USER,    // <user> <mode> <unused> <real>
    OPER,    // <user> <password>
    SERVICE, // <nickname> <reserved> <distribution> <type> <reserved> <info>
    QUIT,    // [<quit message>]
    SQUIT,   // <server> <comment>

    // 3.2 (Channel operations)
    JOIN,    // <channel>{,<channel>} [<key>{,<key>}]
    PART,    // <channel>{,<channel>} [<message>]
    MODE,    // <channel> <modestr> [{<argument>}]  |  <nickname> <modestr>
    TOPIC,   // <channel> [<topic>]
    NAMES,   // [<channel>{,<channel>}]
    LIST,    // [<channel>{,<channel>} [<server>]]
    INVITE,  // <nickname> <channel>
    KICK,    // <channel> <user> [<comment>]

    // 3.3 (Sending messages)
    PRIVMSG, // <receiver>{,<receiver>} <text>
    NOTICE,  // <nickname> <text>

    // 3.4 (Server queries and commands)
    MOTD,    // [<target>]
    LUSERS,  // [<mask> [<target>]]
    VERSION, // [<server>]
    STATS,   // [<query> [<server>]]
    LINKS,   // [[<remote server>] <server mask>]
    TIME,    // [<server>]
    CONNECT, // <target server> [<port> [<remote server>]]
    TRACE,   // [<server>]
    ADMIN,   // [<server>]
    INFO,    // [<server>]

    // 3.5 (Service Query and Commands)
    SERVLIST,// [<mask> [<type>]]
    SQUERY,  // <servicename> <text>

    // 3.6 (User based queries)
    WHO,     // [<name> [<o>]]
    WHOIS,   // [<server>] <nickmask>[,<nickmask>[,...]]
    WHOWAS,  // <nickname> [<count> [<server>]]

    // 3.7 (Miscellaneous messages)
    KILL,    // <nickname> <comment>
    PING,    // <server1> [<server2>]
    PONG,    // <server1> [<server2>]
    ERROR,   // <error message>

    // 4.0 Optionals
    AWAY,    // [message]
    REHASH,
    DIE,
    RESTART,
    SUMMON,  // <user> [<target> [<channel>]]
    USERS,   // [<server>]
    WALLOPS, // <text>
    USERHOST,// <nickname>{<space><nickname>}
    ISON,    // <nickname>{<space><nickname>}

    // Enum end for command_names
    COMMAND_MAX
};

//! Error types used to identify IRC exception types.
enum class protocol_error_type {
    invalid_message,
    invalid_command,
    invalid_prefix,
    login_error,
    no_such_channel,
    no_such_user,
    no_such_mode,
    not_enough_arguments
};

template <typename T>
char const* typed_error_meaning(protocol_error_type t)
{
    switch (t) {
    case protocol_error_type::invalid_message:
        return "invalid message";

    case protocol_error_type::invalid_command:
        return "invalid command";

    case protocol_error_type::invalid_prefix:
        return "invalid prefix";

    case protocol_error_type::login_error:
        return "login error";

    case protocol_error_type::no_such_channel:
        return "no such channel";

    case protocol_error_type::no_such_user:
        return "no such user";

    case protocol_error_type::no_such_mode:
        return "no such mode";

    case protocol_error_type::not_enough_arguments:
        return "not enough parameters";
    }

    return ""; // Not reached
}

//! IRC exception type.
using protocol_error = typed_error<protocol_error_type>;


//! The internal representation of an IRC message and its parts.
struct DLL_PUBLIC message {
    std::string prefix;     //!< User or server prefix of the message origin.
    enum command command;   //!< Command or numeric of the message.

    std::vector<std::string> args;  //!< Message arguments (including trailing)
};

/*!
 * Converts the command enum into its human (and machine) readable string.
 * Numerics will be converted into their three digit string form.
 *
 * \param cmd Command to convert to string.
 * \return Converted string.
 */
extern DLL_PUBLIC
std::string to_string(enum command cmd);

/*!
 * Converts the command string into its enum representation.
 *
 * \param cmd Command to convert to enum
 * \return Converted enum.
 */
extern DLL_PUBLIC
enum command command_from_string(std::string const& str);

extern DLL_PUBLIC
inline std::ostream& operator<<(std::ostream& strm, enum command cmd)
{
    strm << to_string(cmd);
    return strm;
}

/*!
 * Converts a message into its protocol serialization.
 *
 * \param msg The message to stringify.
 * \return The stringified message.
 */
extern DLL_PUBLIC
std::string to_string(message const& msg);

/*!
 * Converts a serialized message into its internal representation.
 *
 * \param str The message to unstringify.
 * \return The unstringified message.
 */
extern DLL_PUBLIC
message message_from_string(std::string str);

extern DLL_PUBLIC
inline std::ostream& operator<<(std::ostream& strm, message const& msg)
{
    strm << to_string(msg);
    return strm;
}

}

#endif // defined CORE_H
