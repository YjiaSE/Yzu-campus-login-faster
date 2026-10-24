#include "config.hpp"

#include "utils.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace yzu {
namespace {

const char* kDefaultUserAgent = "Mozilla/5.0 (X11; Linux x86_64; rv:60.0) Gecko/20100101 Firefox/60.0";

std::map<std::string, std::string> readEnvFile(const std::string& path)
{
    std::ifstream file(path);
    std::map<std::string, std::string> env;
    if (!file.is_open()) {
        return env;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        auto key = trim(line.substr(0, pos));
        auto value = trim(line.substr(pos + 1));
        if (value.size() >= 2) {
            const auto first = value.front();
            const auto last = value.back();
            if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
                value = value.substr(1, value.size() - 2);
            }
        }
        env[key] = value;
    }
    return env;
}

std::string getConfigValue(const std::map<std::string, std::string>& file_env, const std::string& key)
{
    if (const char* value = std::getenv(key.c_str())) {
        return value;
    }

    const auto it = file_env.find(key);
    if (it != file_env.end()) {
        return it->second;
    }

    return "";
}

std::chrono::seconds parseDurationSeconds(const std::string& value, std::chrono::seconds default_value)
{
    if (value.empty()) {
        return default_value;
    }

    std::size_t parsed = 0;
    long long amount = 0;
    try {
        amount = std::stoll(value, &parsed);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid duration: " + value);
    }

    const auto unit = value.substr(parsed);
    if (unit.empty() || unit == "s") {
        return std::chrono::seconds(amount);
    }
    if (unit == "m") {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes(amount));
    }
    if (unit == "h") {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(amount));
    }

    throw std::runtime_error("invalid duration unit: " + value);
}

bool isValidService(const std::string& service)
{
    static const std::vector<std::string> services = {
        "学校互联网服务",
        "移动互联网服务",
        "联通互联网服务",
        "电信互联网服务",
        "校内免费服务",
    };

    for (const auto& item : services) {
        if (item == service) {
            return true;
        }
    }
    return false;
}

void printUsage(const char* program)
{
    std::cout
        << "Usage: " << program << " [-e env_file] [-logout]\n"
        << "  -e <file>   name of env file, default .env\n"
        << "  -logout     logout all online clients for the account\n";
}

} // namespace

Config loadConfig(int argc, char* argv[])
{
    std::string env_file = ".env";
    bool logout = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-e") {
            if (i + 1 >= argc) {
                throw std::runtime_error("-e requires an env file name");
            }
            env_file = argv[++i];
        } else if (arg == "-logout" || arg == "--logout") {
            logout = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }

    const auto file_env = readEnvFile(env_file);

    Config config;
    config.logout = logout;
    config.user_agent = getConfigValue(file_env, "USER_AGENT");
    if (config.user_agent.empty()) {
        config.user_agent = kDefaultUserAgent;
    }

    config.sso_username = getConfigValue(file_env, "SSO_USERNAME");
    if (config.sso_username.empty()) {
        throw std::runtime_error("SSO_USERNAME is not set");
    }

    config.sso_password = getConfigValue(file_env, "SSO_PASSWORD");
    if (config.sso_password.empty()) {
        throw std::runtime_error("SSO_PASSWORD is not set");
    }

    config.campusnet_service = getConfigValue(file_env, "CAMPUSNET_SERVICE");
    if (config.campusnet_service.empty()) {
        throw std::runtime_error("CAMPUSNET_SERVICE is not set");
    }
    if (!isValidService(config.campusnet_service)) {
        throw std::runtime_error("CAMPUSNET_SERVICE is not set correctly");
    }

    config.check_interval = parseDurationSeconds(getConfigValue(file_env, "CHECK_INTERVAL"), std::chrono::seconds(60));
    config.start_delay = parseDurationSeconds(getConfigValue(file_env, "START_DELAY"), std::chrono::seconds(5));
    config.debug = getConfigValue(file_env, "DEBUG") == "true";

    return config;
}

} // namespace yzu
