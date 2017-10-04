#ifndef NumbersFORM_H
#define NumbersFORM_H

#include <QWidget>
#include <QWidget>
#include <QComboBox>
#include <QItemDelegate>
#include "ui_NumbersForm.h"
//=============================================================================
class NumbersItemDelegate : public QItemDelegate
{
  Q_OBJECT
public:
  NumbersItemDelegate(QObject *parent=0):QItemDelegate(parent){}
  virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const;
};
//=============================================================================
class NumbersForm : public QDialog, public Ui::NumbersForm
{
  Q_OBJECT
public:
  explicit NumbersForm(QWidget *parent = 0);

  QStringList itemsList();
private slots:
  virtual void done(int r);
  void cleanTable();
  void restoreDefaults();
};
//=============================================================================
#endif // NumbersFORM_H
