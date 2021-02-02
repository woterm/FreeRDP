#include "qrdpwork.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTime>
#include <QApplication>
#include <QMouseEvent>
#include <QTextCodec>


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/windows.h>

#include <winpr/crt.h>
#include <winpr/credui.h>

#include <freerdp/freerdp.h>
#include <freerdp/constants.h>

#include <freerdp/gdi/dc.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/gdi/gfx.h>
#include <freerdp/gdi/region.h>

#include <freerdp/codec/rfx.h>
#include <freerdp/codec/nsc.h>

#include <freerdp/client/rail.h>
#include <freerdp/client/file.h>
#include <freerdp/client/rdpei.h>

#include <freerdp/client/rdpgfx.h>
#include <freerdp/client/encomsp.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/client/channels.h>
#include <freerdp/types.h>
#include <freerdp/locale/keyboard.h>
#include <freerdp/locale/locale.h>

#include <freerdp/channels/channels.h>

#include <QImage>
#include <QPixmap>

#include "resource.h"
#ifdef Q_OS_WIN
#include <shellapi.h>
#endif
#include <QDebug>
#include <QDesktopWidget>
#include <QClipboard>
#include <QMimeData>

#define RDPMSG_KEYEVENT         (0x1)
#define RDPMSG_KEYSYNC          (0x2)
#define RDPMSG_MOUSEEVENT       (0x3)
#define RDPMSG_WHEEL            (0x6)
#define RDPMSG_PASTETEXT        (0x9)
#define RDPMSG_EXIT             (0x99)
struct RdpMsg {
    uchar type;
    QByteArray data;
};

static BOOL qt_register_pointer(rdpGraphics* graphics);
static BOOL qt_end_paint(rdpContext* context);
static BOOL qt_begin_paint(rdpContext* context);
static BOOL qt_desktop_resize(rdpContext* context);
static BOOL qt_pre_connect(freerdp* instance);
static UINT qt_cliprdr_send_client_format_list(CliprdrClientContext* cliprdr);
static BOOL qt_cliprdr_init(WtContext* wtc, CliprdrClientContext* cliprdr);
static BOOL qt_cliprdr_uninit(WtContext* wtc, CliprdrClientContext* cliprdr);


struct WtContext {
    rdpContext context;
    DEFINE_RDP_CLIENT_COMMON();

    CliprdrClientContext* cliprdr;
    QRdpWork *pwork;
    QCursor cursor;
    UINT32 requestedFormatId;
};

