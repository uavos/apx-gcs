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
#include "NodesFrame.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <ApxMisc/MaterialIcon.h>
#include <ApxMisc/QActionFact.h>
#include <Nodes/NodeField.h>
#include <Nodes/NodeItem.h>
#include <Nodes/Nodes.h>
#include <TreeModel/FactDelegate.h>
#include <QAction>

namespace {
using ValuesProvider = std::function<QStringList(Fact *)>;

class UnitValuesDelegate : public FactDelegate
{
public:
    UnitValuesDelegate(ValuesProvider provider, QObject *parent)
        : FactDelegate(parent)
        , _provider(std::move(provider))
    {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        FactDelegate::paint(painter, option, index);
        if (index.column() != Fact::FACT_MODEL_COLUMN_VALUE || values(index).isEmpty())
            return;

        const QRect r = indicatorRect(option);
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(option.palette.color(QPalette::Mid));
        painter->drawPolygon(QPolygon{QPoint(r.left(), r.top()),
                                      QPoint(r.left(), r.bottom()),
                                      QPoint(r.right(), r.center().y())});
        painter->restore();
    }

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override
    {
        if (event->type() != QEvent::MouseButtonRelease
            || index.column() != Fact::FACT_MODEL_COLUMN_VALUE)
            return FactDelegate::editorEvent(event, model, option, index);

        auto mouseEvent = static_cast<QMouseEvent *>(event);
        const auto list = values(index);
        if (list.isEmpty()
            || !indicatorRect(option).adjusted(-5, -4, 5, 4).contains(mouseEvent->pos()))
            return FactDelegate::editorEvent(event, model, option, index);

        QMenu menu(qobject_cast<QWidget *>(parent()));
        for (const auto &text : list)
            menu.addAction(text);
        auto view = qobject_cast<QAbstractItemView *>(parent());
        auto anchor = view ? view->viewport() : option.widget;
        menu.exec(anchor->mapToGlobal(indicatorRect(option).bottomRight() + QPoint(0, 3)));
        return true;
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto size = FactDelegate::sizeHint(option, index);
        if (index.column() == Fact::FACT_MODEL_COLUMN_VALUE && !values(index).isEmpty())
            size.rwidth() += 14;
        return size;
    }

private:
    QStringList values(const QModelIndex &index) const
    {
        auto fact = index.data(Fact::ModelDataRole).value<Fact *>();
        return fact ? _provider(fact) : QStringList{};
    }

    static QRect indicatorRect(const QStyleOptionViewItem &option)
    {
        return QRect(option.rect.right() - 9, option.rect.center().y() - 3, 6, 9);
    }

    ValuesProvider _provider;
};

QJsonValue configValue(const QJsonObject &config,
                       const NodeItem *currentNode,
                       const NodeField *currentField)
{
    QJsonObject matchedNode;
    for (const auto &item : config.value("nodes").toArray()) {
        const auto node = item.toObject();
        const auto info = node.value("info").toObject();
        if (info.value("uid").toString() == currentNode->uid()) {
            matchedNode = node;
            break;
        }
        if (matchedNode.isEmpty() && info.value("name").toString() == currentNode->title())
            matchedNode = node;
    }
    if (matchedNode.isEmpty())
        return QJsonValue::Undefined;

    auto field = currentField;
    int arrayIndex = -1;
    if (auto parent = qobject_cast<NodeField *>(field->parentFact()); parent && parent->size() > 0) {
        arrayIndex = field->array();
        field = parent;
    }

    const auto values = matchedNode.value("values").toObject();
    if (!values.contains(field->fpath()))
        return QJsonValue::Undefined;

    const auto value = values.value(field->fpath());
    if (arrayIndex < 0)
        return value;
    const auto array = value.toArray();
    return arrayIndex < array.size() ? array.at(arrayIndex) : QJsonValue::Undefined;
}
}

