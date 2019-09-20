#include "overlay.h"

#include "App/AppSettings.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicles.h"
#include <QPainter>

Overlay::Overlay(Fact *parent)
    : Fact(parent, "overlay", tr("Overlay"), tr("Show additional info on video"), Group)
{
    QSettings *settings = AppSettings::settings();

    f_crosshair = new AppSettingFact(settings, this, "crosshair", tr("Crosshair"), "", Bool, false);
    f_crosshair->setIcon("crosshairs");

    f_variables = new AppSettingFact(settings,
                                     this,
                                     "vars",
                                     tr("Variables"),
                                     tr("Comma-separated f.ex. (yaw,pitch,etc...)"),
                                     Text,
                                     "yaw,pitch,roll");
    f_variables->setIcon("format-list-bulleted");

    connect(f_variables, &Fact::valueChanged, this, &Overlay::onVariablesValueChanged);

    AppSettingFact::loadSettings(this);

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

    //crosshair
    if (f_crosshair->value().toBool()) {
        int lineSize = std::min(image.width(), image.height()) / 10;
        QRect rect1((image.width() - lineSize) / 2, (image.height() - 4) / 2, lineSize, 4);
        QRect rect2((image.width() - 4) / 2, (image.height() - lineSize) / 2, 4, lineSize);

        painter.drawRect(rect1);
        painter.drawRect(rect2);
    }
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
