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
#include "App.h"
#include "AppDirs.h"
#include "AppNotifyListModel.h"
#include "AppQuickView.h"
#include "AppWindow.h"

#include <ApxMisc/MaterialIcon.h>
#include <ApxMisc/SvgImageProvider.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QFontDatabase>
#include <QFontInfo>
#include <QGLFormat>
#include <QQuickStyle>
#include <QQuickView>
#include <QScreen>
#include <QStyleFactory>
#include <QtQuick>

//=============================================================================
App *App::_instance = nullptr;
App::App(int &argc, char **argv, const QString &name, const QUrl &url)
    : AppBase(argc, argv, name)
    , m_window(nullptr)
    , m_scale(1.0)
{
    _instance = this;

    //    QStringList ldp;
    //    ldp << qgetenv("LD_LIBRARY_PATH");
    //    ldp << AppDirs::libs().absolutePath();
    //    ldp.removeAll("");
    //    ldp.removeDuplicates();
    //    qputenv("LD_LIBRARY_PATH", ldp.join(':').toUtf8());
    //qDebug() << qgetenv("LD_LIBRARY_PATH");

    m_prefs = new AppPrefs(this);
    m_lang = m_prefs->loadValue("lang").toString();

    //setup logging
    m_appLog = new AppLog(this);
    m_appNotify = new AppNotify(this);
    m_notifyModel = new AppNotifyListModel(m_appNotify);

    connect(m_appNotify, &AppNotify::notification, this, [](QString msg) {
        if (!msg.isEmpty())
            sound(msg);
    });

    //load
    loadTranslations();

    qRegisterMetaType<QScreen *>("QScreen");

    qmlRegisterUncreatableType<App>("APX.Facts", 1, 0, "App", "Reference only");
    qmlRegisterUncreatableType<AppNotify>("APX.Facts", 1, 0, "AppNotify", "Reference only");

    //---------------------------------------
    // command line options
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Ground Control Software by UAVOS (C) Aliaksei Stratsilatau <sa@uavos.com>");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dummyOption("t");
    parser.addOption(dummyOption);

    QCommandLineOption pluginsOption(
        QStringList() << "p"
                      << "plugins",
        QCoreApplication::translate("main", "Load only listed plugins <plugin1,plugin2,...>."),
        QCoreApplication::translate("main", "plugins"));
    parser.addOption(pluginsOption);

    QCommandLineOption qmlMainFile(QStringList() << "c"
                                                 << "qml",
                                   QCoreApplication::translate("main", "Load main qml from <file>."),
                                   QCoreApplication::translate("main", "file"));
    parser.addOption(qmlMainFile);

    parser.process(*qApp);
    oPlugins = parser.value(pluginsOption).split(',', Qt::SkipEmptyParts);
    oQml = parser.value(qmlMainFile);
    //---------------------------------------

    appInstances = new AppInstances(this);

    //styles
    QQuickStyle::setStyle("Material");
    QFile styleSheet(":styles/style-dark.css");
    if (styleSheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(styleSheet.readAll());
    }
#ifdef Q_OS_MAC
    QStyle *style = QStyleFactory::create("Fusion");
#else
    QStyle *style = QStyleFactory::create("Breeze");
#endif
    if (style)
        qApp->setStyle(style);

    connect(this, &QCoreApplication::aboutToQuit, this, &App::appAboutToQuit);
    connect(this, &QGuiApplication::applicationStateChanged, this, &App::appStateChanged);

    //js engine
    m_engine = new AppEngine();

    if (!oQml.isEmpty()) {
        m_engine->globalObject().setProperty("qmlMainFile", oQml);
    } else {
        //check for overrided layout file
        QString fname = AppDirs::userPlugins().absoluteFilePath(
            QString("%1.qml").arg(QCoreApplication::applicationName().remove(' ')));
        QFileInfo fi(fname);
        if (fi.exists()) {
            apxMsg() << tr("Using customized layout").append(':') << fi.fileName();
            QUrl url(fname);
            if (url.scheme().isEmpty())
                url.setScheme("file");
            m_engine->globalObject().setProperty("qmlMainFile", url.toString());
        }
    }
    setGlobalProperty("ui", m_engine->newObject());
    jsexec(QString("ui.__defineGetter__('%1', function(){ return application.%1; });").arg("scale"));

    f_apx = new AppRoot();

    m_engine->jsSyncObject(this);

    loadFonts();

    // main window

    QString title = QString("%1 (%2)").arg(App::applicationName()).arg(App::applicationVersion());
    if (!installed())
        title.append(QString(" - %1").arg(tr("not installed").toUpper()));
    AppQuickView *view = new AppQuickView("main", title);

    view->setFlag(Qt::WindowSystemMenuHint);
    view->setFlag(Qt::WindowMinimizeButtonHint);
    view->setFlag(Qt::WindowFullscreenButtonHint);

    view->setMinimumSize(view->screen()->geometry().size() / 4);

    connect(view, &AppQuickView::closed, this, &App::mainWindowClosed);

    if (m_window != view) {
        connect(view, &QQuickWindow::visibilityChanged, this, &App::visibilityChanged);
        m_window = view;
        emit windowChanged();
    }
    registerUiComponent(view, "window");

    view->setSource(url);
}
App::~App()
{
    qDebug() << QDateTime::currentDateTimeUtc().toString();
}
void App::mainWindowClosed(QCloseEvent *event)
{
    event->ignore();
    emit appQuit();
    QTimer::singleShot(0, this, &App::quit);
}
void App::appAboutToQuit()
{
    apxMsg() << tr("Quit").append("...");
    m_engine->collectGarbage();
    emit appQuit();
}

