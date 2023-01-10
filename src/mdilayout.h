#ifndef MDILAYOUT_H
#define MDILAYOUT_H

#include <Tui/ZColor.h>
#include <Tui/ZPainter.h>

#include <Tui/ZLayout.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindowContainer.h>
#include <Tui/ZWindow.h>
#include <Tui/ZWindowFacet.h>

#include <QString>
#include <QPointer>
#include <QObject>
#include <QVector>

class SizerOverlay : public Tui::ZWidget {
    Q_OBJECT

public:
    SizerOverlay(Tui::ZWidget* parent = nullptr);

public:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void setVertical(bool v);

private:
    bool _vertical = true;
};

class MdiLayout : public Tui::ZLayout {
    Q_OBJECT

public:
    enum class LayoutMode {
        Base, TileV, TileH
    };

public:
    MdiLayout();
    ~MdiLayout() override;

public:
    void addWindow(Tui::ZWidget *w);

    void setMode(LayoutMode _mode);

public:
    void setGeometry(QRect r) override;
    void removeWidgetRecursively(Tui::ZWidget *widget) override;
    QSize sizeHint() const  override;
    Tui::SizePolicy sizePolicyH() const  override;
    Tui::SizePolicy sizePolicyV() const  override;

private:
    void setInteractiveUp(Tui::ZWidget *w);
    void setInteractiveDown(Tui::ZWidget *w);
    void setInteractiveLeft(Tui::ZWidget *w);
    void setInteractiveRight(Tui::ZWidget *w);
    void interactiveHandler(QEvent *event);
    void increaseWeight(Tui::ZWidget *w, int extend);
    void decreaseWeight(Tui::ZWidget *w, int extend);
    Tui::ZWidget *prevWindow(Tui::ZWidget *w);
    Tui::ZWidget *nextWindow(Tui::ZWidget *w);
    void swapPrevWindow(Tui::ZWidget *w);
    void swapNextWindow(Tui::ZWidget *w);
    void setFocus(Tui::ZWidget *w);

private:
    struct Item {
        Tui::ZWidget *item;
        double weight = 1;
        int lastLayoutSize = 0;
    };

    QVector<Item> _items;
    LayoutMode _mode = LayoutMode::TileV;
    Tui::ZWindowContainer windowContainer;
    QPointer<Tui::ZWidget> interactiveSizeSelected;
    Qt::Edge interactiveSizeEdge = Qt::BottomEdge;
    int height = 1;
    SizerOverlay overlay;
};

#endif // MDILAYOUT_H
