#include "main_window.hpp"
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextDocument>
#include <QVBoxLayout>
#include <limits>

namespace yzu {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    buildUi();
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, [this] { start(NetworkWorker::Operation::Monitor); });
    // Load saved non-sensitive preferences, then overlay .env/environment values.
    try {
        defaults_ = loadConfigFile(".env", false);
        applyConfig(defaults_);
    } catch (const std::exception&) {
        // Keep the window usable so an invalid file can be corrected or replaced.
        defaults_.user_agent = "Mozilla/5.0 YzuCampusNet/0.2";
        appendLog(QStringLiteral("默认配置读取失败，请检查 .env / 环境变量，或手动填写配置。"));
    }
    QSettings settings;
    if (username_->text().isEmpty()) {
        username_->setText(settings.value("username").toString());
    }
    // Explicit config values take precedence over previous UI choices.
    if (!QFileInfo::exists(QStringLiteral(".env"))) {
        if (qEnvironmentVariableIsEmpty("CAMPUSNET_SERVICE")) {
            const auto saved = settings.value("service", service_->currentText()).toString();
            const auto index = service_->findText(saved);
            if (index >= 0) service_->setCurrentIndex(index);
        }
        if (qEnvironmentVariableIsEmpty("CHECK_INTERVAL")) interval_->setValue(settings.value("interval", 60).toInt());
        if (qEnvironmentVariableIsEmpty("START_DELAY")) delay_->setValue(settings.value("delay", 5).toInt());
    }
    appendLog(QStringLiteral("就绪。填写账号后可登录或开启自动重连。密码不会保存到应用设置。"));
    updateControls();
}

