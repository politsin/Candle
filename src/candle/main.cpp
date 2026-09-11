// This file is a part of "Candle" application.
// Copyright 2015-2021 Hayrullin Denis Ravilevich

#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QMessageBox>
#include <QDir>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "parser/gcodepreprocessorutils.h"
#include "parser/gcodeparser.h"
#include "parser/gcodeviewparse.h"
#include "logging/fileloghandler.h"
#include "versionconfig.h"

#include "frmmain.h"

void loadTranslationsForLocale(const QString &locale, QCoreApplication &app)
{
    auto translationsFolder = qApp->applicationDirPath() + "/translations/";
    QDir dir(translationsFolder);

    if (!dir.exists())
        return;

    for (const QString &fileName : dir.entryList(QStringList{ "*_" + locale + ".qm" }, QDir::Files))
    {
        auto tr = new QTranslator(&app);

        if (tr->load(dir.absoluteFilePath(fileName)))
            app.installTranslator(tr);
        else
            delete tr;
    }
}

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#ifdef Q_OS_LINUX
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
#endif
    QApplication a(argc, argv);

    QCommandLineParser commandLine;
    commandLine.setApplicationDescription("Candle CNC controller");
    commandLine.addHelpOption();
    commandLine.addVersionOption();
    QCommandLineOption headlessOption("headless", "Run without showing the Candle window; control it through the local automation API.");
    QCommandLineOption apiPortOption("automation-port", "Bind automation API to this loopback TCP port (otherwise select the first free port from 8090).", "port");
    QCommandLineOption noConnectOption("no-connect", "Do not connect to the controller at startup; use POST /api/v1/connect when ready.");
    QCommandLineOption connectOption("connect", "Connect using the saved profile at startup (overrides the console safety default).");
    commandLine.addOption(headlessOption);
    commandLine.addOption(apiPortOption);
    commandLine.addOption(noConnectOption);
    commandLine.addOption(connectOption);
    commandLine.process(a);

#ifdef CANDLE_CONSOLE
    const bool headless = true;
    const bool noAutoConnect = !commandLine.isSet(connectOption);
#else
    const bool headless = commandLine.isSet(headlessOption);
    const bool noAutoConnect = commandLine.isSet(noConnectOption);
#endif
    if (commandLine.isSet(noConnectOption) && commandLine.isSet(connectOption)) {
        qCritical() << "--no-connect and --connect cannot be used together";
        return 2;
    }
    a.setProperty("automationNoAutoConnect", noAutoConnect);
    if (commandLine.isSet(apiPortOption)) {
        bool isPort = false;
        const int port = commandLine.value(apiPortOption).toInt(&isPort);
        if (!isPort || port < 1 || port > 65535) {
            qCritical() << "--automation-port must be a valid TCP port";
            return 2;
        }
        a.setProperty("automationPort", port);
    }

    a.setOrganizationName(APP_NAME);
    a.setApplicationName(APP_NAME);
    a.setApplicationDisplayName(APP_NAME);
    a.setApplicationVersion(APP_VERSION);

    installFileLogHandler();

    QSettings set;
    QString locale = set.value("General/language", "en").toString();

    loadTranslationsForLocale(locale, a);

    a.setStyleSheet(a.styleSheet() + "QWidget {font-size: 8pt}");

    frmMain w;

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    QFile styles(":/styles/frmmaindefault.qss");
#elif defined(Q_OS_MAC)
    QFile styles(":/styles/frmmainmacos.qss");
#else
    QFile styles(":/styles/frmmaindefault.qss");
#endif

    if (styles.open(QFile::ReadOnly))
        w.setStyleSheet(styles.readAll());

    if (!headless)
        w.show();

    return a.exec();
}
