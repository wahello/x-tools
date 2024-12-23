/***************************************************************************************************
 * Copyright 2024 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded in "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source code
 * directory.
 **************************************************************************************************/
#pragma once

#include <QApplication>
#include <QDateTime>
#include <QMainWindow>
#include <QObject>
#include <QSplashScreen>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

#ifdef X_TOOLS_ENABLE_MODULE_STYLESHEET
#include "StyleSheetManager.h"
#endif

static void (*gOutputLog2Ui)(QtMsgType, const QMessageLogContext &, const QString &){nullptr};

namespace xTools {
class xToolsPrivate;
class xTools : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(xTools)
private:
    explicit xTools(QObject *parent = Q_NULLPTR);
    xTools(const xTools &) = delete;
    xTools &operator=(const xTools &) = delete;

public:
    static xTools &singleton();
    static void doSomethingBeforeAppCreated(char *argv[],
                                            const QString &appName,
                                            bool forStore = false);
    static void doSomethingAfterAppExited();

signals:
    void languageChanged();

public:
    // About splash screen
    Q_INVOKABLE bool enableSplashScreen();
    Q_INVOKABLE void setEnableSplashScreen(bool enable);
    Q_INVOKABLE void setSplashScreenMessage(const QString &msg);
    Q_INVOKABLE QSplashScreen *splashScreen();

    // About app info
    Q_INVOKABLE QString appFriendlyName();
    Q_INVOKABLE void setAppFriendlyName(const QString &name);
    Q_INVOKABLE QString appVersion();
    Q_INVOKABLE void appInitializing(const QString &appName, bool forStore);
    Q_INVOKABLE void appInitializingHdpi(const QString &appName, bool forStore);

    // About i18n
    Q_INVOKABLE QString defaultAppLanguage();
    Q_INVOKABLE QStringList supportedAppLanguages();
    Q_INVOKABLE QStringList supportedAppLanguagePrefixes();
    Q_INVOKABLE void setSupportedAppLanguagePrefixes(const QStringList &prefixes);
    Q_INVOKABLE void setupAppLanguageWithPrefix(const QString &language, const QString &prefix);
    Q_INVOKABLE void setupAppLanguage(const QString &language = QString());

    // About git info
    Q_INVOKABLE QString xToolsVersion();
    Q_INVOKABLE QString xToolsLastCommit();
    Q_INVOKABLE QString xToolsLastCommitTime();

    // About high dpi policy
    Q_INVOKABLE QVariantList supportedHdpiPolicies();
    Q_INVOKABLE QString hdpiPolicyName(int policy);
    Q_INVOKABLE bool isValidHdpiPolicy(int policy);

    // About formater
    Q_INVOKABLE QString stringToHexString(const QString &str);
    Q_INVOKABLE QString hexStringToString(const QString &str);
    Q_INVOKABLE QByteArray byteArray2Hex(const QByteArray &source, char separator = '\0');

    // About date time
    Q_INVOKABLE QString dateTimeString(const QString &format);
    Q_INVOKABLE QDateTime buildDateTime();
    Q_INVOKABLE QString buildDateTimeString(const QString &format);
    Q_INVOKABLE QString systemDateFormat();
    Q_INVOKABLE QString systemTimeFormat();

    // About system
    Q_INVOKABLE QString desktopPath();
    Q_INVOKABLE QString clipboardText();
    Q_INVOKABLE void setClipboardText(const QString &text);
    Q_INVOKABLE void openUrl(const QString &url);

    // About icon
    Q_INVOKABLE QIcon toThemeIcon(const QIcon &icon);
    Q_INVOKABLE QIcon cookedIconFile(const QString &iconFile, const QString &color);
    Q_INVOKABLE QIcon cookedIcon(const QIcon &icon, const QString &color);

    // About settings
    Q_INVOKABLE void settingsOpenSettingsFileDir();
    Q_INVOKABLE QString settingsPath();
    Q_INVOKABLE int settingsHdpiPolicy();
    Q_INVOKABLE void settingsSetHdpiPolicy(int policy);
    Q_INVOKABLE QString settingsAppStyle();
    Q_INVOKABLE void settingsSetAppStyle(const QString &style);
    Q_INVOKABLE QString settingsLanguage();
    Q_INVOKABLE void settingsSetLanguage(const QString &lan);
    Q_INVOKABLE bool settingsClearSettings();
    Q_INVOKABLE void settingsSetClearSettings(bool clear);
    Q_INVOKABLE int settingsColorScheme();
    Q_INVOKABLE void settingsSetColorScheme(const int colorScheme);
    Q_INVOKABLE QVariant settingsValue(const QString &key, const QVariant &value = QVariant()) const;
    Q_INVOKABLE void settingsSetValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void settingsSetJsonObjectStringValue(const QString &key, const QString &value);

