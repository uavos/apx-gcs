#ifndef SerialFORM_H
#define SerialFORM_H

#include <QWidget>
#include <QtCore>
//#include "QMandala.h"
#include "comm.h"

class QMandalaItem;
class QMandala;

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
    QMandala *mandala;
    QList<QMetaObject::Connection>mcon;
    Comm uart;
    QSocketNotifier *socketNotifier;

private slots:
    void mandalaCurrentChanged(QMandalaItem *m);
    void btnReset();
    void btnSend();
    void btnForward();
    void serialData(uint portNo,const QByteArray &ba);

    void uartRead();

signals:
    void finished();
};

#endif // SerialFORM_H
