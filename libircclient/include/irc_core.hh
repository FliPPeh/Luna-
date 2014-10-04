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
#ifndef LIBIRCCLIENT_IRC_CORE_HH_INCLUDED
#define LIBIRCCLIENT_IRC_CORE_HH_INCLUDED

#include "macros.h"

#include <string>
#include <vector>
#include <iosfwd>

namespace irc {

//! List of defined numerics and textual commands as per the RFC.
namespace command {
    constexpr char const* RPL_WELCOME             = "001";
    constexpr char const* RPL_YOURHOST            = "002";
    constexpr char const* RPL_CREATED             = "003";
    constexpr char const* RPL_MYINFO              = "004";
    constexpr char const* RPL_ISUPPORT            = "005";
    constexpr char const* RPL_REDIR               = "010";
    constexpr char const* RPL_MAP                 = "015";
    constexpr char const* RPL_MAPMORE             = "016";
    constexpr char const* RPL_MAPEND              = "017";
    constexpr char const* RPL_YOURID              = "020";
    constexpr char const* RPL_TRACELINK           = "200";
    constexpr char const* RPL_TRACECONNECTING     = "201";
    constexpr char const* RPL_TRACEHANDSHAKE      = "202";
    constexpr char const* RPL_TRACEUNKNOWN        = "203";
    constexpr char const* RPL_TRACEOPERATOR       = "204";
    constexpr char const* RPL_TRACEUSER           = "205";
    constexpr char const* RPL_TRACESERVER         = "206";
    constexpr char const* RPL_TRACENEWTYPE        = "208";
    constexpr char const* RPL_TRACECLASS          = "209";
    constexpr char const* RPL_STATSLINKINFO       = "211";
    constexpr char const* RPL_STATSCOMMANDS       = "212";
    constexpr char const* RPL_STATSCLINE          = "213";
    constexpr char const* RPL_STATSNLINE          = "214";
    constexpr char const* RPL_STATSILINE          = "215";
    constexpr char const* RPL_STATSKLINE          = "216";
    constexpr char const* RPL_STATSQLINE          = "217";
    constexpr char const* RPL_STATSYLINE          = "218";
    constexpr char const* RPL_ENDOFSTATS          = "219";
    constexpr char const* RPL_STATSPLINE          = "220";
    constexpr char const* RPL_UMODEIS             = "221";
    constexpr char const* RPL_STATSFLINE          = "224";
    constexpr char const* RPL_STATSDLINE          = "225";
    constexpr char const* RPL_STATSALINE          = "226";
    constexpr char const* RPL_SERVLIST            = "234";
    constexpr char const* RPL_SERVLISTEND         = "235";
    constexpr char const* RPL_STATSLLINE          = "241";
    constexpr char const* RPL_STATSUPTIME         = "242";
    constexpr char const* RPL_STATSOLINE          = "243";
    constexpr char const* RPL_STATSHLINE          = "244";
    constexpr char const* RPL_STATSSLINE          = "245";
    constexpr char const* RPL_STATSXLINE          = "247";
    constexpr char const* RPL_STATSULINE          = "248";
    constexpr char const* RPL_STATSDEBUG          = "249";
    constexpr char const* RPL_STATSCONN           = "250";
    constexpr char const* RPL_LUSERCLIENT         = "251";
    constexpr char const* RPL_LUSEROP             = "252";
    constexpr char const* RPL_LUSERUNKNOWN        = "253";
    constexpr char const* RPL_LUSERCHANNELS       = "254";
    constexpr char const* RPL_LUSERME             = "255";
    constexpr char const* RPL_ADMINME             = "256";
    constexpr char const* RPL_ADMINLOC1           = "257";
    constexpr char const* RPL_ADMINLOC2           = "258";
    constexpr char const* RPL_ADMINEMAIL          = "259";
    constexpr char const* RPL_TRACELOG            = "261";
    constexpr char const* RPL_ENDOFTRACE          = "262";
    constexpr char const* RPL_LOAD2HI             = "263";
    constexpr char const* RPL_LOCALUSERS          = "265";
    constexpr char const* RPL_GLOBALUSERS         = "266";
    constexpr char const* RPL_VCHANEXIST          = "276";
    constexpr char const* RPL_VCHANLIST           = "277";
    constexpr char const* RPL_VCHANHELP           = "278";
    constexpr char const* RPL_ACCEPTLIST          = "281";
    constexpr char const* RPL_ENDOFACCEPT         = "282";
    constexpr char const* RPL_NONE                = "300";
    constexpr char const* RPL_AWAY                = "301";
    constexpr char const* RPL_USERHOST            = "302";
    constexpr char const* RPL_ISON                = "303";
    constexpr char const* RPL_TEXT                = "304";
    constexpr char const* RPL_UNAWAY              = "305";
    constexpr char const* RPL_NOWAWAY             = "306";
    constexpr char const* RPL_USERIP              = "307";
    constexpr char const* RPL_WHOISUSER           = "311";
    constexpr char const* RPL_WHOISSERVER         = "312";
    constexpr char const* RPL_WHOISOPERATOR       = "313";
    constexpr char const* RPL_WHOWASUSER          = "314";
    constexpr char const* RPL_ENDOFWHOWAS         = "369";
    constexpr char const* RPL_WHOISCHANOP         = "316";
    constexpr char const* RPL_WHOISIDLE           = "317";
    constexpr char const* RPL_ENDOFWHOIS          = "318";
    constexpr char const* RPL_WHOISCHANNELS       = "319";
    constexpr char const* RPL_LISTSTART           = "321";
    constexpr char const* RPL_LIST                = "322";
    constexpr char const* RPL_LISTEND             = "323";
    constexpr char const* RPL_CHANNELMODEIS       = "324";
    constexpr char const* RPL_CREATIONTIME        = "329";
    constexpr char const* RPL_NOTOPIC             = "331";
    constexpr char const* RPL_TOPIC               = "332";
    constexpr char const* RPL_TOPICWHOTIME        = "333";
    constexpr char const* RPL_WHOISACTUALLY       = "338";
    constexpr char const* RPL_INVITING            = "341";
    constexpr char const* RPL_INVITELIST          = "346";
    constexpr char const* RPL_ENDOFINVITELIST     = "347";
    constexpr char const* RPL_EXCEPTLIST          = "348";
    constexpr char const* RPL_ENDOFEXCEPTLIST     = "349";
    constexpr char const* RPL_VERSION             = "351";
    constexpr char const* RPL_WHOREPLY            = "352";
    constexpr char const* RPL_ENDOFWHO            = "315";
    constexpr char const* RPL_NAMREPLY            = "353";
    constexpr char const* RPL_ENDOFNAMES          = "366";
    constexpr char const* RPL_KILLDONE            = "361";
    constexpr char const* RPL_CLOSING             = "362";
    constexpr char const* RPL_CLOSEEND            = "363";
    constexpr char const* RPL_LINKS               = "364";
    constexpr char const* RPL_ENDOFLINKS          = "365";
    constexpr char const* RPL_BANLIST             = "367";
    constexpr char const* RPL_ENDOFBANLIST        = "368";
    constexpr char const* RPL_INFO                = "371";
    constexpr char const* RPL_MOTD                = "372";
    constexpr char const* RPL_INFOSTART           = "373";
    constexpr char const* RPL_ENDOFINFO           = "374";
    constexpr char const* RPL_MOTDSTART           = "375";
    constexpr char const* RPL_ENDOFMOTD           = "376";
    constexpr char const* RPL_YOUREOPER           = "381";
    constexpr char const* RPL_REHASHING           = "382";
    constexpr char const* RPL_MYPORTIS            = "384";
    constexpr char const* RPL_NOTOPERANYMORE      = "385";
    constexpr char const* RPL_RSACHALLENGE        = "386";
    constexpr char const* RPL_TIME                = "391";
    constexpr char const* RPL_USERSSTART          = "392";
    constexpr char const* RPL_USERS               = "393";
    constexpr char const* RPL_ENDOFUSERS          = "394";
    constexpr char const* RPL_NOUSERS             = "395";
    constexpr char const* ERR_NOSUCHNICK          = "401";
    constexpr char const* ERR_NOSUCHSERVER        = "402";
    constexpr char const* ERR_NOSUCHCHANNEL       = "403";
    constexpr char const* ERR_CANNOTSENDTOCHAN    = "404";
    constexpr char const* ERR_TOOMANYCHANNELS     = "405";
    constexpr char const* ERR_WASNOSUCHNICK       = "406";
    constexpr char const* ERR_TOOMANYTARGETS      = "407";
    constexpr char const* ERR_NOORIGIN            = "409";
    constexpr char const* ERR_NORECIPIENT         = "411";
    constexpr char const* ERR_NOTEXTTOSEND        = "412";
    constexpr char const* ERR_NOTOPLEVEL          = "413";
    constexpr char const* ERR_WILDTOPLEVEL        = "414";
    constexpr char const* ERR_UNKNOWNCOMMAND      = "421";
    constexpr char const* ERR_NOMOTD              = "422";
    constexpr char const* ERR_NOADMININFO         = "423";
    constexpr char const* ERR_FILEERROR           = "424";
    constexpr char const* ERR_NONICKNAMEGIVEN     = "431";
    constexpr char const* ERR_ERRONEUSNICKNAME    = "432";
    constexpr char const* ERR_NICKNAMEINUSE       = "433";
    constexpr char const* ERR_NICKCOLLISION       = "436";
    constexpr char const* ERR_UNAVAILRESOURCE     = "437";
    constexpr char const* ERR_NICKTOOFAST         = "438";
    constexpr char const* ERR_USERNOTINCHANNEL    = "441";
    constexpr char const* ERR_NOTONCHANNEL        = "442";
    constexpr char const* ERR_USERONCHANNEL       = "443";
    constexpr char const* ERR_NOLOGIN             = "444";
    constexpr char const* ERR_SUMMONDISABLED      = "445";
    constexpr char const* ERR_USERSDISABLED       = "446";
    constexpr char const* ERR_NOTREGISTERED       = "451";
    constexpr char const* ERR_ACCEPTFULL          = "456";
    constexpr char const* ERR_ACCEPTEXIST         = "457";
    constexpr char const* ERR_ACCEPTNOT           = "458";
    constexpr char const* ERR_NEEDMOREPARAMS      = "461";
    constexpr char const* ERR_ALREADYREGISTRED    = "462";
    constexpr char const* ERR_NOPERMFORHOST       = "463";
    constexpr char const* ERR_PASSWDMISMATCH      = "464";
    constexpr char const* ERR_YOUREBANNEDCREEP    = "465";
    constexpr char const* ERR_YOUWILLBEBANNED     = "466";
    constexpr char const* ERR_KEYSET              = "467";
    constexpr char const* ERR_CHANNELISFULL       = "471";
    constexpr char const* ERR_UNKNOWNMODE         = "472";
    constexpr char const* ERR_INVITEONLYCHAN      = "473";
    constexpr char const* ERR_BANNEDFROMCHAN      = "474";
    constexpr char const* ERR_BADCHANNELKEY       = "475";
    constexpr char const* ERR_BADCHANMASK         = "476";
    constexpr char const* ERR_MODELESS            = "477";
    constexpr char const* ERR_BANLISTFULL         = "478";
    constexpr char const* ERR_BADCHANNAME         = "479";
    constexpr char const* ERR_NOPRIVILEGES        = "481";
    constexpr char const* ERR_CHANOPRIVSNEEDED    = "482";
    constexpr char const* ERR_CANTKILLSERVER      = "483";
    constexpr char const* ERR_RESTRICTED          = "484";
    constexpr char const* ERR_BANNEDNICK          = "485";
    constexpr char const* ERR_NOOPERHOST          = "491";
    constexpr char const* ERR_UMODEUNKNOWNFLAG    = "501";
    constexpr char const* ERR_USERSDONTMATCH      = "502";
    constexpr char const* ERR_GHOSTEDCLIENT       = "503";
    constexpr char const* ERR_USERNOTONSERV       = "504";
    constexpr char const* ERR_VCHANDISABLED       = "506";
    constexpr char const* ERR_ALREADYONVCHAN      = "507";
    constexpr char const* ERR_WRONGPONG           = "513";
    constexpr char const* ERR_LONGMASK            = "518";
    constexpr char const* ERR_HELPNOTFOUND        = "524";
    constexpr char const* RPL_MODLIST             = "702";
    constexpr char const* RPL_ENDOFMODLIST        = "703";
    constexpr char const* RPL_HELPSTART           = "704";
    constexpr char const* RPL_HELPTXT             = "705";
    constexpr char const* RPL_ENDOFHELP           = "706";
    constexpr char const* RPL_KNOCK               = "710";
    constexpr char const* RPL_KNOCKDLVR           = "711";
    constexpr char const* ERR_TOOMANYKNOCK        = "712";
    constexpr char const* ERR_CHANOPEN            = "713";
    constexpr char const* ERR_KNOCKONCHAN         = "714";
    constexpr char const* ERR_KNOCKDISABLED       = "715";
    constexpr char const* ERR_LAST_ERR_MSG        = "999";

