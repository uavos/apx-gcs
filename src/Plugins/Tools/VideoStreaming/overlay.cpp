#include "overlay.h"

#include "App/AppSettings.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicles.h"
#include <QPainter>

AbstractOverlayItem::AbstractOverlayItem()
{
    m_timer.setSingleShot(false);
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, [this](){ update(); });
    setAutoRepaint(true);
}

qreal AbstractOverlayItem::getScale() const
{
    return m_scale;
}

void AbstractOverlayItem::setScale(qreal scale)
{
    if (m_scale != scale) {
        m_scale = scale;
        emit scaleChanged();
        update();
    }
}

void AbstractOverlayItem::setAutoRepaint(bool b)
{
    if(b)
        m_timer.start();
    else
        m_timer.stop();
}

void AbstractOverlayItem::paint(QPainter *painter)
{
    QRectF r(x(), y(), width(), height());
    render(r, painter);
}

void OverlayAim::registerQmlType()
{
    qmlRegisterType<OverlayAim>("OverlayAim", 1, 0, "OverlayAim");
}

void OverlayAim::render(const QRectF &box, QPainter *painter)
{
    painter->save();
    const QPointF center = box.center();
    const qreal sideSize = std::round(std::min(box.width(), box.height()) / 7.0 * m_scale);
    const qreal lineWidth = 2;

    if (m_type == Crosshair) {
        QRectF r1;
        r1.setSize(QSizeF(sideSize, lineWidth));
        r1.moveCenter(center);
        QRectF r2;
        r2.setSize(QSizeF(lineWidth, sideSize));
        r2.moveCenter(center);

        painter->setPen(Qt::black);
        painter->setBrush(QBrush(Qt::white, Qt::SolidPattern));

        painter->drawRect(r1);
        painter->drawRect(r2);
    } else if (m_type == Rectangle) {
        QRectF box;
        box.setSize(QSizeF(sideSize, sideSize));
        box.moveCenter(center);

        const qreal lineSize = sideSize / 3.0;

        QVector<QRectF> rects = {
            //tl
            QRectF(box.x(), box.y(), lineSize, lineWidth),
            QRectF(box.x(), box.y(), lineWidth, lineSize),
            //tr
            QRectF(box.topRight().x() - lineSize, box.topRight().y(), lineSize, lineWidth),
            QRectF(box.topRight().x() - lineWidth, box.topRight().y(), lineWidth, lineSize),
            //bl
            QRectF(box.bottomLeft().x(), box.bottomLeft().y() - lineWidth, lineSize, lineWidth),
            QRectF(box.bottomLeft().x(), box.bottomLeft().y() - lineSize, lineWidth, lineSize),
            //br
            QRectF(box.bottomRight().x() - lineSize,
                   box.bottomRight().y() - lineWidth,
                   lineSize,
                   lineWidth),
            QRectF(box.bottomRight().x() - lineWidth,
                   box.bottomRight().y() - lineSize,
                   lineWidth,
                   lineSize),
        };

        painter->setPen(Qt::black);
        painter->setBrush(QBrush(Qt::white, Qt::SolidPattern));

        for (const auto &r : rects)
            painter->drawRect(r);
    }
    painter->restore();
}

int OverlayAim::getType() const
{
    return m_type;
}

void OverlayAim::setType(int type)
{
    if (m_type != type) {
        m_type = type;
        emit typeChanged();
        update();
    }
}

void OverlayVars::registerQmlType()
{
    qmlRegisterType<OverlayVars>("OverlayVars", 1, 0, "OverlayVars");
}

void OverlayVars::render(const QRectF &box, QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::white);
    const QString pattern = "%1: %2";
    const int fontSize = int(round(box.height() / 30.0 * m_scale));
    QFont font(painter->font());
    font.setPixelSize(fontSize);
    painter->setFont(font);
    QFontMetricsF metrics(font);

    int y = 0;
    for (const auto &varname : m_topLeftVars) {
        y += fontSize;
        QString value = var2text(Vehicles::instance()->current()->f_mandala->factByName(varname));
        painter->drawText(0, y, pattern.arg(varname, value));
    }

    y = 0;
    for (const auto &varname : m_topCenterVars) {
        y += fontSize;
        QString value = var2text(Vehicles::instance()->current()->f_mandala->factByName(varname));
        QString text = pattern.arg(varname, value);
        qreal stringWidth = metrics.width(text);
        painter->drawText(int(round(box.center().x() - stringWidth / 2.0)), y, text);
    }

    y = 0;
    for (const auto &varname : m_topRightVars) {
        y += fontSize;
        QString value = var2text(Vehicles::instance()->current()->f_mandala->factByName(varname));
        QString text = pattern.arg(varname, value);
        qreal stringWidth = metrics.width(text);
        painter->drawText(int(round(box.width() - stringWidth)), y, text);
    }
    painter->restore();
}

