#include "chessboard.h"
#include <QMouseEvent>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QDebug>
#include <QPixmap>
#include <QImage>
#include <algorithm>
#include <QElapsedTimer>
#include <QApplication>


Chessboard::Chessboard(QWidget *parent)
    : QWidget(parent)
    , m_isRunning(false)
    , m_hasSolution(false)
    , m_animationStep(0)
    , m_animationSpeed(500) // 默认中等速度
    , m_lightColor(240, 217, 181)    // #f0d9b5（浅棕）
    , m_darkColor(181, 136, 99)      // #b58863（深棕）
    , m_selectedColor(100, 181, 246) // #64b5f6（蓝色，选中起点）
    , m_pathColor(129, 199, 132)     // #81c784（绿色，路径线）
    , m_currentColor(255, 183, 77)   // #ffb74d（橙色，当前位置）
{
    // 初始化配置集中化
    initWidget();
    loadKnightImage();
    initAnimationTimer();
    reset(); // 初始化棋盘状态
}

// 初始化窗口基础配置
void Chessboard::initWidget()
{
    setMinimumSize(MIN_WINDOW_SIZE, MIN_WINDOW_SIZE);
    setWindowTitle("骑士巡游");
    // 设置鼠标指针（棋盘模式时显示手型）
    setCursor(Qt::PointingHandCursor);
}

// 初始化动画定时器
void Chessboard::initAnimationTimer()
{
    m_animationTimer.setInterval(m_animationSpeed);
    connect(&m_animationTimer, &QTimer::timeout, this, &Chessboard::onAnimationTimeout);
}

// 加载马的图片（优化资源加载逻辑）
void Chessboard::loadKnightImage()
{
    // 支持多路径 fallback，提高可靠性
    const QStringList imagePaths = {
        u8":/images/knight.png",
        u8":/images/f2LNqD2fxJ.jpg",
        u8":/images/knight_default.png"
    };

    for (const QString& path : imagePaths) {
        m_knightPixmap.load(path);
        if (!m_knightPixmap.isNull()) {
            qDebug() << "马的图片加载成功，路径：" << path << " 尺寸：" << m_knightPixmap.size();
            return;
        }
    }

    // 完全加载失败时，创建更美观的默认图形
    qWarning() << "警告：所有马的图片路径加载失败！使用默认图形替代";
    QImage defaultImage(64, 64, QImage::Format_ARGB32);
    defaultImage.fill(Qt::transparent); // 透明背景
    QPainter p(&defaultImage);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(72, 61, 139)); // 深紫色
    p.setPen(Qt::white);
    p.drawEllipse(4, 4, 56, 56);     // 圆形底座
    p.setBrush(Qt::white);
    p.drawText(defaultImage.rect(), Qt::AlignCenter, "马"); // 中文标识
    m_knightPixmap = QPixmap::fromImage(defaultImage);
}

// 重置棋盘状态（优化状态清零逻辑）
void Chessboard::reset()
{
    m_animationTimer.stop();

    // 高效重置数组（避免重复 memset）
    std::fill_n(&m_board[0][0], BOARD_SIZE * BOARD_SIZE, 0);
    std::fill_n(&m_visited[0][0], BOARD_SIZE * BOARD_SIZE, false);

    // 状态变量统一重置
    m_path.clear();
    m_startPos = m_currentPos = QPoint(-1, -1);
    m_isRunning = m_hasSolution = false;
    m_animationStep = 0;

    update();
    emit statusChanged(tr("请选择起始位置"));
    emit startBtnEnabled(false);
}

// 设置动画速度（优化参数校验和用户反馈）
void Chessboard::setSpeed(int level)
{
    // 范围校验，避免无效值
    level = qBound(0, level, 2);
    switch (level) {
    case 0: m_animationSpeed = 1000; emit statusChanged(tr("动画速度：慢")); break;
    case 1: m_animationSpeed = 500;  emit statusChanged(tr("动画速度：中")); break;
    case 2: m_animationSpeed = 200;  emit statusChanged(tr("动画速度：快")); break;
    }
    m_animationTimer.setInterval(m_animationSpeed);
}

