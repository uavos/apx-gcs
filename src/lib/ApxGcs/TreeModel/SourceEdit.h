/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Ground Control.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SourceEdit_H
#define SourceEdit_H
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
class Highlighter;
//=============================================================================
class SourceEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit SourceEdit(QWidget *parent);
    void addKeywords(const QStringList &words);

    static QFont getFont();

private:
    Highlighter *highlighter;
    static QFont m_font;

protected:
    void keyPressEvent(QKeyEvent *event);
public slots:
    void cleanText();
};
//=============================================================================
class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    Highlighter(QTextDocument *parent = 0);

    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QTextCharFormat defaultCharFormat;

    HighlightingRule addRule(const QString &pattern,
                             const QColor &color,
                             const QString &style = QString());

protected:
    void highlightBlock(const QString &text) Q_DECL_OVERRIDE;

private:
    QVector<HighlightingRule> highlightingRules;

    QRegExp commentStartExpression;
    QRegExp commentEndExpression;
    enum { f_bold = 1, f_italic = 2 };

    QTextCharFormat multiLineCommentFormat;
};
//=============================================================================
#endif
