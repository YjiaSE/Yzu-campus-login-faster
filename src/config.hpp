#pragma once

#include <chrono>
#include <string>

namespace yzu {

struct Config {
    std::string user_agent;
    std::string sso_username;
    std::string sso_password;
    std::string campusnet_service;
    std::chrono::seconds check_interval{60};
    std::chrono::seconds start_delay{5};
    bool debug{false};
    bool logout{false};
};

Config loadConfig(int argc, char* argv[]);

} // namespace yzu
