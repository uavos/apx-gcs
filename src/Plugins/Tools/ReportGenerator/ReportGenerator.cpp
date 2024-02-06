#include "ReportGenerator.h"
#include "TelemetryExtractor.h"
#include "TelemetryResolver.h"
#include <QDebug>
#include <QtCore>

ReportGenerator::ReportGenerator(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Report generator"),
           tr("Report creation using templates"),
           Group,
           "video")
{
    f_launch = new Fact(this, "launch", tr("Start"), tr("Start simulation"), Action | Apply, "play");
    connect(f_launch, &Fact::triggered, this, &ReportGenerator::test);
}

void ReportGenerator::test()
{
    TelemetryExtractor extractor;
    extractor.sync();
    TelemetryResolver tss{extractor};
    qDebug() << tss.get_value("MAX_ALTITUDE").value_or("LOL");
    qDebug() << "LOL";
}
