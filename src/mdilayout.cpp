// SPDX-License-Identifier: BSL-1.0

#include "mdilayout.h"

#include <Tui/ZPainter.h>
#include <Tui/ZShortcut.h>
#include <Tui/ZWindowFacet.h>


MdiLayout::MdiLayout() {
}

MdiLayout::~MdiLayout() {
}

void MdiLayout::addWindow(Tui::ZWidget *w) {
    Tui::ZWindowFacet *windowFacet = static_cast<Tui::ZWindowFacet*>(w->facet(Tui::ZWindowFacet::staticMetaObject));
    if (windowFacet) {
        windowFacet->setContainer(&windowContainer);
        windowFacet->setManuallyPlaced(false);
    }

    double sum = 0.;

    for (const Item &item: _items) {
        sum += item.weight;
    }

    double newWeight = _items.size() ? sum / _items.size() : 1;

    _items.push_back({w, newWeight});

    QObject::connect(w, &QObject::destroyed, this, [this, w] {
        QMutableVectorIterator<Item> it{_items};
        while (it.hasNext()) {
            it.next();
            if (it.value().item == w) {
                it.remove();
            }
        }
        relayout();
    });

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Up, Qt::ControlModifier), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            setInteractiveUp(w);
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Down, Qt::ControlModifier), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            setInteractiveDown(w);
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Left, Qt::ControlModifier), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            setInteractiveLeft(w);
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Right, Qt::ControlModifier), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            setInteractiveRight(w);
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Up, {}), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            if (_mode == LayoutMode::TileV) {
                auto *prev = prevWindow(w);
                if (prev) {
                    setFocus(prev);
                    prev->raise();
                }
            }
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Down, {}), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            if (_mode == LayoutMode::TileV) {
                auto *next = nextWindow(w);
                if (next) {
                    setFocus(next);
                    next->raise();
                }
            }
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Left, {}), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            if (_mode == LayoutMode::TileH) {
                auto *prev = prevWindow(w);
                if (prev) {
                    setFocus(prev);
                    prev->raise();
                }
            }
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Right, {}), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            if (_mode == LayoutMode::TileH) {
                auto *next = nextWindow(w);
                if (next) {
                    setFocus(next);
                    next->raise();
                }
            }
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Up, Qt::ControlModifier | Qt::ShiftModifier), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            if (_mode == LayoutMode::TileV) {
                swapPrevWindow(w);
            }
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Down, Qt::ControlModifier | Qt::ShiftModifier), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            if (_mode == LayoutMode::TileV) {
                swapNextWindow(w);
            }
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Left, Qt::ControlModifier | Qt::ShiftModifier), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            if (_mode == LayoutMode::TileH) {
                swapPrevWindow(w);
            }
        }
    );

    QObject::connect(new Tui::ZShortcut(
                         Tui::ZKeySequence::forShortcutSequence("e", Qt::ControlModifier, Qt::Key_Right, Qt::ControlModifier | Qt::ShiftModifier), w, Qt::WindowShortcut), &Tui::ZShortcut::activated,
        [this, w] {
            if (_mode == LayoutMode::TileH) {
                swapNextWindow(w);
            }
        }
    );

    relayout();
}

void MdiLayout::setMode(MdiLayout::LayoutMode mode) {
    _mode = mode;
    relayout();
}

