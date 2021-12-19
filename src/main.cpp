#include "edit.h"
#include "filelistparser.h"

#include <QLoggingCategory>
#include <Tui/ZSimpleFileLogger.h>
#include <Tui/ZSimpleStringLogger.h>
#include <PosixSignalManager.h>

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("chr");
    QCoreApplication::setApplicationVersion("%VERSION%");

    QCommandLineParser parser;
    parser.setApplicationDescription("chr.edit");
    parser.addHelpOption();
    parser.addVersionOption();

    // Line Number

    QCommandLineOption lineNumberOption({"l","linenumber"},
                                    QCoreApplication::translate("main", "The line numbers are displayed"));
    parser.addOption(lineNumberOption);

    QCommandLineOption append({"a","append"},
                              QCoreApplication::translate("main", "Only with read from standard input, then append to a file"),
                              QCoreApplication::translate("main", "file"));
    parser.addOption(append);

    QCommandLineOption follow({"f","follow"},QCoreApplication::translate("main", "Only with read from standard input, then follow the input stream"));
    parser.addOption(follow);

    // Big Files
    QCommandLineOption bigOption("b",QCoreApplication::translate("main", "Open bigger files then 1MB"));
    parser.addOption(bigOption);

    // Wrap Lines
    QCommandLineOption wraplines("w",QCoreApplication::translate("main", "Wrap log lines"));
    parser.addOption(wraplines);

    //Safe Attributes
    QCommandLineOption attributesfileOption("attributesfile",
                                            QCoreApplication::translate("main", "Safe file for attributes, default ~/.cache/chr.json"),
                                            QCoreApplication::translate("main", "config file"));
    parser.addOption(attributesfileOption);

    // Config File
    QCommandLineOption configOption({"c","config"},
                                    QCoreApplication::translate("main", "Load customized config file. The default if it exist is ~/.config/chr"),
                                    QCoreApplication::translate("main", "config"));
    parser.addOption(configOption);

    parser.addPositionalArgument("[[+line[,char]] file …]", QCoreApplication::translate("main", "Optional is the line number, several files can be opened in multiple windows."));
    parser.addPositionalArgument("[/directory]", QCoreApplication::translate("main", "Or a directory can be specified to search in the open dialog."));
    parser.process(app);

    parser.parse(QCoreApplication::arguments());
    const QStringList args = parser.positionalArguments();
    QTextStream out(stdout);

    // START EDITOR
    Tui::ZTerminal terminal;
    Editor *root = new Editor();
    terminal.setMainWidget(root);

    // READ CONFIG FILE AND SET DEFAULT OPTIONS
    QString configDir = "";
    if(parser.isSet(configOption)) {
        configDir = parser.value(configOption);
    } else {
        //default
        configDir = QDir::homePath() +"/.config/chr";
    }

    QString attributesfile = "";
    if(parser.isSet(attributesfileOption)) {
        attributesfile = parser.value(attributesfileOption);
    }


    // Default settings
    QSettings * qsettings = new QSettings(configDir, QSettings::IniFormat);
    int tabsize = qsettings->value("tabsize","4").toInt();
    Settings settings;
    settings.tabSize = tabsize;

    bool tab = qsettings->value("tab","true").toBool();
    settings.tabOption = tab;

    bool eatSpaceBeforeTabs = qsettings->value("eatSpaceBeforeTabs","true").toBool();
    settings.eatSpaceBeforeTabs = eatSpaceBeforeTabs;

    bool ln = qsettings->value("line_number","0").toBool() || qsettings->value("linenumber","0").toBool();
    settings.showLineNumber = ln || parser.isSet(lineNumberOption);

    settings.formattingCharacters = qsettings->value("formatting_characters","0").toBool();
    settings.colorTabs = qsettings->value("color_tabs","0").toBool();
    settings.colorSpaceEnd = qsettings->value("color_space_end","0").toBool();

    bool bigfile = qsettings->value("bigfile", "false").toBool();

    QString attributesfileDefault = qsettings->value("attributesfile", QDir::homePath() + "/.cache/chr.json").toString();
    if(attributesfile.isEmpty()) {
        attributesfile = attributesfileDefault;
    }
    settings.attributesFile = attributesfile;

    bool wl = qsettings->value("wrap_lines","false").toBool();
    if (wl || parser.isSet(wraplines)) {
        settings.wrap = ZTextOption::WordWrap;
    } else {
        settings.wrap = ZTextOption::NoWrap;
    }

    bool hb = qsettings->value("highlight_bracket","false").toBool();
    settings.highlightBracket = hb;

    bool scpx0 = qsettings->value("select_cursor_position_x0","true").toBool();
    settings.select_cursor_position_x0 = scpx0;

    root->setInitialFileSettings(settings);

    QString theme = qsettings->value("theme","classic").toString();
    if(theme.toLower() == "dark" || theme.toLower() == "black") {
        root->setTheme(Editor::Theme::dark);
    } else {
        root->setTheme(Editor::Theme::classic);
    }

    QString logfile = qsettings->value("logfile", "").toString();
    if (logfile.isEmpty()) {
        QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, false);
        Tui::ZSimpleStringLogger::install();
    } else {
        Tui::ZSimpleFileLogger::install(logfile);
    }
    qDebug("%i chr starting", (int)QCoreApplication::applicationPid());

    // OPEN FILE
    if(args.empty()) {
        root->newFile("");
    } else {
        QVector<FileListEntry> fles = parseFileList(args);
        if (fles.empty()) {
            out << "Got file offset without file name.\n";
            return 0;
        }
        for (FileListEntry fle: fles) {
            if (fle.fileName == "-") {
                if(parser.isSet(append)) {
                    root->openFile(parser.value(append));
                } else {
                    root->newFile("");
                }
                root->watchPipe();
            } else {
                QFileInfo datei(fle.fileName);
                if(datei.isDir()) {
                    QString tmp = datei.absoluteFilePath();
                    QTimer *t = new QTimer();
                    QObject::connect(t, &QTimer::timeout, t, [t, root, tmp] {
                        root->openFileDialog(tmp);
                        t->deleteLater();
                    });
                    t->setSingleShot(true);
                    t->start(0);
                } else if(datei.isFile()){
                    int maxMB = 100;
                    if(datei.size()/1024/1024 >= maxMB && !parser.isSet(bigOption) && !bigfile) {
                        out << "The file is bigger then " << maxMB << "MB (" << datei.size()/1024/1024 << "MB). Please start with -b for big files.\n";
                        return 0;
                    }
                    root->openFile(datei.absoluteFilePath());
                    if (fle.pos != "") {
                        root->gotoLineInCurrentFile(fle.pos);
                    }
                } else {
                    root->newFile(datei.absoluteFilePath());
                }
            }
        }
    }

    if(parser.isSet(follow)) {
        root->followInCurrentFile();
    }

    QObject::connect(&terminal, &Tui::ZTerminal::terminalConnectionLost, [=] {
        //TODO file save
        //qDebug("%i terminalConnectionLost", (int)QCoreApplication::applicationPid());
        QCoreApplication::quit();
    });

    std::unique_ptr<PosixSignalNotifier> sigIntNotifier;
    sigIntNotifier = std::unique_ptr<PosixSignalNotifier>(new PosixSignalNotifier(SIGINT));
    QObject::connect(sigIntNotifier.get(), &PosixSignalNotifier::activated, [] {
        //qDebug("%i SIGINT", (int)QCoreApplication::applicationPid());
        QCoreApplication::quit();
    });

    app.exec();
    if(Tui::ZSimpleStringLogger::getMessages().size() > 0) {
        terminal.pauseOperation();
        printf("\e[90m%s\e[0m", Tui::ZSimpleStringLogger::getMessages().toUtf8().data());
    }
    return 0;
}
