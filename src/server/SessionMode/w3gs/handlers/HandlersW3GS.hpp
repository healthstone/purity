#include <memory>
#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/w3gs/opcodes/opcodes16.hpp"
#include "src/server/SessionMode/w3gs/opcodes/BNETPacket16.hpp"

namespace HandlersW3GS {

    void dispatch(std::shared_ptr<ClientSession> session, BNETPacket16 &p);
}