MainWindow::~MainWindow()
{
    if (worker_) {
        worker_->requestInterruption();
        worker_->wait();
    }
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("扬州大学 · 校园网助手"));
    resize(760, 760);
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);
    auto* title = new QLabel(QStringLiteral("YZU 校园网助手"));
    title->setObjectName("title");
    layout->addWidget(title);
    auto* subtitle = new QLabel(QStringLiteral("登录校园网，自动检测连接并在断网后重连。"));
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);
    status_ = new QLabel(QStringLiteral("就绪 · 尚未检测网络"));
    status_->setObjectName("status");
    status_->setWordWrap(true);
    status_->setTextFormat(Qt::PlainText);
    layout->addWidget(status_);

    settingsGroup_ = new QGroupBox(QStringLiteral("连接设置"));
    auto* form = new QFormLayout(settingsGroup_);
    form->setContentsMargins(18, 26, 18, 18);
    form->setSpacing(12);
    username_ = new QLineEdit;
    username_->setObjectName("username");
    username_->setPlaceholderText(QStringLiteral("学号 / 校园网账号"));
    form->addRow(QStringLiteral("账号"), username_);
    password_ = new QLineEdit;
    password_->setObjectName("password");
    password_->setEchoMode(QLineEdit::Password);
    password_->setPlaceholderText(QStringLiteral("密码仅保留在本次会话中"));
    auto* passwordRow = new QHBoxLayout;
    passwordRow->addWidget(password_);
    auto* showPassword = new QCheckBox(QStringLiteral("显示"));
    showPassword->setObjectName("showPassword");
    passwordRow->addWidget(showPassword);
    connect(showPassword, &QCheckBox::toggled, this, [this](bool show) {
        password_->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    });
    form->addRow(QStringLiteral("密码"), passwordRow);
    service_ = new QComboBox;
    service_->setObjectName("service");
    service_->addItems({QStringLiteral("学校互联网服务"), QStringLiteral("移动互联网服务"),
                        QStringLiteral("联通互联网服务"), QStringLiteral("电信互联网服务"),
                        QStringLiteral("校内免费服务")});
    form->addRow(QStringLiteral("服务"), service_);
    interval_ = new QSpinBox;
    interval_->setObjectName("interval");
    interval_->setRange(1, std::numeric_limits<int>::max() / 1000);
    interval_->setValue(60);
    interval_->setSuffix(QStringLiteral(" 秒"));
    form->addRow(QStringLiteral("检测间隔"), interval_);
    delay_ = new QSpinBox;
    delay_->setObjectName("delay");
    delay_->setRange(0, std::numeric_limits<int>::max() / 1000);
    delay_->setValue(5);
    delay_->setSuffix(QStringLiteral(" 秒"));
    form->addRow(QStringLiteral("登录前等待"), delay_);
    debug_ = new QCheckBox(QStringLiteral("调试模式：网络已通时也尝试自动登录"));
    debug_->setObjectName("debug");
    form->addRow(debug_);
    layout->addWidget(settingsGroup_);

    auto* actions = new QHBoxLayout;
    login_ = new QPushButton(QStringLiteral("立即登录"));
    login_->setObjectName("login");
    monitor_ = new QPushButton(QStringLiteral("开启自动重连"));
    monitor_->setObjectName("monitor");
    stop_ = new QPushButton(QStringLiteral("停止"));
    stop_->setObjectName("stop");
    actions->addWidget(login_);
    actions->addWidget(monitor_);
    actions->addWidget(stop_);
    layout->addLayout(actions);
    auto* secondary = new QHBoxLayout;
    check_ = new QPushButton(QStringLiteral("检测网络"));
    check_->setObjectName("check");
    import_ = new QPushButton(QStringLiteral("导入 .env"));
    import_->setObjectName("import");
    logout_ = new QPushButton(QStringLiteral("下线全部设备…"));
    logout_->setObjectName("logout");
    secondary->addWidget(check_);
    secondary->addWidget(import_);
    secondary->addWidget(logout_);
    layout->addLayout(secondary);
    auto* logHeader = new QHBoxLayout;
    logHeader->addWidget(new QLabel(QStringLiteral("运行日志")));
    logHeader->addStretch();
    auto* clear = new QPushButton(QStringLiteral("清空日志"));
    logHeader->addWidget(clear);
    layout->addLayout(logHeader);
    log_ = new QPlainTextEdit;
    log_->setObjectName("log");
    log_->setReadOnly(true);
    log_->document()->setMaximumBlockCount(1000);
    layout->addWidget(log_, 1);
    auto* hint = new QLabel(QStringLiteral("关闭窗口会停止自动重连；停止任务不会下线已连接的设备。"));
    hint->setWordWrap(true);
    layout->addWidget(hint);

    connect(clear, &QPushButton::clicked, log_, &QPlainTextEdit::clear);
    connect(login_, &QPushButton::clicked, this, [this] { start(NetworkWorker::Operation::Login); });
    connect(monitor_, &QPushButton::clicked, this, [this] { start(NetworkWorker::Operation::Monitor); });
    connect(check_, &QPushButton::clicked, this, [this] { start(NetworkWorker::Operation::Check); });
    connect(stop_, &QPushButton::clicked, this, &MainWindow::stop);
    connect(import_, &QPushButton::clicked, this, &MainWindow::importConfig);
    connect(logout_, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, QStringLiteral("下线全部设备"),
            QStringLiteral("这将下线此账号的全部在线设备，包括其他电脑和手机。是否继续？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            start(NetworkWorker::Operation::Logout);
        }
    });
    setStyleSheet(QStringLiteral(
        "QMainWindow { background: #f4f7fb; }"
        "QWidget { color: #24354a; font-size: 14px; }"
        "QLabel#title { font-size: 27px; font-weight: 600; color: #133e63; }"
        "QLabel#status { background: #e5edf6; border-radius: 8px; padding: 14px; }"
        "QGroupBox { background: white; border: 1px solid #d8e1ec; border-radius: 10px; margin-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 5px; }"
        "QLineEdit, QComboBox, QSpinBox { background: white; border: 1px solid #b9c8d9; border-radius: 5px; padding: 7px; }"
        "QPushButton { background: #e5edf6; border: 1px solid #c5d4e4; border-radius: 6px; padding: 9px 13px; }"
        "QPushButton:hover { background: #d6e4f5; }"
        "QPushButton#login, QPushButton#monitor { background: #2263a4; color: white; border: 1px solid #2263a4; }"
        "QPushButton#login:hover, QPushButton#monitor:hover { background: #174b80; }"
        "QPushButton:disabled { background: #e9edf2; color: #8b96a4; border-color: #d6dde6; }"
        "QPushButton#login:disabled, QPushButton#monitor:disabled { background: #e9edf2; color: #8b96a4; border-color: #d6dde6; }"
        "QPlainTextEdit { background: white; border: 1px solid #d8e1ec; border-radius: 8px; padding: 8px; }"));
}

void MainWindow::applyConfig(const Config& config)
{
    username_->setText(QString::fromStdString(config.sso_username));
    password_->setText(QString::fromStdString(config.sso_password));
    service_->setCurrentText(QString::fromStdString(config.campusnet_service));
    interval_->setValue(static_cast<int>(config.check_interval.count()));
    delay_->setValue(static_cast<int>(config.start_delay.count()));
    debug_->setChecked(config.debug);
}

