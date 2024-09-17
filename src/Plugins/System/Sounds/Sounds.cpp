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
#include "Sounds.h"
#include <App/App.h>
#include <App/AppDirs.h>
#include <Fact/Fact.h>

Sounds::Sounds(Fact *parent)
    : Fact(parent,
           QString(PLUGIN_NAME).toLower(),
           tr("Sounds"),
           tr("Sound effects and speech"),
           Group | Bool)
    , effect(nullptr)
    , lastEffect(nullptr)
    , tts(nullptr)
{
    QStringList st;

    f_enabled = new Fact(this,
                         "sounds",
                         tr("Enable sounds"),
                         tr("Alarms and speech enable"),
                         Bool | PersistentValue);
    f_enabled->setDefaultValue(true);
    connect(f_enabled, &Fact::valueChanged, this, [this]() { setValue(f_enabled->value()); });
    setValue(f_enabled->value());
    connect(this, &Fact::valueChanged, f_enabled, [this]() { f_enabled->setValue(value()); });
    connect(f_enabled, &Fact::valueChanged, this, &Sounds::enabledChanged);

    f_engine = new Fact(this,
                        "engine",
                        tr("Engine"),
                        tr("Text to speech engine"),
                        Enum | PersistentValue);
    st.clear();
    st << "default";
    st << "internal";
    foreach (QString engine, QTextToSpeech::availableEngines()) {
        st.append(engine);
    }
    f_engine->setEnumStrings(st);
    connect(f_engine, &Fact::valueChanged, this, &Sounds::engineChanged);

    f_lang = new Fact(this, "lang", tr("Language"), tr("Voice locale"), Enum | PersistentValue);
    st.clear();
    st << "default";
    f_lang->setEnumStrings(st);

    f_voice = new Fact(this, "voice", tr("Voice"), tr("Speech voice"), Enum | PersistentValue);
    st.clear();
    st << "default";
    f_voice->setEnumStrings(st);

    f_rate = new Fact(this, "rate", tr("Rate"), tr("Voice rate [-1..+1]"), Float | PersistentValue);
    f_rate->setMin(-1.0);
    f_rate->setMax(1.0);
    connect(f_rate, &Fact::valueChanged, this, [this]() {
        if (tts)
            tts->setRate(f_rate->value().toDouble());
    });

    f_pitch = new Fact(this,
                       "pitch",
                       tr("Pitch"),
                       tr("Voice pitch [-1..+1]"),
                       Float | PersistentValue);
    f_pitch->setMin(-1.0);
    f_pitch->setMax(1.0);
    connect(f_pitch, &Fact::valueChanged, this, [this]() {
        if (tts)
            tts->setPitch(f_pitch->value().toDouble());
    });

    f_test = new Fact(this, "test", tr("Test"), tr("Speech sample text"), Text);
    f_test->setValue("Landing procedure initiated. Touchdown. Configuration accepted.");
    connect(f_test, &Fact::triggered, this, [this]() {
        say(tts ? f_test->text() : "configuration updated", true);
    });

    //read phrases config
    QFile fsys(AppDirs::res().filePath("templates/speech.json"));
    if (fsys.open(QFile::ReadOnly | QFile::Text)) {
        QJsonDocument json = QJsonDocument::fromJson(fsys.readAll());
        fsys.close();
        foreach (QJsonValue v, json["phrases"].toArray()) {
            QString key = v["msg"].toString();
            if (key.isEmpty())
                continue;
            phrases.insert(key, v.toObject().toVariantMap());
        }
        defaultVoices = json["voices"].toObject().toVariantMap();
    }

    //alarms
    QMap<QString, QString> alias;
    alias["error"] = "alarm2";
    alias["warning"] = "alarm";
    alias["disconnected"] = "alternator_off";
    alias["connected"] = "radar_lock";
    //load files and create effects
    //qDebug() << QSoundEffect::supportedMimeTypes();
    // QDir fdir(AppDirs::res().filePath("audio/alerts"), "*.ogg *.wav");
    QDir fdir(":audio/alerts", "*.ogg *.wav");
    foreach (QFileInfo fi, fdir.entryInfoList()) {
        if (!alias.values().contains(fi.baseName()))
            continue;
        auto se = new QSoundEffect(this);
        se->setSource(QUrl::fromLocalFile(fi.absoluteFilePath()));
        effects.insert(alias.key(fi.baseName()), se);
        // qDebug() << alias.key(fi.baseName()) << fi.baseName();
    }

    //say text repeated timeout timer
    lastEffectTimer.setSingleShot(true);
    lastEffectTimer.setInterval(2000);
    connect(&lastEffectTimer, &QTimer::timeout, this, [this]() { lastEffect = nullptr; });

    sayTextTimer.setSingleShot(true);
    sayTextTimer.setInterval(2000);
    connect(&sayTextTimer, &QTimer::timeout, this, [this]() { sayText.clear(); });

    //effect done timer
    effectTimer.setSingleShot(true);
    effectTimer.setInterval(50);
    connect(&effectTimer, &QTimer::timeout, this, &Sounds::effectTimeout);

    connect(App::instance(), &App::playSoundEffect, this, &Sounds::play);

    engineChanged();
}

