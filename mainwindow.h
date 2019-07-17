#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static MainWindow * getMainWinPtr();
    void setText(QPixmap);

private:
    static MainWindow * pMainWindow;

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
