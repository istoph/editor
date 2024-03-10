// SPDX-License-Identifier: BSL-1.0

#include <unistd.h>
#include <termios.h>
#include <memory>

#include <QByteArray>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QProcess>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QTimer>

#include <PosixSignalManager.h>

#include <termpaintx.h>
#include <termpaintx_ttyrescue.h>

class ConnectorBase {
protected:
    ConnectorBase() {
        _rescue = termpaintx_ttyrescue_start_or_nullptr(0, "\n");
        _restoreSequence = "\n";
    }

    void writeFrame(uint8_t type, const QByteArray &data) {
        uint8_t d;
        d = ((data.size() + 3) & 0xff00) >> 8;
        _ioDevice->write((char*)&d, sizeof d);
        d = (data.size() + 3) & 0xff;
        _ioDevice->write((char*)&d, sizeof d);
        d = type;
        _ioDevice->write((char*)&d, sizeof d);
        _ioDevice->write(data);
    }

    void onConnected(QIODevice *ioDevice) {
        _ioDevice = ioDevice;

        if (!PosixSignalManager::isCreated()) {
            PosixSignalManager::create();
        }

        terminalSizeChangeNotifier = std::unique_ptr<PosixSignalNotifier>(new PosixSignalNotifier(SIGWINCH));
        QObject::connect(terminalSizeChangeNotifier.get(), &PosixSignalNotifier::activated, [this] {
            updateSize();
        });


        tcgetattr(0, &_original_terminal_attributes);
        termpaintx_fd_set_termios(0, "");
        termpaintx_ttyrescue_set_restore_termios(_rescue, &_original_terminal_attributes);
        int width, height;
        termpaintx_fd_terminal_size(0, &width, &height);
        QJsonObject data;
        data["width"] = width;
        data["height"] = height;
        data["backspaceIsX08"] = bool(_original_terminal_attributes.c_cc[VERASE] == 0x08);
        writeFrame(1, QJsonDocument(data).toJson());

        inputNotifier.reset(new QSocketNotifier(0, QSocketNotifier::Read));
        QObject::connect(inputNotifier.get(), &QSocketNotifier::activated,
                         [this] (int socket) {
            char buff[100];
            int amount = read (socket, buff, 99);
            if (amount > 0) {
                writeFrame(0, QByteArray{buff, amount});
            }
        });
    }

    void processFramesFromServer() {
        while (_fromServerBuffer.size() > 2) {
            int frameSize = (((unsigned char)_fromServerBuffer[0]) * 256 + ((unsigned char)_fromServerBuffer[1]));
            if (_fromServerBuffer.size() >= frameSize) {
                int type = ((unsigned char)_fromServerBuffer[2]);
                if (type == 0) {
                    write(1, _fromServerBuffer.data() + 3, frameSize - 3);
                }
                if (type == 1) {
                    _restoreSequence = _fromServerBuffer.mid(3, frameSize - 3);
                    termpaintx_ttyrescue_update(_rescue, _restoreSequence.data(), _restoreSequence.size());
                }
                _fromServerBuffer = _fromServerBuffer.mid(frameSize);
            } else {
                break;
            }
        }
    }

    void updateSize() {
        int width, height;
        termpaintx_fd_terminal_size(0, &width, &height);
        QJsonObject data;
        data["width"] = width;
        data["height"] = height;
        writeFrame(2, QJsonDocument(data).toJson());
    }


protected:
    termpaintx_ttyrescue *_rescue = nullptr;
    std::unique_ptr<QSocketNotifier> inputNotifier;
    std::unique_ptr<PosixSignalNotifier> terminalSizeChangeNotifier;

    termios _original_terminal_attributes;
    QByteArray _restoreSequence;

    QIODevice *_ioDevice = nullptr;
    QByteArray _fromServerBuffer;
};


class ConnectorPeerSock : public ConnectorBase {
public:
    ConnectorPeerSock(const QString &code) {
        QObject::connect(&_peersockProcess, QOverload<int>::of(&QProcess::finished), [this] {

            termpaintx_ttyrescue_stop(_rescue);
            write(1, _restoreSequence.data(), _restoreSequence.size());
            tcsetattr(0, TCSAFLUSH, &_original_terminal_attributes);

            //QByteArray err = _peersockProcess.errorString().toUtf8() + "\n";
            //write(2, err.data(), err.size());
            QCoreApplication::instance()->exit(1);
        });

        QObject::connect(&_peersockProcess, &QIODevice::readyRead, [this] {
            _fromServerBuffer += _peersockProcess.read(1024*1024);
            processFramesFromServer();
        });

        QObject::connect(&_peersockProcess, &QProcess::readyReadStandardError, [this] {
            auto msg = _peersockProcess.readAllStandardError();
            _pendungMsgBuffer += msg;

            if (_pendungMsgBuffer.indexOf('\n') != -1) {
                auto msgLine = _pendungMsgBuffer.left(_pendungMsgBuffer.indexOf('\n'));
                _pendungMsgBuffer = _pendungMsgBuffer.mid(_pendungMsgBuffer.indexOf('\n') + 1);

                if (msgLine.startsWith('{')) {
                    auto msgObj = QJsonDocument::fromJson(msgLine).object();

                    if (!_connected) {
                        if (msgObj.value("event") == "code-generated") {
                            QByteArray outputMessage = "Connection Code is: " + msgObj.value("code").toString().toUtf8() + "\n";

                            write(1, outputMessage.data(), outputMessage.length());
                        }

                        if (msgObj.value("event") == "auth-success") {
                            _connected = true;
                            onConnected(&_peersockProcess);
                        }
                    }
                } else {
                    write(1, msgLine.data(), msgLine.length());
                }
            }
        });

        QStringList args = {"--json", "stdio-a"};
        if (code.size()) {
            args.append(code);
        }
        _peersockProcess.start("peersock", args);
    }

private:
    QProcess _peersockProcess;
    QByteArray _pendungMsgBuffer;
    bool _connected = false;
};

class ConnectorLocal : public ConnectorBase {
public:
    ConnectorLocal(const QString &name) {
        QObject::connect(&_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error),
            [this](QLocalSocket::LocalSocketError socketError){
            (void)socketError;

            termpaintx_ttyrescue_stop(_rescue);
            write(1, _restoreSequence.data(), _restoreSequence.size());
            tcsetattr(0, TCSAFLUSH, &_original_terminal_attributes);

            QByteArray err = _socket.errorString().toUtf8() + "\n";
            write(2, err.data(), err.size());
            QCoreApplication::instance()->exit(1);
        });



        QObject::connect(&_socket, &QLocalSocket::readyRead, [this] {
            _fromServerBuffer += _socket.read(1024*1024);
            processFramesFromServer();
        });

        QObject::connect(&_socket, &QLocalSocket::connected, [this] {
            onConnected(&_socket);
        });

        _socket.connectToServer(name);
    }

private:
    QLocalSocket _socket;
};


int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    app.setApplicationVersion("0.0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("twconnect");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("dst",
                    QCoreApplication::translate("main", "Destination to connect to. Either a connection code, or 'local:' followed by a path"));
    parser.process(app);

    QString dst = parser.positionalArguments().value(0);

    if (dst.startsWith("local:")) {
        QString path = dst.mid(6);
        if (!dst.startsWith("/")) {
            path = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + "/" + path;
        }
        auto p = path.toUtf8();
        write(2, p.data(), p.size());
        QTimer::singleShot(0, [=] {
            new ConnectorLocal(path);
        });
    } else {
        QTimer::singleShot(0, [=] {
            new ConnectorPeerSock(dst);
        });
    }

    return app.exec();
}
