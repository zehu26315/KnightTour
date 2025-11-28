#include "chessboard.h"
#include <QMouseEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QDebug>

Chessboard::Chessboard(QWidget *parent)
    : QWidget(parent)
    , m_isRunning(false)
    , m_hasSolution(false)
    , m_animationStep(0)
    , m_animationSpeed(500) // 默认中等速度
{
    // 初始化棋盘
    reset();

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
    m_lightColor = QColor(240, 217, 181); // #f0d9b5
    m_darkColor = QColor(181, 136, 99); // #b58863
    m_selectedColor = QColor(100, 181, 246); // #64b5f6
    m_pathColor = QColor(129, 199, 132); // #81c784
    m_currentColor = QColor(255, 183, 77); // #ffb74d

    // 设置最小尺寸
    setMinimumSize(400, 400);
}

void Chessboard::reset()
{
    // 停止定时器
    m_animationTimer.stop();

    // 重置棋盘数据
    memset(m_board, 0, sizeof(m_board));
    memset(m_visited, false, sizeof(m_visited));
    m_path.clear();

    // 重置状态
    m_startPos = QPoint(-1, -1);
    m_currentPos = QPoint(-1, -1);
    m_isRunning = false;
    m_hasSolution = false;
    m_animationStep = 0;

    // 刷新界面
    update();
    emit statusChanged("请选择起始位置");
    // 重置后禁用开始按钮（关键！）
    emit startBtnEnabled(false);
}

void Chessboard::setSpeed(int level)
{
    // 0=慢(1000ms), 1=中(500ms), 2=快(200ms)
    switch (level) {
    case 0: m_animationSpeed = 1000; break;
    case 1: m_animationSpeed = 500; break;
    case 2: m_animationSpeed = 200; break;
    default: m_animationSpeed = 500;
    }
    m_animationTimer.setInterval(m_animationSpeed);
}

void Chessboard::setStartPosition(const QPoint& pos)
{
    if (pos.x() < 0 || pos.x() >= BOARD_SIZE || pos.y() < 0 || pos.y() >= BOARD_SIZE)
        return;

    m_startPos = pos;
    m_currentPos = pos;
    m_board[pos.x()][pos.y()] = 1;
    m_visited[pos.x()][pos.y()] = true;

    update();
    emit statusChanged(QString("起始位置：(%1, %2)").arg(pos.x()+1).arg(pos.y()+1));
    // 选择起始位置后，启用开始按钮（关键！）
    emit startBtnEnabled(true);
}

void Chessboard::startTour()
{
    if (m_startPos.x() == -1 || m_isRunning)
        return;

    m_isRunning = true;
    emit statusChanged("正在计算路径...");
    // 演示过程中禁用按钮
    emit startBtnEnabled(false);

    // 启动线程计算路径（避免阻塞UI）
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
            // 未找到路径时，重新启用按钮
            emit startBtnEnabled(true);
        }
    });
}

// 以下函数（knightTour、backtrack、getValidMoves 等）保持不变，无需修改
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

    // 回溯求解
    bool success = backtrack(startX, startY, 2);
    return success;
}

bool Chessboard::backtrack(int x, int y, int step)
{
    // 已访问所有格子，检查是否能返回起点（完成回路）
    if (step > BOARD_SIZE * BOARD_SIZE) {
        return canReturnToStart(x, y);
    }

    // 剪枝优化：剩余未访问方格数 = 总方格数 - (step-1)
    int remaining = BOARD_SIZE * BOARD_SIZE - (step - 1);
    // 当前位置的最大可能后续移动数 = 8（马最多8个方向）
    // 若剩余方格数 > 最大可能后续移动数×剩余步数（极端情况），直接返回无解
    if (remaining > 8 * (BOARD_SIZE * BOARD_SIZE - step + 1)) {
        return false;
    }

    // 获取有效移动并按 Warnsdorff 规则排序（优化后）
    QVector<QPoint> moves = getValidMoves(x, y);
    sortMovesByWarnsdorff(moves, x, y);

    // 尝试所有可能的移动
    for (const QPoint& move : moves) {
        int nx = move.x();
        int ny = move.y();

        if (m_visited[nx][ny])
            continue;

        // 标记访问
        m_visited[nx][ny] = true;
        m_board[nx][ny] = step;
        m_path.append(QPoint(nx, ny));

        // 递归回溯
        if (backtrack(nx, ny, step + 1)) {
            return true;
        }

        // 回溯
        m_visited[nx][ny] = false;
        m_board[nx][ny] = 0;
        m_path.removeLast();
    }

    return false;
}

QVector<QPoint> Chessboard::getValidMoves(int x, int y)
{
    QVector<QPoint> validMoves;
    for (const QPoint& dir : MOVE_DIRECTIONS) {
        int nx = x + dir.x();
        int ny = y + dir.y();
        // 检查是否在棋盘内且未访问
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && !m_visited[nx][ny]) {
            validMoves.append(QPoint(nx, ny));
        }
    }
    return validMoves;
}

