#include "Stdafx.hpp"
#include "MainWindow.hpp"
#include "Buffer.hpp"
#include <QProgressBar>
#include <QTime>
#include <QTimer>

CMainWindow::CMainWindow(QWidget* parent): QMainWindow(parent), m_timer(nullptr), m_fps(nullptr)
{
    m_ui.setupUi(this);

    m_timer = new QTimer(this);
    m_timer->start(16);

    m_fps = new QLabel(m_ui.statusBar);
    m_ui.statusBar->addWidget(m_fps);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(TimerUpdate()));
    connect(m_timer, SIGNAL(timeout()), m_ui.glwidget, SLOT(updateGL()));
}

CMainWindow::~CMainWindow()
{
}

void CMainWindow::TimerUpdate()
{
    int currentTime = QTime::currentTime().msecsSinceStartOfDay();
    // TODO: compute and display FPS
    m_fps->setText(""); 
}
