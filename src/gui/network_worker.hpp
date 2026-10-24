#pragma once

#include "config.hpp"
#include <QThread>

namespace yzu {

class NetworkWorker : public QThread {
    Q_OBJECT
public:
    enum class Operation { Check, Login, Monitor, Logout };
    NetworkWorker(Operation operation, Config config, QObject* parent = nullptr);

signals:
    void message(const QString& text);
    void outcome(const QString& text, bool success);

protected:
    void run() override;

private:
    bool connected();
    void checkCancelled();
    void login();
    Operation operation_;
    Config config_;
};

} // namespace yzu
