#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , ffmpegProcess(new QProcess(this))
    , isImage(false)
    , pendingRestart(false)
{
    ui->setupUi(this);
    updateStatus("Idle", "gray");

    this->setWindowTitle("Virtual Camera");
    this->setWindowIcon(QIcon(":/resources/icon.svg"));

    connect(ffmpegProcess, &QProcess::started, [this]() {
        ui->btn_start->setText("Stop Virtual Camera");
        updateStatus("Streaming...", "green");
    });

    connect(ffmpegProcess, &QProcess::finished, [this]() {
        if (pendingRestart) {
            pendingRestart = false;
            QTimer::singleShot(150, this, [this]() {
                startStreaming();
            });
            return;
        }

        ui->btn_start->setText("Start Virtual Camera");
        updateStatus("Stopped", "red");
    });

    updateModuleButton();
}

MainWindow::~MainWindow()
{
    if (ffmpegProcess->state() == QProcess::Running) {
        ffmpegProcess->kill();
        ffmpegProcess->waitForFinished();
    }
    delete ui;
}

void MainWindow::on_btn_select_file_clicked()
{
    const QString filters = "Media Files (*.png *.jpg *.jpeg *.mp4 *.mkv *.avi);;Images (*.png *.jpg *.jpeg);;Videos (*.mp4 *.mkv *.avi)";
    QString file = QFileDialog::getOpenFileName(this, "Select Image or Video", "", filters);

    if (file.isEmpty()) {
        return;
    }

    const QString extension = QFileInfo(file).suffix().toLower();
    const QStringList imageExtensions = {"png", "jpg", "jpeg"};

    selectedFile = file;
    isImage = imageExtensions.contains(extension);
    ui->lbl_file->setText("Selected file: " + QFileInfo(file).fileName());

    restartStreamingIfNeeded();
}

void MainWindow::on_btn_start_clicked()
{
    if (ffmpegProcess->state() == QProcess::Running) {
        pendingRestart = false;
        stopStreaming();
        return;
    }

    startStreaming();
}

void MainWindow::restartStreamingIfNeeded()
{
    if (ffmpegProcess->state() != QProcess::Running) {
        return;
    }

    pendingRestart = true;
    stopStreaming(true);
}

void MainWindow::startStreaming()
{
    if (selectedFile.isEmpty()) {
        QMessageBox::warning(this, "No file selected", "Please select an image or video file first.");
        return;
    }

    QStringList arguments;
    if (isImage) {
        arguments << "-loop" << "1" << "-re" << "-i" << selectedFile
                  << "-vf" << "format=yuv420p"
                  << "-f" << "v4l2" << "/dev/video10";
    } else {
        arguments << "-stream_loop" << "-1" << "-re" << "-i" << selectedFile
                  << "-vf" << "format=yuv420p"
                  << "-f" << "v4l2" << "/dev/video10";
    }

    ffmpegProcess->start("ffmpeg", arguments);

    if (!ffmpegProcess->waitForStarted(3000)) {
        pendingRestart = false;
        ui->btn_start->setText("Start Virtual Camera");
        QMessageBox::critical(this, "Error", "Failed to start ffmpeg.");
        updateStatus("Error", "red");
    }
}

void MainWindow::stopStreaming(bool forRestart)
{
    if (ffmpegProcess->state() != QProcess::Running) {
        return;
    }

    if (forRestart) {
        updateStatus("Applying new file...", "darkorange");
    }

    ffmpegProcess->terminate();
    if (!ffmpegProcess->waitForFinished(1000)) {
        ffmpegProcess->kill();
        ffmpegProcess->waitForFinished();
    }
}

void MainWindow::updateStatus(const QString &text, const QString &color)
{
    ui->lbl_status->setText(text);
    ui->lbl_status->setStyleSheet("color: " + color + "; font-weight: bold;");
}

void MainWindow::on_btn_load_module_clicked()
{
    if (isModuleLoaded("v4l2loopback")) {
        QMessageBox::information(this, "Info", "v4l2loopback is already loaded.");
        return;
    }

    QProcess process;
    process.start("pkexec", {"modprobe", "v4l2loopback",
                             "devices=1",
                             "video_nr=10",
                             "card_label=VirtualCam",
                             "exclusive_caps=1"});
    process.waitForFinished();

    if (process.exitCode() == 0) {
        updateModuleButton();
    } else {
        QMessageBox::critical(this, "Error", "Failed to load v4l2loopback.");
    }
}

void MainWindow::updateModuleButton()
{
    if (isModuleLoaded("v4l2loopback")) {
        ui->btn_load_module->setText("Module Loaded");
        ui->btn_load_module->setStyleSheet("background-color: green; color: white; font-weight: bold;");
    } else {
        ui->btn_load_module->setText("Load Module");
        ui->btn_load_module->setStyleSheet("");
    }
}

bool MainWindow::isModuleLoaded(const QString &moduleName)
{
    QProcess process;
    process.start("lsmod");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    return output.contains(QRegularExpression("^" + moduleName + "\\b", QRegularExpression::MultilineOption));
}
