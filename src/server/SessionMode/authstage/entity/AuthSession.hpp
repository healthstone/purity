#include <cstdint>
#include <memory>
#include "SRP6.hpp"

class AuthSession {
public:
    AuthSession() = default;
    ~AuthSession() = default;

    SRP6* srp() { return srp_.get(); }

private:
    std::shared_ptr<SRP6> srp_ = std::make_shared<SRP6>();
};