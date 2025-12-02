#include "chessboard.h"
#include <QMouseEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QDebug>
#include <QPixmap>
#include <QImage>
#include <algorithm>


Chessboard::Chessboard(QWidget *parent)
    : QWidget(parent)
    , m_isRunning(false)
    , m_hasSolution(false)
    , m_animationStep(0)
    , m_animationSpeed(500) // 默认中等速度
{
    // 初始化棋盘
    reset();

    // 加载马的图片
    loadKnightImage();

    // 动画定时器配置
    connect(&m_animationTimer, &QTimer::timeout, this, [this]() {
        if (m_animationStep < m_path.size()) {
            m_currentPos = m_path[m_animationStep];
            m_animationStep++;
            update(); // 刷新界面
        } else {
            // 演示完成，返回起点
            if (m_hasSolution) {
                m_currentPos = m_startPos;
                m_board[m_startPos.x()][m_startPos.y()] = BOARD_SIZE * BOARD_SIZE + 1;
                update();
                emit statusChanged("遍历完成！已返回起点（共64步）");
                emit tourFinished(true);
            } else {
                emit statusChanged("未找到有效路径，请重新选择起点");
                emit tourFinished(false);
            }
            m_animationTimer.stop();
            m_isRunning = false;
        }
    });

    // 初始化颜色
    m_lightColor = QColor(240, 217, 181); // #f0d9b5（浅棕）
    m_darkColor = QColor(181, 136, 99); // #b58863（深棕）
    m_selectedColor = QColor(100, 181, 246); // #64b5f6（蓝色，选中起点）
    m_pathColor = QColor(129, 199, 132); // #81c784（绿色，路径线）
    m_currentColor = QColor(255, 183, 77); // #ffb74d（橙色，当前位置）

    // 设置最小尺寸
    setMinimumSize(400, 400);
}

// 加载马的图片（资源文件路径需自行调整）
void Chessboard::loadKnightImage()
{
    m_knightPixmap.load(u8":/images/f2LNqD2fxJ.jpg");

    if (m_knightPixmap.isNull()) {
        qWarning() << "警告：马的图片加载失败！使用默认图形替代";
        QImage defaultImage(64, 64, QImage::Format_ARGB32);
        defaultImage.fill(QColor(72, 61, 139, 255)); // 深紫色方块
        m_knightPixmap = QPixmap::fromImage(defaultImage);
    } else {
        qDebug() << "马的图片加载成功，尺寸：" << m_knightPixmap.size();
    }
}

// 重置棋盘状态
void Chessboard::reset()
{
    m_animationTimer.stop();

    // 重置棋盘数据（0=未访问，1~64=访问步骤）
    memset(m_board, 0, sizeof(m_board));
    memset(m_visited, false, sizeof(m_visited));
    m_path.clear();

    // 重置状态变量
    m_startPos = QPoint(-1, -1);
    m_currentPos = QPoint(-1, -1);
    m_isRunning = false;
    m_hasSolution = false;
    m_animationStep = 0;

    update();
    emit statusChanged("请选择起始位置");
    emit startBtnEnabled(false); // 重置后禁用开始按钮
}

// 设置动画速度（0=慢，1=中，2=快）
void Chessboard::setSpeed(int level)
{
    switch (level) {
    case 0: m_animationSpeed = 1000; break;
    case 1: m_animationSpeed = 500; break;
    case 2: m_animationSpeed = 200; break;
    default: m_animationSpeed = 500;
    }
    m_animationTimer.setInterval(m_animationSpeed);
}

// 设置起始位置
void Chessboard::setStartPosition(const QPoint& pos)
{
    if (pos.x() < 0 || pos.x() >= BOARD_SIZE || pos.y() < 0 || pos.y() >= BOARD_SIZE)
        return;

    m_startPos = pos;
    m_currentPos = pos;
    m_board[pos.x()][pos.y()] = 1; // 起始位置标记为第1步
    m_visited[pos.x()][pos.y()] = true;

    update();
    emit statusChanged(QString("起始位置：(%1, %2)").arg(pos.x()+1).arg(pos.y()+1));
    emit startBtnEnabled(true); // 选择起点后启用开始按钮
}

// 开始骑士巡游（启动计算和动画）
void Chessboard::startTour()
{
    if (m_startPos.x() == -1 || m_isRunning)
        return;

    m_isRunning = true;
    emit statusChanged("正在计算路径...");
    emit startBtnEnabled(false);

    // 异步计算路径（避免阻塞UI）
    QTimer::singleShot(0, this, [this]() {
        m_hasSolution = knightTour(m_startPos);

        if (m_hasSolution) {
            emit statusChanged("开始演示遍历过程");
            m_animationStep = 1;
            m_animationTimer.setInterval(m_animationSpeed);
            m_animationTimer.start();
        } else {
            emit statusChanged("未找到有效路径，请重新选择起点");
            m_isRunning = false;
            emit tourFinished(false);
            emit startBtnEnabled(true);
        }
    });
}

