#include "gui/main_window.hpp"
#include "gui/network_worker.hpp"
#include "config.hpp"
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest>

class GuiTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QVERIFY(temp_.isValid());
        oldDirectory_ = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temp_.path()));
        QCoreApplication::setOrganizationName("YzuCampusNetTests");
        QCoreApplication::setApplicationName("gui_tests");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temp_.path());
        for (const auto* key : {"SSO_USERNAME", "SSO_PASSWORD", "CAMPUSNET_SERVICE",
                               "CHECK_INTERVAL", "START_DELAY", "DEBUG", "USER_AGENT"}) {
            if (qEnvironmentVariableIsSet(key)) oldEnvironment_.insert(key, qgetenv(key));
            qunsetenv(key);
        }
    }
    void cleanupTestCase()
    {
        for (auto it = oldEnvironment_.cbegin(); it != oldEnvironment_.cend(); ++it) qputenv(it.key(), it.value());
        QVERIFY(QDir::setCurrent(oldDirectory_));
    }
    void configuration()
    {
        auto config = yzu::loadConfigFile("missing", false);
        QVERIFY(config.sso_username.empty());
        QCOMPARE(config.campusnet_service, std::string("学校互联网服务"));
        QVERIFY_EXCEPTION_THROWN(yzu::loadConfigFile("missing"), std::runtime_error);
        QFile file("test.env");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("SSO_USERNAME=student\nSSO_PASSWORD=\"p=a#ss\"\nCAMPUSNET_SERVICE=学校互联网服务\nCHECK_INTERVAL=2m\nSTART_DELAY=0\n");
        file.close();
        config = yzu::loadConfigFile("test.env");
        QCOMPARE(config.sso_password, std::string("p=a#ss"));
        QCOMPARE(config.check_interval.count(), 120LL);
        qputenv("CHECK_INTERVAL", "0");
        QVERIFY_EXCEPTION_THROWN(yzu::loadConfigFile("test.env"), std::runtime_error);
        qputenv("CHECK_INTERVAL", "-1");
        QVERIFY_EXCEPTION_THROWN(yzu::loadConfigFile("test.env"), std::runtime_error);
        qputenv("CHECK_INTERVAL", "9223372036854775807h");
        QVERIFY_EXCEPTION_THROWN(yzu::loadConfigFile("test.env"), std::runtime_error);
        qputenv("CHECK_INTERVAL", "3s");
        QCOMPARE(yzu::loadConfigFile("test.env").check_interval.count(), 3LL);
        qunsetenv("CHECK_INTERVAL");
    }
    void validationAndPasswordVisibility()
    {
        yzu::MainWindow window;
        window.show();
        auto* password = window.findChild<QLineEdit*>("password");
        QCOMPARE(password->echoMode(), QLineEdit::Password);
        window.findChild<QCheckBox*>("showPassword")->setChecked(true);
        QCOMPARE(password->echoMode(), QLineEdit::Normal);
        window.findChild<QPushButton*>("login")->click();
        QCOMPARE(window.findChild<QLabel*>("status")->text(), QStringLiteral("请先填写账号和密码"));
        QVERIFY(!window.findChild<QPushButton*>("stop")->isEnabled());
    }
    void stopAndCloseDuringLogin()
    {
        yzu::MainWindow window;
        window.show();
        window.findChild<QLineEdit*>("username")->setText("test-user");
        window.findChild<QLineEdit*>("password")->setText("test-secret");
        window.findChild<QSpinBox*>("delay")->setValue(30);
        window.findChild<QPushButton*>("login")->click();
        auto* worker = window.findChild<yzu::NetworkWorker*>();
        QVERIFY(worker);
        QSignalSpy progress(worker, &yzu::NetworkWorker::message);
        QTRY_VERIFY_WITH_TIMEOUT(!progress.isEmpty(), 2000);
        QVERIFY(!window.findChild<QPushButton*>("check")->isEnabled());
        window.findChild<QPushButton*>("stop")->click();
        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<QPushButton*>("login")->isEnabled(), 2000);
        QVERIFY(!QSettings().contains("password"));
        QCOMPARE(QSettings().value("username").toString(), QStringLiteral("test-user"));
        window.findChild<QPushButton*>("login")->click();
        worker = window.findChild<yzu::NetworkWorker*>();
        QVERIFY(worker);
        QSignalSpy progressAgain(worker, &yzu::NetworkWorker::message);
        QTRY_VERIFY_WITH_TIMEOUT(!progressAgain.isEmpty(), 2000);
        window.close();
        QTRY_VERIFY_WITH_TIMEOUT(!window.isVisible(), 2000);
    }
private:
    QTemporaryDir temp_;
    QString oldDirectory_;
    QMap<QByteArray, QByteArray> oldEnvironment_;
};

QTEST_MAIN(GuiTests)
#include "gui_tests.moc"
