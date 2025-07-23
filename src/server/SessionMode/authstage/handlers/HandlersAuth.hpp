#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/authstage/opcodes/AuthPacket.hpp"

namespace HandlersAuth {
    void dispatch(std::shared_ptr<ClientSession> session, AuthPacket &p);

    void handle_ping(std::shared_ptr<ClientSession> session, AuthPacket &p);

    boost::asio::awaitable<void> handle_logon_challenge(std::shared_ptr<ClientSession> session, AuthPacket &p);

    void handle_logon_proof(std::shared_ptr<ClientSession> session, AuthPacket &p);
//
//    void handle_reconnect_challenge(std::shared_ptr<ClientSession> session, AuthPacket &p);
//
//    void handle_reconnect_proof(std::shared_ptr<ClientSession> session, AuthPacket &p);
//
//    void handle_realm_list(std::shared_ptr<ClientSession> session, AuthPacket &p);
}