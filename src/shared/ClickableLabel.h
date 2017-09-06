#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H
#include <QtCore>
#include <QLabel>
//=============================================================================
class ClickableLabel : public QLabel
{
  Q_OBJECT
public:
    explicit ClickableLabel(QWidget * parent = 0 ):QLabel(parent){setCursor(Qt::PointingHandCursor);}
    ~ClickableLabel(){}
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent *){emit clicked();}
};
//=============================================================================
#endif // CLICKABLELABEL_H
