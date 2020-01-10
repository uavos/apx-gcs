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
#include "FactDelegate.h"
#include "FactDelegateArray.h"
#include "FactDelegateScript.h"
#include "FactTreeModel.h"
#include <QtWidgets>
//=============================================================================
FactDelegate::FactDelegate(QObject *parent)
    : QItemDelegate(parent)
{
    progressBar = new QProgressBar();
    progressBar->setObjectName("nodeProgressBar");
    progressBar->setMaximum(100);
}
FactDelegate::~FactDelegate()
{
    delete progressBar;
}
//=============================================================================
QWidget *FactDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    Q_UNUSED(option)
    Fact *f = index.data(Fact::ModelDataRole).value<Fact *>();
    if (!f)
        return QItemDelegate::createEditor(parent, option, index);
    QWidget *e = nullptr;
    QString su = f->units();
    if (f->enumStrings().size() > 1) {
        QComboBox *cb = new QComboBox(parent);
        cb->setFrame(false);
        cb->addItems(f->enumStrings());
        cb->view()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        //cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0));
        e = cb;
    } else {
        switch (f->dataType()) {
        default:
            break;
        case Fact::Bool: {
            QComboBox *cb = new QComboBox(parent);
            cb->setFrame(false);
            cb->addItems(QStringList() << QVariant(false).toString() << QVariant(true).toString());
            cb->view()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
            //cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0));
            e = cb;
        } break;
        case Fact::Int: {
            if (su == "time") {
                QTimeEdit *te = new QTimeEdit(parent);
                te->setDisplayFormat("HH:mm:ss");
                e = te;
                break;
            }
            QSpinBox *sb = new QSpinBox(parent);
            sb->setMinimum(std::numeric_limits<int>::min());
            sb->setMaximum(std::numeric_limits<int>::max());
            sb->setFrame(false);
            sb->setSingleStep(1);
            if (su == "m")
                sb->setSingleStep(10);
            e = sb;
        } break;
        case Fact::Float: {
            QLineEdit *le = new QLineEdit(parent);
            le->setFrame(false);
            e = le;
        } break;
        /*case Fact::ActionData:{
        QPushButton *btn=createButton(parent);
        connect(btn,&QPushButton::clicked,f,&Fact::trigger);
        return btn;
      }break;*/
        case Fact::NoFlags: {
            if (f->treeType() == Fact::Group && f->size() > 1
                && f->child(0)->treeType() == Fact::Group && f->status().startsWith('[')
                && f->status().endsWith(']')) {
                QPushButton *btn = createButton(parent);
                connect(btn, &QPushButton::clicked, this, [=]() {
                    new FactDelegateArray(f, parent->parentWidget());
                });
                return btn;
            }
        } break;
        case Fact::Mandala: {
            QComboBox *cb = new QComboBox(parent);
            cb->setFrame(false);
            cb->setEditable(true);
            cb->addItems(f->mandalaNames());
            cb->view()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
            e = cb;
        } break;
        case Fact::Script: {
            QPushButton *btn = createButton(parent);
            connect(btn, &QPushButton::clicked, this, [=]() {
                new FactDelegateScript(f, parent->parentWidget());
            });
            return btn;
        }
        }
    }
    if (!e)
        e = QItemDelegate::createEditor(parent, option, index);
    //e->setAutoFillBackground(true);
    /*static_cast<QFrame*>(e)->setFrameShape(QFrame::NoFrame);
  static_cast<QFrame*>(e)->setLineWidth(0);
  static_cast<QFrame*>(e)->setMidLineWidth(0);*/
    e->setContentsMargins(-1, -1, -1, -1);
    //QFont font(index.data(Qt::FontRole).value<QFont>());
    //font.setPointSize(font.pointSize()+2);
    //e->setFont(font);

    //number edits
    if (qobject_cast<QSpinBox *>(e)) {
        QSpinBox *sb = static_cast<QSpinBox *>(e);
        if (!f->min().isNull())
            sb->setMinimum(f->min().toInt());
        if (!f->max().isNull()) {
            long long m = f->max().toLongLong();
            int mi = static_cast<int>(m);
            if (static_cast<long long>(mi) == m)
                sb->setMaximum(mi);
        }
        if (!su.isEmpty()) {
            if (su == "hex") {
                sb->setDisplayIntegerBase(16);
            } else {
                sb->setSuffix(su.prepend(" "));
            }
        }
    } else if (qobject_cast<QDoubleSpinBox *>(e)) {
        QDoubleSpinBox *sb = static_cast<QDoubleSpinBox *>(e);
        if (!f->min().isNull())
            sb->setMinimum(f->min().toDouble());
        if (!f->max().isNull())
            sb->setMaximum(f->max().toDouble());
        if (!su.isEmpty()) {
            if (su != "lat" && su != "lon") {
                sb->setSuffix(su.prepend(" "));
            }
        }
    }

    return e;
}
QPushButton *FactDelegate::createButton(QWidget *parent) const
{
    QPushButton *btn = new QPushButton(parent);
    //btn->setFlat(true);
    QPalette newPalette = btn->palette();
    newPalette.setBrush(QPalette::Window, QBrush(QColor(255, 255, 255, 80)));
    btn->setPalette(newPalette);
    btn->setBackgroundRole(QPalette::Window);
    btn->setObjectName("treeViewButton");
    return btn;
}
void FactDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (qobject_cast<QPushButton *>(editor))
        return;
    if (qobject_cast<QTimeEdit *>(editor)) {
        QTimeEdit *te = static_cast<QTimeEdit *>(editor);
        QTime t = QTime(0, 0).addSecs(index.data(Qt::EditRole).toUInt());
        //qDebug()<<t<<index.data(Qt::EditRole).toUInt()<<index.data(Qt::EditRole);
        te->setTime(t);
        return;
    }
    QItemDelegate::setEditorData(editor, index);
}
void FactDelegate::setModelData(QWidget *editor,
                                QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    if (qobject_cast<QPushButton *>(editor))
        return;
    Fact *f = index.data(Fact::ModelDataRole).value<Fact *>();
    if (!f->dataType())
        return;
    if (qobject_cast<QTimeEdit *>(editor)) {
        QTimeEdit *te = static_cast<QTimeEdit *>(editor);
        model->setData(index, -te->time().secsTo(QTime(0, 0)), Qt::EditRole);
        return;
    }
    QItemDelegate::setModelData(editor, model, index);
}
//=============================================================================
void FactDelegate::paint(QPainter *painter,
                         const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    if (index.column() == Fact::FACT_MODEL_COLUMN_DESCR) {
        Fact *f = index.data(Fact::ModelDataRole).value<Fact *>();
        if (f && f->progress() >= 0) {
            //qDebug()<<f<<f->progress();
            if (drawProgress(painter, option, index, f->progress()))
                return;
        }
    }
    QItemDelegate::paint(painter, option, index);
}
//=============================================================================
bool FactDelegate::drawProgress(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index,
                                int progress) const
{
    if (progress < 0)
        return false;
    QStyleOptionViewItem opt(option);
    int w = opt.rect.width() / 2;
    if (w > 150)
        w = 150;
    else if (w < 80)
        w = 80;
    if (w > opt.rect.width() * 0.9)
        w = opt.rect.width() * 0.9;
    opt.rect.setWidth(opt.rect.width() - w);
    QItemDelegate::paint(painter, opt, index);
    QRect rect(option.rect);
    rect.translate(opt.rect.width(), 0);
    rect.setWidth(w);
    painter->fillRect(rect, index.data(Qt::BackgroundRole).value<QColor>());
    progressBar->resize(rect.size());
    //progressBar->setMaximumHeight(12);
    progressBar->setValue(progress);
    painter->save();
    painter->translate(rect.left(), rect.top() + (rect.height() - progressBar->height()) / 2);
    progressBar->render(painter);
    painter->restore();
    return true;
}
//=============================================================================
QSize FactDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QItemDelegate::sizeHint(option, index);
    /*QSize sz=QItemDelegate::sizeHint(option,index);
  sz.setHeight(QFontMetrics(option.font).height());
  return sz;*/
}
//=============================================================================
