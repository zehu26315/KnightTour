#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <QWidget>
#include <QPoint>
#include <QVector>
#include <QTimer>
#include <QColor>
#include <QPainter>
#include <QElapsedTimer>
#include <QString>

// 常量集中定义（与cpp文件保持一致，便于维护）
constexpr int BOARD_SIZE = 8;                  // 棋盘大小（8x8）
constexpr int MAX_BACKTRACK_TIME = 3000;       // 回溯算法超时时间（ms）
constexpr int MIN_WINDOW_SIZE = 400;           // 窗口最小尺寸
constexpr int MOVE_COUNT = 8;                  // 马的移动方向数量

// 马的8种移动方向 (dx, dy)
const QPoint MOVE_DIRECTIONS[MOVE_COUNT] = {
    QPoint(2, 1), QPoint(1, 2), QPoint(-1, 2), QPoint(-2, 1),
    QPoint(-2, -1), QPoint(-1, -2), QPoint(1, -2), QPoint(2, -1)
};

/**
 * @brief 骑士巡游棋盘组件
 * 实现基于 Warnsdorff 算法+回溯法的骑士巡游（哈密顿回路），支持可视化演示
 */
class Chessboard : public QWidget
{
    Q_OBJECT

public:
    explicit Chessboard(QWidget *parent = nullptr);
    ~Chessboard() = default;

    /**
     * @brief 设置起始位置
     * @param pos 棋盘坐标（0-based，x:0-7，y:0-7）
     */
    void setStartPosition(const QPoint& pos);

    /**
     * @brief 开始骑士巡游计算与演示
     * 需先设置有效起始位置，否则不生效
     */
    void startTour();

    /**
     * @brief 重置棋盘状态
     * 清空路径、步骤标记、动画状态，恢复初始状态
     */
    void reset();

    /**
     * @brief 设置动画演示速度
     * @param level 速度等级（0=慢，1=中，2=快），超出范围默认中等速度
     */
    void setSpeed(int level);

signals:
    /**
     * @brief 状态更新信号
     * 用于通知主窗口更新状态提示文本
     * @param status 状态描述字符串
     */
    void statusChanged(const QString& status);

    /**
     * @brief 遍历完成信号
     * 通知主窗口遍历结束（成功/失败）
     * @param success 是否找到有效路径
     */
    void tourFinished(bool success);

    /**
     * @brief 开始按钮启用状态信号
     * 通知主窗口启用/禁用开始按钮
     * @param enabled true=启用，false=禁用
     */
    void startBtnEnabled(bool enabled);

protected:
    /**
     * @brief 重写绘图事件
     * 分层绘制棋盘、路径、步骤数字、马的图标
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 重写鼠标点击事件
     * 未运行时支持点击选择起始位置
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief 重写窗口大小变化事件
     * 窗口缩放时自适应调整棋盘布局
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /**
     * @brief 动画定时器超时处理
     * 控制动画步骤推进、界面刷新
     */
    void onAnimationTimeout();

private:
    // -------------------------- 初始化相关函数 --------------------------
    /**
     * @brief 初始化窗口基础配置
     * 设置最小尺寸、窗口标题、鼠标指针等
     */
    void initWidget();

    /**
     * @brief 初始化动画定时器
     * 设置默认间隔、连接超时信号
     */
    void initAnimationTimer();

    /**
     * @brief 加载马的图标图片
     * 支持多路径 fallback，加载失败时创建默认图形
     */
    void loadKnightImage();

    // -------------------------- 算法核心函数 --------------------------
    /**
     * @brief 路径计算（独立函数，异步执行）
     * 初始化计算状态，调用回溯算法求解路径
     */
    void calculateTour();

    /**
     * @brief 骑士巡游核心算法入口
     * 初始化路径和访问标记，启动回溯求解
     * @param startPos 起始位置
     * @return 是否找到有效回路
     */
    bool knightTour(const QPoint& startPos);

    /**
     * @brief 回溯算法核心
     * 递归探索所有有效移动，求解骑士巡游路径
     * @param x 当前位置x坐标
     * @param y 当前位置y坐标
     * @param step 当前步骤数（从2开始，1为起点）
     * @return 是否找到有效路径
     */
    bool backtrack(int x, int y, int step);

    /**
     * @brief 检查坐标是否有效（在棋盘内）
     * @param pos 待检查坐标
     * @return true=有效，false=无效
     */
    bool isValidPos(const QPoint& pos) const;

