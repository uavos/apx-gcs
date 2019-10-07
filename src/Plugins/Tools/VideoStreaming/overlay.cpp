#include "overlay.h"

#include "App/AppSettings.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicles.h"
#include <QPainter>

void AbstractOverlayItem::paint(QPainter *painter)
{
    QRectF r(x(), y(), width(), height());
    render(r, painter);
}

void Aim::registerQmlType()
{
    qmlRegisterType<Aim>("Aim", 1, 0, "Aim");
}

void Aim::render(const QRectF &box, QPainter *painter)
{
    painter->save();
    if(m_type == Crosshair)
    {
        qreal lineSize = std::min(box.width(), box.height());
        qreal lineWidth = 2;
        QRectF r1((box.width() - lineSize) / 2, (box.height() - lineWidth) / 2, lineSize, lineWidth);
        QRectF r2((box.width() - lineWidth) / 2, (box.height() - lineSize) / 2, lineWidth, lineSize);

        painter->setPen(Qt::black);
        painter->setBrush(QBrush(Qt::white, Qt::SolidPattern));

        painter->drawRect(r1);
        painter->drawRect(r2);
    }
    else if(m_type == Rectangle)
    {
        qreal lineSize = std::min(box.width(), box.height()) / 3;
        qreal lineWidth = 2;

        QVector<QRectF> rects = {
            //tl
            QRectF(0, 0, lineSize, lineWidth),
            QRectF(0, 0, lineWidth, lineSize),
            //tr
            QRectF(box.width() - lineSize, 0, lineSize, lineWidth),
            QRectF(box.width() - lineWidth, 0, lineWidth, lineSize),
            //bl
            QRectF(0, box.height() - lineWidth, lineSize, lineWidth),
            QRectF(0, box.height() - lineSize, lineWidth, lineSize),
            //br
            QRectF(box.width() - lineSize, box.height() - lineWidth, lineSize, lineWidth),
            QRectF(box.width() - lineWidth, box.height() - lineSize, lineWidth, lineSize),
        };

        painter->setPen(Qt::black);
        painter->setBrush(QBrush(Qt::white, Qt::SolidPattern));

        for(const auto &r: rects)
            painter->drawRect(r);
    }
    painter->restore();
}

int Aim::getType() const
{
    return m_type;
}

void Aim::setType(int type)
{
    if(m_type != type)
    {
        m_type = type;
        emit typeChanged();
        update();
    }
}

Overlay::Overlay(Fact *parent)
    : Fact(parent, "overlay", tr("Overlay"), tr("Show additional info on video"), Group)
{
    Aim::registerQmlType();

    QSettings *settings = AppSettings::settings();

    f_aim = new AppSettingFact(settings, this, "aim", tr("Aim"), "", Enum, false);
    f_aim->setIcon("crosshairs");
    f_aim->setEnumStrings({"None", "Crosshair", "Rectangle"});


    f_variables = new AppSettingFact(settings,
                                     this,
                                     "vars",
                                     tr("Variables"),
                                     tr("Comma-separated f.ex. (yaw,pitch,etc...)"),
                                     Text,
                                     "yaw,pitch,roll");
    f_variables->setIcon("format-list-bulleted");

    f_scale = new AppSettingFact(settings, this, "scale", tr("Scale"), "", Float, 0.1);
    f_scale->setIcon("format-size");

    AppSettingFact::loadSettings(this);

    f_scale->setMin(0.01);
    f_scale->setMax(1.0);

    connect(f_aim, &Fact::valueChanged, this, &Overlay::onAimChanged);
    connect(f_variables, &Fact::valueChanged, this, &Overlay::onVariablesValueChanged);

    onVariablesValueChanged();
}

void Overlay::drawOverlay(QImage &image)
{
    int y = 0;
    int x = 0;
    int fontPixelSize = std::max(image.height() / 40, 12);

    QPainter painter(&image);
    QFont font = painter.font();
    font.setPixelSize(fontPixelSize);

    QBrush brush = painter.brush();
    brush.setColor(Qt::white);
    brush.setStyle(Qt::SolidPattern);

    painter.setPen(Qt::white);
    painter.setBrush(brush);
    painter.setFont(font);

    QString pattern = "%1: %2";
    for (auto &varname : m_varnames) {
        y += fontPixelSize;
        float value = Vehicles::instance()->current()->f_mandala->valueByName(varname).toFloat();
        painter.drawText(x, y, pattern.arg(varname, QString::number(value, 'f', 5)));
    }

    painter.setPen(Qt::black);

    QRectF aimBB;
    int side = image.rect().height() * f_scale->value().toDouble();
    aimBB.setTopLeft(QPointF(image.rect().center().x() - side / 2,
                             image.rect().center().y() - side / 2));
    aimBB.setSize(QSize(side, side));

    painter.save();
    painter.translate(aimBB.x(), aimBB.y());
    m_aim.render(aimBB, &painter);
    painter.restore();
}

QStringList Overlay::getVarNames() const
{
    return m_varnames;
}

void Overlay::onVariablesValueChanged()
{
    QString value = f_variables->value().toString();
    m_varnames = value.split(',');
    emit varnamesChanged();
}

void Overlay::onAimChanged()
{
    m_aim.setType(f_aim->value().toInt());
}