int qt_KeyToWinScanCode(int key, int scanCode) {
    char msg[128];
    sprintf(msg, "key:%x - code:%x", key, scanCode);
    qDebug() << msg;
    if(key >= Qt::Key_F1 && key <= Qt::Key_F10) {
        return MAKE_RDP_SCANCODE(key - Qt::Key_F1 + 0x3B, FALSE);
    }
    if(key >= Qt::Key_F13 && key <= Qt::Key_F24) {
        return MAKE_RDP_SCANCODE(key - Qt::Key_F13 + 0x64, FALSE);
    }
    if(key >= Qt::Key_1 && key <= Qt::Key_9) {
        return MAKE_RDP_SCANCODE(key - Qt::Key_1 + 0x2, FALSE);
    }
    switch (key) {
    case Qt::Key_Escape:
        return RDP_SCANCODE_ESCAPE;
    case Qt::Key_0:
        return RDP_SCANCODE_KEY_0;
    case Qt::Key_Minus:
        return RDP_SCANCODE_OEM_MINUS;
    case Qt::Key_Plus:
        return RDP_SCANCODE_ADD;
    case Qt::Key_Backspace:
        return RDP_SCANCODE_BACKSPACE;
    case Qt::Key_Tab:
        return RDP_SCANCODE_TAB;
    case Qt::Key_Q:
        return RDP_SCANCODE_KEY_Q;
    case Qt::Key_W:
        return RDP_SCANCODE_KEY_W;
    case Qt::Key_E:
        return RDP_SCANCODE_KEY_E;
    case Qt::Key_R:
        return RDP_SCANCODE_KEY_R;
    case Qt::Key_T:
        return RDP_SCANCODE_KEY_T;
    case Qt::Key_Y:
        return RDP_SCANCODE_KEY_Y;
    case Qt::Key_U:
        return RDP_SCANCODE_KEY_U;
    case Qt::Key_I:
        return RDP_SCANCODE_KEY_I;
    case Qt::Key_O:
        return RDP_SCANCODE_KEY_O;
    case Qt::Key_P:
        return RDP_SCANCODE_KEY_P;
    case Qt::Key_BracketLeft:
        return RDP_SCANCODE_OEM_4;
    case Qt::Key_BracketRight:
        return RDP_SCANCODE_OEM_6;
    case Qt::Key_Return:
        return RDP_SCANCODE_RETURN;
    case Qt::Key_Control:
#ifdef Q_OS_WIN
        if(scanCode == 0x1d) {
            return RDP_SCANCODE_LCONTROL;
        }
        return RDP_SCANCODE_RCONTROL;
#else
#ifdef Q_OS_LINUX
        if(scanCode == 0x25) {
            return RDP_SCANCODE_LCONTROL;
        }
        return RDP_SCANCODE_RCONTROL;
#else
        if(scanCode == 1) {
            return RDP_SCANCODE_LCONTROL;
        }
        return RDP_SCANCODE_RCONTROL;
#endif
#endif
    case Qt::Key_A:
        return RDP_SCANCODE_KEY_A;
    case Qt::Key_S:
        return RDP_SCANCODE_KEY_S;
    case Qt::Key_D:
        return RDP_SCANCODE_KEY_D;
    case Qt::Key_F:
        return RDP_SCANCODE_KEY_F;
    case Qt::Key_G:
        return RDP_SCANCODE_KEY_G;
    case Qt::Key_H:
        return RDP_SCANCODE_KEY_H;
    case Qt::Key_J:
        return RDP_SCANCODE_KEY_J;
    case Qt::Key_K:
        return RDP_SCANCODE_KEY_K;
    case Qt::Key_L:
        return RDP_SCANCODE_KEY_L;
    case Qt::Key_QuoteLeft:
        return RDP_SCANCODE_OEM_3;
    case Qt::Key_Shift:
#ifdef Q_OS_WIN
        if(scanCode == 0x2a) {
            return RDP_SCANCODE_LSHIFT;
        }
        return RDP_SCANCODE_RSHIFT;
#else
#ifdef Q_OS_LINUX
        if(scanCode == 0x32) {
            return RDP_SCANCODE_LSHIFT;
        }
        return RDP_SCANCODE_RSHIFT;
#else
        if(scanCode == 1) {
            return RDP_SCANCODE_LSHIFT;
        }
        return RDP_SCANCODE_RSHIFT;
#endif
#endif
    case Qt::Key_Slash:
        return RDP_SCANCODE_OEM_2;
    case Qt::Key_Z:
        return RDP_SCANCODE_KEY_Z;
    case Qt::Key_X:
        return RDP_SCANCODE_KEY_X;
    case Qt::Key_C:
        return RDP_SCANCODE_KEY_C;
    case Qt::Key_V:
        return RDP_SCANCODE_KEY_V;
    case Qt::Key_B:
        return RDP_SCANCODE_KEY_B;
    case Qt::Key_N:
        return RDP_SCANCODE_KEY_N;
    case Qt::Key_M:
        return RDP_SCANCODE_KEY_M;
    case Qt::Key_Comma:
        return RDP_SCANCODE_OEM_COMMA;
    case Qt::Key_Period:
        return RDP_SCANCODE_OEM_PERIOD;
    case Qt::Key_Backslash:
        return RDP_SCANCODE_OEM_5;
    case Qt::Key_Equal:
        return RDP_SCANCODE_OEM_PLUS;
    case Qt::Key_multiply:
        return RDP_SCANCODE_MULTIPLY;
    case Qt::Key_Alt:
#ifdef Q_OS_WIN
        if(scanCode == 0x38) {
            return RDP_SCANCODE_LMENU;
        }
        return RDP_SCANCODE_RMENU;
#else
#ifdef Q_OS_LINUX
        if(scanCode == 0x40) {
            return RDP_SCANCODE_LMENU;
        }
        return RDP_SCANCODE_RMENU;
#else
        if(scanCode == 1) {
            return RDP_SCANCODE_LMENU;
        }
        return RDP_SCANCODE_RMENU;
#endif
#endif
    case Qt::Key_Space:
        return RDP_SCANCODE_SPACE;
    case Qt::Key_CapsLock:
        return RDP_SCANCODE_CAPSLOCK;
    case Qt::Key_NumLock:
        return RDP_SCANCODE_NUMLOCK;
    case Qt::Key_ScrollLock:
        return RDP_SCANCODE_SCROLLLOCK;
    case Qt::Key_F11:
        return RDP_SCANCODE_F11;
    case Qt::Key_F12:
        return RDP_SCANCODE_F12;
    case Qt::Key_Sleep:
        return RDP_SCANCODE_SLEEP;
    case Qt::Key_Zoom:
        return RDP_SCANCODE_ZOOM;
    case Qt::Key_Help:
        return RDP_SCANCODE_HELP;
    case Qt::Key_division:
        return RDP_SCANCODE_DIVIDE;
    case Qt::Key_Pause:
        return RDP_SCANCODE_PAUSE;
    case Qt::Key_Home:
        return RDP_SCANCODE_HOME;
    case Qt::Key_Up:
        return RDP_SCANCODE_UP;
    case Qt::Key_PageUp:
        return RDP_SCANCODE_PRIOR;
    case Qt::Key_Left:
        return RDP_SCANCODE_LEFT;
    case Qt::Key_Right:
        return RDP_SCANCODE_RIGHT;
    case Qt::Key_End:
        return RDP_SCANCODE_END;
    case Qt::Key_Down:
        return RDP_SCANCODE_DOWN;
    case Qt::Key_PageDown:
        return RDP_SCANCODE_NEXT;
    case Qt::Key_Insert:
        return RDP_SCANCODE_INSERT;
    case Qt::Key_Delete:
        return RDP_SCANCODE_DELETE;
    case Qt::Key_Meta:
#ifdef Q_OS_WIN
        if(scanCode == 0x15b) {
            return RDP_SCANCODE_LWIN;
        }
        return RDP_SCANCODE_RWIN;
#else
#ifdef Q_OS_LINUX
        if(scanCode == 0x85) {
            return RDP_SCANCODE_LWIN;
        }
        return RDP_SCANCODE_RWIN;
#else
        if(scanCode == 1) {
            return RDP_SCANCODE_LWIN;
        }
        return RDP_SCANCODE_RWIN;
#endif
#endif
    case Qt::Key_Semicolon:
        return RDP_SCANCODE_OEM_1;
    case Qt::Key_Apostrophe:
        return RDP_SCANCODE_OEM_7;
    case Qt::Key_Less:
        return RDP_SCANCODE_OEM_COMMA;
    case Qt::Key_Greater:
        return RDP_SCANCODE_OEM_PERIOD;
    case Qt::Key_Question:
        return RDP_SCANCODE_OEM_2;
    case Qt::Key_QuoteDbl:
        return RDP_SCANCODE_OEM_7;
    case Qt::Key_Colon:
        return RDP_SCANCODE_OEM_1;
    case Qt::Key_BraceLeft:
        return RDP_SCANCODE_OEM_4;
    case Qt::Key_BraceRight:
        return RDP_SCANCODE_OEM_6;
    case Qt::Key_Bar:
        return RDP_SCANCODE_OEM_5;
    case Qt::Key_Underscore:
        return RDP_SCANCODE_OEM_MINUS;
    case Qt::Key_ParenLeft:
        return RDP_SCANCODE_KEY_9;
    case Qt::Key_ParenRight:
        return RDP_SCANCODE_KEY_0;
    case Qt::Key_Asterisk:
#ifdef Q_OS_WIN
        if(scanCode == 0x37) {
            return RDP_SCANCODE_MULTIPLY;
        }
        return RDP_SCANCODE_KEY_8;
#else
#ifdef Q_OS_LINUX
        if(scanCode == 0x11) {
            return RDP_SCANCODE_MULTIPLY;
        }
        return RDP_SCANCODE_KEY_8;
#else
        if(scanCode == 1) {
            return RDP_SCANCODE_LWIN;
        }
        return RDP_SCANCODE_RWIN;
#endif
#endif
    case Qt::Key_Ampersand:
        return RDP_SCANCODE_KEY_7;
    case Qt::Key_AsciiCircum:
        return RDP_SCANCODE_KEY_6;
    case Qt::Key_Percent:
        return RDP_SCANCODE_KEY_5;
    case Qt::Key_Dollar:
        return RDP_SCANCODE_KEY_4;
    case Qt::Key_NumberSign:
        return RDP_SCANCODE_KEY_3;
    case Qt::Key_At:
        return RDP_SCANCODE_KEY_2;
    case Qt::Key_Exclam:
        return RDP_SCANCODE_KEY_1;
    case Qt::Key_AsciiTilde:
        return RDP_SCANCODE_OEM_3;
    }
    return RDP_SCANCODE_UNKNOWN;
}

