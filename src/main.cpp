#include "edit.h"
#include <QLoggingCategory>
#include <Tui/ZSimpleFileLogger.h>
#include <Tui/ZSimpleStringLogger.h>

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("chr");
    QCoreApplication::setApplicationVersion("%VERSION%");

    QCommandLineParser parser;
    parser.setApplicationDescription("chr.edit");
    parser.addHelpOption();
    parser.addVersionOption();

    // Line Number
    QCommandLineOption numberOption({"n","number"},
                                    QCoreApplication::translate("main", "The cursor will be positioned on line \"num\".  If \"num\" is missing, the cursor will be positioned on the last line."),
                                    QCoreApplication::translate("main", "num"));
    parser.addOption(numberOption);

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

    // Dokument
    parser.addPositionalArgument("file", QCoreApplication::translate("main", "The file to open."));
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
    root->file->setTabsize(tabsize);

    bool tab = qsettings->value("tab","true").toBool();
    root->file->setTabOption(tab);

    bool ln = qsettings->value("line_number","0").toBool();
    root->file->setLineNumber(ln || parser.isSet(lineNumberOption));

    bool fb = qsettings->value("formatting_characters","1").toBool();
    root->file->setFormattingCharacters(fb);

    bool bigfile = qsettings->value("bigfile", "false").toBool();

    QString attributesfileDefault = qsettings->value("attributesfile", QDir::homePath() + "/.cache/chr.json").toString();
    if(attributesfile.isEmpty()) {
        attributesfile = attributesfileDefault;
    }
    root->file->setAttributesfile(attributesfile);

    bool wl = qsettings->value("wrap_lines","false").toBool();
    root->setWrap(wl || parser.isSet(wraplines));

    bool hb = qsettings->value("highlight_bracket","false").toBool();
    root->file->setHighlightBracket(hb);

    bool scpx0 = qsettings->value("select_cursor_position_x0","ture").toBool();
    root->file->select_cursor_position_x0 = scpx0;

    QString logfile = qsettings->value("logfile", "").toString();
    if (logfile.isEmpty()) {
        QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, false);
        Tui::ZSimpleStringLogger::install();
    } else {
        Tui::ZSimpleFileLogger::install(logfile);
    }
    qDebug("chr starting");

    // OPEN FILE
    int lineNumber = -1, lineChar = 0;
    if(!args.isEmpty()) {
        QStringList p = args;
        if (p.first().mid(0,1) == "+") {
            QString n = p.first().mid(1);
            QStringList list1 = n.split(',');
            if(list1.count()>0) {
                lineNumber = list1[0].toInt();
            }
            if(list1.count()>1) {
                lineChar = list1[1].toInt() -1;
            }
            p.removeFirst();
        }
        if (!p.isEmpty()) {
            if (p.first() != "-") {
                QFileInfo datei(p.first());
                if(datei.isFile()) {
                    int maxMB = 100;
                    if(datei.size()/1024/1024 >= maxMB && !parser.isSet(bigOption) && !bigfile) {
                        out << "The file is bigger then "<< maxMB << "MB ("<< datei.size()/1024/1024 <<"MB). Please start with -b for big files.\n";
                        return 0;
                    }
                    root->openFile(datei.absoluteFilePath());
                } else if(datei.isDir()) {
                    QString tmp = datei.absoluteFilePath();
                    QTimer *t = new QTimer();
                    QObject::connect(t, &QTimer::timeout, t, [t, root, tmp] {
                        root->openFileDialog(tmp);
                        t->deleteLater();
                    });
                    t->setSingleShot(true);
                    t->start(0);
                } else {
                    root->newFile(datei.absoluteFilePath());
                }
            } else {
                if(parser.isSet(append)) {
                    root->openFile(parser.value(append));
                } else {
                    root->newFile();
                }
                root->watchPipe();
            }
        } else {
            out << "Got file offset without file name.\n";
            return 0;
        }
    } else {
        root->newFile();
    }

    //Goto Line
    if (lineNumber > 0 || lineChar > 0) {
        root->file->gotoline(lineNumber,lineChar);
    } else if(parser.isSet(numberOption)) {
        QString n = parser.value(numberOption);
        QStringList list1 = n.split(',');
        if(list1.count()>0) {
            lineNumber = list1[0].toInt();
        }
        if(list1.count()>1) {
            lineChar = list1[1].toInt() -1;
        }
        root->file->gotoline(lineNumber,lineChar);
    }

    if(parser.isSet(follow)) {
        root->setFollow(true);
    }

    root->file->setFocus();
    root->file->setCursorStyle(Tui::CursorStyle::Underline);

    app.exec();
    if(Tui::ZSimpleStringLogger::getMessages().size() > 0) {
        terminal.pauseOperation();
        printf("\e[90m%s\e[0m", Tui::ZSimpleStringLogger::getMessages().toUtf8().data());
    }
    return 0;
}