void App::loadApp()
{
    apxConsole() << QObject::tr("Loading application").append("...");

    //plugins
    plugins = new AppPlugins(f_apx->f_pluginsSettings, this);
    connect(plugins, &AppPlugins::loadedTool, f_apx, &AppRoot::addToolPlugin);
    connect(plugins, &AppPlugins::loadedWindow, f_apx, &AppRoot::addWindowPlugin);
    connect(plugins, &AppPlugins::loadedControl, f_apx, &AppRoot::addControlPlugin);
    connect(this, &QCoreApplication::aboutToQuit, plugins, &AppPlugins::unload);

    jsync(f_apx);

    loadServices();
    updateSurfaceFormat();

    plugins->load(oPlugins);

    apxConsole() << QObject::tr("Loading finished");
    emit loadingFinished();
}

void App::loadServices()
{
    apxConsole() << QObject::tr("Loading services").append("...");

    SvgImageProvider *svgProvider = new SvgImageProvider(":/");
    m_engine->addImageProvider("svg", svgProvider);
    m_engine->rootContext()->setContextProperty("svgRenderer", svgProvider);
}

void App::updateSurfaceFormat()
{
    if (!m_window)
        return;
    if (!AppSettings::instance())
        return;

    Fact *f_opengl = AppSettings::instance()->findChild("graphics.opengl");
    Fact *f_effects = AppSettings::instance()->findChild("graphics.effects");

    QSurfaceFormat fmt = m_window->format();
    //fmt.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    //m_window->setFormat(fmt);
    //return;

    if (f_opengl) {
        connect(f_opengl, &Fact::valueChanged, this, &App::updateSurfaceFormat, Qt::UniqueConnection);

        int v = f_opengl->value().toInt();
        QGLFormat::OpenGLVersionFlags gl = QGLFormat::openGLVersionFlags();
        switch (v) {
        default:
            fmt.setRenderableType(QSurfaceFormat::DefaultRenderableType);
            break;
        case 1:
            if (!gl) {
                apxMsgW() << tr("Render type not supported") << "OpenGL";
                break;
            }
            fmt.setRenderableType(QSurfaceFormat::OpenGL);
            break;
        case 2:
            if (!(gl & QGLFormat::OpenGL_ES_Version_2_0)) {
                apxMsgW() << tr("Render type not supported") << "OpenGLES";
                break;
            }
            fmt.setRenderableType(QSurfaceFormat::OpenGLES);
            break;
        case 3:
            //if(!(gl&QGLFormat::OpenGL_ES_Version_2_0))break;
            //fmt.setRenderableType(QSurfaceFormat::OpenVG);
            break;
        }
    }

    /*
    // map qml lines antialiasing
    f = AppSettings::instance()->findChild("graphics.antialiasing");
    if (f) {
        int v = f->value().toInt();
        fmt.setSamples(v == 2 ? 4 : 0);
    }*/

    if (f_effects) {
        connect(f_effects,
                &Fact::valueChanged,
                this,
                &App::updateSurfaceFormat,
                Qt::UniqueConnection);

        int v = f_effects->value().toInt();
        if (v < 2) {
            fmt.setStencilBufferSize(8);
            fmt.setDepthBufferSize(8);
        }
    }

    //qDebug()<<fmt;
    m_window->setFormat(fmt);
}

