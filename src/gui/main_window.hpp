#pragma once

#include "config.hpp"
#include "network_worker.hpp"
#include <QMainWindow>
#include <QTimer>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QGroupBox;

namespace yzu {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();
    void applyConfig(const Config& config);
    Config currentConfig() const;
    void importConfig();
    void savePreferences();
    void start(NetworkWorker::Operation operation);
    void stop();
    void updateControls();
    void appendLog(const QString& text);
    void setStatus(const QString& text, bool success = false);

    Config defaults_;
    QLineEdit* username_{};
    QLineEdit* password_{};
    QComboBox* service_{};
    QSpinBox* interval_{};
    QSpinBox* delay_{};
    QCheckBox* debug_{};
    QGroupBox* settingsGroup_{};
    QPlainTextEdit* log_{};
    QLabel* status_{};
    QPushButton* login_{};
    QPushButton* monitor_{};
    QPushButton* stop_{};
    QPushButton* check_{};
    QPushButton* logout_{};
    QPushButton* import_{};
    QTimer timer_;
    NetworkWorker* worker_{};
    bool monitoring_{false};
    bool closing_{false};
};

} // namespace yzu
