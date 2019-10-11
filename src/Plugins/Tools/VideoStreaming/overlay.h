#ifndef OVERLAY_H
#define OVERLAY_H

#include "Fact/Fact.h"
#include <QImage>
#include <QQuickPaintedItem>
#include <QTimer>

class AbstractOverlayItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(qreal scale READ getScale WRITE setScale NOTIFY scaleChanged)
public:
    AbstractOverlayItem();
    virtual void render(const QRectF &box, QPainter *painter) = 0;
    qreal getScale() const;
    void setScale(qreal scale);
    void setAutoRepaint(bool b);

protected:
    qreal m_scale = 1.0;

private:
    QTimer m_timer;
    void paint(QPainter *painter) override;

signals:
    void scaleChanged();
};

class OverlayAim : public AbstractOverlayItem
{
    Q_OBJECT
    Q_PROPERTY(int type READ getType WRITE setType NOTIFY typeChanged)
public:
    enum AimType { None, Crosshair, Rectangle };
    Q_ENUM(AimType)
    OverlayAim() = default;
    static void registerQmlType();
    void render(const QRectF &box, QPainter *painter) override;

    int getType() const;
    void setType(int type);

private:
    int m_type = Crosshair;

signals:
    void typeChanged();
};

class OverlayVars : public AbstractOverlayItem
{
    Q_OBJECT
    Q_PROPERTY(QString topLeftVars READ getTopLeftVars WRITE setTopLeftVars NOTIFY topLeftVarsChanged)
    Q_PROPERTY(QString topCenterVars READ getTopCenterVars WRITE setTopCenterVars NOTIFY
                   topCenterVarsChanged)
    Q_PROPERTY(
        QString topRightVars READ getTopRightVars WRITE setTopRightVars NOTIFY topRightVarsChanged)
public:
    OverlayVars() = default;
    static void registerQmlType();
    void render(const QRectF &box, QPainter *painter) override;

    QString getTopLeftVars() const;
    void setTopLeftVars(const QString &topLeftVars);

    QString getTopCenterVars() const;
    void setTopCenterVars(const QString &topCenterVars);

    QString getTopRightVars() const;
    void setTopRightVars(const QString &topRightVars);

private:
    QStringList m_topLeftVars;
    QStringList m_topCenterVars;
    QStringList m_topRightVars;
    QString var2text(Fact *var);

signals:
    void topLeftVarsChanged();
    void topCenterVarsChanged();
    void topRightVarsChanged();
};

class OverlayGimbal : public AbstractOverlayItem
{
    Q_OBJECT
    Q_PROPERTY(QString yawVar READ getYawVar WRITE setYawVar NOTIFY yawVarChanged)
    Q_PROPERTY(QString pitchVar READ getPitchVar WRITE setPitchVar NOTIFY pitchVarChanged)
public:
    OverlayGimbal();
    static void registerQmlType();
    void render(const QRectF &box, QPainter *painter) override;

    QString getYawVar() const;
    void setYawVar(const QString &yawVar);

    QString getPitchVar() const;
    void setPitchVar(const QString &pitchVar);

signals:
    void yawVarChanged();

    void pitchVarChanged();

private:
    QImage m_gimbalTop;
    QImage m_gimbalTopArrow;
    QImage m_gimbalProfile;
    QImage m_gimbalProfileArrow;
    QString m_yawVar;
    QString m_pitchVar;
};

class Overlay : public Fact
{
    Q_OBJECT
public:
    Overlay() = default;
    explicit Overlay(Fact *parent);

    Fact *f_aim;
    Fact *f_topLeftVars;
    Fact *f_topCenterVars;
    Fact *f_topRightVars;
    Fact *f_scale;
    Fact *f_gimbalYawVar;
    Fact *f_gimbalPitchVar;

    QStringList getVarNames() const;

    void drawOverlay(QImage &image);

private:
    OverlayAim m_aim;
    OverlayVars m_vars;
    OverlayGimbal m_gimbal;

private slots:
    void onVariablesValueChanged();
    void onAimChanged();
    void onScaleChanged();
    void onGimbalVarsChanged();
};

#endif // OVERLAY_H
