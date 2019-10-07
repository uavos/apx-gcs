#ifndef OVERLAY_H
#define OVERLAY_H

#include <QImage>
#include <QQuickPaintedItem>
#include "Fact/Fact.h"

class AbstractOverlayItem: public QQuickPaintedItem
{
public:
    AbstractOverlayItem() = default;
    virtual void render(const QRectF &window, QPainter *painter) = 0;

private:
    void paint(QPainter *painter) override;
};

class Aim: public AbstractOverlayItem
{
    Q_OBJECT
    Q_PROPERTY(int type READ getType WRITE setType NOTIFY typeChanged)
public:
    enum AimType {
        None,
        Crosshair,
        Rectangle
    };
    Q_ENUM(AimType)
    Aim() = default;
    static void registerQmlType();
    void render(const QRectF &box, QPainter *painter) override;

    int getType() const;
    void setType(int type);

private:
    int m_type = Crosshair;

signals:
    void typeChanged();
};

class Overlay : public Fact
{
    Q_OBJECT
    Q_PROPERTY(QStringList varnames READ getVarNames NOTIFY varnamesChanged)
public:
    Overlay() = default;
    explicit Overlay(Fact *parent);

    Fact *f_aim;
    Fact *f_variables;
    Fact *f_scale;

    QStringList getVarNames() const;

    void drawOverlay(QImage &image);

private:
    Aim m_aim;
    QStringList m_varnames;

private slots:
    void onVariablesValueChanged();
    void onAimChanged();

signals:
    void varnamesChanged();
};

#endif // OVERLAY_H