void MdiLayout::setGeometry(QRect r) {
    height = r.height();
    width = r.width();
    if (_mode == LayoutMode::Base) {
        for (const Item &item: _items) {
            auto *const childWidget = item.item;
            Tui::ZWindowFacet *windowFacet = static_cast<Tui::ZWindowFacet*>(childWidget->facet(Tui::ZWindowFacet::staticMetaObject));
            if (windowFacet) {
                if (windowFacet->container() == &windowContainer && !windowFacet->isManuallyPlaced()) {
                    childWidget->setGeometry(r);
                }
            } else {
                childWidget->setGeometry(r);
            }
        }
    } else {
        QVector<Item*> fit;
        double sum = 0;
        for (Item &item: _items) {
            auto *const childWidget = item.item;
            Tui::ZWindowFacet *windowFacet = static_cast<Tui::ZWindowFacet*>(childWidget->facet(Tui::ZWindowFacet::staticMetaObject));
            if (windowFacet) {
                if (windowFacet->container() == &windowContainer && !windowFacet->isManuallyPlaced()) {
                    fit.append(&item);
                    sum += item.weight;
                }
            } else {
                fit.append(&item);
                sum += item.weight;
            }
        }
        if (fit.size() == 0) {
            return;
        }
        if (_mode == LayoutMode::TileV) {
            double factor = r.height() / sum;
            double y = r.top();
            for (int i = 0; i < fit.size(); i++) {
                Item *item = fit[i];
                double h = item->weight * factor;
                int height = static_cast<int>(y + h) - static_cast<int>(y);
                if (i + 1 == fit.size() && static_cast<int>(y) + height <= r.bottom()) {
                    height += 1;
                }
                item->item->setGeometry({r.left(), static_cast<int>(y), r.width(), height});
                item->lastLayoutSize = height;
                if (item->item == interactiveSizeSelected) {
                    if (interactiveSizeEdge == Qt::TopEdge) {
                        overlay.setGeometry({r.left() + r.width() / 4, static_cast<int>(y) - 1, r.width() / 2, 3});
                    } else {
                        overlay.setGeometry({r.left() + r.width() / 4, static_cast<int>(y + h) - 1, r.width() / 2, 3});
                    }
                }
                y += h;
            }
        }
        if (_mode == LayoutMode::TileH) {
            double factor = r.width() / sum;
            double x = r.left();
            for (int i = 0; i < fit.size(); i++) {
                Item *item = fit[i];
                double w = item->weight * factor;
                int width = static_cast<int>(x + w) - static_cast<int>(x);
                if (i + 1 == fit.size() && static_cast<int>(x) + width <= r.bottom()) {
                    width += 1;
                }
                item->item->setGeometry({static_cast<int>(x), r.top(), width, r.height()});
                item->lastLayoutSize = width;
                if (item->item == interactiveSizeSelected) {
                    if (interactiveSizeEdge == Qt::LeftEdge) {
                        overlay.setGeometry({static_cast<int>(x) - 1, r.top() + r.height() / 4, 3, r.height() / 2});
                    } else {
                        overlay.setGeometry({static_cast<int>(x + w) - 1, r.top() + r.height() / 4, 3, r.height() / 2});
                    }
                }
                x += w;
            }
        }
    }
}

void MdiLayout::removeWidgetRecursively(Tui::ZWidget *widget) {
    QMutableVectorIterator<Item> it{_items};
    while (it.hasNext()) {
        it.next();
        if (it.value().item == widget) {
            it.remove();
        }
    }
    relayout();
}

QSize MdiLayout::sizeHint() const {
    return {1, 1};
}

Tui::SizePolicy MdiLayout::sizePolicyH() const {
    return Tui::SizePolicy::Expanding;
}

Tui::SizePolicy MdiLayout::sizePolicyV() const {
    return Tui::SizePolicy::Expanding;
}

void MdiLayout::setInteractiveUp(Tui::ZWidget *w) {
    if (_mode == LayoutMode::TileH || _items.size() == 0 || _items.first().item == w) {
        return;
    }
    interactiveSizeSelected = w;
    interactiveSizeEdge = Qt::TopEdge;
    overlay.setVertical(true);
    overlay.setParent(qobject_cast<Tui::ZWidget*>(parent()->parent()));
    relayout();

    w->grabKeyboard([this] (QEvent *event) {
        interactiveHandler(event);
    });
}

void MdiLayout::setInteractiveDown(Tui::ZWidget *w) {
    if (_mode == LayoutMode::TileH || _items.size() == 0 || _items.last().item == w) {
        return;
    }
    interactiveSizeSelected = w;
    interactiveSizeEdge = Qt::BottomEdge;
    overlay.setVertical(true);
    overlay.setParent(qobject_cast<Tui::ZWidget*>(parent()->parent()));
    relayout();

    w->grabKeyboard([this] (QEvent *event) {
        interactiveHandler(event);
    });
}