// 骑士巡游主函数（入口）
bool Chessboard::knightTour(const QPoint& startPos)
{
    // 重置路径和访问标记
    m_path.clear();
    memset(m_visited, false, sizeof(m_visited));
    memset(m_board, 0, sizeof(m_board));

    int startX = startPos.x();
    int startY = startPos.y();
    m_visited[startX][startY] = true;
    m_board[startX][startY] = 1;
    m_path.append(startPos);

    // 回溯求解（从第2步开始）
    return backtrack(startX, startY, 2);
}

// 回溯算法核心（递归求解）
bool Chessboard::backtrack(int x, int y, int step)
{
    // 终止条件：已访问所有64个格子，且能返回起点（形成回路）
    if (step > BOARD_SIZE * BOARD_SIZE) {
        return canReturnToStart(x, y);
    }

    // 获取当前位置的所有有效移动（未访问且在棋盘内）
    QVector<QPoint> validMoves = getValidMoves(x, y);

    // 关键优化：按Warnsdorff规则排序有效移动
    sortMovesByWarnsdorff(validMoves, x, y);

    // 尝试所有排序后的移动
    for (const QPoint& move : validMoves) {
        int nx = move.x();
        int ny = move.y();

        // 标记为已访问，记录步骤和路径
        m_visited[nx][ny] = true;
        m_board[nx][ny] = step;
        m_path.append(QPoint(nx, ny));

        // 递归探索下一步
        if (backtrack(nx, ny, step + 1)) {
            return true; // 找到解，直接返回
        }

        // 回溯：撤销标记
        m_visited[nx][ny] = false;
        m_board[nx][ny] = 0;
        m_path.removeLast();
    }

    return false; // 无有效移动，回溯
}

// 获取当前位置的所有有效移动（未访问且在棋盘内）
QVector<QPoint> Chessboard::getValidMoves(int x, int y)
{
    QVector<QPoint> validMoves;
    int dirCount = sizeof(MOVE_DIRECTIONS) / sizeof(MOVE_DIRECTIONS[0]); // 计算方向数组长度
    for (int i = 0; i < dirCount; ++i) {
        const QPoint& dir = MOVE_DIRECTIONS[i]; // 通过下标访问
        int nx = x + dir.x();
        int ny = y + dir.y();
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && !m_visited[nx][ny]) {
            validMoves.append(QPoint(nx, ny));
        }
    }
    return validMoves;
}

// 核心优化：按Warnsdorff规则排序有效移动
void Chessboard::sortMovesByWarnsdorff(QVector<QPoint>& moves, int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);

    // 定义一个结构体存储移动的关键信息（避免重复计算）
    struct MoveInfo {
        QPoint pos;          // 移动目标位置
        int nextValidCount;  // 从该位置出发的有效移动数（Warnsdorff核心）
        int gridIndex;       // 方格序号（x*8 + y，用于相同优先级时排序）

        MoveInfo(const QPoint& p, int count, int index)
            : pos(p), nextValidCount(count), gridIndex(index) {}
    };

    QVector<MoveInfo> moveInfos;
    moveInfos.reserve(moves.size());

    // 为每个有效移动计算「后续有效移动数」和「方格序号」
    for (const QPoint& p : moves) {
        int nextCount = getValidMoves(p.x(), p.y()).size(); // 关键：计算后续可移动数
        int index = p.x() * BOARD_SIZE + p.y();            // 方格序号（行优先）
        moveInfos.emplace_back(p, nextCount, index);
    }

    // 按Warnsdorff规则排序：
    // 1. 优先选择「后续有效移动数少」的位置（避免提前陷入死胡同）
    // 2. 若后续移动数相同，优先选择「方格序号小」的位置（行优先，左上到右下）
    std::sort(moveInfos.begin(), moveInfos.end(), [](const MoveInfo& a, const MoveInfo& b) {
        if (a.nextValidCount != b.nextValidCount) {
            return a.nextValidCount < b.nextValidCount; // 规则1：后续移动数少的优先
        }
        return a.gridIndex < b.gridIndex; // 规则2：方格序号小的优先
    });

    // 重新构造排序后的移动列表
    moves.clear();
    moves.reserve(moveInfos.size());
    for (const MoveInfo& info : moveInfos) {
        moves.append(info.pos);
    }
}

// 检查当前位置是否能返回起点（形成回路）
bool Chessboard::canReturnToStart(int x, int y)
{
    int dirCount = sizeof(MOVE_DIRECTIONS) / sizeof(MOVE_DIRECTIONS[0]); // 计算方向数组长度
    for (int i = 0; i < dirCount; ++i) {
        const QPoint& dir = MOVE_DIRECTIONS[i]; // 通过下标访问
        int nx = x + dir.x();
        int ny = y + dir.y();
        if (nx == m_startPos.x() && ny == m_startPos.y()) {
            return true;
        }
    }
    return false;
}