/* channel */
void qt_OnChannelConnectedEventHandler(void* context, ChannelConnectedEventArgs* e)
{
    rdpContext* c = (rdpContext*)context;
    rdpSettings* settings = c->settings;

    if (strcmp(e->name, RDPGFX_DVC_CHANNEL_NAME) == 0) {
        gdi_graphics_pipeline_init(c->gdi, (RdpgfxClientContext*)e->pInterface);
    }else if (strcmp(e->name, CLIPRDR_SVC_CHANNEL_NAME) == 0)
    {
        WtContext *wtc = (WtContext*)c;
        qt_cliprdr_init(wtc, (CliprdrClientContext*)e->pInterface);
    }
}

void qt_OnChannelDisconnectedEventHandler(void* context, ChannelDisconnectedEventArgs* e)
{
    rdpContext* c = (rdpContext*)context;
    rdpSettings* settings = c->settings;

    if (strcmp(e->name, RDPGFX_DVC_CHANNEL_NAME) == 0) {
        gdi_graphics_pipeline_uninit(c->gdi, (RdpgfxClientContext*)e->pInterface);
    }
}

static BOOL qt_mouse_event(rdpContext* context, rdpInput* input, UINT16 flags, UINT16 x, UINT16 y)
{
    MouseEventEventArgs eventArgs;

    if (freerdp_input_send_mouse_event(input, flags, x, y)){
        return FALSE;
    }

    eventArgs.flags = flags;
    eventArgs.x = x;
    eventArgs.y = y;
    PubSub_OnMouseEvent(context->pubSub, context, &eventArgs);
    return TRUE;
}

static int idxcall = 0;
static UINT qt_cliprdr_send_client_format_data_request(CliprdrClientContext* cliprdr, UINT32 formatId)
{
    qDebug() << "qt_cliprdr_send_client_format_data_request" << idxcall++;
    if (!cliprdr){
        return ERROR_INVALID_PARAMETER;
    }
    WtContext *wtc = (WtContext*)cliprdr->custom;
    if (!wtc || !cliprdr->ClientFormatDataRequest){
        return ERROR_INVALID_PARAMETER;
    }

    CLIPRDR_FORMAT_DATA_REQUEST formatDataRequest;
    ZeroMemory(&formatDataRequest, sizeof(CLIPRDR_FORMAT_DATA_REQUEST));
    formatDataRequest.msgType = CB_FORMAT_DATA_REQUEST;
    formatDataRequest.msgFlags = 0;
    formatDataRequest.requestedFormatId = formatId;
    wtc->requestedFormatId = formatId;
    return cliprdr->ClientFormatDataRequest(cliprdr, &formatDataRequest);
}