// 设置起始位置（优化参数校验和状态一致性）
void Chessboard::setStartPosition(const QPoint& pos)
{
    // 严格坐标校验
    if (!isValidPos(pos)) {
        qWarning() << "无效的起始位置：" << pos;
        return;
    }

    m_startPos = m_currentPos = pos;
    m_visited[pos.x()][pos.y()] = true;
    m_board[pos.x()][pos.y()] = 1;
    m_path.append(pos);

    update();
    emit statusChanged(tr("起始位置：(%1, %2)").arg(pos.x()+1).arg(pos.y()+1));
    emit startBtnEnabled(true);
}

// 开始骑士巡游（优化异步逻辑和用户体验）
void Chessboard::startTour()
{
    if (!isValidPos(m_startPos) || m_isRunning) {
        emit statusChanged(tr("无法开始：请先选择有效起始位置"));
        return;
    }

    m_isRunning = true;
    emit statusChanged(tr("正在计算路径..."));
    emit startBtnEnabled(false);

    // 异步计算（使用Qt::QueuedConnection确保UI响应）
    QMetaObject::invokeMethod(this, &Chessboard::calculateTour, Qt::QueuedConnection);
}

// 路径计算（独立函数，便于调试和维护）
void Chessboard::calculateTour()
{
    QElapsedTimer timer;
    timer.start();

    // 重新初始化计算相关状态（避免残留数据影响）
    std::fill_n(&m_visited[0][0], BOARD_SIZE * BOARD_SIZE, false);
    std::fill_n(&m_board[0][0], BOARD_SIZE * BOARD_SIZE, 0);
    m_path.clear();

    m_visited[m_startPos.x()][m_startPos.y()] = true;
    m_board[m_startPos.x()][m_startPos.y()] = 1;
    m_path.append(m_startPos);

    m_hasSolution = backtrack(m_startPos.x(), m_startPos.y(), 2);
    qDebug() << "路径计算耗时：" << timer.elapsed() << "ms，是否找到解：" << m_hasSolution;

    if (m_hasSolution) {
        emit statusChanged(tr("开始演示遍历过程（共%1步）").arg(m_path.size()));
        m_animationStep = 1;
        m_animationTimer.start();
    } else {
        emit statusChanged(tr("未找到有效路径（计算耗时%1ms），请重新选择起点").arg(timer.elapsed()));
        m_isRunning = false;
        emit tourFinished(false);
        emit startBtnEnabled(true);
    }
}

// 动画定时器超时处理（独立槽函数，逻辑清晰）
void Chessboard::onAnimationTimeout()
{
    if (m_animationStep < m_path.size()) {
        m_currentPos = m_path[m_animationStep];
        m_animationStep++;
        update();
        // 实时更新进度
        emit statusChanged(tr("遍历中：第%1步").arg(m_animationStep));
    } else {
        finishAnimation();
    }
}

// 动画结束处理（统一收尾逻辑）
void Chessboard::finishAnimation()
{
    m_animationTimer.stop();
    m_isRunning = false;

    if (m_hasSolution) {
        // 标记返回起点的步骤（视觉优化）
        m_board[m_startPos.x()][m_startPos.y()] = BOARD_SIZE * BOARD_SIZE + 1;
        m_currentPos = m_startPos;
        update();
        emit statusChanged(tr("遍历完成！已返回起点（共%1步）").arg(BOARD_SIZE * BOARD_SIZE + 1));
        emit tourFinished(true);
    } else {
        emit statusChanged(tr("遍历中断：未找到完整路径"));
        emit tourFinished(false);
    }
}

// 回溯算法核心（优化剪枝和性能）
bool Chessboard::backtrack(int x, int y, int step)
{
    // 超时保护（避免无限递归）
    static QElapsedTimer backtrackTimer;
    if (step == 2) { // 首次调用时启动计时器
        backtrackTimer.start();
    } else if (backtrackTimer.elapsed() > MAX_BACKTRACK_TIME) {
        qWarning() << "回溯超时，终止计算";
        return false;
    }

    const int totalSteps = BOARD_SIZE * BOARD_SIZE;
    // 终止条件：完成所有格子遍历
    if (step > totalSteps) {
        return canReturnToStart(x, y);
    }

    // 获取并排序有效移动（Warnsdorff优化）
    QVector<QPoint> validMoves = getValidMoves(x, y);
    if (validMoves.isEmpty()) {
        return false;
    }
    sortMovesByWarnsdorff(validMoves, x, y, step);

    // 尝试所有移动（优化循环效率）
    for (const QPoint& dir : validMoves) {
        const int nx = x + dir.x();
        const int ny = y + dir.y();

        // 跳过已访问的位置（双重校验，避免无效操作）
        if (m_visited[nx][ny]) {
            continue;
        }

        // 标记访问状态
        m_visited[nx][ny] = true;
        m_board[nx][ny] = step;
        m_path.append(QPoint(nx, ny));

        // 递归探索（提前返回，减少栈开销）
        if (backtrack(nx, ny, step + 1)) {
            return true;
        }

        // 回溯：撤销标记（严格对称操作）
        m_visited[nx][ny] = false;
        m_board[nx][ny] = 0;
        m_path.removeLast();
    }

    return false;
}