QString OverlayVars::getTopLeftVars() const
{
    return m_topLeftVars.join(',');
}

void OverlayVars::setTopLeftVars(const QString &topLeftVars)
{
    m_topLeftVars = topLeftVars.split(',');
    emit topLeftVarsChanged();
    update();
}

QString OverlayVars::getTopCenterVars() const
{
    return m_topCenterVars.join(',');
}

void OverlayVars::setTopCenterVars(const QString &topCenterVars)
{
    m_topCenterVars = topCenterVars.split(',');
    emit topCenterVarsChanged();
    update();
}

QString OverlayVars::getTopRightVars() const
{
    return m_topRightVars.join(',');
}

void OverlayVars::setTopRightVars(const QString &topRightVars)
{
    m_topRightVars = topRightVars.split(',');
    emit topRightVarsChanged();
    update();
}

QString OverlayVars::var2text(Fact *var)
{
    if(var->name() == "gps_time")
        return QDateTime::fromSecsSinceEpoch(var->value().toLongLong()).time().toString("HH-mm-ss");
    else
        return var->text();
}

OverlayGimbal::OverlayGimbal()
    : m_gimbalTop(QString(":/%1/sprites/gimbal_top.svg").arg(PLUGIN_NAME))
    , m_gimbalTopArrow(QString(":/%1/sprites/gimbal_top_a.svg").arg(PLUGIN_NAME))
    , m_gimbalProfile(QString(":/%1/sprites/gimbal_profile.svg").arg(PLUGIN_NAME))
    , m_gimbalProfileArrow(QString(":/%1/sprites/gimbal_profile_a.svg").arg(PLUGIN_NAME))
    , m_yawVar("cam_yaw")
    , m_pitchVar("cam_pitch")
{}

void OverlayGimbal::registerQmlType()
{
    qmlRegisterType<OverlayGimbal>("OverlayGimbal", 1, 0, "OverlayGimbal");
}

void OverlayGimbal::render(const QRectF &box, QPainter *painter)
{
    painter->save();

    const int padding = int(std::round(box.height() / 100.0));
    const int gimbalSpriteSize = int(std::round(box.height() / 10.0 * m_scale));
    qreal yaw = Vehicles::instance()->current()->f_mandala->factByName(m_yawVar)->value().toReal();
    qreal pitch
        = Vehicles::instance()->current()->f_mandala->factByName(m_pitchVar)->value().toReal();

    QMatrix m1;
    m1.rotate(yaw);
    QMatrix m2;
    m2.rotate(pitch);

    QImage gimbalTop = m_gimbalTop.scaled(gimbalSpriteSize, gimbalSpriteSize);
    QImage gimbalTopArrow = m_gimbalTopArrow.scaled(gimbalSpriteSize, gimbalSpriteSize)
                                .transformed(m1);
    QImage gimbalProfile = m_gimbalProfile.scaledToWidth(gimbalSpriteSize);
    QImage gimbalProfileArrow = m_gimbalProfileArrow.scaledToWidth(gimbalSpriteSize).transformed(m2);
    gimbalTopArrow = gimbalTopArrow.copy((gimbalTopArrow.width() - gimbalSpriteSize) / 2,
                                         (gimbalTopArrow.height() - gimbalSpriteSize) / 2,
                                         gimbalSpriteSize,
                                         gimbalSpriteSize);
    gimbalProfileArrow = gimbalProfileArrow.copy((gimbalProfileArrow.width() - gimbalSpriteSize) / 2,
                                                 (gimbalProfileArrow.height() - gimbalSpriteSize)
                                                     / 2,
                                                 gimbalSpriteSize,
                                                 gimbalSpriteSize);

    QPoint center = box.center().toPoint();
    painter->drawImage(0, center.y() - gimbalTop.height() - padding, gimbalTop);
    painter->drawImage(0, center.y() - gimbalTopArrow.height() - padding, gimbalTopArrow);
    painter->drawImage(0, center.y() + padding, gimbalProfile);
    painter->drawImage(0,
                       center.y() + padding + gimbalProfile.height() - gimbalTopArrow.height(),
                       gimbalProfileArrow);
    painter->restore();
}

QString OverlayGimbal::getYawVar() const
{
    return m_yawVar;
}

void OverlayGimbal::setYawVar(const QString &yawVar)
{
    if (m_yawVar != yawVar) {
        m_yawVar = yawVar;
        emit yawVarChanged();
    }
}

QString OverlayGimbal::getPitchVar() const
{
    return m_pitchVar;
}

void OverlayGimbal::setPitchVar(const QString &pitchVar)
{
    if (m_pitchVar != pitchVar) {
        m_pitchVar = pitchVar;
        emit pitchVarChanged();
    }
}

