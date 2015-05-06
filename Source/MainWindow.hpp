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

private:
    Ui::CMainWindowClass m_ui;
    QTimer* m_timer;
    QLabel* m_fps;
};

#endif // MAINWINDOW_HPP

