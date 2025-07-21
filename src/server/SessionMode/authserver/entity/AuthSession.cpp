#include "AuthSession.hpp"

AuthSession::AuthSession() {
    srp_ = std::make_shared<SRP>();
}