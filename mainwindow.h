#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btn_select_file_clicked();
    void on_btn_start_clicked();
    void on_btn_load_module_clicked();
private:
    Ui::MainWindow *ui;
    QProcess *ffmpegProcess;
    QString selectedFile;
    bool isImage;  // true if image, false if video
    bool wasRunningBeforeFileChange;
    void updateStatus(const QString &text, const QString &color = "black");
    void restartStreamingIfNeeded();
    bool isModuleLoaded(const QString &moduleName);
    void updateModuleButton();
};

#endif // MAINWINDOW_H
