#ifndef THEME_H
#define THEME_H
#include <testtui_lib.h>

class Editor;

class ThemeDialog : public Dialog {
    Q_OBJECT

public:
    ThemeDialog(Editor *edit);

public slots:
    void open();
    void close();

private:
    ListView *_lv = nullptr;
    Button *_cancelButton = nullptr;
    Button *_okButton = nullptr;
};

#endif // THEME_H
