// SPDX-License-Identifier: BSL-1.0

#include <QLocalServer>
#include <QLocalSocket>
#include <QPoint>
#include <QProcess>

#include <Tui/ZTerminal.h>
#include <Tui/ZWidget.h>
#include <Tui/ZImage.h>

class TwConnectMirrorWidget : public Tui::ZWidget {
public:
    TwConnectMirrorWidget(Tui::ZTerminal *master);

public:
    QSize sizeHint() const override;
    bool event(QEvent *event) override;

protected:
    void paintEvent(Tui::ZPaintEvent *event) override;
    void keyEvent(Tui::ZKeyEvent *event) override;
    void pasteEvent(Tui::ZPasteEvent *event) override;

private:
    void updateMasterData();

private:
    Tui::ZTerminal *_master = nullptr;
    Tui::ZImage _image;
    QPoint _cursorPos;
    Tui::CursorStyle _cursorStyle = Tui::CursorStyle::Unset;
    bool _cursorVisible = true;
    int _cursorR = -1;
    int _cursorG = -1;
    int _cursorB = -1;

};

class TwConnectConnection : public QObject, Tui::ZTerminal::TerminalConnectionDelegate {
    Q_OBJECT
public:
    TwConnectConnection(QIODevice *sock);

    void write(const char *data, int length) override;
    void flush() override;
    void restoreSequenceUpdated(const char *data, int len) override;
    void deinit(bool awaitingResponse) override;

    Tui::ZTerminal::TerminalConnection *connection();

signals:
    void connectionReady();

private:
    void sendInput(QByteArray in);
    void processFramesFromClient();
    void writeFrame(uint8_t type, const QByteArray &data);

private:
    QByteArray _fromClientBuffer;

    QIODevice *_sock;
    Tui::ZTerminal::TerminalConnection _conn;
};

class TwConnectServer : public QObject {
    Q_OBJECT
public:
    TwConnectServer(Tui::ZTerminal *master);
    TwConnectServer(Tui::ZTerminal *master, QString path);

public:
private:
    QLocalServer _server;
    Tui::ZTerminal *_master = nullptr;
};

class TwConnectServerPeersock : public QObject {
    Q_OBJECT
public:
    TwConnectServerPeersock(Tui::ZTerminal *master, QString code, std::function<void(const QJsonObject&)> messageCallback);

public:
private:
    Tui::ZTerminal *_master = nullptr;
    QProcess _peersockProcess;
    QByteArray _pendungMsgBuffer;
};
