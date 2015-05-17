#include "Stdafx.hpp"
#include "MainWindow.hpp"

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
    connect(m_ui.noThreading, SIGNAL(toggled(bool)), this, SLOT(UseSingleBuffer(bool)));
    connect(m_ui.tripleBuffer, SIGNAL(toggled(bool)), this, SLOT(UseTripleBuffer(bool)));

    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
}

CMainWindow::~CMainWindow()
{
}

void CMainWindow::TimerUpdate()
{
    static int prevTime = QTime::currentTime().msecsSinceStartOfDay();
    static int numFrame = 0;
    int currentTime = QTime::currentTime().msecsSinceStartOfDay();

    // update fps every 0.5 sec
    numFrame++;
    if (currentTime - prevTime > 500) {
        float fps = (float)(currentTime - prevTime) / (float)numFrame;
        fps = 1000.f / fps;
        QString msg = "fps = " + QString::number(fps);
        m_fps->setText(msg);
        prevTime = currentTime;
        numFrame = 0;
    }
}

void CMainWindow::UseTripleBuffer(bool checked)
{
    if (checked)
        m_ui.glwidget->ChangeBufferMode(CGLWidget::BF_TRIPLE);
}

void CMainWindow::UseSingleBuffer(bool checked)
{
    if (checked)
        m_ui.glwidget->ChangeBufferMode(CGLWidget::BF_SINGLE);
}