    ///
    // Most commands we could possibly receive as a client plus their format
    // according to RFC 2812.

    ///
    // 3.1 (Connection Registration)

    // <password>
    constexpr char const* PASS     = "PASS";

    // <nickname>
    constexpr char const* NICK     = "NICK";

    // <user> <mode> <unused> <real>
    constexpr char const* USER     = "USER";

    // <user> <password>
    constexpr char const* OPER     = "OPER";

    // <nickname> <reserved> <distribution> <type> <reserved> <info>
    constexpr char const* SERVICE  = "SERVICE";

    // [<quit message>]
    constexpr char const* QUIT     = "QUIT";

    // <server> <comment>
    constexpr char const* SQUIT    = "SQUIT";

    ///
    // 3.2 (Channel operations)

    // <channel>{,<channel>} [<key>{,<key>}]
    constexpr char const* JOIN     = "JOIN";

    // <channel>{,<channel>} [<message>]
    constexpr char const* PART     = "PART";

    // <channel> <modestr> [{<argument>}]  |  <nickname> <modestr>
    constexpr char const* MODE     = "MODE";

    // <channel> [<topic>]
    constexpr char const* TOPIC    = "TOPIC";

    // [<channel>{,<channel>}]
    constexpr char const* NAMES    = "NAMES";

