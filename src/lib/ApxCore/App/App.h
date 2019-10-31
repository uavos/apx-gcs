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

public:
    explicit App(int &argc, char **argv, const QString &name, const QUrl &url);
    ~App();
    static App *instance() { return _instance; }

    void load(const QUrl &qml);

    Q_INVOKABLE static void sound(const QString &v) { _instance->playSoundEffect(v); }

    //js
    Q_INVOKABLE static QJSValue jsexec(const QString &s) { return instance()->engine()->jsexec(s); }
    static void jsync(Fact *fact) { instance()->engine()->jsSync(fact); }

    template<class T>
    static T propertyValue(const QString &path)
    {
        return instance()->engine()->jsGetProperty(path).toVariant().value<T>();
    }
    void setGlobalProperty(const QString &path, const QJSValue &value);

    Q_INVOKABLE static QFont getMonospaceFont();
    Q_INVOKABLE static bool isFixedPitch(const QFont &font);
    Q_INVOKABLE static QChar materialIconChar(const QString &name);

    QStringList languages() const { return m_languages; }

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
    QUrl url;
    AppRoot *f_apx;

private slots:
    void loadUrl();
    void appStateChanged(Qt::ApplicationState state);

    void quitRequested();

    void updateSurfaceFormat();

public slots:
    void registerUiComponent(QObject *item, QString name);

    //load
    virtual void loadApp();
    virtual void loadFonts();
    virtual void loadServices();

signals:
    void loadingFinished();
    void uiComponentLoaded(QString name, QJSValue object);

    void visibilityChanged(QWindow::Visibility visibility);
    void playSoundEffect(const QString &v);

signals:
    void about();

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

protected:
    AppEngine *m_engine;
    QQuickWindow *m_window;
    double m_scale;
    AppNotify *m_appNotify;
    AppNotifyListModel *m_notifyModel;
    AppLog *m_appLog;
signals:
    void windowChanged();
    void scaleChanged();
};
//=============================================================================
#endif
