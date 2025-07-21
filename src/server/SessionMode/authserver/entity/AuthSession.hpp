#include <cstdint>
#include <memory>
#include "SRP.hpp"

class AuthSession {
public:
    AuthSession();
    ~AuthSession() = default;

    SRP* srp() { return srp_.get(); }

private:
    std::shared_ptr<SRP> srp_;
};