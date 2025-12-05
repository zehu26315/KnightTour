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

    // 安全添加棋盘：检查 centralWidget 是否有布局
    QLayout *layout = ui->centralWidget->layout();
    if (!layout) {
        layout = new QVBoxLayout(ui->centralWidget);
        layout->setContentsMargins(0, 0, 0, 0);
    }
    layout->addWidget(m_chessboard);

    // 连接信号槽
    connect(m_chessboard, &Chessboard::statusChanged, this, &MainWindow::updateStatus);
    connect(m_chessboard, &Chessboard::tourFinished, this, &MainWindow::onTourFinished);
    connect(m_chessboard, &Chessboard::startBtnEnabled, ui->startBtn, &QPushButton::setEnabled);
    connect(m_chessboard, &Chessboard::pausedChanged, this, [this](bool paused) {
        ui->pauseBtn->setText(paused ? "继续" : "暂停");
        ui->speedBtn->setEnabled(paused); // 暂停时启用速度按钮，继续时禁用
    });

    // 初始化UI状态
    setWindowTitle("国际象棋马的遍历 - 哈密顿回路");
    ui->startBtn->setEnabled(false);
    ui->speedBtn->setText("速度：中等");
    ui->pauseBtn->setVisible(false);
    updateStatus("请选择起始位置");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startBtn_clicked()
{
    // 开始后：禁用 start,reset 和 speed，显示 pause
    ui->resetBtn->setEnabled(false);
    ui->startBtn->setEnabled(false);
    ui->speedBtn->setEnabled(false);
    ui->pauseBtn->setVisible(true);
    m_chessboard->startTour();
}

void MainWindow::on_resetBtn_clicked()
{
    // 重置棋盘并恢复初始UI状态
    m_chessboard->reset();
    ui->pauseBtn->setText("暂停");
    m_chessboard->setPaused(false);
    ui->pauseBtn->setVisible(false);
    ui->speedBtn->setText("速度：中等");
    ui->speedBtn->setEnabled(true); // 重置后速度按钮必须启用
    m_speedLevel = 1;
    m_chessboard->setSpeed(m_speedLevel);
    // 注意：startBtn 状态由 Chessboard 通过 startBtnEnabled(false) 自动控制
}

void MainWindow::on_speedBtn_clicked()
{
    // 循环切换速度等级：0(慢) → 1(中) → 2(快) → 0
    m_speedLevel = (m_speedLevel + 1) % 3;
    switch (m_speedLevel) {
    case 0:
        ui->speedBtn->setText("速度：慢速");
        break;
    case 1:
        ui->speedBtn->setText("速度：中等");
        break;
    case 2:
        ui->speedBtn->setText("速度：快速");
        break;
    }
    m_chessboard->setSpeed(m_speedLevel);
}

void MainWindow::on_pauseBtn_clicked()
{
    // 切换暂停状态
    ui->resetBtn->setEnabled(true);
    bool currentlyPaused = (ui->pauseBtn->text() == "继续");
    m_chessboard->setPaused(!currentlyPaused);
}

void MainWindow::updateStatus(const QString &status)
{
    ui->statusLabel->setText(status);
}


void MainWindow::onTourFinished(bool success)
{
    // 遍历结束：隐藏暂停按钮，启用 speedBtn resetBtn
    ui->pauseBtn->setVisible(false);
    ui->resetBtn->setEnabled(true);
    ui->speedBtn->setEnabled(true);
    if (!success) {
        ui->startBtn->setEnabled(true); // 失败时允许重新开始
    }
    // 成功时 startBtn 保持禁用（需先重置或选新起点）
}
