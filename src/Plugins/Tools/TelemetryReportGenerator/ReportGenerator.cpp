#include "ReportGenerator.h"
#include <QtCore>

ReportGenerator::ReportGenerator(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Telemetry report generator"),
           tr("Plugin for report creation using templates"),
           Group,
           "video")
{
    f_launch = new Fact(this, "launch", tr("Start"), tr("Start simulation"), Action | Apply, "play");
    connect(f_launch, &Fact::triggered, this, &ReportGenerator::test);
}

void ReportGenerator::test()
{
    qDebug() << "Button pressed";
}
