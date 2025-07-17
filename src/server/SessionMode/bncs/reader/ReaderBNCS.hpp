#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/bncs/opcodes/opcodes8.hpp"

namespace ReaderBNCS {

    void process_read_buffer_as_bncs(std::shared_ptr<ClientSession> session);

    void process_valid_packet(std::shared_ptr<ClientSession> session,
                                          BNETOpcode8 id,
                                          const std::vector<uint8_t>& body);

    size_t get_packet_expected_size(BNETOpcode8 id);
}