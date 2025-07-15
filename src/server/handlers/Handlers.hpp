#include <memory>
#include "src/server/ClientSession/ClientSession.hpp"
#include "Packet.hpp"

namespace Handlers {

    void dispatch(std::shared_ptr<ClientSession> session, Packet &p);

    void handle_ping(std::shared_ptr<ClientSession> session, Packet &p);

    void handle_auth_info(std::shared_ptr<ClientSession> session, Packet &p);

    void handle_chat_message(std::shared_ptr<ClientSession> session, Packet &p);

    boost::asio::awaitable<void> handle_auth_select(std::shared_ptr<ClientSession> session, Packet &p);

}
