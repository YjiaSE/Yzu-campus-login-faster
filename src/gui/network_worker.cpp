#include "network_worker.hpp"
#include "campus_net.hpp"
#include "http_client.hpp"
#include "self_service.hpp"
#include <stdexcept>
#include <utility>

namespace yzu {

NetworkWorker::NetworkWorker(Operation operation, Config config, QObject* parent)
    : QThread(parent), operation_(operation), config_(std::move(config))
{
}

void NetworkWorker::checkCancelled()
{
    if (isInterruptionRequested()) {
        throw std::runtime_error("cancelled");
    }
}

bool NetworkWorker::connected()
{
    checkCancelled();
    try {
        CurlHttpClient client;
        return client.get("http://111.13.141.31/generate_204",
                          {"Host: connect.rom.miui.com"}).status_code == 204;
    } catch (const std::exception&) {
        checkCancelled();
        return false;
    }
}

void NetworkWorker::login()
{
    emit message(QStringLiteral("将在 %1 秒后登录…").arg(config_.start_delay.count()));
    // Short sleeps allow Stop and window close to interrupt the delay promptly.
    for (long long tick = 0; tick < config_.start_delay.count() * 10; ++tick) {
        checkCancelled();
        msleep(100);
    }
    checkCancelled();
    emit message(QStringLiteral("正在获取校园网认证入口…"));
    CurlHttpClient client;
    CampusNet campus(client, config_.user_agent);
    const auto portal = campus.getPortalUrl();
    checkCancelled();
    emit message(QStringLiteral("正在登录所选服务…"));
    campus.oldLoginService(portal, config_.sso_username,
                           config_.sso_password, config_.campusnet_service);
    checkCancelled();
    emit message(QStringLiteral("认证成功，正在检查互联网连接…"));
    const bool online = connected();
    checkCancelled();
    emit outcome(online ? QStringLiteral("已连接互联网")
                        : QStringLiteral("认证成功，但互联网检测未通过"), online);
}

void NetworkWorker::run()
{
    try {
        checkCancelled();
        if (operation_ == Operation::Check || operation_ == Operation::Monitor) {
            emit message(QStringLiteral("正在检查互联网连接…"));
            const bool online = connected();
            checkCancelled();
            if (operation_ == Operation::Check || (online && !config_.debug)) {
                emit outcome(online ? QStringLiteral("已连接互联网")
                                    : QStringLiteral("互联网检测未通过"), online);
                return;
            }
            emit message(QStringLiteral("准备自动登录校园网…"));
        }
        if (operation_ == Operation::Logout) {
            CurlHttpClient client;
            SelfService service(client, config_.user_agent);
            emit message(QStringLiteral("正在登录自助服务…"));
            service.login(config_.sso_username, config_.sso_password);
            checkCancelled();
            const auto onlines = service.getOnlines();
            for (const auto& ip : onlines) {
                checkCancelled();
                service.logout(config_.sso_username, ip);
                emit message(QStringLiteral("已下线设备：%1").arg(QString::fromStdString(ip)));
            }
            checkCancelled();
            emit outcome(QStringLiteral("已下线全部在线设备（%1 台）").arg(onlines.size()), true);
            return;
        }
        login();
    } catch (const std::exception& error) {
        if (isInterruptionRequested()) {
            emit outcome(QStringLiteral("操作已停止"), false);
        } else {
            // Servers may echo credentials in error responses. Never log them.
            auto detail = QString::fromUtf8(error.what());
            if (!config_.sso_password.empty()) {
                detail.replace(QString::fromStdString(config_.sso_password), QStringLiteral("***"));
            }
            emit outcome(QStringLiteral("操作失败：%1").arg(detail), false);
        }
    }
}

} // namespace yzu
