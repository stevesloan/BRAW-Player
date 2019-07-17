#include <QString>
#include <QPixmap>
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

void MainWindow::setText(QPixmap image)
{
//    ui->image_label->setText(QString{"test"});
//    ui->image_label->setMargin(100);
//    ui->image_label->imag
      ui->image_label->setPixmap(image);
//    ui->image_label->setPixmap(QPixmap::fromImage(QImage(image, 100, 100, 10, QImage::Format_RGBA)));
}
