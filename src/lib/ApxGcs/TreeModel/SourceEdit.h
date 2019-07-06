/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
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

private:
    Highlighter *highlighter;

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
