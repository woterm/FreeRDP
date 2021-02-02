#pragma once

#include "qrdpdef.h"

#include <QThread>
#include <QPointer>
#include <QImage>
#include <QCursor>
#include <QMutex>


struct RdpMsg;
struct WtContext;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
struct RdpMsg;

class QRDP_EXPORT QRdpWork : public QThread
{
    Q_OBJECT
public:
    // printf("    %s /u:CONTOSO\\JohnDoe /p:Pwd123! /v:rdp.contoso.com\n", name);
    struct TargetInfo {
        QString host;
        QString user;
        int port;
        QString password;
    };
public:
    explicit QRdpWork(QObject *parent=nullptr);
    virtual ~QRdpWork();
    bool hasRunning();
    void start(const QString& host, int width, int height, QString user, QString password, int port=3389);
    void start(const TargetInfo& ti, int width, int height);
    void mousePressEvent(QMouseEvent *ev, const QSize& sz);
    void mouseMoveEvent(QMouseEvent *ev, const QSize& sz);
    void mouseReleaseEvent(QMouseEvent *ev, const QSize& sz);
    void wheelEvent(QWheelEvent *ev, const QSize& sz);
    void keyPressEvent(QKeyEvent *ev);
    void keyReleaseEvent(QKeyEvent *ev);
    void restoreKeyboardStatus();
    void stop();
    QImage capture();
    uchar *buffer();
    void resetDesktop(int width, int height);
    QRect clip(const QSize& sz);
public://for internel use.
    void update(int x, int y, int width, int height);
    void updateClipboard(const QString& txt);
    bool authenticate(char** username, char** password, char** domain);
    bool gatewayAuthenticate(char** username, char** password, char** domain);
private:
    virtual void run();
    int running(void *context);
    int nativeScanCode(QKeyEvent *ev);
    void push(uchar type, const QByteArray &data=QByteArray());
    bool pop(uchar &type, QByteArray &data);
    void restoreKeyState(int code);
    void syncToggleKeyState();
    void resetCapsState(QKeyEvent *ev);    
private slots:
    void onClipboardDataChanged();
    void onClipboardUpdate(const QString& txt);
    void onDisconnected();

private:
    bool m_capson;
    TargetInfo m_ti;
    int m_width, m_height;
    QImage m_image;
    QRect m_rt;
    WtContext *m_wtc;
    QMutex m_mtx;
    QList<RdpMsg> m_queue;
    quint64 m_handle;
};

class QRDP_EXPORT QWoRdpFactory : public QObject
{
    Q_OBJECT
public:
    static QWoRdpFactory *instance();
    QRdpWork *create();
    void release(QRdpWork *obj);

private slots:
    void onFinished();
    void onAboutToQuit();
protected:
    explicit QWoRdpFactory(QObject *parent=nullptr);
    virtual ~QWoRdpFactory();
private:
    void cleanup();
private:
    QList<QPointer<QRdpWork>> m_dels;
};
