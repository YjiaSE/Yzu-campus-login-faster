#include "main_window.hpp"
#include "http_client.hpp"
#include <QApplication>
#include <QMessageBox>
#include <exception>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("YzuCampusNet"));
    QApplication::setApplicationName(QStringLiteral("netool-gui"));
    QApplication::setApplicationVersion(QStringLiteral("0.2.0"));
    try {
        yzu::CurlGlobal curl;
        yzu::MainWindow window;
        window.show();
        return app.exec();
    } catch (const std::exception& error) {
        QMessageBox::critical(nullptr, QStringLiteral("启动失败"), QString::fromUtf8(error.what()));
        return 1;
    }
}
