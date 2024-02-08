#include "TemplateParser.h"

QVector<QPoint> TemplateParser::getCommandPositions(QString template_raw)
{
    QVector<QPoint> positions;
    int n = template_raw.length();
    int depth = 0;
    int startPos = -1;

    for (int i = 0; i < n; ++i) {
        if (template_raw[i] == '{') {
            if (depth == 0) {
                startPos = i;
            }
            depth++;
        } else if (template_raw[i] == '}') {
            depth--;
            if (depth == 0 && startPos != -1) {
                positions.push_back({startPos, i});
                startPos = -1;
            }
        }
    }

    return positions;
}
