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
    void setText(QPixmap, int, int);
    void setText(QString text);
    void updateProgress(int);

protected:
    void closeEvent(QCloseEvent *) override;

private:
    static MainWindow * pMainWindow;
    void on_btnLoad_clicked();

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
