#ifndef FILE_H
#define FILE_H

#include <testtui_lib.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

class File : public Tui::ZWidget {
    Q_OBJECT

public:
    File();
    explicit File(Tui::ZWidget *parent);
    bool setFilename(QString filename);
    QString getFilename();
    bool newText();
    bool saveText();
    bool openText();
    bool setTabsize(int tab);
    int getTabsize();
    bool setFormatting_characters(bool fb);
    bool getformatting_characters();
    void select(int x, int y);
    void resetSelect();
    void getSelect(bool start, int &x, int &y);
    bool isSelect(int x, int y);
    void setOverwrite();
    bool isOverwrite();

    void cut();
    void copy();
    void paste();
    bool isInsertable();

public:
//    QString text() const;
//    void setText(const QString &t);
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;

signals:
    void cursorPositionChanged(int x, int y);
    void scrollPositionChanged(int x, int y);
    void textMax(int x, int y);

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;

private:
    void adjustScrollPosition();
    QString filename;

private:
    QVector<QString> _text;
    int _scrollPositionX = 0;
    int _scrollPositionY = 0;
    int _lastCursorPositionX = -1;
    int _tabsize = 8;
    bool _formatting_characters = true;
    int startSelectX = -1;
    int startSelectY = -1;
    int endSelectX = -1;
    int endSelectY = -1;
    QString _clipboard;
    //QVector<QString> _clipboard;
    bool overwrite = false;
};

#endif // FILE_H
