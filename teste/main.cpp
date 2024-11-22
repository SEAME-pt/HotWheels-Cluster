#include "mainwindow.h"
#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.showFullScreen();

    // Create a QTimer to update the label every 1 second
    QTimer *timer = new QTimer(&w);  // Timer is parented to MainWindow

    int i = 1;  // Start from 1 KMH

    // Connect the QTimer timeout signal to a lambda function that updates the label
    QObject::connect(timer, &QTimer::timeout, [&]() {
        w.setLabelText(QString::number(i++));  // Update label with current value of i
        if (i > 10) i = 1;  // Reset counter after 10 KMH to 1 KMH
    });

    // Start the timer to trigger every 1000 milliseconds (1 second)
    timer->start(1000);  // Interval of 1 second

    return a.exec();
}