// 绘制棋盘（重写paintEvent）
void Chessboard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿

    // 计算格子大小（自适应窗口，正方形格子）
    m_cellSize = qMin(width(), height()) / BOARD_SIZE;
    // 棋盘居中偏移
    int offsetX = (width() - m_cellSize * BOARD_SIZE) / 2;
    int offsetY = (height() - m_cellSize * BOARD_SIZE) / 2;
    painter.translate(offsetX, offsetY);

    // 绘制棋盘格子
    drawBoard(painter);
    // 绘制路径和步骤数
    drawPath(painter);
    // 绘制马的图标
    if (m_startPos.x() != -1 || m_isRunning) {
        drawKnight(painter);
    }
}

// 绘制棋盘格子（交替颜色）
void Chessboard::drawBoard(QPainter& painter)
{
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            QColor color = (x + y) % 2 == 0 ? m_lightColor : m_darkColor;
            // 起始位置高亮（未运行时）
            if (x == m_startPos.x() && y == m_startPos.y() && !m_isRunning) {
                color = m_selectedColor;
            }
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            painter.drawRect(x * m_cellSize, y * m_cellSize, m_cellSize, m_cellSize);
        }
    }
}

// 绘制路径线条和步骤数字
void Chessboard::drawPath(QPainter& painter)
{
    // 绘制路径线条（绿色）
    painter.setPen(QPen(m_pathColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (int i = 1; i < m_animationStep && i < m_path.size(); i++) {
        QPoint prev = m_path[i-1];
        QPoint curr = m_path[i];
        // 线条起点和终点（格子中心）
        int x1 = prev.x() * m_cellSize + m_cellSize / 2;
        int y1 = prev.y() * m_cellSize + m_cellSize / 2;
        int x2 = curr.x() * m_cellSize + m_cellSize / 2;
        int y2 = curr.y() * m_cellSize + m_cellSize / 2;
        painter.drawLine(x1, y1, x2, y2);
    }

    // 绘制步骤数字（白色，带黑色半透明背景）
    QFont font;
    font.setPointSize(m_cellSize / 4);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            int step = m_board[x][y];
            if (step > 0 && step <= m_animationStep) {
                // 绘制数字背景（黑色半透明圆形）
                painter.setBrush(QColor(0, 0, 0, 150));
                painter.drawEllipse(x * m_cellSize + 5, y * m_cellSize + 5, m_cellSize / 3, m_cellSize / 3);
                // 绘制步骤数字（居中）
                painter.drawText(x * m_cellSize + 5, y * m_cellSize + 5, m_cellSize / 3, m_cellSize / 3,
                                 Qt::AlignCenter, QString::number(step));
            }
        }
    }

    // 高亮当前位置（橙色半透明）
    if (m_currentPos.x() != -1) {
        painter.setBrush(QColor(m_currentColor.rgba() & 0x7FFFFFFF)); // 半透明处理（保留RGB，Alpha=127）
        painter.setPen(Qt::NoPen);
        painter.drawRect(m_currentPos.x() * m_cellSize, m_currentPos.y() * m_cellSize, m_cellSize, m_cellSize);
    }
}

// 绘制马的图标（使用图片或默认图形）
void Chessboard::drawKnight(QPainter& painter)
{
    if (m_currentPos.x() == -1 || m_knightPixmap.isNull())
        return;

    // 马的图标居中显示（占格子80%大小）
    int x = m_currentPos.x() * m_cellSize;
    int y = m_currentPos.y() * m_cellSize;
    int displaySize = m_cellSize * 0.8;
    int offset = (m_cellSize - displaySize) / 2; // 居中偏移

    // 平滑缩放图片
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPixmap scaledPixmap = m_knightPixmap.scaled(
        displaySize, displaySize,
        Qt::KeepAspectRatioByExpanding, // 保持比例并填充
        Qt::SmoothTransformation        // 平滑变换
        );

    // 绘制马的图标
    painter.drawPixmap(x + offset, y + offset, scaledPixmap);
}

// 鼠标点击选择起始位置
void Chessboard::mousePressEvent(QMouseEvent *event)
{
    if (m_isRunning)
        return;

    // 计算点击的格子坐标
    int offsetX = (width() - m_cellSize * BOARD_SIZE) / 2;
    int offsetY = (height() - m_cellSize * BOARD_SIZE) / 2;
    //用 position() 获取鼠标位置，转换为 int 类型
    int x = (event->position().x() - offsetX) / m_cellSize;
    int y = (event->position().y() - offsetY) / m_cellSize;

    // 检查是否在棋盘范围内
    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
        reset(); // 重置之前的选择
        setStartPosition(QPoint(x, y));
    }
}
