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

public:
//    QString text() const;
//    void setText(const QString &t);
    int _cursorPositionX = 0;
    int _cursorPositionY = 0;
    bool formatting_characters = true;

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
};

#endif // FILE_H
