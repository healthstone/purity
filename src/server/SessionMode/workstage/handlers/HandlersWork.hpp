#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/workstage/opcodes/WorkPacket.hpp"

namespace HandlersWork {
    void dispatch(std::shared_ptr<ClientSession> session, WorkPacket &p);

    void handle_ping(std::shared_ptr<ClientSession> session, WorkPacket &p);

    void handle_message(std::shared_ptr<ClientSession> session, WorkPacket &p);
}