#pragma once

#include <QWidget>
#include <QPointer>

class QRdpWork;

class QRdpWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QRdpWidget(QWidget *parent=nullptr);
    virtual ~QRdpWidget();
private:
    Q_INVOKABLE void reconnect();
signals:
    void aboutToClose(QCloseEvent* event);
private slots:
    void onTimeout();
    void onFinishArrived();
private:
    void closeEvent(QCloseEvent *event);
    void paintEvent(QPaintEvent *ev);
    void mousePressEvent(QMouseEvent* ev);
    void mouseMoveEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void wheelEvent(QWheelEvent *ev);
    void keyPressEvent(QKeyEvent *ev);
    void keyReleaseEvent(QKeyEvent *ev);
    void focusInEvent(QFocusEvent *ev);
    void focusOutEvent(QFocusEvent *ev);
private:
    QPointer<QRdpWork> m_rdp;    
};
