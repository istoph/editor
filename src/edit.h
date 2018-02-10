#ifndef EDIT_H
#define EDIT_H

#include <QTimer>
#include <QCoreApplication>

#include <Tui/ZCommandNotifier.h>
#include <Tui/ZPalette.h>
#include <Tui/ZSymbol.h>
#include <Tui/ZRoot.h>

#include <testtui_lib.h>

#include <QFile>
#include <QTextStream>

class File : public Tui::ZWidget {
    Q_OBJECT

public:
    File();
    explicit File(Tui::ZWidget *parent);
    bool setFilename(QString filename);
    QString getFilename();
    bool saveText();
    bool openText();

public:
//    QString text() const;
//    void setText(const QString &t);
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    bool formatting_characters = true;

//signals:

protected:
    void paintEvent(Tui::ZPaintEvent *event);
    void keyEvent(Tui::ZKeyEvent *event);

private:
    void adjustScrollPosition();
    QString filename;

private:
    QVector<QString> _text;
    int _scrollPositionX = 0;
};



class Editor : public Tui::ZRoot {
    Q_OBJECT

public:
    Editor();

    WindowWidget *win;
    WindowWidget *option_tab;
    Label *statusBar;
    Label *statusBarL;
    File *file;
    Label *file_name;
    int tab = 8;

};

#endif // EDIT_H
