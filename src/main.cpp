// SPDX-License-Identifier: BSL-1.0

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QSettings>

#include <PosixSignalManager.h>

#include <Tui/ZSimpleFileLogger.h>
#include <Tui/ZSimpleStringLogger.h>
#include <Tui/ZTerminal.h>

#include "edit.h"
#include "filecategorize.h"
#include "filelistparser.h"
#include "git_version.h"

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("chr");
    const QString version = "%VERSION% "+ QString(GIT_VERSION_ID);
    QCoreApplication::setApplicationVersion(version);

    QCommandLineParser parser;
    parser.setApplicationDescription("chr.edit");
    parser.addHelpOption();
    parser.addVersionOption();

    // line number
    QCommandLineOption lineNumberOption({"l", "linenumber"},
                    QCoreApplication::translate("main", "The line numbers are displayed"));
    parser.addOption(lineNumberOption);

    // append mode
    QCommandLineOption append({"a", "append"},
                    QCoreApplication::translate("main", "Only with read from standard input, then append to a file"),
                    QCoreApplication::translate("main", "file"));
    parser.addOption(append);

    // follow mode
    QCommandLineOption follow({"f", "follow"},
                    QCoreApplication::translate("main", "Only with read from standard input, then follow the input stream"));
    parser.addOption(follow);

    // big file
    QCommandLineOption bigOption("b",
                    QCoreApplication::translate("main", "Open bigger file then 1MB"));
    parser.addOption(bigOption);

    // wrap log lines
    QCommandLineOption wraplines("w",
                    QCoreApplication::translate("main", "Wrap log lines"));
    parser.addOption(wraplines);

    // safe attributes
    QCommandLineOption attributesfileOption("attributesfile",
                    QCoreApplication::translate("main", "Safe file for attributes, default ~/.cache/chr.json"),
                    QCoreApplication::translate("main", "config"));
    parser.addOption(attributesfileOption);

    // config file
    QCommandLineOption configOption({"c", "config"},
                    QCoreApplication::translate("main", "Load customized config file. The default if it exist is ~/.config/chr"),
                    QCoreApplication::translate("main", "config"));
    parser.addOption(configOption);

    // goto line
    parser.addPositionalArgument("[[+line[,char]] file …]",
                    QCoreApplication::translate("main", "Optional is the line number, several files can be opened in multiple windows."));

    // open directory
    parser.addPositionalArgument("[/directory]",
                    QCoreApplication::translate("main", "Or a directory can be specified to search in the open dialog."));
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
    if (parser.isSet(configOption)) {
        configDir = parser.value(configOption);
    } else {
        //default config
        configDir = QDir::homePath() +"/.config/chr";
    }

    QString attributesfile = "";
    if (parser.isSet(attributesfileOption)) {
        attributesfile = parser.value(attributesfileOption);
    }


    // default settings
    QSettings * qsettings = new QSettings(configDir, QSettings::IniFormat);
    int tabsize = qsettings->value("tabsize", "4").toInt();
    Settings settings;
    settings.tabSize = tabsize;

    bool tab = qsettings->value("tab","true").toBool();
    settings.tabOption = tab;

    bool eatSpaceBeforeTabs = qsettings->value("eatSpaceBeforeTabs", "true").toBool();
    settings.eatSpaceBeforeTabs = eatSpaceBeforeTabs;

    bool ln = qsettings->value("line_number", "0").toBool() || qsettings->value("linenumber", "0").toBool();
    settings.showLineNumber = ln || parser.isSet(lineNumberOption);

    settings.formattingCharacters = qsettings->value("formatting_characters", "0").toBool();
    settings.colorTabs = qsettings->value("color_tabs", "0").toBool();
    settings.colorSpaceEnd = qsettings->value("color_space_end", "0").toBool();

    bool bigfile = qsettings->value("bigfile", "false").toBool();

    QString attributesfileDefault = qsettings->value("attributesfile", QDir::homePath() + "/.cache/chr.json").toString();
    if (attributesfile.isEmpty()) {
        attributesfile = attributesfileDefault;
    }
    settings.attributesFile = attributesfile;

    bool wl = qsettings->value("wrap_lines", "false").toBool();
    if (wl || parser.isSet(wraplines)) {
        settings.wrap = Tui::ZTextOption::WordWrap;
    } else {
        settings.wrap = Tui::ZTextOption::NoWrap;
    }

    settings.rightMarginHint = qsettings->value("right_margin_hint", "0").toInt();

    bool hb = qsettings->value("highlight_bracket", "false").toBool();
    settings.highlightBracket = hb;

    root->setInitialFileSettings(settings);

    QString theme = qsettings->value("theme", "classic").toString();
    if (theme.toLower() == "dark" || theme.toLower() == "black") {
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
    if (args.empty()) {
        root->newFile("");
    } else {
        QVector<FileListEntry> fles = parseFileList(args);
        if (fles.empty()) {
            out << "Got file offset without file name.\n";
            return 0;
        }
        for (FileListEntry fle: fles) {
            FileCategory filecategory = fileCategorize(fle.fileName);
            if (filecategory == FileCategory::stdin_file) {
                if (parser.isSet(append) &&
                        (fileCategorize(parser.value(append)) == FileCategory::new_file ||
                         fileCategorize(parser.value(append)) == FileCategory::open_file )
                        ) {
                    root->openFile(parser.value(append));
                } else {
                    root->newFile("");
                }
                root->watchPipe();

            } else if (filecategory == FileCategory::dir) {
                QFileInfo datei(fle.fileName);
                QString tmp = datei.absoluteFilePath();
                QTimer *t = new QTimer();
                QObject::connect(t, &QTimer::timeout, t, [t, root, tmp] {
                    root->openFileDialog(tmp);
                    t->deleteLater();
                });
                t->setSingleShot(true);
                t->start(0);

            } else if (filecategory == FileCategory::new_file) {
                QFileInfo datei(fle.fileName);
                root->newFile(datei.absoluteFilePath());

            } else if (filecategory == FileCategory::open_file) {
                QFileInfo datei(fle.fileName);
                int maxMB = 100;
                if (datei.size()/1024/1024 >= maxMB && !parser.isSet(bigOption) && !bigfile) {
                    out << "The file is bigger then " << maxMB << "MB (" << datei.size()/1024/1024 << "MB). Please start with -b for big files.\n";
                    return 0;
                }
                FileWindow* win = root->openFile(datei.absoluteFilePath());
                if (fle.pos != "") {
                    win->getFileWidget()->gotoline(fle.pos);
                }

            } else if (filecategory == FileCategory::invalid_filetype) {
                out << "File type cannot be opened.\n";
                return 1;
            } else if (filecategory == FileCategory::invalid_dir_not_exist) {
                out << "Directory not exist, cannot be opened.\n";
                return 1;
            } else if (filecategory == FileCategory::invalid_dir_not_writable) {
                out << "Directory not writable, cannot be opened.\n";
                return 1;
            } else {
                out << "Invalid error.\n";
                return 255;
            }
        }
    }

    if (parser.isSet(follow)) {
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
    if (Tui::ZSimpleStringLogger::getMessages().size() > 0) {
        terminal.pauseOperation();
        printf("\e[90m%s\e[0m", Tui::ZSimpleStringLogger::getMessages().toUtf8().data());
    }
    return 0;
}