// 检查坐标是否有效（工具函数，减少重复代码）
bool Chessboard::isValidPos(const QPoint& pos) const
{
    return pos.x() >= 0 && pos.x() < BOARD_SIZE &&
           pos.y() >= 0 && pos.y() < BOARD_SIZE;
}

// 获取有效移动（优化循环效率）
QVector<QPoint> Chessboard::getValidMoves(int x, int y) const
{
    QVector<QPoint> moves;
    moves.reserve(MOVE_COUNT); // 预分配空间，避免多次扩容

    for (const QPoint& dir : MOVE_DIRECTIONS) {
        const int nx = x + dir.x();
        const int ny = y + dir.y();
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && !m_visited[nx][ny]) {
            moves.append(dir);
        }
    }

    return moves;
}

// Warnsdorff规则排序（优化性能和路径成功率）
void Chessboard::sortMovesByWarnsdorff(QVector<QPoint>& moves, int x, int y, int step)
{
    const int totalSteps = BOARD_SIZE * BOARD_SIZE;
    const bool isFinalStep = (step == totalSteps);

    // 按优先级排序：1. 是否能返回起点（最后一步） 2. 后续有效移动数 3. 坐标序号
    std::sort(moves.begin(), moves.end(), [this, x, y, isFinalStep](const QPoint& a, const QPoint& b) {
        const int ax = x + a.x();
        const int ay = y + a.y();
        const int bx = x + b.x();
        const int by = y + b.y();

        // 最后一步优先选择能返回起点的移动
        if (isFinalStep) {
            const bool aCanReturn = canReturnToStart(ax, ay);
            const bool bCanReturn = canReturnToStart(bx, by);
            if (aCanReturn != bCanReturn) {
                return aCanReturn;
            }
        }

        // 核心规则：后续有效移动数少的优先（避免死胡同）
        const int aCount = countValidMoves(ax, ay);
        const int bCount = countValidMoves(bx, by);
        if (aCount != bCount) {
            return aCount < bCount;
        }

        // 辅助排序：坐标序号（确保排序稳定性）
        const int aIndex = ax * BOARD_SIZE + ay;
        const int bIndex = bx * BOARD_SIZE + by;
        return aIndex < bIndex;
    });
}

// 计数有效移动（const优化，避免修改成员）
int Chessboard::countValidMoves(int x, int y) const
{
    int count = 0;
    for (const QPoint& dir : MOVE_DIRECTIONS) {
        const int nx = x + dir.x();
        const int ny = y + dir.y();
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && !m_visited[nx][ny]) {
            count++;
        }
    }
    return count;
}

// 检查是否能返回起点（优化循环效率）
bool Chessboard::canReturnToStart(int x, int y) const
{
    for (const QPoint& dir : MOVE_DIRECTIONS) {
        if (x + dir.x() == m_startPos.x() && y + dir.y() == m_startPos.y()) {
            return true;
        }
    }
    return false;
}

// 绘制棋盘（优化绘制效率和视觉效果）
void Chessboard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHints({QPainter::Antialiasing, QPainter::SmoothPixmapTransform});

    // 计算棋盘布局（自适应窗口，保持正方形）
    const int cellSize = qMin(width(), height()) / BOARD_SIZE;
    const int offsetX = (width() - cellSize * BOARD_SIZE) / 2;
    const int offsetY = (height() - cellSize * BOARD_SIZE) / 2;

    // 保存画家状态，避免影响其他绘制
    painter.save();
    painter.translate(offsetX, offsetY);

    // 分层绘制（按顺序优化渲染效率）
    drawBoardGrid(painter, cellSize);
    drawPathLines(painter, cellSize);
    drawStepNumbers(painter, cellSize);
    drawCurrentPosition(painter, cellSize);
    drawKnightIcon(painter, cellSize);

    painter.restore();
}