void Sounds::engineChanged()
{
    if (tts) {
        delete tts;
        tts = nullptr;
    }
    int i = f_engine->value().toInt();
    if (i == 0) {
#ifdef Q_OS_MAC
        tts = new QTextToSpeech(this);
#else
        tts = nullptr;
#endif
    } else if (i == 1) {
        tts = nullptr;
    } else
        tts = new QTextToSpeech(f_engine->text(), this);

    disconnect(f_lang, &Fact::valueChanged, this, &Sounds::langChanged);
    QStringList st;
    st << "default";
    locales.clear();
    if (tts) {
        connect(tts, &QTextToSpeech::stateChanged, this, &Sounds::ttsStateChanged);
        locales.append(QLocale(QLocale::English, QLocale::UnitedStates));
        foreach (const QLocale &locale, tts->availableLocales()) {
            st << QString("%1 (%2)").arg(QLocale::languageToString(locale.language()),
                                         QLocale::territoryToString(locale.territory()));
            locales.append(locale);
        }
    }
    f_lang->setEnumStrings(st);
    if (f_lang->value().toInt() < 0) {
        f_lang->setValue(0);
    }
    connect(f_lang, &Fact::valueChanged, this, &Sounds::langChanged);
    langChanged();
}
void Sounds::langChanged()
{
    disconnect(f_voice, &Fact::valueChanged, this, &Sounds::voiceChanged);
    if (tts) {
        int i = f_lang->value().toInt();
        if (i >= 0 && i < locales.size())
            tts->setLocale(locales.at(i));
    }

    QStringList st;
    st << "default";
    voices.clear();
    if (tts) {
        voices.append(tts->voice());
        QString defaultVoiceName = defaultVoices.value(tts->locale().name()).toString();
        //qDebug()<<defaultVoiceName<<defaultVoices;
        foreach (const QVoice &voice, tts->availableVoices()) {
            st << QString("%1 - %2 - %3")
                      .arg(voice.name(),
                           QVoice::genderName(voice.gender()),
                           QVoice::ageName(voice.age()));
            voices.append(voice);
            if (voice.name() == defaultVoiceName)
                voices[0] = voice;
        }
    } else {
        QDir voicep(AppDirs::res().filePath("audio/speech"));
        foreach (QString s, voicep.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            st << s;
    }
    f_voice->setEnumStrings(st);
    if (f_voice->value().toInt() < 0) {
        f_voice->setValue(0);
    }

    connect(f_voice, &Fact::valueChanged, this, &Sounds::voiceChanged);
    voiceChanged();
}
void Sounds::voiceChanged()
{
    int i = f_voice->value().toInt();
    if (i < 0)
        return;
    if (tts) {
        if (i < voices.size()) {
            tts->setVoice(voices.at(i));
            if (f_rate->value().toDouble() != 0.0)
                tts->setRate(f_rate->value().toDouble());
            if (f_pitch->value().toDouble() != 0.0)
                tts->setPitch(f_pitch->value().toDouble());
        }
    } else {
        //load Sounds files
        speech.clear();
        QString lang = App::instance()->lang();
        QString voice = f_voice->text();
        if (i == 0) {
            if (lang == "ru")
                voice = "ru_milena";
            else
                voice = "vicki";
        }
        QDir fdir(":audio/speech/" + voice, "*.ogg *.wav");
        // QDir fdir(AppDirs::res().filePath("audio/speech/" + voice), "*.ogg *.wav");
        //qDebug()<<fdir.absolutePath();
        foreach (QString file, fdir.entryList()) {
            auto se = new QSoundEffect(this);
            se->setSource(QUrl::fromLocalFile(fdir.absoluteFilePath(file)));
            speech.insert(file.left(file.indexOf('.')), se);
            //qDebug()<<file;
        }
    }
}
void Sounds::enabledChanged()
{
    if (!f_enabled->value().toBool()) {
        if (tts)
            tts->stop();
        effectsQueue.clear();
        if (effect)
            effect->stop();
    }
}

void Sounds::play(QString text)
{
    //qDebug() << text;
    if (text.contains('\n')) {
        foreach (QString s, text.split('\n', Qt::SkipEmptyParts)) {
            play(s);
        }
        return;
    }
    if (!f_enabled->value().toBool())
        return;
    //qDebug()<<"play"<<text;
    say(text.replace(':', ' ').simplified().toLower(), false);
}

void Sounds::queue(QSoundEffect *e)
{
    if (effect) {
        if (!effectsQueue.contains(e))
            effectsQueue.append(e);
        return;
    }
    effect = e;
    e->play();
    effectTimer.start();
}

void Sounds::effectTimeout()
{
    if (effect && effect->isPlaying()) {
        effectTimer.start();
        return;
    }
    effect = nullptr;
    sayNext();
}

void Sounds::say(const QString &text, bool ttsForce)
{
    //qDebug() << text;
    QStringList st;
    if (tts) {
        foreach (QString key, phrases.keys()) {
            if (!text.contains(key, Qt::CaseInsensitive))
                continue;
            const QVariantMap &m = phrases.value(key);
            QString s = m.value(tts->voice().name(), m.value("say", key)).toString();
            if (s.isEmpty())
                continue;
            //qDebug()<<key<<s<<m;
            st.append(s);
        }
        if (st.isEmpty() && ttsForce)
            st.append(text);
        //qDebug()<<st<<text;
    } else {
        foreach (QString key, speech.keys()) {
            if (!text.contains(key, Qt::CaseInsensitive))
                continue;
            st.append(key);
        }
    }
    if (st.isEmpty()) {
        //try effects
        foreach (QString key, effects.keys()) {
            if (text.contains(key, Qt::CaseInsensitive))
                st.append(key);
        }
        if (st.isEmpty())
            return;
        QSoundEffect *e = effects.value(st.last());
        if (!e)
            return;
        if (lastEffect == e)
            return;
        lastEffect = e;
        if (!lastEffectTimer.isActive())
            lastEffectTimer.start();
        queue(e);
        return;
    }
    //qDebug()<<st;
    st.sort();
    QString s = st.last();
    if (sayText == s)
        return;
    sayText = s;
    if (!sayTextTimer.isActive())
        sayTextTimer.start();
    if (!sayQueue.contains(s))
        sayQueue.enqueue(s);
    sayNext();
}
void Sounds::sayNext()
{
    if (tts) {
        if (tts->state() == QTextToSpeech::Speaking)
            return;
        if (sayQueue.isEmpty())
            return;
        QString s = sayQueue.dequeue();
        //qDebug()<<s;
        tts->say(s);
    } else {
        if (effect)
            return;
        if (sayQueue.isEmpty())
            return;
        QSoundEffect *e = speech.value(sayQueue.dequeue());
        effect = e;
        e->play();
        effectTimer.start();
    }
}
void Sounds::ttsStateChanged(QTextToSpeech::State state)
{
    //qDebug()<<state;
    if (state == QTextToSpeech::Speaking)
        return;
    QTimer::singleShot(0, this, &Sounds::sayNext);
}
