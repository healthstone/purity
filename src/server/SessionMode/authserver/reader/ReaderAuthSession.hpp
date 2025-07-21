#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/authserver/opcodes/AuthOpcodes.hpp"

namespace ReaderAuthSession {

    void process_read_buffer_as_authserver(std::shared_ptr<ClientSession> session);

}