void MdiLayout::setInteractiveLeft(Tui::ZWidget *w) {
    if (_mode == LayoutMode::TileV || _items.size() == 0 || _items.first().item == w) {
        return;
    }
    interactiveSizeSelected = w;
    interactiveSizeEdge = Qt::LeftEdge;
    overlay.setVertical(false);
    overlay.setParent(qobject_cast<Tui::ZWidget*>(parent()->parent()));
    relayout();

    w->grabKeyboard([this] (QEvent *event) {
        interactiveHandler(event);
    });
}

void MdiLayout::setInteractiveRight(Tui::ZWidget *w) {
    if (_mode == LayoutMode::TileV || _items.size() == 0 || _items.last().item == w) {
        return;
    }
    interactiveSizeSelected = w;
    interactiveSizeEdge = Qt::RightEdge;
    overlay.setVertical(false);
    overlay.setParent(qobject_cast<Tui::ZWidget*>(parent()->parent()));
    relayout();

    w->grabKeyboard([this] (QEvent *event) {
        interactiveHandler(event);
    });
}

void MdiLayout::interactiveHandler(QEvent *event) {
    if (event->type() == Tui::ZEventType::key()) {
        auto *keyEvent = static_cast<Tui::ZKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Enter) {
            interactiveSizeSelected->releaseKeyboard();
            interactiveSizeSelected = nullptr;
            overlay.setParent(nullptr);
            relayout();
        } else if (keyEvent->key() == Qt::Key_Escape) {
            interactiveSizeSelected->releaseKeyboard();
            interactiveSizeSelected = nullptr;
            overlay.setParent(nullptr);
            relayout();
        } else {
            if (interactiveSizeEdge == Qt::TopEdge) {
                if (keyEvent->key() == Qt::Key_Up) {
                    increaseWeight(interactiveSizeSelected, height);
                } else if (keyEvent->key() == Qt::Key_Down) {
                    decreaseWeight(interactiveSizeSelected, height);
                }
            } else if (interactiveSizeEdge == Qt::BottomEdge) {
                if (keyEvent->key() == Qt::Key_Up) {
                    auto w = nextWindow(interactiveSizeSelected);
                    if (w) {
                        increaseWeight(w, height);
                    }
                } else if (keyEvent->key() == Qt::Key_Down) {
                    auto w = nextWindow(interactiveSizeSelected);
                    if (w) {
                        decreaseWeight(w, height);
                    }
                }
            } else if (interactiveSizeEdge == Qt::LeftEdge) {
                if (keyEvent->key() == Qt::Key_Left) {
                    increaseWeight(interactiveSizeSelected, width);
                } else if (keyEvent->key() == Qt::Key_Right) {
                    decreaseWeight(interactiveSizeSelected, width);
                }
            } else if (interactiveSizeEdge == Qt::RightEdge) {
                if (keyEvent->key() == Qt::Key_Left) {
                    auto w = nextWindow(interactiveSizeSelected);
                    if (w) {
                        increaseWeight(w, width);
                    }
                } else if (keyEvent->key() == Qt::Key_Right) {
                    auto w = nextWindow(interactiveSizeSelected);
                    if (w) {
                        decreaseWeight(w, width);
                    }
                }
            }
        }
    }
}

void MdiLayout::increaseWeight(Tui::ZWidget *w, int extend) {
    double sum = 0;
    for (Item &item: _items) {
        sum += item.weight;
    }
    double adjust = sum / extend;

    Item *prevItem = nullptr;
    for (Item &item: _items) {
        if (item.item == w) {
            if (prevItem && prevItem->lastLayoutSize > 2) {
                prevItem->weight -= adjust;
                item.weight += adjust;
            }
        }
        prevItem = &item;
    }
    relayout();

}

