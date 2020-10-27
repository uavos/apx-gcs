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
#ifndef Sounds_H
#define Sounds_H
#include <QSound>
#include <QTextToSpeech>
#include <QtCore>

#include <Fact/Fact.h>
//=============================================================================
class Sounds : public Fact
{
    Q_OBJECT
public:
    Sounds(Fact *parent = nullptr);

    Fact *f_enabled;
    Fact *f_engine;
    Fact *f_lang;
    Fact *f_voice;
    Fact *f_rate;
    Fact *f_pitch;

    Fact *f_test;

private:
    QHash<QString, QSound *> speech;
    QHash<QString, QSound *> effects;
    QList<QSound *> effectsQueue;
    QSound *effect;
    QSound *lastEffect;

    QTimer lastEffectTimer;
    QTimer effectTimer;

    QTextToSpeech *tts;
    QMap<QString, QVariantMap> phrases;
    QList<QLocale> locales;
    QList<QVoice> voices;
    QVariantMap defaultVoices;

    QQueue<QString> sayQueue;
    QString sayText;
    QTimer sayTextTimer;
    void say(const QString &text, bool ttsForce);
    void sayNext();

private slots:
    void engineChanged();
    void langChanged();
    void voiceChanged();
    void enabledChanged();

    void effectTimeout();
    void queue(QSound *e);

    void ttsStateChanged(QTextToSpeech::State state);

public slots:
    void play(QString text);
};
//=============================================================================
#endif
