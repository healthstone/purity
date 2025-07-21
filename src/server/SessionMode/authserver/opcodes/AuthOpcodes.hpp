#pragma once

#include <cstdint>

/// TrinityCore style Auth opcodes for WoW 3.3.5a
enum class AuthOpcode : uint8_t {
    // Client -> Server (CMSG = Client Message)
    AUTH_CMSG_LOGON_CHALLENGE = 0x00,  ///< Client requests a logon challenge
    AUTH_CMSG_LOGON_PROOF     = 0x01,  ///< Client sends proof for logon challenge
    AUTH_CMSG_RECONNECT_CHALLENGE = 0x02, ///< Client requests reconnect challenge
    AUTH_CMSG_RECONNECT_PROOF = 0x03,  ///< Client sends reconnect proof
    AUTH_CMSG_REALM_LIST      = 0x10,  ///< Client requests realm list
    AUTH_CMSG_PING            = 0x06,  ///< Client sends ping

    // Server -> Client (SMSG = Server Message)
    AUTH_SMSG_LOGON_CHALLENGE = 0x00,  ///< Server replies with logon challenge
    AUTH_SMSG_LOGON_PROOF     = 0x01,  ///< Server replies with logon proof result
    AUTH_SMSG_RECONNECT_CHALLENGE = 0x02, ///< Server sends reconnect challenge
    AUTH_SMSG_RECONNECT_PROOF = 0x03,  ///< Server replies with reconnect proof result
    AUTH_SMSG_REALM_LIST      = 0x10,  ///< Server sends realm list
    AUTH_SMSG_PONG            = 0x07,  ///< Server replies to ping

    // Extra / Legacy / Placeholder (if needed)
    UNKNOWN                   = 0xFF   ///< Fallback for unknown opcodes
};
