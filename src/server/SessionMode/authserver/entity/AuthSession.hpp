#include <cstdint>
#include <memory>
#include "SRP.hpp"

class AuthSession {
public:
    AuthSession() = default;
    ~AuthSession() = default;

    SRP* srp() { return srp_.get(); }

    const std::string& get_salt() const { return salt_; }
    void set_salt(const std::string& s) { salt_ = s; }

    const std::string& get_verifier() const { return verifier_; }
    void set_verifier(const std::string& v) { verifier_ = v; }

private:
    std::shared_ptr<SRP> srp_ = std::make_shared<SRP>();

    std::string salt_;
    std::string verifier_;
};