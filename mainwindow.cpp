#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , ffmpegProcess(new QProcess(this))
    , isImage(false)
    , restartPending(false)
{
    ui->setupUi(this);

    this->setWindowTitle("Virtual Camera");
    this->setWindowIcon(QIcon(":/resources/icon.svg"));

    ui->btn_load_module->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    ui->btn_select_file->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->btn_start->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    const QSize buttonIconSize(18, 18);
    ui->btn_load_module->setIconSize(buttonIconSize);
    ui->btn_select_file->setIconSize(buttonIconSize);
    ui->btn_start->setIconSize(buttonIconSize);

    connect(ffmpegProcess, &QProcess::started, this, [this]() {
        updateStartButtonState(true);
    });
    connect(ffmpegProcess, &QProcess::finished, this, [this]() {
        updateStartButtonState(false);
    });

    updateSelectFileButtonText();
    updateStartButtonState(false);
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

// Select image or video
void MainWindow::on_btn_select_file_clicked()
{
    const QString filters = "Media Files (*.png *.jpg *.jpeg *.mp4 *.mkv *.avi);;Images (*.png *.jpg *.jpeg);;Videos (*.mp4 *.mkv *.avi)";
    QString file = QFileDialog::getOpenFileName(this, "Select Image or Video", "", filters);

    if (file.isEmpty()) {
        return;
    }

    QString extension = QFileInfo(file).suffix().toLower();
    const QStringList imageExtensions = {"png", "jpg", "jpeg"};

    selectedFile = file;
    isImage = imageExtensions.contains(extension);
    updateSelectFileButtonText();
    ui->btn_select_file->setToolTip(selectedFile);

    restartStreamingIfNeeded();
}

void MainWindow::updateSelectFileButtonText()
{
    if (selectedFile.isEmpty()) {
        ui->btn_select_file->setText("Select Media File (No file selected)");
        ui->btn_select_file->setToolTip("");
        return;
    }

    const QString fileName = QFileInfo(selectedFile).fileName();
    ui->btn_select_file->setText("Media: " + fileName);
}

void MainWindow::on_btn_start_clicked()
{
    if (ffmpegProcess->state() == QProcess::Running) {
        stopStreaming();
        return;
    }

    startStreaming();
}

bool MainWindow::startStreaming()
{
    if (selectedFile.isEmpty()) {
        QMessageBox::warning(this, "No file selected", "Please select an image or video file first.");
        return false;
    }

    QStringList arguments;
    if (isImage) {
        arguments << "-nostdin" << "-loop" << "1" << "-re" << "-i" << selectedFile
                  << "-vf" << "format=yuv420p"
                  << "-f" << "v4l2" << "/dev/video10";
    } else {
        arguments << "-nostdin" << "-stream_loop" << "-1" << "-re" << "-i" << selectedFile
                  << "-vf" << "format=yuv420p"
                  << "-f" << "v4l2" << "/dev/video10";
    }

    ffmpegProcess->start("ffmpeg", arguments);

    if (!ffmpegProcess->waitForStarted(3000)) {
        QMessageBox::critical(this, "Error", "Failed to start ffmpeg.");
        updateStartButtonState(false);
        return false;
    }

    updateStartButtonState(true);
    return true;
}

bool MainWindow::stopStreaming()
{
    if (ffmpegProcess->state() == QProcess::NotRunning) {
        updateStartButtonState(false);
        return true;
    }

    ffmpegProcess->terminate();
    if (!ffmpegProcess->waitForFinished(1500)) {
        ffmpegProcess->kill();
        if (!ffmpegProcess->waitForFinished(3000)) {
            QMessageBox::warning(this, "Warning", "Failed to stop ffmpeg cleanly.");
            return false;
        }
    }

    updateStartButtonState(false);
    return true;
}

void MainWindow::restartStreamingIfNeeded()
{
    if (ffmpegProcess->state() != QProcess::Running && !restartPending) {
        return;
    }

    restartPending = true;
    if (ffmpegProcess->state() == QProcess::Running && !stopStreaming()) {
        restartPending = false;
        return;
    }

    QTimer::singleShot(3000, this, [this]() {
        if (!restartPending) {
            return;
        }
        restartPending = false;
        startStreaming();
    });
}

void MainWindow::updateStartButtonState(bool streaming)
{
    if (streaming) {
        ui->btn_start->setText("Stop Streaming");
        ui->btn_start->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
        ui->btn_start->setStyleSheet(
            "QPushButton {"
            "  background-color: #166534;"
            "  color: #ffffff;"
            "}"
        );
    } else {
        ui->btn_start->setText("Start Streaming");
        ui->btn_start->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        ui->btn_start->setStyleSheet("");
    }
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
        ui->btn_load_module->setStyleSheet(
            "QPushButton {"
            "  background-color: #166534;"
            "  color: #ffffff;"
            "}"
        );
    } else {
        ui->btn_load_module->setText("Load v4l2loopback Module");
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
