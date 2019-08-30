#include "NodesFrame.h"
#include <ApxApp.h>
#include <ApxDirs.h>
#include <ApxMisc/QActionFact.h>
#include <ApxMisc/SvgMaterialIcon.h>
#include <Nodes/Nodes.h>
#include <QAction>
//=============================================================================
NodesFrame::NodesFrame(QWidget *parent)
    : QWidget(parent)
    , vehicle(nullptr)
{
    vlayout = new QVBoxLayout(this);
    setLayout(vlayout);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    toolBar = new QToolBar(this);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    vlayout->addWidget(toolBar);
    lbUavName = new QLabel(this);
    vlayout->addWidget(lbUavName);

    treeWidget = new FactTreeWidget(AppRoot::instance(), true, false, this);
    vlayout->addWidget(treeWidget);
    connect(treeWidget->tree,
            &FactTreeView::customContextMenuRequested,
            this,
            &NodesFrame::treeContextMenu);

    connect(treeWidget->tree->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &NodesFrame::updateActions);

    connect(Vehicles::instance(), &Vehicles::vehicleSelected, this, &NodesFrame::vehicleSelected);
    vehicleSelected(Vehicles::instance()->current());
}
//=============================================================================
void NodesFrame::vehicleSelected(Vehicle *v)
{
    if (vehicle)
        disconnect(vehicle);
    vehicle = v;
    Nodes *fNodes = v->f_nodes;
    treeWidget->setRoot(fNodes);
    lbUavName->setText(v->title());
    lbUavName->setToolTip(QString("squawk: %1").arg(v->squawk(), 4, 16, QChar('0')).toUpper());

    connect(vehicle->f_nodes, &Nodes::modifiedChanged, this, &NodesFrame::updateActions);

    foreach (QAction *a, toolBar->actions()) {
        toolBar->removeAction(a);
        a->deleteLater();
    }
    //make actions
    QAction *a;
    a = new QActionFact(vehicle->f_nodes->f_upload);
    toolBar->addAction(a);
    // toolBar->widgetForAction(a)->setObjectName("greenAction");

    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_request));
    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_reload));
    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_stop));
    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_nstat));
    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_clear));

    QAction *aLookup = new QActionFact(vehicle->f_nodes->a_lookup);
    connect(aLookup, &QAction::triggered, vehicle->f_nodes->f_lookup, &Fact::requestDefaultMenu);
    toolBar->addAction(aLookup);

    toolBar->addAction(new QActionFact(vehicle->f_nodes->f_save));

    QAction *aShare = new QActionFact(vehicle->f_nodes->a_share);
    connect(aShare, &QAction::triggered, vehicle->f_nodes->f_share, &Fact::requestDefaultMenu);
    toolBar->addAction(aShare);

    aUndo = toolBar->addAction(SvgMaterialIcon("undo"),
                               tr("Revert"),
                               this,
                               &NodesFrame::aUndo_triggered);

    //all actions signals
    foreach (QAction *a, toolBar->actions()) {
        toolBar->widgetForAction(a)->setObjectName(a->objectName());
        connect(a, &QAction::triggered, [this]() { treeWidget->tree->setFocus(); });
    }
    connect(vehicle->f_nodes->f_request, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(vehicle->f_nodes->f_reload, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(vehicle->f_nodes->f_nstat, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(vehicle->f_nodes->f_clear, &Fact::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(aLookup, &QAction::triggered, treeWidget, &FactTreeWidget::resetFilter);
    connect(aShare, &QAction::triggered, treeWidget, &FactTreeWidget::resetFilter);

    updateActions();
}
//=============================================================================
void NodesFrame::updateActions(void)
{
    bool bMod = vehicle->f_nodes->modified();
    if (!bMod) {
        foreach (Fact *i, selectedItems()) {
            if (i->modified()) {
                bMod = true;
                break;
            }
        }
    }
    aUndo->setEnabled(bMod);
}
//=============================================================================
void NodesFrame::treeContextMenu(const QPoint &pos)
{
    if (vehicle->f_nodes->nodesCount() <= 0)
        return;
    //scan selected items
    NodesList nlist = selectedItems<NodeItem>();
    QMenu m(treeWidget);

    //editor actions
    QAction *a_revert = nullptr;
    foreach (NodesBase *f, selectedItems<NodesBase>()) {
        if (!f->modified())
            continue;
        if (!a_revert) {
            if (!m.isEmpty())
                m.addSeparator();
            a_revert = new QAction(f->f_revert->descr(), &m);
            m.addAction(a_revert);
        }
        connect(a_revert, &QAction::triggered, f->f_revert, &Fact::trigger);
    }

    //node tools
    if (!nlist.isEmpty()) {
        QHash<QString, QList<Fact *>> cmdHash;
        QHash<QString, QStringList> sectionHash;
        QStringList sections;
        foreach (NodeItem *node, nlist) {
            for (int i = 0; i < node->tools->model()->count(); ++i) {
                Fact *c = node->tools->model()->get(i);
                if (!c->enabled())
                    continue;
                if (c->name() == "rebootall")
                    continue;
                if (nlist.size() != 1) {
                    if (c->name() == "backups")
                        continue;
                }
                QString aname = c->title();
                //if((!ApxApp::devMode()) && aname.contains("_dev_"))continue;
                cmdHash[aname].append(c);
                if (!sections.contains(c->section()))
                    sections.append(c->section());
                QStringList &sl = sectionHash[c->section()];
                if (!sl.contains(aname))
                    sl.append(aname);
            }
        }
        if (!cmdHash.isEmpty()) {
            QHash<QString, QMenu *> groups;
            foreach (QString sect, sections) {
                if (!m.isEmpty())
                    m.addSeparator();
                foreach (QString cname, sectionHash.value(sect)) {
                    QList<Fact *> nl = cmdHash.value(cname);
                    if (nlist.size() > 1 && nl.size() == 1) {
                        NodeItem *node = nl.first()->findParent<NodeItem *>();
                        if (node)
                            cname += " (" + node->title() + ")";
                    }
                    if (nl.size() > 1)
                        cname += " [" + QString::number(nl.size()) + "]";

                    //create menu
                    Fact *c = nl.first();
                    QAction *a = new QAction(cname, &m);
                    DatabaseLookup *dbq = qobject_cast<DatabaseLookup *>(c);
                    if (dbq || c->size() > 0) { //group menu
                        QMenu *mGroup = groups.value(cname);
                        if (!mGroup) {
                            mGroup = m.addMenu(cname);
                            groups.insert(cname, mGroup);
                        }
                        if (dbq) {
                            dbq->trigger(); //refresh db (delayed thread)
                            int cnt = dbq->dbModel()->count();
                            if (cnt > 30)
                                cnt = 30;
                            else if (cnt <= 0)
                                mGroup->setEnabled(false);
                            for (int i = 0; i < cnt; ++i) {
                                QVariantMap modelData = dbq->dbModel()->get(i);
                                QString s = modelData.value("title").toString();
                                QString s2 = modelData.value("status").toString();
                                if (!s2.isEmpty())
                                    s.append(QString(" (%1)").arg(s2));
                                a = new QAction(s, &m);
                                mGroup->addAction(a);
                                connect(a, &QAction::triggered, [dbq, modelData]() {
                                    dbq->triggerItem(modelData);
                                });
                            }
                        } else {
                            for (int i = 0; i < c->size(); ++i) {
                                Fact *f = c->child(i);
                                a = new QAction(f->title(), &m);
                                mGroup->addAction(a);
                                foreach (Fact *c, nl) {
                                    f = c->child(i);
                                    if (!f)
                                        continue;
                                    connect(a, &QAction::triggered, f, &Fact::trigger);
                                }
                            }
                        }
                    } else
                        m.addAction(a);
                    foreach (Fact *c, nl) {
                        connect(a, &QAction::triggered, c, &Fact::trigger);
                    }
                }
            }
        }
    }

    //reset all nodes action
    if (!m.isEmpty())
        m.addSeparator();
    Fact *c = vehicle->f_nodes->nodes().first()->tools->f_rebootall;
    QAction *a = new QAction(c->title(), &m);
    connect(a, &QAction::triggered, c, &Fact::trigger);
    m.addAction(a);

    if (!m.isEmpty())
        m.exec(treeWidget->tree->mapToGlobal(pos));
}
//=============================================================================
void NodesFrame::aUndo_triggered(void)
{
    treeWidget->tree->setFocus();
    QList<NodesBase *> list = selectedItems<NodesBase>();
    foreach (NodesBase *f, list) {
        if (f->modified())
            f->f_revert->trigger();
    }
    if (list.isEmpty())
        vehicle->f_nodes->f_revert->trigger();
}
//=============================================================================
