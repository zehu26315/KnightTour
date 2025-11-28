#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "chessboard.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startBtn_clicked();
    void on_resetBtn_clicked();
    void on_speedBtn_clicked();
    void updateStatus(const QString& status);
    void onTourFinished(bool success);

private:
    Ui::MainWindow *ui;
    Chessboard *m_chessboard;
    int m_speedLevel; // 0=慢,1=中,2=快
};

#endif // MAINWINDOW_H