Overlay::Overlay(Fact *parent)
    : Fact(parent, "overlay", tr("Overlay"), tr("Show additional info on video"), Group)
{
    OverlayAim::registerQmlType();
    OverlayVars::registerQmlType();
    OverlayGimbal::registerQmlType();

    m_aim.setAutoRepaint(false);
    m_gimbal.setAutoRepaint(false);
    m_vars.setAutoRepaint(false);

    QSettings *settings = AppSettings::settings();

    f_aim = new AppSettingFact(settings, this, "aim", tr("Aim"), "", Enum, 0);
    f_aim->setIcon("crosshairs");
    f_aim->setEnumStrings({"none", "crosshair", "rectangle"});

    f_topLeftVars = new AppSettingFact(settings,
                                       this,
                                       "top_left_vars",
                                       tr("Top-Left variables"),
                                       tr("Comma-separated f.ex. (yaw,pitch,etc...)"),
                                       Text,
                                       "yaw,pitch,roll");
    f_topLeftVars->setIcon("format-list-bulleted");
    f_topCenterVars = new AppSettingFact(settings,
                                         this,
                                         "top_center_vars",
                                         tr("Top-Center variables"),
                                         tr("Comma-separated f.ex. (yaw,pitch,etc...)"),
                                         Text,
                                         "gps_time");
    f_topCenterVars->setIcon("format-list-bulleted");
    f_topRightVars = new AppSettingFact(settings,
                                        this,
                                        "top_right_vars",
                                        tr("Top-Right variables"),
                                        tr("Comma-separated f.ex. (yaw,pitch,etc...)"),
                                        Text,
                                        "cam_zoom");
    f_topRightVars->setIcon("format-list-bulleted");

    f_gimbalShow = new AppSettingFact(settings,
                                      this,
                                      "show_gimbal",
                                      tr("Show gimbal position"),
                                      "",
                                      Bool,
                                      false);
    f_gimbalYawVar = new AppSettingFact(settings,
                                        this,
                                        "gimbal_yaw_var",
                                        tr("Gimbal yaw"),
                                        tr("Gimbal yaw position variable"),
                                        Text,
                                        "cam_yaw");
    f_gimbalPitchVar = new AppSettingFact(settings,
                                          this,
                                          "gimbal_pitch_var",
                                          tr("Gimbal pitch"),
                                          tr("Gimbal pitch position variable"),
                                          Text,
                                          "cam_pitch");

    f_scale = new AppSettingFact(settings, this, "scale", tr("Scale"), "", Float, 1);
    f_scale->setIcon("format-size");

    connect(f_aim, &Fact::valueChanged, this, &Overlay::onAimChanged);
    connect(f_topLeftVars, &Fact::valueChanged, this, &Overlay::onVariablesValueChanged);
    connect(f_topCenterVars, &Fact::valueChanged, this, &Overlay::onVariablesValueChanged);
    connect(f_topRightVars, &Fact::valueChanged, this, &Overlay::onVariablesValueChanged);
    connect(f_scale, &Fact::valueChanged, this, &Overlay::onScaleChanged);
    connect(f_gimbalYawVar, &Fact::valueChanged, this, &Overlay::onGimbalVarsChanged);
    connect(f_gimbalPitchVar, &Fact::valueChanged, this, &Overlay::onGimbalVarsChanged);

    AppSettingFact::loadSettings(this);

    f_scale->setMin(0.1);
    f_scale->setMax(5.0);

    onAimChanged();
    onScaleChanged();
    onVariablesValueChanged();
    onGimbalVarsChanged();
}

void Overlay::drawOverlay(QImage &image)
{
    QPainter painter(&image);

    m_aim.render(image.rect(), &painter);
    m_vars.render(image.rect(), &painter);
    if(f_gimbalShow->value().toBool())
        m_gimbal.render(image.rect(), &painter);
}

void Overlay::onVariablesValueChanged()
{
    m_vars.setTopLeftVars(f_topLeftVars->value().toString());
    m_vars.setTopCenterVars(f_topCenterVars->value().toString());
    m_vars.setTopRightVars(f_topRightVars->value().toString());
}

void Overlay::onAimChanged()
{
    m_aim.setType(f_aim->value().toInt());
}

void Overlay::onScaleChanged()
{
    qreal scale = f_scale->value().toReal();
    m_aim.setScale(scale);
    m_vars.setScale(scale);
    m_gimbal.setScale(scale);
}

void Overlay::onGimbalVarsChanged()
{
    m_gimbal.setYawVar(f_gimbalYawVar->value().toString());
    m_gimbal.setPitchVar(f_gimbalPitchVar->value().toString());
}