Config MainWindow::currentConfig() const
{
    auto config = defaults_;
    config.sso_username = username_->text().trimmed().toStdString();
    config.sso_password = password_->text().toStdString();
    config.campusnet_service = service_->currentText().toStdString();
    config.check_interval = std::chrono::seconds(interval_->value());
    config.start_delay = std::chrono::seconds(delay_->value());
    config.debug = debug_->isChecked();
    return config;
}

void MainWindow::importConfig()
{
    const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("导入环境变量文件"),
        QString(), QStringLiteral("环境变量文件 (*.env .env .env.*);;所有文件 (*)"));
    if (path.isEmpty()) return;
    if (!QFileInfo(path).isReadable()) {
        setStatus(QStringLiteral("无法读取所选文件"));
        return;
    }
    try {
        const auto config = loadConfigFile(path.toStdString(), false);
        defaults_ = config;
        applyConfig(config);
        appendLog(QStringLiteral("已导入配置（同名系统环境变量优先）。"));
        setStatus(QStringLiteral("配置已导入"), true);
    } catch (const std::exception&) {
        setStatus(QStringLiteral("导入失败：请检查服务名称和时间格式。"));
        appendLog(QStringLiteral("配置导入失败，原有设置已保留。"));
    }
}

void MainWindow::savePreferences()
{
    QSettings settings;
    settings.setValue("username", username_->text().trimmed());
    settings.setValue("service", service_->currentText());
    settings.setValue("interval", interval_->value());
    settings.setValue("delay", delay_->value());
}

void MainWindow::start(NetworkWorker::Operation operation)
{
    if (worker_ || closing_) return;
    const auto config = currentConfig();
    if (operation != NetworkWorker::Operation::Check &&
        (config.sso_username.empty() || config.sso_password.empty())) {
        setStatus(QStringLiteral("请先填写账号和密码"));
        (config.sso_username.empty() ? username_ : password_)->setFocus();
        return;
    }
    savePreferences();
    monitoring_ = operation == NetworkWorker::Operation::Monitor;
    timer_.stop();
    worker_ = new NetworkWorker(operation, config, this);
    connect(worker_, &NetworkWorker::message, this, &MainWindow::appendLog);
    connect(worker_, &NetworkWorker::outcome, this, [this](const QString& text, bool success) {
        appendLog(text);
        if (!closing_) setStatus(text, success);
    });
    connect(worker_, &QThread::finished, this, [this] {
        auto* finished = worker_;
        worker_ = nullptr;
        finished->deleteLater();
        if (closing_) {
            close();
            return;
        }
        if (monitoring_) {
            timer_.start(interval_->value() * 1000);
            appendLog(QStringLiteral("自动重连运行中，%1 秒后再次检测。要修改设置请先停止。").arg(interval_->value()));
        }
        updateControls();
    });
    setStatus(monitoring_ ? QStringLiteral("自动重连 · 正在检测") : QStringLiteral("正在处理…"));
    updateControls();
    worker_->start();
}

void MainWindow::stop()
{
    monitoring_ = false;
    timer_.stop();
    if (worker_) {
        worker_->requestInterruption();
        setStatus(QStringLiteral("正在停止，等待当前请求结束…"));
    } else {
        setStatus(QStringLiteral("已停止自动重连"));
    }
    appendLog(QStringLiteral("已请求停止。"));
    updateControls();
}

void MainWindow::updateControls()
{
    const bool busy = worker_ || monitoring_ || closing_;
    settingsGroup_->setEnabled(!busy);
    for (auto* button : {login_, monitor_, check_, logout_, import_}) button->setEnabled(!busy);
    stop_->setEnabled(!closing_ && (monitoring_ || (worker_ && !worker_->isInterruptionRequested())));
    monitor_->setText(monitoring_ ? QStringLiteral("自动重连运行中") : QStringLiteral("开启自动重连"));
}

void MainWindow::appendLog(const QString& text)
{
    log_->appendPlainText(QDateTime::currentDateTime().toString("[HH:mm:ss] ") + text);
}

void MainWindow::setStatus(const QString& text, bool success)
{
    status_->setText(text);
    status_->setStyleSheet(success ? QStringLiteral("background: #e0f2e9; color: #195b3b;")
                                  : QStringLiteral("background: #e5edf6; color: #24354a;"));
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    savePreferences();
    monitoring_ = false;
    timer_.stop();
    if (worker_) {
        closing_ = true;
        worker_->requestInterruption();
        setStatus(QStringLiteral("正在退出，等待当前请求结束（最多约 5 秒）…"));
        updateControls();
        event->ignore();
    } else {
        event->accept();
    }
}

} // namespace yzu