// 绘制棋盘格子（优化颜色切换逻辑）
void Chessboard::drawBoardGrid(QPainter& painter, int cellSize)
{
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            QColor color = ((x + y) % 2 == 0) ? m_lightColor : m_darkColor;
            // 未运行时高亮起点
            if (!m_isRunning && x == m_startPos.x() && y == m_startPos.y()) {
                color = m_selectedColor;
            }
            painter.fillRect(x * cellSize, y * cellSize, cellSize, cellSize, color);
        }
    }
}

// 绘制路径线条（优化线条绘制效率）
void Chessboard::drawPathLines(QPainter& painter, int cellSize)
{
    if (m_path.size() < 2 || m_animationStep < 2) {
        return;
    }

    painter.setPen(QPen(m_pathColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    const int halfSize = cellSize / 2;

    for (int i = 1; i < m_animationStep && i < m_path.size(); i++) {
        const QPoint& prev = m_path[i-1];
        const QPoint& curr = m_path[i];
        const int x1 = prev.x() * cellSize + halfSize;
        const int y1 = prev.y() * cellSize + halfSize;
        const int x2 = curr.x() * cellSize + halfSize;
        const int y2 = curr.y() * cellSize + halfSize;
        painter.drawLine(x1, y1, x2, y2);
    }
}

// 绘制步骤数字（优化字体适配和视觉效果）
void Chessboard::drawStepNumbers(QPainter& painter, int cellSize)
{
    QFont font;
    font.setPointSizeF(cellSize * 0.25); // 自适应字体大小
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);

    const int dotSize = cellSize / 3;
    const QRect dotRect(0, 0, dotSize, dotSize);

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            const int step = m_board[x][y];
            if (step > 0 && step <= m_animationStep) {
                // 绘制半透明黑色背景圆
                painter.setBrush(QColor(0, 0, 0, 180));
                painter.drawEllipse(x * cellSize + 5, y * cellSize + 5, dotSize, dotSize);
                // 绘制居中数字
                painter.drawText(x * cellSize + 5, y * cellSize + 5, dotSize, dotSize,
                                 Qt::AlignCenter, QString::number(step));
            }
        }
    }
}

// 绘制当前位置（优化高亮效果）
void Chessboard::drawCurrentPosition(QPainter& painter, int cellSize)
{
    if (!isValidPos(m_currentPos)) {
        return;
    }

    // 半透明橙色覆盖（不影响底层数字）
    QColor highlightColor = m_currentColor;
    highlightColor.setAlpha(127);
    painter.fillRect(m_currentPos.x() * cellSize, m_currentPos.y() * cellSize,
                     cellSize, cellSize, highlightColor);
}

// 绘制马的图标（优化缩放和居中）
void Chessboard::drawKnightIcon(QPainter& painter, int cellSize)
{
    if (!isValidPos(m_currentPos) || m_knightPixmap.isNull()) {
        return;
    }

    const int iconSize = cellSize * 0.8;
    const int offset = (cellSize - iconSize) / 2;
    const QRect iconRect(m_currentPos.x() * cellSize + offset,
                         m_currentPos.y() * cellSize + offset,
                         iconSize, iconSize);

    // 保持比例缩放，避免变形
    painter.drawPixmap(iconRect, m_knightPixmap.scaled(iconRect.size(),
                                                       Qt::KeepAspectRatio,
                                                       Qt::SmoothTransformation));
}

// 鼠标点击处理（优化坐标计算和用户体验）
void Chessboard::mousePressEvent(QMouseEvent *event)
{
    if (m_isRunning) {
        emit statusChanged(tr("遍历中，无法选择起点"));
        return;
    }

    // 计算棋盘实际位置和格子坐标
    const int cellSize = qMin(width(), height()) / BOARD_SIZE;
    const int offsetX = (width() - cellSize * BOARD_SIZE) / 2;
    const int offsetY = (height() - cellSize * BOARD_SIZE) / 2;

    const QPointF mousePos = event->position();
    const int x = (mousePos.x() - offsetX) / cellSize;
    const int y = (mousePos.y() - offsetY) / cellSize;

    if (isValidPos(QPoint(x, y))) {
        reset(); // 重置之前的选择
        setStartPosition(QPoint(x, y));
    } else {
        emit statusChanged(tr("点击位置无效，请点击棋盘内格子"));
    }
}

// 窗口大小变化时自适应（优化响应式布局）
void Chessboard::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    update(); // 窗口大小变化时刷新棋盘
}
