#include "AccountInfo.hpp"
#include "Logger.hpp"

void AccountInfo::handle_auth_state() {
    setIsAuthenticated(true);
    Logger::get()->info("Account {} was successfully authorized", srp()->getUserName());
}

void AccountInfo::handle_close_state() {
    Logger::get()->info("Account {} was successfully closed connection", srp()->getUserName());
}