    // About log
    Q_INVOKABLE static void googleLogInitializing(char *argv0);
    Q_INVOKABLE static void googleLogShutdown();
    Q_INVOKABLE static void googleLogToQtLog(QtMsgType type,
                                             const QMessageLogContext &context,
                                             const QString &msg);

    // Other functions
    Q_INVOKABLE QMainWindow *mainWindow();
    Q_INVOKABLE void moveToScreenCenter(QWidget *widget);
    Q_INVOKABLE static bool tryToReboot();
    Q_INVOKABLE void tryToClearSettings();
};

/**
 * \brief exec: Using this function to start the application with a main window, you can specified
 *  the central widget type of main window. If MainWindow is the same as CentralWidget, the
 *  main window will not be created and the instance of CentralWidget will be the main window.
 * \param argc: The number of command line arguments.
 * \param argv: The command line arguments.
 * \param appName: The name of the application.
 * \param version: The version of the application.
 * \param doSomethingBeforAppExec: The function will be called before the application exec. The
 * parameter of the function is the instance of the main window and the instance of the application.
 * \return
 */
template<typename CentralWidgetT, typename MainWindowT>
int exec(int argc,
         char *argv[],
         const QString &appName,
         const QString &appVersion = QString("0.0.0"),
         const std::function<void(void *, void *)> &doSomethingBeforAppExec = nullptr)
{
    QCoreApplication::setApplicationVersion(appVersion);
    xTools::doSomethingBeforeAppCreated(argv, appName);

    QApplication app(argc, argv);
    xTools &xTools = xTools::singleton();
    xTools.setupAppLanguage();

    QStyleHints *styleHints = QApplication::styleHints();
    styleHints->setColorScheme(Qt::ColorScheme::Dark);

    const QString dtStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    xTools.settingsSetValue("startUpTime", dtStr);

#ifdef X_TOOLS_ENABLE_MODULE_STYLESHEET
    auto &styleSheetManager = StyleSheetManager::singleton();
    const QString styleSheet = styleSheetManager.styleSheet();
    if (!styleSheet.isEmpty() && !styleSheetManager.currentTheme().isEmpty()) {
        app.setStyleSheet(styleSheet);
    }
#endif

    const QStringList keys = QStyleFactory::keys();
    qInfo() << "The supported application styles are:" << qPrintable(keys.join(QChar(',')));
    const QString style = xTools.settingsAppStyle();
    if (style.isEmpty()) {
        qWarning() << "The application style is not specified, the default style is:"
                   << qPrintable(QApplication::style()->objectName());
    } else if (keys.contains(style) || keys.contains(style.toLower())) {
        qInfo() << "The current style of application is:" << qPrintable(style);
        QApplication::setStyle(QStyleFactory::create(style));
    }

    QWidget *ui;
    if (std::is_same<CentralWidgetT, MainWindowT>::value) {
        auto widget = new CentralWidgetT();
        ui = widget;
    } else {
        auto mainWindow = new MainWindowT();
        auto centralWidget = new CentralWidgetT(mainWindow);
        mainWindow->setWindowTitle(appName);
        mainWindow->setCentralWidget(centralWidget);
        ui = mainWindow;
    }

    QSplashScreen *splashScreen = xTools.splashScreen();
    splashScreen->finish(ui);
    ui->show();
    ui->resize(static_cast<int>(static_cast<qreal>(ui->height()) * 1.732), ui->height());
    xTools.moveToScreenCenter(ui);
    qInfo() << "The size of window is" << ui->size();

    if (doSomethingBeforAppExec) {
        doSomethingBeforAppExec(ui, &app);
    }

    const int ret = app.exec();
    xTools::doSomethingAfterAppExited();
    return ret;
}

template<typename CentralWidget>
int execCentralWidget(int argc,
                      char *argv[],
                      const QString &appName,
                      const QString &appVersion = QString("0.0.0"),
                      const std::function<void(void *, void *)> &doSomethingBeforAppExec = nullptr)
{
    return exec<CentralWidget, xTools::MainWindow>(argc,
                                                   argv,
                                                   appName,
                                                   appVersion,
                                                   doSomethingBeforAppExec);
}

} // namespace xTools
