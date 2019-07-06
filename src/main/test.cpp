#include <QApplication>
#include <QtCore>
#include <QtWidgets>
#include <Mandala.h>
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