NodesFrame::NodesFrame(QWidget *parent)
    : QWidget(parent)
    , _unit(nullptr)
{
    vlayout = new QVBoxLayout(this);
    setLayout(vlayout);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);

    toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    vlayout->addWidget(toolBar);
    lbUavName = new QLabel(this);
    lbUavName->setFont(App::font());
    vlayout->addWidget(lbUavName);

    treeWidget = new FactTreeWidget(nullptr, true, false, this);
    auto oldDelegate = treeWidget->tree->itemDelegate();
    treeWidget->tree->setItemDelegate(new UnitValuesDelegate(
        [this](Fact *fact) {
            QStringList values;
            auto field = qobject_cast<NodeField *>(fact);
            if (!_unit || !field || fact->size() > 0 || !fact->modified())
                return values;

            auto parent = fact->parentFact();
            while (parent && !qobject_cast<NodeItem *>(parent))
                parent = parent->parentFact();
            auto node = qobject_cast<NodeItem *>(parent);
            if (!node)
                return values;

            const auto appendValue = [&](const QJsonObject &config, const QString &title) {
                const auto rawValue = configValue(config, node, field);
                if (rawValue.isUndefined())
                    return;
                const auto text = field->toText(rawValue.toVariant());
                const auto display = field->units().isEmpty()
                                         ? text
                                         : QString("%1 %2").arg(text, field->units());
                values.append(QString("%1 — %2").arg(title, display));
            };

            appendValue(_unit->initialConfiguration(), _unit->title() + " (original)");
            for (const auto &config : _unit->loadedConfigurations()) {
                appendValue(config, config.value("title").toString());
            }
            return values;
        },
        treeWidget->tree));
    oldDelegate->deleteLater();
    vlayout->addWidget(treeWidget);
    connect(treeWidget->tree,
            &FactTreeView::customContextMenuRequested,
            this,
            &NodesFrame::treeContextMenu);

    connect(treeWidget, &FactTreeWidget::selectionChanged, this, &NodesFrame::updateActions);

    connect(Fleet::instance(), &Fleet::unitSelected, this, &NodesFrame::unitSelected);
    unitSelected(Fleet::instance()->current());
}

