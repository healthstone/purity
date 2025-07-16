#include <memory>
#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/bncs/opcodes/opcodes8.hpp"
#include "src/server/SessionMode/bncs/opcodes/BNETPacket8.hpp"

namespace HandlersBNCS {

    void dispatch(std::shared_ptr<ClientSession> session, BNETPacket8 &p);

    void handle_sid_null(std::shared_ptr<ClientSession> session, BNETPacket8 &p);

    void handle_sid_init(std::shared_ptr<ClientSession> session, BNETPacket8 &p);

    void handle_ping(std::shared_ptr<ClientSession> session, BNETPacket8 &p);

    void handle_auth_info(std::shared_ptr<ClientSession> session, BNETPacket8 &p);
}
