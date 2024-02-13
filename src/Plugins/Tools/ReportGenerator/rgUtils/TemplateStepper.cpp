#include "TemplateStepper.h"

QString TemplateStepper::replaceCommands(const QString &text,
                                         const QString &pattern,
                                         std::function<QString(QString)> resolver)
{
    QString output = text;
    QRegExp regex(pattern);
    int pos = 0;
    while ((pos = regex.indexIn(output, pos)) != -1) {
        QString matchedText = regex.cap(0);
        QString command = regex.cap(1);
        QString resolvedValue = resolver(command);
        output.replace(pos, matchedText.length(), resolvedValue);
        pos += resolvedValue.length();
    }
    return output;
}
