#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <QWidget>
#include <QPoint>
#include <QVector>
#include <QTimer>
#include <QColor>
#include <QPainter>

// 棋盘大小
const int BOARD_SIZE = 8;
// 马的8种移动方向 (dx, dy)
const QPoint MOVE_DIRECTIONS[] = {
    QPoint(2, 1), QPoint(1, 2), QPoint(-1, 2), QPoint(-2, 1),
    QPoint(-2, -1), QPoint(-1, -2), QPoint(1, -2), QPoint(2, -1)
};

class Chessboard : public QWidget
{
    Q_OBJECT

public:
    explicit Chessboard(QWidget *parent = nullptr);
    ~Chessboard() = default;

    // 设置起始位置
    void setStartPosition(const QPoint& pos);
    // 开始遍历演示
    void startTour();
    // 重置棋盘
    void reset();
    // 切换演示速度
    void setSpeed(int level); // 0=慢, 1=中, 2=快

signals:
    // 状态更新信号
    void statusChanged(const QString& status);
    // 遍历完成信号
    void tourFinished(bool success);
    // 新增：通知主窗口启用/禁用开始按钮（关键！）
    void startBtnEnabled(bool enabled);

protected:
    // 重写绘图事件
    void paintEvent(QPaintEvent *event) override;
    // 重写鼠标点击事件
    void mousePressEvent(QMouseEvent *event) override;

private:
    // 核心算法：Warnsdorff 算法+回溯法求解哈密顿回路
    bool knightTour(const QPoint& startPos);
    // 回溯函数
    bool backtrack(int x, int y, int step);
    // 获取有效移动（未访问且在棋盘内）
    QVector<QPoint> getValidMoves(int x, int y);
    // Warnsdorff 排序：选择后续移动最少的方向
    void sortMovesByWarnsdorff(QVector<QPoint>& moves, int x, int y);
    // 检查是否能返回起点（完成回路）
    bool canReturnToStart(int x, int y);

    // 绘制棋盘
    void drawBoard(QPainter& painter);
    // 绘制棋子
    void drawKnight(QPainter& painter);
    // 绘制路径和步骤
    void drawPath(QPainter& painter);

    // 棋盘数据
    int m_board[BOARD_SIZE][BOARD_SIZE]; // 存储步骤（0=未访问，1~64=步骤，65=返回起点）
    bool m_visited[BOARD_SIZE][BOARD_SIZE]; // 标记是否访问
    QVector<QPoint> m_path; // 遍历路径

    // 状态变量
    QPoint m_startPos; // 起始位置（0-based）
    QPoint m_currentPos; // 当前位置
    bool m_isRunning; // 是否正在演示
    bool m_hasSolution; // 是否找到解
    int m_animationStep; // 动画当前步骤
    QTimer m_animationTimer; // 动画定时器
    int m_animationSpeed; // 动画速度（毫秒）

    // 绘图参数
    int m_cellSize; // 格子大小
    QColor m_lightColor; // 浅色格子
    QColor m_darkColor; // 深色格子
    QColor m_selectedColor; // 选中格子颜色
    QColor m_pathColor; // 路径颜色
    QColor m_currentColor; // 当前位置颜色
};

#endif // CHESSBOARD_H