static UINT qt_cliprdr_send_client_capabilities(CliprdrClientContext* cliprdr)
{
    qDebug() << "qt_cliprdr_send_client_capabilities" << idxcall++;
    CLIPRDR_CAPABILITIES capabilities;
    CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet;

    if (!cliprdr || !cliprdr->ClientCapabilities){
        return ERROR_INVALID_PARAMETER;
    }

    capabilities.cCapabilitiesSets = 1;
    capabilities.capabilitySets = (CLIPRDR_CAPABILITY_SET*)&(generalCapabilitySet);
    generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
    generalCapabilitySet.capabilitySetLength = 12;
    generalCapabilitySet.version = CB_CAPS_VERSION_2;
    generalCapabilitySet.generalFlags = CB_USE_LONG_FORMAT_NAMES;
    return cliprdr->ClientCapabilities(cliprdr, &capabilities);
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_monitor_ready(CliprdrClientContext* cliprdr, const CLIPRDR_MONITOR_READY* monitorReady)
{
    qDebug() << "qt_cliprdr_monitor_ready" << idxcall++;
    int err = qt_cliprdr_send_client_capabilities(cliprdr);
    if(err != CHANNEL_RC_OK) {
        return err;
    }
    err = qt_cliprdr_send_client_format_list(cliprdr);
    if(err != CHANNEL_RC_OK) {
        return err;
    }
    return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_capabilities(CliprdrClientContext* cliprdr, const CLIPRDR_CAPABILITIES* capabilities)
{
    qDebug() << "qt_cliprdr_server_capabilities" << idxcall++;
    return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_format_list(CliprdrClientContext* cliprdr, const CLIPRDR_FORMAT_LIST* formatList)
{
    CLIPRDR_FORMAT* format;

    qDebug() << "qt_cliprdr_server_format_list" << idxcall++;
    if (!cliprdr || !formatList){
        return ERROR_INVALID_PARAMETER;
    }
    WtContext* wtc = (WtContext*)cliprdr->custom;

    if (!wtc){
        return ERROR_INVALID_PARAMETER;
    }

    for (int i = 0; i < formatList->numFormats; i++) {
        CLIPRDR_FORMAT *format = &(formatList->formats[i]);
        if (format->formatId == CF_UNICODETEXT) {
            int rc = qt_cliprdr_send_client_format_data_request(cliprdr, CF_UNICODETEXT);
            if (rc != CHANNEL_RC_OK){
                return rc;
            }
            break;
        } else if (format->formatId == CF_TEXT) {
            int rc = qt_cliprdr_send_client_format_data_request(cliprdr, CF_TEXT);
            if (rc != CHANNEL_RC_OK){
                return rc;
            }
            break;
        }
    }
    return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_format_list_response(CliprdrClientContext* cliprdr, const CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
    qDebug() << "qt_cliprdr_server_format_list_response" << idxcall++;
    if (!cliprdr || !formatListResponse){
        return ERROR_INVALID_PARAMETER;
    }

    return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_lock_clipboard_data(CliprdrClientContext* cliprdr, const CLIPRDR_LOCK_CLIPBOARD_DATA* lockClipboardData)
{
    qDebug() << "qt_cliprdr_server_lock_clipboard_data" << idxcall++;
    if (!cliprdr || !lockClipboardData){
        return ERROR_INVALID_PARAMETER;
    }

    return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_unlock_clipboard_data(CliprdrClientContext* cliprdr, const CLIPRDR_UNLOCK_CLIPBOARD_DATA* unlockClipboardData)
{
    qDebug() << "qt_cliprdr_server_unlock_clipboard_data" << idxcall++;
    if (!cliprdr || !unlockClipboardData){
        return ERROR_INVALID_PARAMETER;
    }

    return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_format_data_request(CliprdrClientContext* cliprdr, const CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
    qDebug() << "qt_cliprdr_server_format_data_request" << idxcall++;
    UINT32 formatId = formatDataRequest->requestedFormatId;
    QByteArray buf;
    QClipboard *clipboard = QGuiApplication::clipboard();
    const QString txt = clipboard->text();
    if(formatId == CF_UNICODETEXT) {
        std::wstring wstr = txt.toStdWString();
        buf.append((char*)wstr.data(), wstr.length() * 2);
    }else if(formatId == CF_TEXT){
        std::string str = txt.toStdString();
        buf.append(str.data(), str.length());
    }
    CLIPRDR_FORMAT_DATA_RESPONSE response;
    response.msgFlags = CB_RESPONSE_OK;
    response.dataLen = buf.size();
    response.requestedFormatData = (BYTE*)buf.data();
    return cliprdr->ClientFormatDataResponse(cliprdr, &response);
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_format_data_response(CliprdrClientContext* cliprdr, const CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
    qDebug() << "qt_cliprdr_server_format_data_response" << idxcall++;
    if (!cliprdr || !formatDataResponse){
        return ERROR_INVALID_PARAMETER;
    }
    WtContext* wtc = (WtContext*)cliprdr->custom;

    if (!wtc){
        return ERROR_INVALID_PARAMETER;
    }

    UINT32 formatId = wtc->requestedFormatId;
    UINT32 size = formatDataResponse->dataLen;
    if(size > 0) {
        qDebug() << "data can paste response" << size;
    }
    QString content;
    if(formatId == CF_UNICODETEXT) {
        QString txt = QString::fromWCharArray((wchar_t*)formatDataResponse->requestedFormatData, size);
        content.append(txt);
    }else if(formatId == CF_TEXT) {
        QByteArray txt((char*)formatDataResponse->requestedFormatData, size);
        content.append(txt);
    }
    if(!content.isEmpty()) {
        wtc->pwork->updateClipboard(content);
    }
    return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_file_contents_request(CliprdrClientContext* cliprdr, const CLIPRDR_FILE_CONTENTS_REQUEST* fileContentsRequest)
{
    qDebug() << "qt_cliprdr_server_file_contents_request" << idxcall++;
    if (!cliprdr || !fileContentsRequest)
        return ERROR_INVALID_PARAMETER;

    return CHANNEL_RC_OK;
}

/**
 * Function description
 *
 * @return 0 on success, otherwise a Win32 error code
 */
static UINT qt_cliprdr_server_file_contents_response(CliprdrClientContext* cliprdr, const CLIPRDR_FILE_CONTENTS_RESPONSE* fileContentsResponse)
{
    qDebug() << "qt_cliprdr_server_file_contents_response" << idxcall++;
    if (!cliprdr || !fileContentsResponse)
        return ERROR_INVALID_PARAMETER;

    return CHANNEL_RC_OK;
}

UINT qt_cliprdr_send_client_format_list(CliprdrClientContext* cliprdr)
{
    qDebug() << "qt_cliprdr_send_client_format_list" << idxcall++;
    if (!cliprdr){
        return ERROR_INVALID_PARAMETER;
    }

    WtContext* wtc = (WtContext*)cliprdr->custom;

    if (!wtc || !wtc->cliprdr) {
        return ERROR_INVALID_PARAMETER;
    }

    CLIPRDR_FORMAT formats[10]={0};
    CLIPRDR_FORMAT_LIST formatList;
    ZeroMemory(&formatList, sizeof(CLIPRDR_FORMAT_LIST));
    QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if(!mimeData->hasText()) {
        return CHANNEL_RC_OK;
    }
    formats[0].formatId = CF_TEXT;
    formats[0].formatName = NULL;
    formats[1].formatId = CF_UNICODETEXT;
    formats[1].formatName = NULL;

    formatList.msgFlags = CB_RESPONSE_OK;
    formatList.numFormats = 2;
    formatList.formats = formats;
    formatList.msgType = CB_FORMAT_LIST;
    if(!wtc->cliprdr->ClientFormatList) {
        return ERROR_INTERNAL_ERROR;
    }
    UINT rc = wtc->cliprdr->ClientFormatList(wtc->cliprdr, &formatList);
    return rc;
}

BOOL qt_cliprdr_init(WtContext* wtc, CliprdrClientContext* cliprdr)
{
    wtc->cliprdr = cliprdr;
    cliprdr->custom = wtc;
    cliprdr->MonitorReady = qt_cliprdr_monitor_ready;
    cliprdr->ServerCapabilities = qt_cliprdr_server_capabilities;
    cliprdr->ServerFormatList = qt_cliprdr_server_format_list;
    cliprdr->ServerFormatListResponse = qt_cliprdr_server_format_list_response;
    cliprdr->ServerLockClipboardData = qt_cliprdr_server_lock_clipboard_data;
    cliprdr->ServerUnlockClipboardData = qt_cliprdr_server_unlock_clipboard_data;
    cliprdr->ServerFormatDataRequest = qt_cliprdr_server_format_data_request;
    cliprdr->ServerFormatDataResponse = qt_cliprdr_server_format_data_response;
    cliprdr->ServerFileContentsRequest = qt_cliprdr_server_file_contents_request;
    cliprdr->ServerFileContentsResponse = qt_cliprdr_server_file_contents_response;
    return TRUE;
}

BOOL qt_cliprdr_uninit(WtContext* wtc, CliprdrClientContext* cliprdr)
{
    return TRUE;
}

/*   Pointer */
static BOOL qt_Pointer_New(rdpContext* context, const rdpPointer* pointer)
{
    if (!context || !pointer){
        return FALSE;
    }
#if 0
    WtContext *wtc = (WtContext*)context;
    int width = pointer->width;
    int height = pointer->height;
    int xhot = pointer->xPos;
    int yhot = pointer->yPos;
    QImage img = QImage(width, height, QImage::Format_ARGB32);
    if (!freerdp_image_copy_from_pointer_data((BYTE*)img.bits(), PIXEL_FORMAT_ARGB32, 0, 0, 0, pointer->width, pointer->height,
                                              pointer->xorMaskData, pointer->lengthXorMask, pointer->andMaskData,
                                              pointer->lengthAndMask, pointer->xorBpp, &context->gdi->palette)) {
        return FALSE;
    }
    wtc->cursor = QCursor(QPixmap::fromImage(img), xhot, yhot);
#endif
    return TRUE;
}

static BOOL qt_Pointer_Free(rdpContext* context, rdpPointer* pointer)
{
    if (!context || !pointer){
        return FALSE;
    }

    return TRUE;
}

static BOOL qt_Pointer_Set(rdpContext* context, const rdpPointer* pointer)
{
    WtContext *wtc = (WtContext*)context;
    if (!context || !pointer){
        return FALSE;
    }
    //wtc->pwork->setCursor(wtc->cursor);
    return TRUE;
}

static BOOL qt_Pointer_SetNull(rdpContext* context)
{
    WtContext *wtc = (WtContext*)context;
    if (!context){
        return FALSE;
    }
    return TRUE;
}

static BOOL qt_Pointer_SetDefault(rdpContext* context)
{
    WtContext *wtc = (WtContext*)context;
    if (!context){
        return FALSE;
    }
    return TRUE;
}

static BOOL qt_Pointer_SetPosition(rdpContext* context, UINT32 x, UINT32 y)
{
    WtContext *wtc = (WtContext*)context;
    if (!context){
        return FALSE;
    }
    return TRUE;
}

static BOOL qt_register_pointer(rdpGraphics* graphics)
{
    rdpPointer pointer;

    if (!graphics){
        return FALSE;
    }

    ZeroMemory(&pointer, sizeof(rdpPointer));
    pointer.size = sizeof(rdpPointer);
    pointer.New = (pPointer_New)qt_Pointer_New;
    pointer.Free = (pPointer_Free)qt_Pointer_Free;
    pointer.Set = (pPointer_Set)qt_Pointer_Set;
    pointer.SetNull = qt_Pointer_SetNull;
    pointer.SetDefault = qt_Pointer_SetDefault;
    pointer.SetPosition = qt_Pointer_SetPosition;
    graphics_register_pointer(graphics, &pointer);
    return TRUE;
}

static BOOL qt_end_paint(rdpContext* context)
{
    WtContext *wtc = (WtContext*)context;
    rdpGdi* gdi;
    int ninvalid;
    HGDI_RGN cinvalid;
    REGION16 invalidRegion;
    RECTANGLE_16 invalidRect;
    const RECTANGLE_16* extents;
    gdi = context->gdi;
    ninvalid = gdi->primary->hdc->hwnd->ninvalid;
    cinvalid = gdi->primary->hdc->hwnd->cinvalid;

    if (ninvalid < 1)
        return TRUE;

    region16_init(&invalidRegion);

    for (int i = 0; i < ninvalid; i++)
    {
        invalidRect.left = cinvalid[i].x;
        invalidRect.top = cinvalid[i].y;
        invalidRect.right = cinvalid[i].x + cinvalid[i].w;
        invalidRect.bottom = cinvalid[i].y + cinvalid[i].h;
        region16_union_rect(&invalidRegion, &invalidRegion, &invalidRect);
    }

    if (!region16_is_empty(&invalidRegion))
    {
        extents = region16_extents(&invalidRegion);
        int left = extents->left;
        int top = extents->top;
        int right = extents->right;
        int bottom = extents->bottom;
        wtc->pwork->update(left, top, right - left, bottom - top);
    }
    region16_uninit(&invalidRegion);
    return TRUE;
}

static BOOL qt_begin_paint(rdpContext* context)
{
    HGDI_DC hdc;

    if (!context || !context->gdi || !context->gdi->primary || !context->gdi->primary->hdc)
        return FALSE;

    hdc = context->gdi->primary->hdc;

    if (!hdc || !hdc->hwnd || !hdc->hwnd->invalid)
        return FALSE;

    hdc->hwnd->invalid->null = TRUE;
    hdc->hwnd->ninvalid = 0;
    return TRUE;
}

static BOOL qt_desktop_resize(rdpContext* context)
{
    BOOL same;
    rdpSettings* settings;

    if (!context || !context->settings)
        return FALSE;

    settings = context->settings;

    return TRUE;
}

static BOOL qt_pre_connect(freerdp* instance)
{
    int desktopWidth;
    int desktopHeight;
    rdpContext* context;
    rdpSettings* settings;

    if (!instance || !instance->context || !instance->settings){
        return FALSE;
    }

    context = instance->context;
    settings = instance->settings;
    settings->OsMajorType = OSMAJORTYPE_WINDOWS;
    settings->OsMinorType = OSMINORTYPE_WINDOWS_NT;
    desktopWidth = settings->DesktopWidth;
    desktopHeight = settings->DesktopHeight;

    if (!freerdp_client_load_addins(context->channels, instance->settings)){
        return -1;
    }
#ifdef Q_OS_WIN
    freerdp_set_param_uint32(settings, FreeRDP_KeyboardLayout, (int)GetKeyboardLayout(0) & 0x0000FFFF);
#endif
    PubSub_SubscribeChannelConnected(instance->context->pubSub, qt_OnChannelConnectedEventHandler);
    PubSub_SubscribeChannelDisconnected(instance->context->pubSub, qt_OnChannelDisconnectedEventHandler);
    return TRUE;
}

static BOOL qt_post_connect(freerdp* instance)
{
    rdpContext* context = instance->context;
    WtContext *wtc = (WtContext*)context;
    rdpSettings* settings = instance->settings;
    int width = settings->DesktopWidth;
    int height = settings->DesktopHeight;
    wtc->pwork->resetDesktop(width, height);
    QImage img = wtc->pwork->capture();
    if (!gdi_init_ex(instance, PIXEL_FORMAT_RGB16, img.bytesPerLine(), wtc->pwork->buffer(), NULL)){
        return FALSE;
    }
    instance->update->BeginPaint = qt_begin_paint;
    instance->update->DesktopResize = qt_desktop_resize;
    instance->update->EndPaint = qt_end_paint;
    qt_register_pointer(context->graphics);
    return TRUE;
}

static void qt_post_disconnect(freerdp* instance)
{
    if (!instance || !instance->context || !instance->settings)
        return;
}

static BOOL qt_authenticate(freerdp* instance, char** username, char** password, char** domain)
{
    //rdpContext* context = instance->context;
    //WtContext *wtc = (WtContext*)context;
    //return wtc->pwork->authenticate(username, password, domain);
    return TRUE;
}

static BOOL qt_gw_authenticate(freerdp* instance, char** username, char** password, char** domain)
{
    //rdpContext* context = instance->context;
    //WtContext *wtc = (WtContext*)context;
    //return wtc->pwork->gatewayAuthenticate(username, password, domain);
    return TRUE;
}

static DWORD qt_verify_certificate_ex(freerdp* instance, const char* host, UINT16 port,
                                      const char* common_name, const char* subject,
                                      const char* issuer, const char* fingerprint, DWORD flags)
{
    WCHAR* buffer;
    WCHAR* caption;
    int what = IDCANCEL;

    return 1;
}

static DWORD qt_verify_changed_certificate_ex(freerdp* instance, const char* host, UINT16 port,
                                              const char* common_name, const char* subject,
                                              const char* issuer, const char* new_fingerprint,
                                              const char* old_subject, const char* old_issuer,
                                              const char* old_fingerprint, DWORD flags)
{
    WCHAR* buffer;
    WCHAR* caption;
    int what = IDCANCEL;

    return 1;
}


static BOOL qfreerdp_client_global_init(void)
{
#ifdef Q_OS_WIN
    WSADATA wsaData;
    WSAStartup(0x101, &wsaData);
#endif
    return TRUE;
}

static void qfreerdp_client_global_uninit(void)
{
#ifdef Q_OS_WIN
    WSACleanup();
#endif
}

static BOOL qfreerdp_client_new(freerdp* instance, rdpContext* context)
{
    WtContext * wtc = (WtContext*)context;
    if (!(qfreerdp_client_global_init()))
        return FALSE;

    instance->PreConnect = qt_pre_connect;
    instance->PostConnect = qt_post_connect;
    instance->PostDisconnect = qt_post_disconnect;
    instance->Authenticate = qt_authenticate;
    instance->GatewayAuthenticate = qt_gw_authenticate;
    instance->VerifyCertificateEx = qt_verify_certificate_ex;
    instance->VerifyChangedCertificateEx = qt_verify_changed_certificate_ex;

    return TRUE;
}

static void qfreerdp_client_free(freerdp* instance, rdpContext* context)
{
    if (!context)
        return;
}

static int qfreerdp_client_start(rdpContext* context)
{
    freerdp* instance = context->instance;

    return 0;
}

static int qfreerdp_client_stop(rdpContext* context)
{
    return 0;
}

static int RdpClientEntry(RDP_CLIENT_ENTRY_POINTS* pEntryPoints)
{
    pEntryPoints->Version = 1;
    pEntryPoints->Size = sizeof(RDP_CLIENT_ENTRY_POINTS_V1);
    pEntryPoints->GlobalInit = qfreerdp_client_global_init;
    pEntryPoints->GlobalUninit = qfreerdp_client_global_uninit;
    pEntryPoints->ContextSize = sizeof(WtContext);
    pEntryPoints->ClientNew = qfreerdp_client_new;
    pEntryPoints->ClientFree = qfreerdp_client_free;
    pEntryPoints->ClientStart = qfreerdp_client_start;
    pEntryPoints->ClientStop = qfreerdp_client_stop;
    return 0;
}


QRdpWork::QRdpWork(QObject *parent)
    : QThread(parent)
{
    m_capson = false;
    m_handle = (quint64)CreateEvent(NULL, TRUE, TRUE, NULL);
    QObject::connect(this, SIGNAL(finished()), this, SLOT(onDisconnected()));
}

QRdpWork::~QRdpWork()
{
    CloseHandle((HANDLE)m_handle);
}

bool QRdpWork::hasRunning()
{
    return isRunning();
}

void QRdpWork::start(const QString &host, int width, int height, QString user, QString password, int port)
{
    m_ti.host = host.toUtf8();
    m_ti.port = port;
    m_ti.user = user;
    m_ti.password = password;
    resetDesktop(width, height);
    QThread::start();
}

void QRdpWork::start(const QRdpWork::TargetInfo &ti, int width, int height)
{
    m_ti = ti;
    resetDesktop(width, height);
    QThread::start();
}

void QRdpWork::mousePressEvent(QMouseEvent *ev, const QSize& sz)
{
    QPoint pt = ev->pos();
    //qDebug() << "mousePressEvent" << pt;
    int x = pt.x() * m_width / sz.width();
    int y = pt.y() * m_height / sz.height();
    int flags = PTR_FLAGS_DOWN;
    switch (ev->button()) {
    case Qt::LeftButton:
        flags |= PTR_FLAGS_BUTTON1;
        break;
    case Qt::RightButton:
        flags |= PTR_FLAGS_BUTTON2;
        break;
    case Qt::MiddleButton:
        flags |= PTR_FLAGS_BUTTON3;
        break;
    default:
        break;
    }

    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << flags << x << y;
    push(RDPMSG_MOUSEEVENT, buf);
}

void QRdpWork::mouseMoveEvent(QMouseEvent *ev, const QSize& sz)
{
    QPoint pt = ev->pos();
    int x = pt.x() * m_width / sz.width();
    int y = pt.y() * m_height / sz.height();
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    int flags = PTR_FLAGS_MOVE;
    ds << flags << x << y;
    push(RDPMSG_MOUSEEVENT, buf);
    //rdpInput* input = m_wtc->context.input;
    //qt_mouse_event(m_wtc, input, PTR_FLAGS_MOVE, x, y);
}

void QRdpWork::mouseReleaseEvent(QMouseEvent *ev, const QSize& sz)
{
    QPoint pt = ev->pos();
    int x = pt.x() * m_width / sz.width();
    int y = pt.y() * m_height / sz.height();
    int flags = 0;
    switch (ev->button()) {
    case Qt::LeftButton:
        flags |= PTR_FLAGS_BUTTON1;
        break;
    case Qt::RightButton:
        flags |= PTR_FLAGS_BUTTON2;
        break;
    case Qt::MiddleButton:
        flags |= PTR_FLAGS_BUTTON3;
        break;
    default:
        break;
    }
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << flags << x << y;
    push(RDPMSG_MOUSEEVENT, buf);
}

void QRdpWork::wheelEvent(QWheelEvent *ev, const QSize& sz)
{
    QPoint pt = ev->pos();
    int x = pt.x() * m_width / sz.width();
    int y = pt.y() * m_height / sz.height();

    int flags = PTR_FLAGS_WHEEL;
    int delta = ev->delta();
    if (delta < 0) {
        flags |= PTR_FLAGS_WHEEL_NEGATIVE;
        delta = -delta;
    }
    flags |= delta;
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << flags << x << y;
    push(RDPMSG_MOUSEEVENT, buf);
}



void QRdpWork::keyPressEvent(QKeyEvent *ev)
{
    int code = nativeScanCode(ev);
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << true << code;
    push(RDPMSG_KEYEVENT, buf);
    resetCapsState(ev);
    if(code == RDP_SCANCODE_CAPSLOCK) {
        syncToggleKeyState();
    }
}

void QRdpWork::keyReleaseEvent(QKeyEvent *ev)
{
    int code = nativeScanCode(ev);
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << false << code;
    push(RDPMSG_KEYEVENT, buf);
    if(code == RDP_SCANCODE_CAPSLOCK) {
        syncToggleKeyState();
    }
}

void QRdpWork::restoreKeyboardStatus()
{
    restoreKeyState(RDP_SCANCODE_LCONTROL);
    restoreKeyState(RDP_SCANCODE_RCONTROL);
    restoreKeyState(RDP_SCANCODE_LSHIFT);
    restoreKeyState(RDP_SCANCODE_RSHIFT);
    restoreKeyState(RDP_SCANCODE_LMENU);
    restoreKeyState(RDP_SCANCODE_RMENU);
}


void QRdpWork::restoreKeyState(int code)
{
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << false << code;
    push(RDPMSG_KEYEVENT, buf);
}

void QRdpWork::resetCapsState(QKeyEvent *ev)
{
    if(ev->key() < Qt::Key_A || ev->key() > Qt::Key_Z) {
        return;
    }
    QString key = ev->text();
    if(key.length() != 1) {
        return;
    }
    int v = key.toStdString().at(0);
    if(ev->modifiers() & Qt::ShiftModifier) {
        m_capson = v >= 'a' && v <= 'z';
    }else{
        m_capson = v >= 'A' && v <= 'Z';
    }
    qDebug() << "m_capson" << key << m_capson;
}

void QRdpWork::resetDesktop(int width, int height)
{
    QMutexLocker mtx(&m_mtx);
    /* FIXME: desktopWidth has a limitation that it should be divisible by 4,
     *        otherwise the screen will crash when connecting to an XP desktop.*/
    width = (width + 3) & (~3);
    m_width = width;
    m_height = height;
    m_image = QImage(width, height, QImage::Format_RGB16);
    m_image.fill(Qt::black);
}

#ifdef Q_OS_WIN
#define IsKeyToggled(nVirtKey)  ((GetKeyState(nVirtKey) & 1) != 0)
void QRdpWork::syncToggleKeyState() {
    int syncFlags = 0;
    if (IsKeyToggled(VK_NUMLOCK)){
        syncFlags |= KBD_SYNC_NUM_LOCK;
    }
    if (IsKeyToggled(VK_CAPITAL)){
        syncFlags |= KBD_SYNC_CAPS_LOCK;
    }
    if (IsKeyToggled(VK_SCROLL)){
        syncFlags |= KBD_SYNC_SCROLL_LOCK;
    }
    if (IsKeyToggled(VK_KANA)){
        syncFlags |= KBD_SYNC_KANA_LOCK;
    }
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << syncFlags;
    push(RDPMSG_KEYSYNC, buf);
}

#else

void QRdpWork::syncToggleKeyState() {
    int syncFlags = 0;
    if(!m_capson) {
        syncFlags |= KBD_SYNC_CAPS_LOCK;
    }
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds << syncFlags;
    push(RDPMSG_KEYSYNC, buf);
}
#endif

void QRdpWork::stop()
{
    for(int i = 0; i < 50; i++) {
        push(RDPMSG_EXIT);
    }
    qDebug() << "left" << left;
    //freerdp_abort_connect(m_wtc->context.instance);
}

void QRdpWork::push(uchar type, const QByteArray &data)
{
    QMutexLocker locker(&m_mtx);
    RdpMsg tmp;
    tmp.type = type;
    tmp.data = data;
    m_queue.append(tmp);
    //qDebug() <<  "push" << m_queue.length() << type << data;
    //::send(m_msgWrite, (char*)&type, 1, 0);
    SetEvent((HANDLE)m_handle);
}

bool QRdpWork::pop(uchar &type, QByteArray &data)
{
    QMutexLocker locker(&m_mtx);
    if(m_queue.isEmpty()) {
        return false;
    }
    RdpMsg tmp = m_queue.takeFirst();
    type = tmp.type;
    data = tmp.data;
    //qDebug() <<  "pop" << m_queue.length() << type << data;
    return true;
}


QImage QRdpWork::capture()
{
    QMutexLocker mtx(&m_mtx);
    return m_image;
}

uchar *QRdpWork::buffer()
{
    QMutexLocker mtx(&m_mtx);
    return m_image.bits();
}

QRect QRdpWork::clip(const QSize& sz)
{
    int x = m_rt.x() * sz.width() / m_width;
    int y = m_rt.y() * sz.height() / m_height;
    int w = m_rt.width() * sz.width() / m_width;
    int h = m_rt.height() * sz.height() / m_height;
    m_rt = QRect();
    return QRect(x-2, y-2, w+4, h+4);
}

void QRdpWork::update(int x, int y, int width, int height)
{
    m_rt |= QRect(x, y, width, height);
}

void QRdpWork::updateClipboard(const QString &txt)
{
    QMetaObject::invokeMethod(this, "onClipboardUpdate", Qt::QueuedConnection, Q_ARG(QString, txt));
}

bool QRdpWork::authenticate(char **username, char **password, char **domain)
{
    return true;
}

bool QRdpWork::gatewayAuthenticate(char **username, char **password, char **domain)
{
    return true;
}

void QRdpWork::run()
{
    RDP_CLIENT_ENTRY_POINTS rdpEntry={0};
    RdpClientEntry(&rdpEntry);
    rdpContext* context = freerdp_client_context_new(&rdpEntry);
    if(context == nullptr) {
        return;
    }
    m_wtc = (WtContext*)context;
    m_wtc->pwork = this;
    rdpSettings* settings = context->settings;
    freerdp *instance = context->instance;
    QByteArray path = QDir::toNativeSeparators(QApplication::instance()->applicationFilePath()).toUtf8();
    QByteArray host = QString("/v:%1").arg(m_ti.host).toUtf8();
    QByteArray port = QString("/port:%1").arg(m_ti.port).toUtf8();
    QByteArray user = QString("/u:%1").arg(m_ti.user).toUtf8();
    QByteArray pass = QString("/p:%1").arg(m_ti.password).toUtf8();
    QByteArray width = QString("/w:%1").arg(m_width).toUtf8();
    QByteArray height = QString("/h:%1").arg(m_height).toUtf8();
    char *argv[] = {path.data(), host.data(), port.data(), user.data(), pass.data(), width.data(), height.data()};
    int argc = sizeof(argv) / sizeof(char*);
    int status = freerdp_client_settings_parse_command_line(settings, argc, argv, FALSE);
    if (status) {
        freerdp_client_settings_command_line_status_print(settings, status, argc, argv);
        return;
    }
    if (freerdp_client_start(context) != 0){
        freerdp_client_context_free(context);
        return;
    }

    running(context);

    freerdp_client_stop(context);
    freerdp_client_context_free(context);
}

int QRdpWork::running(void *c)
{
    rdpContext* context = (rdpContext*)c;
    WtContext *wtc = (WtContext*)context;
    rdpSettings* settings = context->settings;
    freerdp *instance = context->instance;
    int itick = 0;

    if(!freerdp_connect(instance)) {
        int result = freerdp_get_last_error(instance->context);
        qDebug() << "connection failure 0x%08" << result;
        return result;
    }
    if (instance->settings->AuthenticationOnly) {
        int result = freerdp_get_last_error(instance->context);
        qDebug() << "Authentication only, exit status:" << result;
        freerdp_abort_connect(instance);
        freerdp_disconnect(instance);
        return -1;
    }

    while (!freerdp_shall_disconnect(instance)) {
        if (freerdp_focus_required(instance)){
            restoreKeyboardStatus();
            qDebug() << "focus required";
        }

        HANDLE handles[64] = {0};
        int nCount = freerdp_get_event_handles(instance->context, &handles[0], 64);
        if (nCount == 0) {
            qDebug() << "freerdp_get_event_handles failed" << __FUNCTION__;
            break;
        }

        handles[nCount] = (HANDLE)m_handle;
        nCount++;
        int status = WaitForMultipleObjects(nCount, handles, FALSE, 100);

        if (status == WAIT_FAILED) {
            qDebug() << "WaitForMultipleObjects failed with " << PRIu32 << __FUNCTION__ << status;
            break;
        }


        if(status == WAIT_OBJECT_0 + nCount - 1) {
            ResetEvent((HANDLE)m_handle);
            uchar type;
            QByteArray data;
            while(pop(type, data)) {
                if(type == RDPMSG_EXIT) {
                    freerdp_disconnect(instance);
                    qDebug() << "running exit now" << m_ti.host;
                    return 0;
                }else if(type == RDPMSG_KEYEVENT) {
                    QDataStream buf(data);
                    bool pressed;
                    int code;
                    buf >> pressed >> code;
                    rdpInput* input = context->input;
                    freerdp_input_send_keyboard_event_ex(input, pressed, code);
                }else if(type == RDPMSG_MOUSEEVENT) {
                    QDataStream buf(data);
                    int flags, x, y;
                    buf >> flags >> x >> y;
                    rdpInput* input = context->input;
                    qt_mouse_event(context, input, flags, x, y);
                }else if(type == RDPMSG_PASTETEXT) {
                    qt_cliprdr_send_client_format_list(m_wtc->cliprdr);
                }else if(type == RDPMSG_KEYSYNC) {
                    QDataStream buf(data);
                    int flags;
                    buf >> flags;
                    rdpInput* input = context->input;
                    input->SynchronizeEvent(input, flags);
                }
            }
        }

        if (!freerdp_check_event_handles(instance->context)) {
            if (freerdp_get_last_error(instance->context) == FREERDP_ERROR_SUCCESS){
                qDebug() << "Failed to check FreeRDP event handles";
            }
            break;
        }
    }
    freerdp_disconnect(instance);
    return 0;
}

int QRdpWork::nativeScanCode(QKeyEvent *ev)
{
    int scanCode = qt_KeyToWinScanCode(ev->key(), ev->nativeScanCode());
    if (scanCode == RDP_SCANCODE_NUMLOCK_EXTENDED) {
        scanCode = RDP_SCANCODE_NUMLOCK;
    }else if(scanCode == RDP_SCANCODE_RSHIFT_EXTENDED) {
        scanCode = RDP_SCANCODE_RSHIFT;
    }
    return scanCode;
}

void QRdpWork::onClipboardDataChanged()
{
    push(RDPMSG_PASTETEXT);
}

void QRdpWork::onClipboardUpdate(const QString &txt)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(txt);
}

void QRdpWork::onDisconnected()
{

}

QWoRdpFactory::QWoRdpFactory(QObject *parent)
{

}

QWoRdpFactory::~QWoRdpFactory()
{

}

QWoRdpFactory *QWoRdpFactory::instance()
{
    static QPointer<QWoRdpFactory> factory = new QWoRdpFactory();
    return factory;
}

QRdpWork *QWoRdpFactory::create()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QRdpWork *work = new QRdpWork(this);
    QObject::connect(clipboard, SIGNAL(dataChanged()), work, SLOT(onClipboardDataChanged()));
    return work;
}

void QWoRdpFactory::release(QRdpWork *obj)
{
    if(obj == nullptr) {
        return;
    }
    obj->disconnect();
    QObject::connect(obj, SIGNAL(finished()), this, SLOT(onFinished()));
    if(!obj->hasRunning()) {
        obj->deleteLater();
        return;
    }
    obj->stop();
    m_dels.append(obj);
}

void QWoRdpFactory::onFinished()
{
    cleanup();
}

void QWoRdpFactory::onAboutToQuit()
{

}

void QWoRdpFactory::cleanup()
{
    for(QList<QPointer<QRdpWork>>::iterator iter = m_dels.begin(); iter != m_dels.end(); ) {
        QRdpWork *obj = *iter;
        if(obj == nullptr) {
            iter = m_dels.erase(iter);
            continue;
        }
        if(!obj->hasRunning()) {
            obj->deleteLater();
            iter = m_dels.erase(iter);
            continue;
        }
        obj->stop();
        iter++;
    }
}

