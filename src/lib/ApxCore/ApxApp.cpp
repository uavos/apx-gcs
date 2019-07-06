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
#include "ApxApp.h"
#include <version.h>
#include <App/AppWindow.h>
#include <ApxMisc/SvgMaterialIcon.h>
#include <ApxMisc/SvgImageProvider.h>
#include <ApxDirs.h>

#include <QFontDatabase>
#include <QFontInfo>
#include <QApplication>
#include <QGLFormat>
#include <QQuickStyle>
#include <QStyleFactory>
#include <QScreen>
//=============================================================================
ApxApp *ApxApp::_instance = nullptr;
ApxApp::ApxApp(int &argc, char **argv, const QString &name, const QUrl &url)
    : AppBase(argc, argv, name)
    , url(url)
    , m_window(nullptr)
    , m_scale(1.0)
{
    _instance = this;

    qRegisterMetaType<QScreen *>("QScreen");

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
    oPlugins = parser.value(pluginsOption).split(',', QString::SkipEmptyParts);
    oQml = parser.value(qmlMainFile);
    //---------------------------------------

    appInstances = new AppInstances(this);

    //styles
    QQuickStyle::setStyle("Material");
    QFile styleSheet(":styles/style-old.css");
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

    connect(this, &QCoreApplication::aboutToQuit, this, &ApxApp::quitRequested);
    connect(this, &QGuiApplication::applicationStateChanged, this, &ApxApp::appStateChanged);

    //js engine
    m_engine = new AppEngine(this);
    if (!oQml.isEmpty()) {
        m_engine->globalObject().setProperty("qmlMainFile", oQml);
    } else {
        //check for overrided layout file
        QString fname = ApxDirs::userPlugins().absoluteFilePath(
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

    f_app = new AppRoot(this);

    connect(this, &QCoreApplication::aboutToQuit, f_app->f_settings, &AppSettings::setReadOnly);

    //plugins
    plugins = new AppPlugins(f_app->f_pluginsSettings, this);
    connect(plugins, &AppPlugins::loadedTool, f_app, &AppRoot::addToolPlugin);
    connect(plugins, &AppPlugins::loadedWindow, f_app, &AppRoot::addWindowPlugin);
    connect(plugins, &AppPlugins::loadedControl, f_app, &AppRoot::addControlPlugin);
    connect(this, &QCoreApplication::aboutToQuit, plugins, &AppPlugins::unload);

    jsync(f_app);

    m_engine->jsSyncObject(this);

    connect(m_engine, &QQmlApplicationEngine::quit, m_engine, &QQmlApplicationEngine::deleteLater);
    connect(m_engine, &QQmlApplicationEngine::destroyed, this, &QGuiApplication::quit);

    QTimer::singleShot(1, this, &ApxApp::loadUrl);
}
ApxApp::~ApxApp() {}
//=============================================================================
void ApxApp::loadApp()
{
    apxConsole() << QObject::tr("Loading application").append("...");
    loadTranslations();
    loadFonts();
    loadServices();
    plugins->load(oPlugins);
    apxConsole() << QObject::tr("Loading finished");
    emit loadingFinished();
}
//=============================================================================
void ApxApp::quitRequested()
{
    apxMsg() << tr("Quit").append("...");
    //jsexec("ui.map.destroy()");
}
//=============================================================================
//=============================================================================
void ApxApp::loadServices()
{
    apxConsole() << QObject::tr("Loading services").append("...");

    SvgMaterialIcon::initialize(m_engine);
    SvgImageProvider *svgProvider = new SvgImageProvider(":/");
    m_engine->addImageProvider("svg", svgProvider);
    m_engine->rootContext()->setContextProperty("svgRenderer", svgProvider);

    //SoundEffects *soundEffects=new SoundEffects(this);
    //QObject::connect(this,&ApxApp::playSoundEffect,soundEffects,&SoundEffects::play);
}
//=============================================================================
void ApxApp::loadUrl()
{
    load(url);
}
void ApxApp::load(const QUrl &qml)
{
    if (qml.isEmpty())
        return;
    m_engine->load(qml);
    QQuickWindow *w = m_engine->rootObjects().size()
                          ? qobject_cast<QQuickWindow *>(m_engine->rootObjects().first())
                          : nullptr;
    if (!w)
        return;

    //update property
    if (m_window != w) {
        connect(w, &QQuickWindow::visibilityChanged, this, &ApxApp::visibilityChanged);
        m_window = w;
        emit windowChanged();
    }

    //surface format
    Fact *f;
    f = AppSettings::instance()->findChild("graphics.opengl");
    if (f)
        connect(f, &Fact::valueChanged, this, &ApxApp::updateSurfaceFormat);
    f = AppSettings::instance()->findChild("graphics.antialiasing");
    if (f)
        connect(f, &Fact::valueChanged, this, &ApxApp::updateSurfaceFormat);
    updateSurfaceFormat();

    //update global property
    registerUiComponent(w, "window"); //setGlobalProperty("ui.window",m_engine->newQObject(w));
}
//=============================================================================
void ApxApp::updateSurfaceFormat()
{
    if (!m_window)
        return;

    QSurfaceFormat fmt = m_window->format();
    //fmt.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    //m_window->setFormat(fmt);
    //return;

    Fact *f;

    f = AppSettings::instance()->findChild("graphics.opengl");
    if (f) {
        int v = f->value().toInt();
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

    f = AppSettings::instance()->findChild("graphics.effects");
    if (f) {
        int v = f->value().toInt();
        if (v < 2) {
            fmt.setStencilBufferSize(8);
            fmt.setDepthBufferSize(8);
        }
    }

    //qDebug()<<fmt;
    m_window->setFormat(fmt);
}
//=============================================================================
void ApxApp::appStateChanged(Qt::ApplicationState state)
{
    if (m_window && state == Qt::ApplicationActive) {
        if (m_window->visibility() != QWindow::FullScreen)
            m_window->raise();
    }
}
//=============================================================================
void ApxApp::setGlobalProperty(const QString &path, const QJSValue &value)
{
    QJSValue v = m_engine->globalObject();

    QStringList list = path.split('.');
    for (int i = 0; i < list.size(); ++i) {
        const QString pname = list.at(i);
        if (i == (list.size() - 1)) {
            v.setProperty(pname, value);
            break;
        }
        QJSValue vp = v.property(pname);
        if (vp.isUndefined() || (!vp.isObject())) {
            vp = m_engine->newObject();
            v.setProperty(pname, vp);
        }
        v = vp;
    }
    //m_engine->collectGarbage();
}
//=============================================================================
void ApxApp::registerUiComponent(QObject *item, QString name)
{
    //qDebug()<<item<<name;
    setGlobalProperty(QString("ui.%1").arg(name), m_engine->newQObject(item));
    emit uiComponentLoaded(name);
}
//=============================================================================
//=============================================================================
void ApxApp::loadFonts()
{
    apxConsole() << QObject::tr("Loading fonts").append("...");
    QFile res;
    res.setFileName(":/fonts/BebasNeue.otf");
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

    m_engine->rootContext()->setContextProperty("font_narrow", "Bebas Neue");
    m_engine->rootContext()->setContextProperty("font_mono", "FreeMono");
    m_engine->rootContext()->setContextProperty("font_condenced", "Ubuntu Condensed");
#ifdef Q_OS_MAC
    m_engine->rootContext()->setContextProperty("font_fixed", "Menlo");
#else
    m_engine->rootContext()->setContextProperty("font_fixed", "FreeMono");
#endif
}
bool ApxApp::isFixedPitch(const QFont &font)
{
    const QFontInfo fi(font);
    //qDebug() << fi.family() << fi.fixedPitch();
    return fi.fixedPitch();
}
QFont ApxApp::getMonospaceFont()
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
//=============================================================================
void ApxApp::loadTranslations()
{
    apxConsole() << QObject::tr("Loading translations").append("...");
    QSettings *s = AppSettings::createSettings();
    s->beginGroup("apx.settings");
    QString lang = s->value("lang").toString();
    if (QLocale().country() == QLocale::Belarus && (lang == "ru")) {
        lang = "by";
        s->setValue("lang", lang);
    }
    delete s;
    QDir langp(ApxDirs::lang());
    QString langf;
    langf = langp.filePath(lang + ".qm");
    if (QFile::exists(langf)) {
        QTranslator *translator = new QTranslator();
        translator->load(langf);
        qApp->installTranslator(translator);
        apxConsole() << QObject::tr("Translator added").append(": ").append(langf);
    }
    QDir langsp("/usr/share/qt5/translations/");
    QString qt_langf = langsp.filePath("qt_" + lang + ".qm");
    if (QFile::exists(qt_langf)) {
        QTranslator *translator = new QTranslator();
        translator->load(qt_langf);
        qApp->installTranslator(translator);
        apxConsole() << QObject::tr("Translator added").append(": ").append(qt_langf);
    }
}
//=============================================================================
//=============================================================================
QQuickWindow *ApxApp::window() const
{
    return m_window;
}
AppEngine *ApxApp::engine() const
{
    return m_engine;
}
double ApxApp::scale() const
{
    return m_scale;
}
void ApxApp::setScale(double v)
{
    if (m_scale == v)
        return;
    m_scale = v;
    emit scaleChanged();
}
//=============================================================================