void NodesFrame::unitSelected(Unit *unit)
{
    if (_unit) {
        _unit->disconnect(this);
        disconnect(_unit);
    }

    _unit = unit;
    Nodes *fNodes = unit->f_nodes;
    treeWidget->setRoot(fNodes);
    lbUavName->setText(unit->title());

    connect(unit->f_nodes, &Nodes::modifiedChanged, this, &NodesFrame::updateActions);
    connect(unit, &Unit::configurationLoaded, treeWidget->tree->viewport(), qOverload<>(&QWidget::update));

    for (auto a : toolBar->actions()) {
        toolBar->removeAction(a);
        a->deleteLater();
    }
    //make actions
    QAction *a;
    a = new QActionFact(unit->f_nodes->f_upload);
    toolBar->addAction(a);

    toolBar->addAction(new QActionFact(unit->f_nodes->f_search));
    toolBar->addAction(new QActionFact(unit->f_nodes->f_reload));
    toolBar->addAction(new QActionFact(unit->f_nodes->f_stop));
    toolBar->addAction(new QActionFact(unit->f_nodes->f_clear));

    QAction *aLookup = new QActionFact(unit->f_storage);
    toolBar->addAction(aLookup);
    connect(aLookup, &QAction::triggered, treeWidget, &FactTreeWidget::resetFilter);

    QAction *aShare = new QActionFact(unit->f_share);
    toolBar->addAction(aShare);
    connect(aShare, &QAction::triggered, treeWidget, &FactTreeWidget::resetFilter);

    aUndo = toolBar->addAction(MaterialIcon("undo"),
                               tr("Revert"),
                               this,
                               &NodesFrame::aUndo_triggered);

    aDefaults = toolBar->addAction(MaterialIcon("lock-reset"),
                                   tr("Restore defaults"),
                                   this,
                                   &NodesFrame::aDefaults_triggered);

    //all actions signals
    for (auto a : toolBar->actions()) {
        toolBar->widgetForAction(a)->setObjectName(a->objectName());
        connect(a, &QAction::triggered, treeWidget, [this]() { treeWidget->tree->setFocus(); });
    }
    connect(unit->f_nodes->f_search, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(unit->f_nodes->f_reload, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(unit->f_nodes->f_clear, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);

    updateActions();
}

void NodesFrame::updateActions(void)
{
    bool bMod = _unit->f_nodes->modified();
    if (!bMod) {
        for (auto f : treeWidget->tree->selectedItems()) {
            if (f->modified()) {
                bMod = true;
                break;
            }
        }
    }
    aUndo->setEnabled(bMod);
}

void NodesFrame::treeContextMenu(const QPoint &pos)
{
    //scan selected items
    QList<NodeItem *> nlist = treeWidget->tree->selectedItems<NodeItem>();
    QMenu m(treeWidget);

    //editor actions
    QAction *a = new QAction(aDefaults->icon(), aDefaults->text(), &m);
    connect(a, &QAction::triggered, aDefaults, &QAction::trigger);
    m.addAction(a);
    for (auto i : treeWidget->tree->selectedItems<Fact>()) {
        if (!i->modified())
            continue;
        a = new QAction(aUndo->icon(), aUndo->text(), &m);
        connect(a, &QAction::triggered, aUndo, &QAction::trigger);
        m.addAction(a);
        break;
    }
    m.addSeparator();

    //node tools
    for (auto node : nlist) {
        QString sect;
        for (int i = 0; i < node->tools->size(); ++i) {
            Fact *f = node->tools->child(i);
            if (sect.isEmpty())
                sect = f->section();
            else if (f->section() != sect && !m.isEmpty()) {
                m.addSeparator();
                sect = f->section();
            }
            addNodeTools(&m, f, node->title());
        }
    }
    if (nlist.size() > 1)
        updateMenuTitles(&m);

    if (!m.isEmpty())
        m.exec(treeWidget->tree->mapToGlobal(pos));
}

void NodesFrame::aUndo_triggered(void)
{
    treeWidget->tree->setFocus();
    QList<Fact *> list = treeWidget->tree->selectedItems<Fact>();
    for (auto i : list) {
        if (i->modified())
            i->restore();
    }
    if (list.isEmpty())
        _unit->f_nodes->restore();
}
void NodesFrame::aDefaults_triggered(void)
{
    treeWidget->tree->setFocus();
    QList<Fact *> list = treeWidget->tree->selectedItems<Fact>();
    for (auto i : list) {
        i->restoreDefaults();
    }
    if (list.isEmpty())
        _unit->f_nodes->restoreDefaults();
}

void NodesFrame::addNodeTools(QMenu *menu, Fact *fact, QString nodeName)
{
    if (fact->menu()) {
        if (fact->name() == "modules")
            return;

        //fact is group
        QMenu *m = nullptr;
        for (auto i : menu->findChildren<QMenu *>()) {
            if (i->objectName() == fact->title()) {
                m = i;
                break;
            }
        }
        if (!m) {
            m = menu->addMenu(fact->title());
            m->setObjectName(fact->title());
            m->setToolTip(nodeName);
        } else {
            //list nodes in tooltip
            QStringList st = m->toolTip().split(',', Qt::SkipEmptyParts);
            st.append(nodeName);
            st.removeDuplicates();
            m->setToolTip(st.join(','));
        }
        // add sub items

        auto storage = qobject_cast<NodeStorage *>(fact);
        if (storage) {
            if (!m->actions().isEmpty()) {
                m->setEnabled(false);
                return;
            }
            auto model = storage->dbmodel();
            connect(model, &DatabaseModel::recordsListChanged, m, [this, m, storage, nodeName]() {
                auto model = storage->dbmodel();
                int cnt = model->rowCount();
                if (cnt > 30)
                    cnt = 30;
                else if (cnt <= 0)
                    m->setEnabled(false);
                for (int i = 0; i < cnt; ++i) {
                    auto a = new QAction(m);

                    const auto jso = model->get(i);
                    a->setData(jso.value("key").toVariant());

                    QString s = jso.value("title").toString();
                    QString s2 = jso.value("value").toString();
                    if (!s2.isEmpty())
                        s.append(QString(" (%1)").arg(s2));

                    a->setText(s);
                    a->setObjectName(s);
                    a->setToolTip(nodeName);
                    m->addAction(a);

                    // link action to db record
                    connect(a, &QAction::triggered, storage, [storage, a]() {
                        storage->loadNodeConf(a->data().toULongLong());
                    });

                    // update caption
                    connect(model,
                            &DatabaseModel::dataChanged,
                            a,
                            [model, a, i](const QModelIndex &topLeft) {
                                if (topLeft.row() != i)
                                    return;
                                const auto jso = model->get(i);
                                QString s = jso.value("title").toString();
                                QString s2 = jso.value("value").toString();
                                if (!s2.isEmpty())
                                    s.append(QString(" (%1)").arg(s2));
                                a->setText(s);
                                a->setData(jso.value("key").toVariant());
                            });
                }
            });

            emit model->requestRecordsList(); //refresh db (delayed thread)
            return;
        }

        for (int i = 0; i < fact->size(); ++i) {
            addNodeTools(m, fact->child(i), nodeName);
        }
        return;
    }

    //fact is command
    QAction *a = nullptr;
    for (auto i : menu->actions()) {
        if (i->objectName() == fact->title()) {
            a = i;
            break;
        }
    }
    if (!a) {
        a = menu->addAction(fact->title());
        a->setObjectName(fact->title());
        a->setToolTip(nodeName);
    } else {
        //list nodes in tooltip
        QStringList st = a->toolTip().split(',', Qt::SkipEmptyParts);
        st.append(nodeName);
        st.removeDuplicates();
        a->setToolTip(st.join(','));
    }

    connect(a, &QAction::triggered, fact, [fact]() { fact->trigger(); });
}
void NodesFrame::updateMenuTitles(QMenu *menu)
{
    QStringList st = menu->toolTip().split(',', Qt::SkipEmptyParts);
    QString s = st.size() > 3 ? QString::number(st.size()) : st.join(',');
    menu->setTitle(QString("%1 [%2]").arg(menu->objectName(), s));

    for (auto a : menu->actions()) {
        QStringList st = a->toolTip().split(',', Qt::SkipEmptyParts);
        QString s = st.size() > 3 ? QString::number(st.size()) : st.join(',');
        a->setText(QString("%1 [%2]").arg(a->objectName(), s));
    }

    for (auto i : menu->findChildren<QMenu *>()) {
        updateMenuTitles(i);
    }
}
