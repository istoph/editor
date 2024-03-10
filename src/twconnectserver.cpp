// SPDX-License-Identifier: BSL-1.0

#include "twconnectserver.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <Tui/ZCommon.h>
#include <Tui/ZImage.h>
#include <Tui/ZPainter.h>

TwConnectServer::TwConnectServer(Tui::ZTerminal *master)
    : TwConnectServer(master, "twconnect-chr-" + QString::number(QCoreApplication::instance()->applicationPid())) {
}

TwConnectServer::TwConnectServer(Tui::ZTerminal *master, QString path) : _master(master) {
    _server.setSocketOptions(QLocalServer::UserAccessOption);

    if (!path.startsWith("/")) {
        path = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + "/" + path;
    }

    _server.listen(path);

    QObject::connect(&_server, &QLocalServer::newConnection, this, [this] {
        QLocalSocket *sock = _server.nextPendingConnection();
        _server.close();
        auto *con = new TwConnectConnection(sock);
        QObject::connect(con, &TwConnectConnection::connectionReady, [con, master=_master] () mutable {
            auto *term = new Tui::ZTerminal(con->connection(), {});
            term->setMainWidget(new TwConnectMirrorWidget(master));
        });
    });
}


TwConnectServerPeersock::TwConnectServerPeersock(Tui::ZTerminal *master, QString code,
                                                 std::function<void(const QJsonObject&)> messageCallback)
        : _master(master) {
    QObject::connect(&_peersockProcess, &QProcess::readyReadStandardError, this, [this, messageCallback] {
        _pendungMsgBuffer += _peersockProcess.readAllStandardError();

        if (_pendungMsgBuffer.indexOf('\n') != -1) {
            auto msgLine = _pendungMsgBuffer.left(_pendungMsgBuffer.indexOf('\n'));
            _pendungMsgBuffer = _pendungMsgBuffer.mid(_pendungMsgBuffer.indexOf('\n') + 1);
            auto msgObj = QJsonDocument::fromJson(msgLine).object();
            messageCallback(msgObj);
        }

    });

    QStringList args = {"--json", "stdio-b"};
    if (code.size()) {
        args.append(code);
    }
    _peersockProcess.start("peersock", args);
    auto *con = new TwConnectConnection(&_peersockProcess);
    QObject::connect(con, &TwConnectConnection::connectionReady, [con, master=_master] () mutable {
        auto *term = new Tui::ZTerminal(con->connection(), {});
        term->setMainWidget(new TwConnectMirrorWidget(master));
    });
}


TwConnectConnection::TwConnectConnection(QIODevice *sock) : _sock(sock) {
    _conn.setDelegate(this);

    QObject::connect(_sock, &QLocalSocket::readyRead, [this] {
        _fromClientBuffer += _sock->read(1024*1024);
        processFramesFromClient();
    });

}


void TwConnectConnection::write(const char *data, int length) {
    writeFrame(0, QByteArray{data, length});
}

void TwConnectConnection::flush() {
}

void TwConnectConnection::restoreSequenceUpdated(const char *data, int len) {
    writeFrame(1, QByteArray{data, len});
}

void TwConnectConnection::deinit(bool awaitingResponse) {
    // TODO
}

Tui::ZTerminal::TerminalConnection *TwConnectConnection::connection() {
    return &_conn;
}

void TwConnectConnection::sendInput(QByteArray in) {
    _conn.terminalInput(in.data(), in.size());
}

void TwConnectConnection::processFramesFromClient() {
    while (_fromClientBuffer.size() > 2) {
        int frameSize = (((unsigned char)_fromClientBuffer[0]) * 256 + ((unsigned char)_fromClientBuffer[1]));
        if (_fromClientBuffer.size() >= frameSize) {
            int type = ((unsigned char)_fromClientBuffer[2]);
            if (type == 0) {
                sendInput(_fromClientBuffer.mid(3, frameSize - 3));
            } else if (type == 1 && frameSize >= 8) {
                QJsonObject data = QJsonDocument::fromJson(_fromClientBuffer.mid(3, frameSize - 3)).object();
                int width = std::max(0, data["width"].toInt());
                int height = std::max(0, data["height"].toInt());
                bool backspaceIsX08 = data["backspaceIsX08"].toBool();
                _conn.setBackspaceIsX08(backspaceIsX08);
                _conn.setSize(width, height);
                Q_EMIT connectionReady();
            } else if (type == 2 && frameSize >= 7) {
                QJsonObject data = QJsonDocument::fromJson(_fromClientBuffer.mid(3, frameSize - 3)).object();
                int width = std::max(0, data["width"].toInt());
                int height = std::max(0, data["height"].toInt());
                _conn.setSize(width, height);
            }
            _fromClientBuffer = _fromClientBuffer.mid(frameSize);
        } else {
            break;
        }
    }
}

void TwConnectConnection::writeFrame(uint8_t type, const QByteArray &data) {
    uint8_t d;
    d = ((data.size() + 3) & 0xff00) >> 8;
    _sock->write((char*)&d, sizeof d);
    d = (data.size() + 3) & 0xff;
    _sock->write((char*)&d, sizeof d);
    d = type;
    _sock->write((char*)&d, sizeof d);
    _sock->write(data);
}

TwConnectMirrorWidget::TwConnectMirrorWidget(Tui::ZTerminal *master) : _master(master), _image{_master, 0, 0} {
    updateMasterData();
    //_image = _master->grabCurrentImage();
    //_cursorPos = _master->grabCursorPosition();
    //_cursorVisible = _master->grabCursorVisibility();
    //_cursorStyle = _master->grabCursorStyle();
    //std::tie(_cursorR, _cursorG, _cursorB) = _master->grabCursorColor();
    //setMinimumSize(_image.size().width(), _image.size().height());

    // TODO do this also when acquiring terminal
    QObject::connect(_master, &Tui::ZTerminal::afterRendering, this, [this] {
        updateMasterData();
        update();
    });
}

void TwConnectMirrorWidget::updateMasterData() {
    _image = _master->grabCurrentImage();
    _cursorPos = _master->grabCursorPosition();
    _cursorVisible = _master->grabCursorVisibility();
    _cursorStyle = _master->grabCursorStyle();
    std::tie(_cursorR, _cursorG, _cursorB) = _master->grabCursorColor();
    setMinimumSize(_image.size().width(), _image.size().height());
    auto *term = terminal();
    if (term) {
        term->setTitle(_master->title());
        term->setIconTitle(_master->iconTitle());
    }
}

QSize TwConnectMirrorWidget::sizeHint() const {
    return _image.size();
}

bool TwConnectMirrorWidget::event(QEvent *event) {
    if (event->type() == Tui::ZEventType::terminalChange()) {
        updateMasterData();
    }

    return ZWidget::event(event);
}

void TwConnectMirrorWidget::paintEvent(Tui::ZPaintEvent *event) {
    auto painter = event->painter();
    painter->drawImage(0,0, _image);
    setCursorStyle(_cursorStyle);
    setCursorColor(_cursorR, _cursorG, _cursorB);
    if (_cursorVisible) {
        painter->setCursor(_cursorPos.x(), _cursorPos.y());
    }
}

void TwConnectMirrorWidget::keyEvent(Tui::ZKeyEvent *event) {
    _master->dispatchKeyboardEvent(*event);
}

void TwConnectMirrorWidget::pasteEvent(Tui::ZPasteEvent *event) {
    _master->dispatchPasteEvent(*event);
}
