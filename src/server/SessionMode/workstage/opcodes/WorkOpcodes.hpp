#pragma once

#include <cstdint>

enum class WorkOpcodes : uint16_t {
    EMPTY_STAGE = 0x00, // for clear stage

    CMSG_PING = 0x01,  ///< Client sends ping
    SMSG_PONG = 0x02,  ///< Server replies to ping

    CMSG_MESSAGE = 0x03,
    SMSG_MESSAGE = 0x04,
};