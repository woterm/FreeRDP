#include "qrdpwidget.h"
#include "qrdpwork.h"

#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPainter>
#include <QTimer>
#include <QDebug>


QRdpWidget::QRdpWidget(QWidget *parent)
    : QWidget (parent)
{
    setFocusPolicy(Qt::StrongFocus);
    QMetaObject::invokeMethod(this, "reconnect", Qt::QueuedConnection);
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
    timer->start(50);
}

QRdpWidget::~QRdpWidget()
{
    QWoRdpFactory::instance()->release(m_rdp);
}

void QRdpWidget::reconnect()
{
    if(m_rdp) {
        QWoRdpFactory::instance()->release(m_rdp);
    }
    m_rdp = QWoRdpFactory::instance()->create();
    QObject::connect(m_rdp, SIGNAL(finished()), this, SLOT(onFinishArrived()));
    QDesktopWidget desk;
    QRect rt = desk.screenGeometry(this);
    int width = rt.width();
    int height = rt.height();

    m_rdp->start("192.168.10.148", width, height, "abc", "123456");
}

void QRdpWidget::onTimeout()
{
    if(m_rdp) {
        QRect rt = m_rdp->clip(size());
        if(!rt.isEmpty()){
            update(rt);
        }
    }
}

void QRdpWidget::onFinishArrived()
{
    close();
}

void QRdpWidget::closeEvent(QCloseEvent *event)
{
    emit aboutToClose(event);
    if(event->isAccepted()) {
        return;
    }
    QWidget::closeEvent(event);
}

void QRdpWidget::paintEvent(QPaintEvent *ev)
{
    if(m_rdp == nullptr) {
        return;
    }
    QPainter p(this);
    p.setClipRegion(ev->region());
    QImage img = m_rdp->capture();
    QRect drawRt = rect();
    p.drawImage(drawRt, img);
}

void QRdpWidget::mousePressEvent(QMouseEvent *ev)
{
    QWidget::mousePressEvent(ev);
    if(m_rdp){
        m_rdp->mousePressEvent(ev, size());
    }    
}

void QRdpWidget::mouseMoveEvent(QMouseEvent *ev)
{
    QWidget::mouseMoveEvent(ev);
    if(m_rdp){
        m_rdp->mouseMoveEvent(ev, size());
    }    
}

void QRdpWidget::mouseReleaseEvent(QMouseEvent *ev)
{
    QWidget::mouseReleaseEvent(ev);
    if(m_rdp){
        m_rdp->mouseReleaseEvent(ev, size());
    }
}

void QRdpWidget::wheelEvent(QWheelEvent *ev)
{
    QWidget::wheelEvent(ev);
    if(m_rdp){
        m_rdp->wheelEvent(ev, size());
    }
}

void QRdpWidget::keyPressEvent(QKeyEvent *ev)
{
    QWidget::keyPressEvent(ev);
    qDebug() << "keyPressEvent";
    if(m_rdp){
        if(ev->key() == Qt::Key_F5) {
            m_rdp->stop();
        }else{
            m_rdp->keyPressEvent(ev);
        }
    }
}

void QRdpWidget::keyReleaseEvent(QKeyEvent *ev)
{
    QWidget::keyReleaseEvent(ev);
    qDebug() << "keyReleaseEvent";
    if(m_rdp){
        m_rdp->keyReleaseEvent(ev);
    }
}

void QRdpWidget::focusInEvent(QFocusEvent *ev)
{
    QWidget::focusInEvent(ev);
    if(m_rdp) {
        m_rdp->restoreKeyboardStatus();
    }
}

void QRdpWidget::focusOutEvent(QFocusEvent *ev)
{
    QWidget::focusOutEvent(ev);
    if(m_rdp) {
        m_rdp->restoreKeyboardStatus();
    }
}
