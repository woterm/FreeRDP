/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Windows Client
 *
 * Copyright 2009-2011 Jay Sorg
 * Copyright 2010-2011 Vic Lee
 * Copyright 2010-2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <winpr/windows.h>

#include <winpr/crt.h>
#include <winpr/credui.h>

#include <freerdp/freerdp.h>
#include <freerdp/constants.h>

#include <freerdp/client/file.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/client/channels.h>
#include <freerdp/channels/channels.h>

#ifdef Q_OS_WIN
#include <shellapi.h>
#endif

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMainWindow>
#include <QImage>

#include "qrdpwidget.h"

void myexit(void) {
    printf("exit now");
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    static QApplication app(argc, argv);
#else
    QApplication app(argc, argv);
#endif
    atexit(myexit);

    QString path = QApplication::applicationDirPath();
    QApplication::addLibraryPath(path);
    QString libPath = QDir::cleanPath(path + "/../lib");
    if(QFile::exists(libPath)) {
        QApplication::addLibraryPath(libPath);
    }

    QMainWindow main;
    QRdpWidget desk(&main);
    main.setCentralWidget(&desk);
    main.resize(1024, 768);
    main.show();

    return app.exec();
}
