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
#ifndef App_H
#define App_H
#include <QApplication>
#include <QFont>
#include <QQuickWindow>
#include <QtCore>

#include "AppBase.h"
#include "AppEngine.h"
#include "AppInstances.h"
#include "AppLog.h"
#include "AppNotify.h"
#include "AppNotifyListModel.h"
#include "AppPlugins.h"
#include "AppPrefs.h"
#include "AppRoot.h"
#include "AppSettings.h"
//=============================================================================
class App : public AppBase
{
    Q_OBJECT
    Q_PROPERTY(AppEngine *engine READ engine CONSTANT)
    Q_PROPERTY(QQuickWindow *window READ window NOTIFY windowChanged)
    Q_PROPERTY(double scale READ scale NOTIFY scaleChanged)

    Q_PROPERTY(AppLog *appLog READ appLog CONSTANT)
    Q_PROPERTY(AppNotify *appNotify READ appNotify CONSTANT)
    Q_PROPERTY(AppNotifyListModel *notifyModel READ notifyModel CONSTANT)

    Q_PROPERTY(AppPrefs *prefs READ prefs CONSTANT)

    Q_PROPERTY(QString lang READ lang CONSTANT)

public:
    explicit App(int &argc, char **argv, const QString &name, const QUrl &url);
    ~App();
    static App *instance() { return _instance; }

    Q_INVOKABLE static void sound(const QString &v) { _instance->playSoundEffect(v); }

    //js
    static void jsync(Fact *fact);
    Q_INVOKABLE static QJSValue jsexec(const QString &s);
    Q_INVOKABLE static QObject *loadQml(const QString &qmlFile, const QVariantMap &opts);

    template<class T>
    static T propertyValue(const QString &path)
    {
        if (!_instance->engine())
            return T();
        return _instance->engine()->jsGetProperty(path).toVariant().value<T>();
    }
    static void setGlobalProperty(const QString &path, const QVariant &value);
    static void setGlobalProperty(const QString &path, const QJSValue &value);
    static void setGlobalProperty(const QString &path, QObject *object);

    static void setContextProperty(const QString name, const QVariant &value);
    static void setContextProperty(const QString name, QObject *object);

    Q_INVOKABLE static QFont getMonospaceFont();
    Q_INVOKABLE static bool isFixedPitch(const QFont &font);
    Q_INVOKABLE static QChar materialIconChar(const QString &name);

    QStringList languages() const { return m_languages; }

    static AppPlugin *plugin(QString name);

private:
    static App *_instance;

    QStringList oPlugins;
    QString oQml;

    QStringList m_languages;

    AppPlugins *plugins;
    AppInstances *appInstances;

    void loadTranslations();
    void loadTranslator(const QString &fileName);

protected:
    AppRoot *f_apx;

    virtual void loadFonts();
    virtual void loadServices();

private slots:
    void appStateChanged(Qt::ApplicationState state);

    void mainWindowClosed(QCloseEvent *event);
    void appAboutToQuit();

    void updateSurfaceFormat();

public slots:
    void registerUiComponent(QObject *item, QString name);

    //load
    virtual void loadApp();

signals:
    void loadingFinished();
    void uiComponentLoaded(QString name, QJSValue object);

    void visibilityChanged(QWindow::Visibility visibility);
    void playSoundEffect(const QString &v);

    void about();

    void appQuit();

    //---------------------------------------
    //PROPERTIES
public:
    AppEngine *engine() const;

    QQuickWindow *window() const;
    double scale() const;
    void setScale(double v);

    AppLog *appLog() const;
    AppNotify *appNotify() const;
    AppNotifyListModel *notifyModel() const;

    AppPrefs *prefs() const;

    QString lang() const;

protected:
    AppEngine *m_engine;
    QQuickWindow *m_window;
    double m_scale;
    AppNotify *m_appNotify;
    AppNotifyListModel *m_notifyModel;
    AppLog *m_appLog;
    AppPrefs *m_prefs;
    QString m_lang;

signals:
    void windowChanged();
    void scaleChanged();
};
//=============================================================================
#endif