    // [<channel>{,<channel>} [<server>]]
    constexpr char const* LIST     = "LIST";

    // <nickname> <channel>
    constexpr char const* INVITE   = "INVITE";

    // <channel> <user> [<comment>]
    constexpr char const* KICK     = "KICK";

    ///
    // 3.3 (Sending messages)

    // <receiver>{,<receiver>} <text>
    constexpr char const* PRIVMSG  = "PRIVMSG";

    // <nickname> <text>
    constexpr char const* NOTICE   = "NOTICE";

    /// 3.4 (Server queries and commands)
    //

    // [<target>]
    constexpr char const* MOTD     = "MOTD";

    // [<mask> [<target>]]
    constexpr char const* LUSERS   = "LUSERS";

    // [<server>]
    constexpr char const* VERSION  = "VERSION";

    // [<query> [<server>]]
    constexpr char const* STATS    = "STATS";

    // [[<remote server>] <server mask>]
    constexpr char const* LINKS    = "LINKS";

    // [<server>]
    constexpr char const* TIME     = "TIME";

    // <target server> [<port> [<remote server>]]
    constexpr char const* CONNECT  = "CONNECT";

    // [<server>]
    constexpr char const* TRACE    = "TRACE";

    // [<server>]
    constexpr char const* ADMIN    = "ADMIN";

