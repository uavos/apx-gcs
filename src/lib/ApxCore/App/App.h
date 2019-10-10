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
#include <QQuickWindow>
#include <QtCore>

#include <App/AppBase.h>
#include <App/AppEngine.h>
#include <App/AppInstances.h>
#include <App/AppLog.h>
#include <App/AppPlugins.h>
#include <App/AppRoot.h>
#include <App/AppSettings.h>
//=============================================================================
class App : public AppBase
{
    Q_OBJECT
    Q_PROPERTY(AppEngine *engine READ engine CONSTANT)
    Q_PROPERTY(QQuickWindow *window READ window NOTIFY windowChanged)
    Q_PROPERTY(double scale READ scale NOTIFY scaleChanged)

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

    static QFont getMonospaceFont();
    static bool isFixedPitch(const QFont &font);

private:
    static App *_instance;

    QStringList oPlugins;
    QString oQml;

    AppLog log;

    AppPlugins *plugins;
    AppInstances *appInstances;

protected:
    QUrl url;
    AppRoot *f_app;

private slots:
    void loadUrl();
    void appStateChanged(Qt::ApplicationState state);

    void quitRequested();

    void updateSurfaceFormat();

public slots:
    void registerUiComponent(QObject *item, QString name);

    //load
    virtual void loadApp();
    virtual void loadTranslations();
    virtual void loadFonts();
    virtual void loadServices();

signals:
    void loadingFinished();
    void uiComponentLoaded(QString name, QJSValue object);

    void visibilityChanged(QWindow::Visibility visibility);
    void playSoundEffect(const QString &v);

    // notifications and text messages
public:
    enum NotifyFlag {
        //message origin [enum]
        NotifySourceMask = 0x0F,
        FromApp = 0,     // msg from app
        FromInput = 1,   // msg from user terminal input
        FromVehicle = 2, // msg from vehicle

        //importance [enum]
        NotifyTypeMask = 0xF0,
        Info = 0 << 4,
        Important = 1 << 4,
        Warning = 2 << 4,
        Error = 3 << 4,

        //options [bits]
        NotifyOptionsMask = 0xF00,
        Console = 1 << 8, // msg from log stream
    };
    Q_DECLARE_FLAGS(NotifyFlags, NotifyFlag)
    Q_FLAG(NotifyFlags)
    Q_ENUM(NotifyFlag)

public slots:
    void report(QString msg,
                App::NotifyFlags flags = App::NotifyFlags(FromApp),
                QString subsystem = QString());
    void report(Fact *fact);
private slots:
    void logInfoMessage(QString msg);
    void logWarningMessage(QString msg);

signals:
    void notification(QString msg, QString subsystem, App::NotifyFlags flags, Fact *fact);

    //---------------------------------------
    //PROPERTIES
public:
    AppEngine *engine() const;
    QQuickWindow *window() const;
    double scale() const;
    void setScale(double v);

protected:
    AppEngine *m_engine;
    QQuickWindow *m_window;
    double m_scale;
signals:
    void windowChanged();
    void scaleChanged();
};
Q_DECLARE_OPERATORS_FOR_FLAGS(App::NotifyFlags)
//=============================================================================
#endif
