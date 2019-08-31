#ifndef SerialFORM_H
#define SerialFORM_H

#include <QWidget>
#include <QtCore>
//#include "comm.h"

namespace Ui {
class SerialForm;
}

class SerialForm : public QWidget
{
    Q_OBJECT

public:
    explicit SerialForm(QWidget *parent = 0);
    ~SerialForm();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::SerialForm *ui;
    //Comm uart;
    QSocketNotifier *socketNotifier;

    QFile dumpFile;

private slots:
    void btnReset();
    void btnSend();
    void btnForward();
    void serialData(uint portNo, QByteArray ba);

    void uartRead();

signals:
    void finished();
};

#endif // SerialFORM_H
