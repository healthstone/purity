#pragma once

#include <cstdint>

enum class Opcode : uint16_t {
    // --- BNCS / BNET ---
    SID_NULL                      = 0x00,
    SID_PING                      = 0x25,
    SID_AUTH_INFO                 = 0x50,
    SID_AUTH_CHECK                = 0x51,
    SID_AUTH_ACCOUNTLOGON         = 0x53,
    SID_AUTH_ACCOUNTLOGONPROOF    = 0x54,
    SID_AUTH_ACCOUNTCREATE        = 0x55,
    SID_AUTH_ACCOUNTCHANGE        = 0x56,
    SID_CHATCOMMAND               = 0x0E,
    SID_MESSAGEBOX                = 0x19,
    SID_ENTERCHAT                 = 0x0A,
    SID_JOINCHANNEL               = 0x0C,
    SID_GETCHANNELLIST            = 0x0B,
    SID_CHATEVENT                 = 0x0F,

    // --- Warcraft III W3Route ---
    CLIENT_W3ROUTE_REQ            = 0x1EF7,
    SERVER_W3ROUTE_READY          = 0x14F7,
    CLIENT_W3ROUTE_LOADINGDONE    = 0x23F7,
    SERVER_W3ROUTE_LOADINGACK     = 0x08F7,
    SERVER_W3ROUTE_STARTGAME1     = 0x0AF7,
    SERVER_W3ROUTE_STARTGAME2     = 0x0BF7,
    CLIENT_W3ROUTE_ABORT          = 0x21F7,
    CLIENT_W3ROUTE_CONNECTED      = 0x3BF7,
    SERVER_W3ROUTE_PLAYERINFO     = 0x06F7,
    SERVER_W3ROUTE_LEVELINFO      = 0x47F7,
    SERVER_W3ROUTE_ACK            = 0x04F7,
    SERVER_W3ROUTE_ECHOREQ        = 0x01F7,
    CLIENT_W3ROUTE_ECHOREPLY      = 0x46F7,
    CLIENT_W3ROUTE_GAMERESULT     = 0x2EF7,
    CLIENT_W3ROUTE_GAMERESULT_W3XP = 0x3AF7
};
