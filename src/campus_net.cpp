#include "campus_net.hpp"

#include "utils.hpp"

#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
 
namespace yzu {

CampusNet::CampusNet(CurlHttpClient& client, std::string user_agent)
    : client_(client), user_agent_(std::move(user_agent))
{
}

std::pair<std::string, std::string> CampusNet::getHostQuery(const std::string& portal_url) const
{
    const std::regex pattern(R"(http://(.+?)/.*?\?(.+))");
    std::smatch match;
    if (!std::regex_search(portal_url, match, pattern) || match.size() < 3) {
        throw std::runtime_error("invalid portal URL");
    }
    return {match[1].str(), match[2].str()};
}

std::string CampusNet::getPortalUrl()
{
    const auto response = client_.get("http://123.123.123.123", {
        "User-Agent: " + user_agent_,
    });

    if (response.status_code != 200) {
        throw std::runtime_error("cannot get portal URL: HTTP " + std::to_string(response.status_code));
    }

    const std::regex pattern(R"(href='(.*?)')");
    std::smatch match;
    if (!std::regex_search(response.body, match, pattern) || match.size() < 2) {
        throw std::runtime_error("cannot get portal URL: cannot find URL");
    }
    return match[1].str();
}

void CampusNet::loginService(const std::string& portal_url, const std::string& user_id, const std::string& service)
{
    const auto [host, query_string] = getHostQuery(portal_url);
    const auto encoded_service = client_.escape(service);

    const auto body = formEncode(client_, {
        {"userId", user_id},
        {"flag", "casauthofservicecheck"},
        {"service", encoded_service},
        {"queryString", query_string},
        {"operatorPwd", ""},
        {"operatorUserId", ""},
        {"passwordEncrypt", "false"},
    });

    const auto response = client_.post(
        "http://" + host + "/eportal/InterFace.do?method=loginOfCas",
        body,
        {
            "User-Agent: " + user_agent_,
            "Content-Type: application/x-www-form-urlencoded",
        });

    if (response.status_code != 200) {
        throw std::runtime_error("error logging in to service: HTTP " + std::to_string(response.status_code));
    }

    const auto result = jsonStringField(response.body, "result");
    if (result != "success") {
        const auto message = jsonStringField(response.body, "message");
        throw std::runtime_error(message.empty() ? "service login failed" : message);
    }
}

void CampusNet::oldLoginService(
    const std::string& portal_url,
    const std::string& user_id,
    const std::string& password,
    const std::string& service)
{
    const auto [host, query_string] = getHostQuery(portal_url);
    const auto encoded_service = client_.escape(service);

    const auto body = formEncode(client_, {
        {"userId", user_id},
        {"password", password},
        {"service", encoded_service},
        {"queryString", query_string},
        {"operatorPwd", ""},
        {"operatorUserId", ""},
        {"validcode", ""},
        {"passwordEncrypt", "false"},
    });

    const auto response = client_.post(
        "http://" + host + "/eportal/InterFace.do?method=login",
        body,
        {
            "User-Agent: " + user_agent_,
            "Content-Type: application/x-www-form-urlencoded",
        });

    if (response.status_code != 200) {
        throw std::runtime_error("error logging in to service: HTTP " + std::to_string(response.status_code));
    }

    const auto result = jsonStringField(response.body, "result");
    if (result != "success") {
        const auto message = jsonStringField(response.body, "message");
        throw std::runtime_error(message.empty() ? "service login failed" : message);
    }
}

} // namespace yzu