    /**
     * @brief 获取当前位置的所有有效移动
     * 筛选未访问且在棋盘内的移动方向
     * @param x 当前位置x坐标
     * @param y 当前位置y坐标
     * @return 有效移动方向列表（相对坐标）
     */
    QVector<QPoint> getValidMoves(int x, int y) const;

    /**
     * @brief 按 Warnsdorff 规则排序有效移动
     * 优先选择后续有效移动最少的方向，提高求解效率
     * @param moves 待排序的有效移动列表
     * @param x 当前位置x坐标
     * @param y 当前位置y坐标
     * @param step 当前步骤数（用于最后一步特殊处理）
     */
    void sortMovesByWarnsdorff(QVector<QPoint>& moves, int x, int y, int step);

    /**
     * @brief 计数目标位置的有效移动数
     * 用于 Warnsdorff 排序的优先级计算
     * @param x 目标位置x坐标
     * @param y 目标位置y坐标
     * @return 有效移动数
     */
    int countValidMoves(int x, int y) const;

    /**
     * @brief 检查当前位置是否能返回起点（形成回路）
     * @param x 当前位置x坐标
     * @param y 当前位置y坐标
     * @return true=可以返回起点，false=不能
     */
    bool canReturnToStart(int x, int y) const;

    /**
     * @brief 判断两个位置是否为马的有效移动
     * @param pos1 起始位置
     * @param pos2 目标位置
     * @return true=有效移动，false=无效
     */
    bool isMoveValid(const QPoint& pos1, const QPoint& pos2) const;

    // -------------------------- 绘制相关函数 --------------------------
    /**
     * @brief 绘制棋盘格子（交替颜色）
     * @param painter 绘图对象
     * @param cellSize 格子大小（像素）
     */
    void drawBoardGrid(QPainter& painter, int cellSize);

    /**
     * @brief 绘制路径线条（连接已遍历的步骤）
     * @param painter 绘图对象
     * @param cellSize 格子大小（像素）
     */
    void drawPathLines(QPainter& painter, int cellSize);

    /**
     * @brief 绘制步骤数字（每个格子的访问顺序）
     * @param painter 绘图对象
     * @param cellSize 格子大小（像素）
     */
    void drawStepNumbers(QPainter& painter, int cellSize);

    /**
     * @brief 高亮当前动画位置
     * @param painter 绘图对象
     * @param cellSize 格子大小（像素）
     */
    void drawCurrentPosition(QPainter& painter, int cellSize);

    /**
     * @brief 绘制马的图标（居中显示在当前位置）
     * @param painter 绘图对象
     * @param cellSize 格子大小（像素）
     */
    void drawKnightIcon(QPainter& painter, int cellSize);

    /**
     * @brief 动画结束处理
     * 停止定时器、更新状态、通知主窗口
     */
    void finishAnimation();

    // -------------------------- 成员变量 --------------------------
    // 棋盘数据
    int m_board[BOARD_SIZE][BOARD_SIZE] = {{0}};        // 步骤标记（0=未访问，1~64=步骤，65=返回起点）
    bool m_visited[BOARD_SIZE][BOARD_SIZE] = {{false}}; // 访问标记
    QVector<QPoint> m_path;                           // 遍历路径存储

    // 状态变量
    QPoint m_startPos = {-1, -1};    // 起始位置（默认无效）
    QPoint m_currentPos = {-1, -1};  // 动画当前位置
    bool m_isRunning = false;        // 是否正在演示动画
    bool m_hasSolution = false;      // 是否找到有效路径
    int m_animationStep = 0;         // 动画当前步骤索引
    int m_animationSpeed = 500;      // 动画速度（毫秒/步）

    // 工具对象
    QTimer m_animationTimer;         // 动画定时器
    QPixmap m_knightPixmap;          // 马的图标图片

    // 绘图颜色配置（默认初始化，可在构造函数调整）
    QColor m_lightColor = QColor(240, 217, 181);    // 浅色格子（#f0d9b5）
    QColor m_darkColor = QColor(181, 136, 99);      // 深色格子（#b58863）
    QColor m_selectedColor = QColor(100, 181, 246); // 选中起点颜色（#64b5f6）
    QColor m_pathColor = QColor(129, 199, 132);     // 路径颜色（#81c784）
    QColor m_currentColor = QColor(255, 183, 77);   // 当前位置颜色（#ffb74d）
};

#endif // CHESSBOARD_H