    // [<server>]
    constexpr char const* INFO     = "INFO";

    /// 3.5 (Service Query and Commands)
    //

    // [<mask> [<type>]]
    constexpr char const* SERVLIST = "SERVLIST";

    // <servicename> <text>
    constexpr char const* SQUERY   = "SQUERY";

    /// 3.6 (User based queries)
    //

    // [<name> [<o>]]
    constexpr char const* WHO      = "WHO";

    // [<server>] <nickmask>[,<nickmask>[,...]]
    constexpr char const* WHOIS    = "WHOIS";

    // <nickname> [<count> [<server>]]
    constexpr char const* WHOWAS   = "WHOWAS";

    /// 3.7 (Miscellaneous messages)
    //

    // <nickname> <comment>
    constexpr char const* KILL     = "KILL";

    // <server1> [<server2>]
    constexpr char const* PING     = "PING";

    // <server1> [<server2>]
    constexpr char const* PONG     = "PONG";

    // <error message>
    constexpr char const* ERROR    = "ERROR";

    /// 4.0 Optionals
    //

    // [message]
    constexpr char const* AWAY     = "AWAY";

    constexpr char const* REHASH   = "REHASH";
    constexpr char const* DIE      = "DIE";
    constexpr char const* RESTART  = "RESTART";

    // <user> [<target> [<channel>]]
    constexpr char const* SUMMON   = "SUMMON";

    // [<server>]
    constexpr char const* USERS    = "USERS";

    // <text>
    constexpr char const* WALLOPS  = "WALLOPS";

    // <nickname>{<space><nickname>}
    constexpr char const* USERHOST = "USERHOST";

    // <nickname>{<space><nickname>}
    constexpr char const* ISON     = "ISON";

} // namespace command


//! The internal representation of an IRC message and its parts.
struct DLL_PUBLIC message {
    std::string prefix;     //!< User or server prefix of the message origin.
    std::string command;    //!< Command or numeric of the message.

    std::vector<std::string> args;  //!< Message arguments (including trailing)
};

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

#endif // defined LIBIRCCLIENT_IRC_CORE_HH_INCLUDED
