#pragma once

#include <cstdint>

enum class AuthOpcodes : uint16_t {
    EMPTY_STAGE          = 0x00, // for clear stage

    CMSG_PING            = 0x01,  ///< Client sends ping
    SMSG_PONG            = 0x02,  ///< Server replies to ping

    CMSG_AUTH_LOGON_CHALLENGE     = 0x03,
    SMSG_AUTH_LOGON_CHALLENGE     = 0x04,
    CMSG_AUTH_LOGON_PROOF         = 0x05,
    SMSG_AUTH_LOGON_PROOF         = 0x06,
};