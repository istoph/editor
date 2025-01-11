### How to build

To build the editor on your system, you need a set of dependencies:

* meson (>= 0.59.0) https://github.com/mesonbuild/meson
* ninja-build  https://github.com/ninja-build/ninja
* pkg-config https://www.freedesktop.org/wiki/Software/pkg-config/
* qmake (>5.11.3) https://doc.qt.io/qt-5/qmake-manual.html
* qtbase (>5.11.3) https://code.qt.io/cgit/qt/qtbase.git/log/?h=5.15
* termpaint (>= 0.3.0) https://github.com/termpaint/termpaint docs: https://termpaint.namepad.de/latest/
* posixsignalmanager (>= 0.3) https://github.com/textshell/posixsignalmanager/
* tuiwidgets (>= 0.2.1) https://github.com/tuiwidgets/tuiwidgets docs: https://tuiwidgets.namepad.de/latest/

For syntax highlighting:
* libkf5syntaxhighlighting https://api.kde.org/frameworks/syntax-highlighting/html/index.html

For building the tests without bundled copy:
* catch https://github.com/catchorg/Catch2

## How to build the editor

# build termpaint library
```
git clone https://github.com/termpaint/termpaint
cd termpaint
meson setup _build -Dprefix=$HOME/opt/tuiwidgets-prefix
meson compile -C _build
meson install -C _build
cd ..
```

# build tuiwidgets and posixsignalmanager library
```
git clone https://github.com/tuiwidgets/tuiwidgets
cd tuiwidgets
PKG_CONFIG_PATH=$HOME/opt/tuiwidgets-prefix/lib/x86_64-linux-gnu/pkgconfig meson setup _build -Dprefix=$HOME/opt/tuiwidgets-prefix -Drpath=$HOME/opt/tuiwidgets-prefix/
lib/x86_64-linux-gnu/
meson compile -C _build
meson install -C _build
cd ..
```

# build chr editor
```
git clone https://github.com/istoph/editor
cd editor
PKG_CONFIG_PATH=$HOME/opt/tuiwidgets-prefix/lib/x86_64-linux-gnu/pkgconfig meson setup _build -Dsyntax_highlighting=true -Drpath=$HOME/opt/tuiwidgets-prefix/lib/x86_64-linux-gnu/
meson compile -C _build
meson install -C _build
cd ..
```

## Dockerfiles
These Dockerfiles are for testing whether the build still works on your distro.
Or you can see how it works.

https://github.com/istoph/editor/tree/main/doc/build/

```
docker build -f Dockerfile.debian\:sid -t sid .
docker run -it sid chr
```