void Chessboard::sortMovesByWarnsdorff(QVector<QPoint>& moves, int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);

    // 缓存每个移动的（后续有效移动数，方格序号），避免重复计算
    QVector<std::tuple<int, int, QPoint>> moveInfo; // (count, index, point)
    for (const QPoint& p : moves) {
        int count = getValidMoves(p.x(), p.y()).size();
        int index = p.x() * BOARD_SIZE + p.y();
        moveInfo.emplace_back(count, index, p);
    }

    // 按规则排序
    std::sort(moveInfo.begin(), moveInfo.end(), [](const auto& t1, const auto& t2) {
        int count1 = std::get<0>(t1);
        int count2 = std::get<0>(t2);
        if (count1 != count2) {
            return count1 < count2;
        }
        int index1 = std::get<1>(t1);
        int index2 = std::get<1>(t2);
        return index1 < index2;
    });

    // 重新构造排序后的 moves 向量
    moves.clear();
    for (const auto& info : moveInfo) {
        moves.append(std::get<2>(info));
    }
}

bool Chessboard::canReturnToStart(int x, int y)
{
    // 检查是否能从当前位置返回起点
    for (const QPoint& dir : MOVE_DIRECTIONS) {
        int nx = x + dir.x();
        int ny = y + dir.y();
        if (nx == m_startPos.x() && ny == m_startPos.y()) {
            return true;
        }
    }
    return false;
}

void Chessboard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿

    // 计算格子大小（自适应窗口）
    m_cellSize = qMin(width(), height()) / BOARD_SIZE;
    // 设置棋盘居中
    int offsetX = (width() - m_cellSize * BOARD_SIZE) / 2;
    int offsetY = (height() - m_cellSize * BOARD_SIZE) / 2;
    painter.translate(offsetX, offsetY);

    // 绘制棋盘
    drawBoard(painter);
    // 绘制路径
    drawPath(painter);
    // 绘制棋子（如果已选择起点或正在演示）
    if (m_startPos.x() != -1 || m_isRunning) {
        drawKnight(painter);
    }
}

void Chessboard::drawBoard(QPainter& painter)
{
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            // 交替颜色
            QColor color = (x + y) % 2 == 0 ? m_lightColor : m_darkColor;
            // 起始位置高亮
            if (x == m_startPos.x() && y == m_startPos.y() && !m_isRunning) {
                color = m_selectedColor;
            }
            // 绘制格子
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            painter.drawRect(x * m_cellSize, y * m_cellSize, m_cellSize, m_cellSize);
        }
    }
}

void Chessboard::drawPath(QPainter& painter)
{
    // 绘制路径线条
    painter.setPen(QPen(m_pathColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (int i = 1; i < m_animationStep && i < m_path.size(); i++) {
        QPoint prev = m_path[i-1];
        QPoint curr = m_path[i];
        int x1 = prev.x() * m_cellSize + m_cellSize / 2;
        int y1 = prev.y() * m_cellSize + m_cellSize / 2;
        int x2 = curr.x() * m_cellSize + m_cellSize / 2;
        int y2 = curr.y() * m_cellSize + m_cellSize / 2;
        painter.drawLine(x1, y1, x2, y2);
    }

    // 绘制步骤数字
    QFont font;
    font.setPointSize(m_cellSize / 4);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            int step = m_board[x][y];
            if (step > 0 && step <= m_animationStep) {
                // 绘制步骤背景
                painter.setBrush(QColor(0, 0, 0, 150));
                painter.drawEllipse(x * m_cellSize + 5, y * m_cellSize + 5, m_cellSize / 3, m_cellSize / 3);
                // 绘制步骤数字
                painter.drawText(x * m_cellSize + 5, y * m_cellSize + 5, m_cellSize / 3, m_cellSize / 3,
                                 Qt::AlignCenter, QString::number(step));
            }
        }
    }

    // 高亮当前位置
    if (m_currentPos.x() != -1) {
        painter.setBrush(QColor(m_currentColor.rgba() & 0x7FFFFFFF)); // 半透明
        painter.setPen(Qt::NoPen);
        painter.drawRect(m_currentPos.x() * m_cellSize, m_currentPos.y() * m_cellSize, m_cellSize, m_cellSize);
    }
}

void Chessboard::drawKnight(QPainter& painter)
{
    if (m_currentPos.x() == -1)
        return;

    // 计算棋子位置（居中显示）
    int x = m_currentPos.x() * m_cellSize + m_cellSize / 2;
    int y = m_currentPos.y() * m_cellSize + m_cellSize / 2;
    int size = m_cellSize * 0.7;

    // 绘制马的图标（这里使用简单图形模拟，实际可替换为图片）
    painter.setBrush(QColor(72, 61, 139)); // 深紫色
    painter.setPen(Qt::NoPen);

    // 马的头部
    painter.drawEllipse(x - size/2, y - size/2, size/2, size/2);
    // 马的身体
    painter.drawRect(x - size/4, y - size/4, size/2, size);
    // 马的四肢
    painter.drawRect(x - size/2, y + size/4, size/6, size/3);
    painter.drawRect(x + size/3, y + size/4, size/6, size/3);
    painter.drawRect(x - size/2, y - size/12, size/6, size/3);
    painter.drawRect(x + size/3, y - size/12, size/6, size/3);
    // 马的尾巴
    painter.drawEllipse(x + size/4, y - size/2, size/4, size/2);
}

void Chessboard::mousePressEvent(QMouseEvent *event)
{
    if (m_isRunning)
        return;

    // 计算点击的格子坐标
    int offsetX = (width() - m_cellSize * BOARD_SIZE) / 2;
    int offsetY = (height() - m_cellSize * BOARD_SIZE) / 2;
    int x = (event->x() - offsetX) / m_cellSize;
    int y = (event->y() - offsetY) / m_cellSize;

    // 检查是否在棋盘内
    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
        reset(); // 重置之前的选择
        setStartPosition(QPoint(x, y));
    }
}
