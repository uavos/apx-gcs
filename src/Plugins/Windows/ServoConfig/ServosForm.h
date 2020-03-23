#ifndef SERVOSFORM_H
#define SERVOSFORM_H

#include <QWidget>
#include <QtCore>

namespace Ui {
class ServosForm;
}

class Vehicle;
class ServosForm : public QWidget
{
    Q_OBJECT

public:
    explicit ServosForm(QWidget *parent = 0);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::ServosForm *ui;

    QList<QMetaObject::Connection> clist;

    uint counter;
    void sendVolz(uint cmd, uint id, uint arg);
    void sendFutabaAddr(uint servoID, uint newAddr);

private slots:
    void vehicleSelected(Vehicle *vehicle);

    void btnFind();
    void btnMove();
    void btnSetAdr();

    void do_find(void);
    void serialData(quint16 portNo, QByteArray ba);
signals:
    void finished();
};

#endif