void MdiLayout::decreaseWeight(Tui::ZWidget *w, int extend) {
    double sum = 0;
    for (Item &item: _items) {
        sum += item.weight;
    }
    double adjust = sum / extend;

    Item *prevItem = nullptr;
    for (Item &item: _items) {
        if (item.item == w) {
            if (prevItem && item.lastLayoutSize > 2) {
                prevItem->weight += adjust;
                item.weight -= adjust;
            }
        }
        prevItem = &item;
    }
    relayout();
}

Tui::ZWidget *MdiLayout::prevWindow(Tui::ZWidget *w) {
    Tui::ZWidget *prev = nullptr;
    for (Item &item: _items) {
        if (item.item == w) {
            return prev;
        }
        prev = item.item;
    }
    return nullptr;
}

Tui::ZWidget *MdiLayout::nextWindow(Tui::ZWidget *w) {
    bool found = false;
    for (Item &item: _items) {
        if (found) {
            return item.item;
        }
        if (item.item == w) {
            found = true;
        }
    }
    return nullptr;
}

void MdiLayout::swapPrevWindow(Tui::ZWidget *w) {
    Item *prev = nullptr;
    for (Item &item: _items) {
        if (item.item == w && prev) {
            using namespace std;
            swap(item, *prev);
            relayout();
            break;
        }
        prev = &item;
    }
}

void MdiLayout::swapNextWindow(Tui::ZWidget *w) {
    Item *found = nullptr;
    for (Item &item: _items) {
        if (found) {
            using namespace std;
            swap(item, *found);
            relayout();
            break;
        }
        if (item.item == w) {
            found = &item;
        }
    }

}

void MdiLayout::setFocus(Tui::ZWidget *w) {
    w = w->placeFocus();
    if (w) {
        w->setFocus(Qt::FocusReason::ActiveWindowFocusReason);
    }
}
SizerOverlay::SizerOverlay(Tui::ZWidget *parent) : Tui::ZWidget(parent) {
    setGeometry({0, 0, 3, 3});
}

void SizerOverlay::paintEvent(Tui::ZPaintEvent *event) {
    auto painter = event->painter();
    auto bg = Tui::Colors::blue;
    auto fg = Tui::Colors::brightGreen;

    if (_vertical) {
        if (geometry().width() > 6) {
            painter->writeWithColors(0, 0, "↑↑↑", fg, bg);
            //painter->writeWithColors(0, 1, "|||", fg, bg);
            painter->writeWithColors(0, 2, "↓↓↓", fg, bg);

            painter->writeWithColors(geometry().width() - 3, 0, "↑↑↑", fg, bg);
            //painter->writeWithColors(geometry().width() - 3, 1, "|||", fg, bg);
            painter->writeWithColors(geometry().width() - 3, 2, "↓↓↓", fg, bg);
        } else {
            painter->writeWithColors(geometry().width() / 2 - 1, 0, "↑↑↑", fg, bg);
            //painter->writeWithColors(geometry().width() / 2 - 1, 1, "|||", fg, bg);
            painter->writeWithColors(geometry().width() / 2 - 1, 2, "↓↓↓", fg, bg);
        }
    } else {
        if (geometry().height() > 6) {
            painter->writeWithColors(0, 0, "←-→", fg, bg);
            //painter->writeWithColors(0, 1, "←-→", fg, bg);
            painter->writeWithColors(0, 2, "←-→", fg, bg);

            painter->writeWithColors(0, geometry().height() - 3, "←-→", fg, bg);
            //painter->writeWithColors(0, geometry().height() - 2, "←-→", fg, bg);
            painter->writeWithColors(0, geometry().height() - 1, "←-→", fg, bg);
        } else {
            painter->writeWithColors(0, geometry().height() / 2 - 1, "←-→", fg, bg);
            //painter->writeWithColors(0, geometry().height() / 2 + 0, "←-→", fg, bg);
            painter->writeWithColors(0, geometry().height() / 2 + 1, "←-→", fg, bg);
        }
    }

}

void SizerOverlay::setVertical(bool v) {
    _vertical = v;
}
