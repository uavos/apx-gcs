#include <QApplication>
#include <Mandala.h>
#include <QtCore>
#include <QtWidgets>
//============================================================================
int main(int argc, char *argv[])
{
    qDebug() << "HELLO";

    Mandala m;
    QApplication app(argc, argv);

    QPushButton hello(QPushButton::tr("Hello world!"));
    hello.resize(400, 200);
    hello.show();
    return app.exec();
}
//============================================================================
