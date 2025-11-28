#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_speedLevel(1) // 默认中等速度
{
    ui->setupUi(this);

    // 初始化棋盘
    m_chessboard = new Chessboard(this);
    ui->centralWidget->layout()->addWidget(m_chessboard);

    // 连接信号槽
    connect(m_chessboard, &Chessboard::statusChanged, this, &MainWindow::updateStatus);
    connect(m_chessboard, &Chessboard::tourFinished, this, &MainWindow::onTourFinished);
    connect(m_chessboard, &Chessboard::startBtnEnabled, this, [this](bool enabled) {
        ui->startBtn->setEnabled(enabled);
    });

    // 初始化UI
    setWindowTitle("国际象棋马的遍历 - 哈密顿回路");
    ui->startBtn->setEnabled(false);
    ui->speedBtn->setText("速度：中等");
    updateStatus("请选择起始位置");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startBtn_clicked()
{
    ui->startBtn->setEnabled(false);
    ui->resetBtn->setEnabled(false);
    ui->speedBtn->setEnabled(false);
    m_chessboard->startTour();
}

void MainWindow::on_resetBtn_clicked()
{
    m_chessboard->reset();
    ui->startBtn->setEnabled(false);
    ui->speedBtn->setText("速度：中等");
    m_speedLevel = 1;
    m_chessboard->setSpeed(m_speedLevel);
}

void MainWindow::on_speedBtn_clicked()
{
    // 切换速度等级
    m_speedLevel = (m_speedLevel + 1) % 3;
    switch (m_speedLevel) {
    case 0: ui->speedBtn->setText("速度：慢速"); break;
    case 1: ui->speedBtn->setText("速度：中等"); break;
    case 2: ui->speedBtn->setText("速度：快速"); break;
    }
    m_chessboard->setSpeed(m_speedLevel);
}

void MainWindow::updateStatus(const QString& status)
{
    ui->statusLabel->setText(status);
}

void MainWindow::onTourFinished(bool success)
{
    ui->resetBtn->setEnabled(true);
    ui->speedBtn->setEnabled(true);
    if (!success) {
        ui->startBtn->setEnabled(true);
    }
}
