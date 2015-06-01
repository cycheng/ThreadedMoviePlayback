#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#ifndef Q_MOC_RUN
#include "ui_MainWindow.h"
#include <QMainWindow>
#include <memory>
#endif // Q_MOC_RUN

class QProgressBar;

class CMainWindow: public QMainWindow
{
    Q_OBJECT

public:
    CMainWindow(QWidget* parent = nullptr);
    ~CMainWindow();

private slots:
    void TimerUpdate();
    void UseTripleBuffer(bool checked);
    void UseDoubleBuffer(bool checked);
    void UseSingleBuffer(bool checked);
    void UpdateTransparencyLabel(int value);
    void EnableFractalFX(bool enabled);
    void EnableFluidFX(bool enabled);
    void EnablePageCurlFX(bool enabled);
    void OpenVideoFile();

private:
    void keyPressEvent(QKeyEvent* event) override;
    Ui::CMainWindowClass m_ui;
    QTimer* m_timer;
    QLabel* m_fps;
};

#endif // MAINWINDOW_HPP

