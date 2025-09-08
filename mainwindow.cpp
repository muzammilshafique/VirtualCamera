#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , ffmpegProcess(new QProcess(this))
    , isImage(false)
{
    ui->setupUi(this);
    updateStatus("Idle", "gray");

    this->setWindowTitle("Virtual Camera");
    this->setWindowIcon(QIcon(":/resources/icon.svg"));

    connect(ffmpegProcess, &QProcess::started, [this]() {
        updateStatus("Streaming...", "green");
    });
    connect(ffmpegProcess, &QProcess::finished, [this]() {
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

// Select image
void MainWindow::on_btn_select_image_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select Image", "", "Images (*.png *.jpg *.jpeg)");
    if (!file.isEmpty()) {
        selectedFile = file;
        isImage = true;
        ui->lbl_file->setText("Selected: " + QFileInfo(file).fileName());
    }
}

// Select video
void MainWindow::on_btn_select_video_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "Select Video", "", "Videos (*.mp4 *.mkv *.avi)");
    if (!file.isEmpty()) {
        selectedFile = file;
        isImage = false;
        ui->lbl_file->setText("Selected: " + QFileInfo(file).fileName());
    }
}

void MainWindow::on_btn_start_clicked()
{
    if (ffmpegProcess->state() == QProcess::Running) {
        // 🔴 Stop ffmpeg
        ffmpegProcess->kill();
        ffmpegProcess->waitForFinished();
        ui->btn_start->setText("Start");
        updateStatus("Stopped", "red");
        return;
    }

    // 🟢 Start ffmpeg
    if (selectedFile.isEmpty()) {
        QMessageBox::warning(this, "No file selected", "Please select an image or video first.");
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

    if (ffmpegProcess->waitForStarted(2000)) {
        ui->btn_start->setText("Stop");
        updateStatus("Streaming...", "green");
    } else {
        QMessageBox::critical(this, "Error", "Failed to start ffmpeg.");
        updateStatus("Error", "red");
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

