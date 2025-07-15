#pragma once

enum class Opcode : uint16_t {
    CMSG_CHAT_MESSAGE = 1,

    CMSG_PING = 2,
    SMSG_PONG = 3,

    CMSG_ACCOUNT_LOOKUP_BY_NAME = 4,
    SMSG_ACCOUNT_LOOKUP_BY_NAME = 5,

    CMSG_ACCOUNT_INSERT = 6,
    SMSG_ACCOUNT_INSERT = 7
};