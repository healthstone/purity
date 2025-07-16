#include <memory>
#include "src/server/ClientSession/ClientSession.hpp"

namespace HandlersW3GS {

    void dispatch(std::shared_ptr<ClientSession> session, BNETPacket16 &p);
}
