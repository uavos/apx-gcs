#include "NumbersForm.h"
#include "ui_NumbersForm.h"
#include "QMandala.h"
//==============================================================================
NumbersForm::NumbersForm(QWidget *parent)
 : QDialog(parent)
{
  setupUi(this);
  restoreGeometry(QSettings().value(objectName()).toByteArray());
  tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  tableWidget->horizontalHeader()->setMinimumSectionSize(70);

  tableWidget->setItemDelegateForColumn(0,new NumbersItemDelegate(tableWidget));
  foreach(QString sline,QSettings().value("Numbers").toStringList()){
    int row=tableWidget->rowCount();
    int col=0;
    tableWidget->insertRow(row);
    foreach(QString s,sline.trimmed().split(':')){
      QTableWidgetItem *newItem=new QTableWidgetItem(s);
      //newItem->setFlags(newItem->flags() & ~(Qt::ItemIsDropEnabled));
      tableWidget->setItem(row,col++,newItem);
    }
  }
  cleanTable();
  connect(tableWidget,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(cleanTable()));
  connect((QObject*)buttonBox->button(QDialogButtonBox::RestoreDefaults),SIGNAL(clicked()),this,SLOT(restoreDefaults()));
}
//==============================================================================
void NumbersForm::done(int r)
{
  QSettings().setValue(objectName(),saveGeometry());
  QDialog::done(r);
}
//=============================================================================
void NumbersForm::cleanTable()
{//return;
  for(int row=0;row<tableWidget->rowCount();row++){
    QTableWidgetItem *i=tableWidget->item(row,0);
    if((!i) || i->text().isEmpty() || (!(QMandala::instance()->current->field(i->text())->name()==i->text()||i->text().contains(".")))){
      tableWidget->removeRow(row);
      continue;
    }
  }
  tableWidget->insertRow(0);
  tableWidget->insertRow(tableWidget->rowCount());
}
//==============================================================================
QStringList NumbersForm::itemsList()
{
  cleanTable();
  QStringList st;
  for(int row=0;row<tableWidget->rowCount();row++){
    QString s;
    int sepCnt=0;
    for(int col=0;col<tableWidget->columnCount();col++){
      QTableWidgetItem *i=tableWidget->item(row,col);
      if(!i)continue;
      for(;sepCnt<col;sepCnt++)s+=':';
      s+=i->text();
    }
    if(s.size()>0)st.append(s);
  }
  return st;
}
//==============================================================================
//=============================================================================
QWidget *NumbersItemDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem &option,const QModelIndex &index) const
{
  Q_UNUSED(option);
  if(index.column()>0)
    return QItemDelegate::createEditor(parent,option,index);
  QComboBox *cb=new QComboBox(parent);
  cb->setFrame(false);
  cb->addItem("");
  cb->addItems(QMandala::instance()->local->names);
  cb->setEditable(true);
  cb->view()->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Ignored);
  cb->view()->setMaximumWidth(cb->view()->sizeHintForColumn(0)*2);
  QWidget *e=cb;
  e->setFont(index.data(Qt::FontRole).value<QFont>());
  e->setAutoFillBackground(true);
  return e;
}
//=============================================================================
void NumbersForm::restoreDefaults()
{
  tableWidget->clear();
  accept();
}
//=============================================================================
