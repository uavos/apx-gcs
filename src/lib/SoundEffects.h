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
#ifndef SoundEffects_H
#define SoundEffects_H
#include <QtCore>
#include <QMediaPlayer>
//=============================================================================
#include <QtCore>
#include <QMediaPlayer>
#include <QSoundEffect>
class QMandala;
//=============================================================================
class SoundEffects : public QObject
{
  Q_OBJECT
public:
  SoundEffects(QObject *parent=0);
private:
  QMandala *mandala;
  QHash<QString, QSoundEffect*> speech;
  QHash<QString, QSoundEffect*> effects;
  QList<QSoundEffect *> effectsQueue;
  QSoundEffect *lastEffect;

  QTimer timeoutTimer;
private slots:
  void timeout();
  void effectPlayingChanged();
public slots:
  void play(QString text);
};
//=============================================================================
#endif
