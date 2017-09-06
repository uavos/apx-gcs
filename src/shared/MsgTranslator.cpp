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
#include "MsgTranslator.h"
#include <QDomDocument>
//=============================================================================
MsgTranslator::MsgTranslator(QObject *parent)
  : QTranslator(parent)
{
}
//=============================================================================
void MsgTranslator::loadXml(const QString &fname)
{
  QFile file(fname);
  if(!file.open(QFile::ReadOnly | QFile::Text)) return;
  QDomDocument doc;
  if(!(doc.setContent(&file) && doc.documentElement().nodeName()=="TS")){
    file.close();
    return;
  }
  file.close();
  QDomNode dom=doc.documentElement();
  for(QDomElement e=dom.firstChildElement("context");!e.isNull();e=e.nextSiblingElement(e.tagName())){
    QDomElement ecn=e.firstChildElement("name");
    if(ecn.isNull())continue;
    const QString cn=ecn.text();
    for(QDomElement em=e.firstChildElement("message");!em.isNull();em=em.nextSiblingElement(em.tagName())){
      const QString src=em.firstChildElement("source").text().trimmed();
      const QString trs=em.firstChildElement("translation").text().trimmed();
      if(src.isEmpty()||trs.isEmpty())continue;
      map[cn][src]=trs;
      //qDebug()<<cn<<src<<trs;
    }
  }
}
//=============================================================================
QString MsgTranslator::translate(const char * context, const char * sourceText, const char * disambiguation, int n) const
{
  Q_UNUSED(disambiguation)
  Q_UNUSED(n)
  QString s(sourceText);
  if(!map.contains(context))return s;
  const QMap<QString,QString> &m=map.value(context);
  const QList<QString> &keys=m.keys();
  if(keys.contains(s))return m.value(s);
  //try to find partial string
  QStringList st;
  foreach(const QString &k,keys){
    if(s.startsWith(k)||s.endsWith(k))
      st.append(k);
  }
  if(st.isEmpty())return s;
  st.sort();
  const QString &k=st.last();
  if(s.startsWith(k))return m.value(k)+s.mid(k.size());
  return s.left(s.size()-k.size())+m.value(k);
}
//=============================================================================