AppPlugin *App::plugin(QString name)
{
    return _instance->plugins->plugin(name);
}

//=============================================================================
void App::appStateChanged(Qt::ApplicationState state)
{
    if (m_window && state == Qt::ApplicationActive) {
        if (m_window->visibility() != QWindow::FullScreen)
            m_window->raise();
    }
}
//=============================================================================
//=============================================================================
void App::registerUiComponent(QObject *item, QString name)
{
    if (!m_engine)
        return;
    //qDebug()<<item<<name;
    QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
    QJSValue obj = m_engine->newQObject(item);
    setGlobalProperty(QString("ui.%1").arg(name), obj);
    emit uiComponentLoaded(name, obj);
}
//=============================================================================
QJSValue App::jsexec(const QString &s)
{
    if (!_instance->engine())
        return QJSValue();
    return _instance->engine()->jsexec(s);
}
void App::jsync(Fact *fact)
{
    if (!_instance->engine())
        return;
    _instance->engine()->jsSync(fact);
}
QObject *App::loadQml(const QString &qmlFile, const QVariantMap &opts)
{
    if (!_instance->engine())
        return nullptr;
    return _instance->engine()->loadQml(qmlFile, opts);
}
void App::setGlobalProperty(const QString &path, const QVariant &value)
{
    AppEngine *e = _instance->engine();
    if (!e)
        return;
    setGlobalProperty(path, e->toScriptValue(value));
}
void App::setGlobalProperty(const QString &path, const QJSValue &value)
{
    AppEngine *e = _instance->engine();
    if (!e)
        return;

    QJSValue v = e->globalObject();

    QStringList list = path.split('.');
    for (int i = 0; i < list.size(); ++i) {
        const QString pname = list.at(i);
        if (i == (list.size() - 1)) {
            v.setProperty(pname, value);
            break;
        }
        QJSValue vp = v.property(pname);
        if (vp.isUndefined() || (!vp.isObject())) {
            vp = e->newObject();
            v.setProperty(pname, vp);
        }
        v = vp;
    }
    //e->collectGarbage();
    if (value.isQObject() || value.isObject()) {
        e->jsProtectPropertyWrite(path);
    }
}
void App::setGlobalProperty(const QString &path, QObject *object)
{
    AppEngine *e = _instance->engine();
    if (!e)
        return;
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    setGlobalProperty(path, e->newQObject(object));
}
void App::setContextProperty(const QString name, const QVariant &value)
{
    AppEngine *e = _instance->engine();
    if (!e)
        return;
    e->rootContext()->setContextProperty(name, value);
}
void App::setContextProperty(const QString name, QObject *object)
{
    AppEngine *e = _instance->engine();
    if (!e)
        return;
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
    e->rootContext()->setContextProperty(name, object);
}
//=============================================================================
void App::loadFonts()
{
    apxConsole() << QObject::tr("Loading fonts").append("...");
    QFile res;
    res.setFileName(":/fonts/ApxNarrow.ttf");
    if (res.open(QIODevice::ReadOnly)) {
        QFontDatabase::addApplicationFontFromData(res.readAll());
        res.close();
    }
    res.setFileName(":/fonts/ApxCondenced.ttf");
    if (res.open(QIODevice::ReadOnly)) {
        QFontDatabase::addApplicationFontFromData(res.readAll());
        res.close();
    }
    res.setFileName(":/fonts/FreeMono.ttf");
    if (res.open(QIODevice::ReadOnly)) {
        QFontDatabase::addApplicationFontFromData(res.readAll());
        res.close();
    }
    res.setFileName(":/fonts/FreeMonoBold.ttf");
    if (res.open(QIODevice::ReadOnly)) {
        QFontDatabase::addApplicationFontFromData(res.readAll());
        res.close();
    }
    res.setFileName(":/fonts/Ubuntu-C.ttf");
    if (res.open(QIODevice::ReadOnly)) {
        QFontDatabase::addApplicationFontFromData(res.readAll());
        res.close();
    }
    res.setFileName(":/fonts/Bierahinia.ttf");
    if (res.open(QIODevice::ReadOnly)) {
        QFontDatabase::addApplicationFontFromData(res.readAll());
        res.close();
    }

    m_engine->rootContext()->setContextProperty("font_narrow", "ApxNarrow");
    m_engine->rootContext()->setContextProperty("font_narrow_regular", "ApxCondenced");
    m_engine->rootContext()->setContextProperty("font_mono", "FreeMono");
    m_engine->rootContext()->setContextProperty("font_condenced", "Ubuntu Condensed");
    //m_engine->rootContext()->setContextProperty("font_condenced", "ApxCondenced");
#ifdef Q_OS_MAC
    m_engine->rootContext()->setContextProperty("font_fixed", "Menlo");
#else
    m_engine->rootContext()->setContextProperty("font_fixed", "FreeMono");
    qApp->setFont(QFont("Arial"));
#endif
}
bool App::isFixedPitch(const QFont &font)
{
    const QFontInfo fi(font);
    //qDebug() << fi.family() << fi.fixedPitch();
    return fi.fixedPitch();
}
QFont App::getMonospaceFont()
{
    QFont font("FreeMono");
    if (isFixedPitch(font))
        return font;
    font.setFamily("Menlo");
    if (isFixedPitch(font))
        return font;
    font.setStyleHint(QFont::Monospace);
    if (isFixedPitch(font))
        return font;
    font.setStyleHint(QFont::TypeWriter);
    if (isFixedPitch(font))
        return font;
    font.setFamily("monospace");
    if (isFixedPitch(font))
        return font;
    font.setFamily("courier");
    if (isFixedPitch(font))
        return font;
    return font;
}
//=============================================================================
QString App::materialIconChar(const QString &name)
{
    return MaterialIcon::getChar(name);
}
//=============================================================================
//=============================================================================
void App::loadTranslations()
{
    apxConsole() << QObject::tr("Loading translations").append("...");

    QDir trDir(AppDirs::res().absoluteFilePath("translations"));
    for (auto fi : trDir.entryInfoList(QStringList() << "*.qm", QDir::Files)) {
        m_languages.append(fi.baseName());
    }
    QDir trDirUser(AppDirs::user().absoluteFilePath("Translations"));
    for (auto fi : trDirUser.entryInfoList(QStringList() << "*.qm", QDir::Files)) {
        m_languages.append(fi.baseName());
    }
    m_languages.removeDuplicates();
    m_languages.sort();

    QFileInfo fi = QFileInfo(trDir.absoluteFilePath(lang() + ".qm"));
    if (fi.exists()) {
        loadTranslator(fi.absoluteFilePath());
    }
    fi = QFileInfo(trDirUser.absoluteFilePath(lang() + ".qm"));
    if (fi.exists()) {
        loadTranslator(fi.absoluteFilePath());
    }

    /*QDir langsp("/usr/share/qt5/translations/");
    QString qt_langf = langsp.filePath("qt_" + lang + ".qm");
    if (QFile::exists(qt_langf)) {
        QTranslator *translator = new QTranslator();
        translator->load(qt_langf);
        qApp->installTranslator(translator);
        apxConsole() << QObject::tr("Translator added").append(": ").append(qt_langf);
    }*/
}
void App::loadTranslator(const QString &fileName)
{
    QTranslator *translator = new QTranslator();
    translator->load(fileName);
    installTranslator(translator);
    apxConsole() << "Translator added:" << QFileInfo(fileName).fileName();
}
//=============================================================================
//=============================================================================
QQuickWindow *App::window() const
{
    return m_window;
}
AppEngine *App::engine() const
{
    return m_engine;
}
double App::scale() const
{
    return m_scale;
}
void App::setScale(double v)
{
    if (m_scale == v)
        return;
    m_scale = v;
    emit scaleChanged();
}
AppLog *App::appLog() const
{
    return m_appLog;
}
AppNotify *App::appNotify() const
{
    return m_appNotify;
}
AppNotifyListModel *App::notifyModel() const
{
    return m_notifyModel;
}
AppPrefs *App::prefs() const
{
    return m_prefs;
}
QString App::lang() const
{
    return m_lang;
}
//=============================================================================
