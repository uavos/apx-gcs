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
#include "SourceEdit.h"
#include <QTextBlock>
#include <QTextDocumentFragment>
//=============================================================================
SourceEdit::SourceEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    QFont f(font());
#ifdef Q_OS_MAC
    f.setFamily("Andale Mono");
#else
    f.setFamily("Monospace");
#endif
    f.setPointSize(f.pointSize() * 1.2);
    f.setFixedPitch(true);
    f.setKerning(false);
    setFont(f);
    document()->setDefaultFont(f);
    setWordWrapMode(QTextOption::NoWrap);
    //setAcceptRichText(false);
    highlighter = new Highlighter(document());
    document()->setIndentWidth(2);
}
//=============================================================================
void SourceEdit::addKeywords(const QStringList &words)
{
    foreach (QString w, words)
        highlighter->addRule(QString("\\b%1\\b").arg(w), "orange", "bold");
}
//=============================================================================
void SourceEdit::keyPressEvent(QKeyEvent *event)
{
    Qt::KeyboardModifiers m = event->modifiers();
    QTextCursor cur = textCursor();
    int key = event->key();
    int pos = cur.position();
    // Auto-indentation
    if (key == Qt::Key_Return
        && (m == Qt::NoModifier || m == Qt::ControlModifier || m == Qt::ShiftModifier)) {
        QString data = cur.block().text(); //toPlainText();
        pos -= cur.block().position();
        QPlainTextEdit::keyPressEvent(event);
        int i;
        for (i = pos - 2; i >= 0; i--) {
            if (data.mid(i, 1) == "\n")
                break;
        }
        while (data.mid(i + 1, 1) == " ") {
            cur.insertText(" ");
            i++;
        }
        return;
    }
    // TAB-indentation
    if ((key == Qt::Key_Tab
         || key == Qt::Key_Backtab)) { // && (event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::ShiftModifier)) {
        int a = cur.anchor();
        if ((a == pos || m == Qt::ControlModifier) && key != Qt::Key_Backtab) { //no selection
            int cnt = 0;
            if (m != Qt::ControlModifier
                && cur.block().text().left(cur.positionInBlock()).trimmed().isEmpty()) {
                cnt = document()->indentWidth();
            } else {
                cnt = 8 - cur.positionInBlock() % 8;
                //QPlainTextEdit::keyPressEvent(event);
            }
            for (int c = 0; c < cnt; c++)
                cur.insertText(" ");
        } else { //selection
            // save a new anchor at the beginning of the line of the selected text
            cur.setPosition(a);
            cur.movePosition(QTextCursor::StartOfBlock);
            a = cur.position();
            cur.setPosition(pos, QTextCursor::KeepAnchor);
            cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            pos = cur.position();
            QStringList st = cur.selection().toPlainText().split('\n');
            for (int i = 0; i < st.size(); i++) {
                if (st.at(i).trimmed().isEmpty())
                    continue;
                for (int c = 0; c < document()->indentWidth(); c++) {
                    if (key == Qt::Key_Backtab) { //event->modifiers()==Qt::ShiftModifier){
                        if (st.at(i).startsWith(' ')) {
                            st[i].remove(0, 1);
                            pos--;
                        }
                    } else {
                        st[i].prepend(' ');
                        pos++;
                    }
                }
            }
            cur.removeSelectedText();
            cur.insertText(st.join('\n'));
            // reselect the text for more indents
            cur.setPosition(a);
            cur.setPosition(pos, QTextCursor::KeepAnchor);
            setTextCursor(cur);
        }
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}
//=============================================================================
void SourceEdit::cleanText()
{
    for (QTextBlock block = document()->begin(); block != document()->end(); block = block.next()) {
        QString s = block.text();
        if (!(s.endsWith(' ') || s.endsWith('\t')))
            continue;
        QTextCursor cur(block);
        cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cur.removeSelectedText();
        cur.insertText(s.remove(QRegExp("\\s+$")));
    }
}
//=============================================================================
//=============================================================================
Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    defaultCharFormat.setFont(document()->defaultFont());
    commentStartExpression = QRegExp("/\\*");
    commentEndExpression = QRegExp("\\*/");
    QStringList keywords = QStringList() << "new"
                                         << "const"
                                         << "printf"
                                         << "continue"
                                         << "if"
                                         << "else"
                                         << "for"
                                         << "from"
                                         << "global"
                                         << "if"
                                         << "import"
                                         << "return"
                                         << "while"
                                         << "yield"
                                         << "true"
                                         << "false";
    QStringList operators = QStringList() << "=" <<
                            // Comparison
                            "=="
                                          << "!="
                                          << "<"
                                          << "<="
                                          << ">"
                                          << ">=" <<
                            // Arithmetic
                            "\\+"
                                          << "-"
                                          << "\\*"
                                          << "/"
                                          << "//"
                                          << "%"
                                          << "\\*\\*" <<
                            // In-place
                            "\\+="
                                          << "-="
                                          << "\\*="
                                          << "/="
                                          << "%=" <<
                            // Bitwise
                            "\\^"
                                          << "\\|"
                                          << "&"
                                          << "~"
                                          << ">>"
                                          << "<<";
    QStringList braces = QStringList() << "\\{"
                                       << "\\}"
                                       << "\\("
                                       << "\\)"
                                       << "\\["
                                       << "\\]";
    foreach (QString currKeyword, keywords)
        addRule(QString("\\b%1\\b").arg(currKeyword), "lightBlue", "bold");
    foreach (QString currOperator, operators)
        addRule(QString("%1").arg(currOperator), "orange");
    foreach (QString currBrace, braces)
        addRule(QString("%1").arg(currBrace), "yellow");

    //function
    addRule("\\b[A-Za-z0-9_\\ ]+(?=\\()", Qt::darkCyan);

    //function
    addRule("\\bevt_[A-Za-z0-9_]+(?=\\()", Qt::blue, "italic");

    // 'self'
    //addRule("\\bself\\b","white","italic");

    // Numeric literals
    QColor c = QColor(100, 255, 100);
    addRule("\\b[+-]?[0-9]\\b", c);                  // r'\b[+-]?[0-9]+[lL]?\b'
    addRule("\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b", c); // r'\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\b'
    addRule("\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b",
            c,
            "italic"); // r'\b[+-]?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\b'

    //fill mandala constants
    /*QMandalaItem *var=QMandala::instance()->current;
  foreach(QMandalaField *f,var->fields)
    addRule(QString("\\b%1\\b").arg("f_"+f->name()),"orange","bold");
  foreach(QString name,var->constants.keys())
    addRule(QString("\\b%1\\b").arg(name),"orange","bold");*/

    // Double-quoted string, possibly containing escape sequences
    addRule("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", Qt::magenta);
    // Single-quoted string, possibly containing escape sequences
    addRule("'[^'\\\\]*(\\\\.[^'\\\\]*)*'", Qt::magenta);
    // 'def' followed by an identifier
    addRule("\\b@\\b\\s*(\\w+)", Qt::white, "bold");
    //  'class' followed by an identifier
    addRule("\\bclass\\b\\s*(\\w+)", Qt::gray, "bold");
    // From '#' until a newline
    addRule("#[^\\n]*", Qt::darkGreen, "italic");

    //comment
    multiLineCommentFormat = addRule("//[^\n]*", QColor(80, 80, 80)).format;
}
Highlighter::HighlightingRule Highlighter::addRule(const QString &pattern,
                                                   const QColor &color,
                                                   const QString &style)
{
    QTextCharFormat charFormat;
    //QColor color(colorName);
    //QFont f(document()->defaultFont());
    charFormat.setFont(document()->defaultFont());
    charFormat.setFontFixedPitch(true);
    charFormat.setForeground(color.lighter(150));
    if (style.contains("bold", Qt::CaseInsensitive)) //f.setBold(true);
        charFormat.setFontWeight(QFont::Bold);
    if (style.contains("italic", Qt::CaseInsensitive)) //f.setItalic(true);
        charFormat.setFontItalic(true);
    //charFormat.setFont(f);
    HighlightingRule rule;
    rule.format = charFormat;
    rule.pattern = QRegExp(pattern);
    highlightingRules.append(rule);
    return rule;
}
void Highlighter::highlightBlock(const QString &text)
{
    setFormat(0, text.length(), defaultCharFormat);
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = commentStartExpression.indexIn(text);

    while (startIndex >= 0) {
        int endIndex = commentEndExpression.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
    }
}
//=============================================================================
