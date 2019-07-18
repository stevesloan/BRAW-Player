#include <QString>
#include <QPixmap>
#include <QApplication>
#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "c.h"

MainWindow * MainWindow::pMainWindow = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}


MainWindow::~MainWindow()
{
    delete ui;
}

// kind of singleton reference.
MainWindow *MainWindow::getMainWinPtr()
{
    return pMainWindow;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    exit(0);
}

void MainWindow::setText(QPixmap image, int width, int height)
{
    ui->image_label->setPixmap(image);
    ui->image_label->setGeometry(0, 0, width, height);
}

void MainWindow::setText(QString text)
{
    ui->image_label->setText(text);
}
