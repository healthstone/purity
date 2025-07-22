#pragma once

#include <cstdint>

/// TrinityCore style Auth opcodes for WoW 3.3.5a
enum class AuthCmd : uint8_t {
    AUTH_LOGON_CHALLENGE     = 0x00,
    AUTH_LOGON_PROOF         = 0x01,
    AUTH_RECONNECT_CHALLENGE = 0x02,
    AUTH_RECONNECT_PROOF     = 0x03,
    REALM_LIST               = 0x10,

    XFER_INITIATE            = 0x30,
    XFER_DATA                = 0x31,
    XFER_ACCEPT              = 0x32,
    XFER_RESUME              = 0x33,
    XFER_CANCEL              = 0x34,

    AUTH_CMSG_PING            = 0x06,  ///< Client sends ping
    AUTH_SMSG_PONG            = 0x07,  ///< Server replies to ping
};
