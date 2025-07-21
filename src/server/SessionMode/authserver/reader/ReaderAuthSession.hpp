#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/authserver/opcodes/AuthOpcodes.hpp"

namespace ReaderAuthSession {
    void process_read_buffer_as_authserver(std::shared_ptr<ClientSession> session);

    void process_valid_packet(std::shared_ptr<ClientSession> session,
                              AuthOpcode id,
                              const std::vector<uint8_t>& body);
}