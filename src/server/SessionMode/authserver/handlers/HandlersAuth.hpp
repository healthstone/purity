#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/authserver/opcodes/AuthPacket.hpp"

namespace HandlersAuth {
    void dispatch(std::shared_ptr<ClientSession> session, AuthPacket &p);

    void handle_ping(std::shared_ptr<ClientSession> session, AuthPacket &p);
}