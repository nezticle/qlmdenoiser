/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTemporaryDir>
#include <QDir>
#include <QLibraryInfo>
#include <OpenImageDenoise/oidn.h>

#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 1
#define TINYEXR_USE_THREAD 1
#include "tinyexr.h"

static void printError(const char *msg, ...)
{
    va_list arglist;
    va_start(arglist, msg);
    vfprintf(stderr, msg, arglist);
    fputs("\n", stderr);
    va_end(arglist);
}

static void toRGBAndAlpha(const float *rgba, QByteArray *rgb, QByteArray *alpha, int width, int height)
{
    const float *inP = rgba;
    float *outP = reinterpret_cast<float *>(rgb->data());
    float *alphaP = reinterpret_cast<float *>(alpha->data());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            *outP++ = *inP++;
            *outP++ = *inP++;
            *outP++ = *inP++;
            *alphaP++ = *inP++;
        }
    }
}

static void combineRGBAndAlpha(const char *rgb, const char *alpha, QByteArray *rgba, int width, int height)
{
    const float *inP = reinterpret_cast<const float *>(rgb);
    const float *alphaP = reinterpret_cast<const float *>(alpha);
    float *outP = reinterpret_cast<float *>(rgba->data());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            *outP++ = *inP++;
            *outP++ = *inP++;
            *outP++ = *inP++;
            *outP++ = *alphaP++;
        }
    }

}

static bool processFile(const QString &fn, OIDNDeviceImpl *device)
{
    const QByteArray inFilename = fn.toUtf8();
    float *inOrigData = nullptr;
    int width = 0;
    int height = 0;
    const char *err = nullptr;
    qDebug("Loading EXR image %s", inFilename.constData());
    if (LoadEXR(&inOrigData, &width, &height, inFilename.constData(), &err) < 0) {
        printError("Failed to load EXR image: %s", err);
        return false;
    }
    QByteArray inData(width * height * 3 * sizeof(float), Qt::Uninitialized);
    QByteArray alpha(width * height * sizeof(float), Qt::Uninitialized);
    toRGBAndAlpha(inOrigData, &inData, &alpha, width, height);
    free(inOrigData);

    QByteArray outData(width * height * 3 * sizeof(float), Qt::Uninitialized);

    qDebug("Denoising");
    OIDNFilter filter = oidnNewFilter(device, "RTLightmap");
    oidnSetSharedFilterImage(filter, "color", inData.data(), OIDN_FORMAT_FLOAT3, width, height, 0, 0, 0);
    oidnSetSharedFilterImage(filter, "output", outData.data(), OIDN_FORMAT_FLOAT3, width, height, 0, 0, 0);
    oidnSetFilter1b(filter, "hdr", true);
    oidnCommitFilter(filter);
    oidnExecuteFilter(filter);

    const char *msg;
    if (oidnGetDeviceError(device, &msg) != OIDN_ERROR_NONE) {
        fprintf(stderr, "Error from denoiser: %s\n", msg);
        return false;
    }

    oidnReleaseFilter(filter);

    QByteArray rgba(width * height * 4 * sizeof(float), Qt::Uninitialized);
    combineRGBAndAlpha(outData.constData(), alpha.constData(), &rgba, width, height);

    const QString tempFn = QDir::tempPath() + QLatin1String("/") + QFileInfo(fn).fileName();
    qDebug("Saving");
    if (SaveEXR(reinterpret_cast<const float *>(rgba.constData()), width, height, 4, false, tempFn.toUtf8().constData(), &err) < 0) {
        printError("Failed to save EXR image: %s", err);
        return false;
    }

    if (!QDir().remove(fn)) {
        printError("Failed to remove source file");
        return false;
    }
    if (!QDir().rename(tempFn, fn)) {
        printError("Rename failed");
        return false;
    }


    qDebug("Done %s", inFilename.constData());
    return true;
}

static bool processListFile(const QString &fn, OIDNDeviceImpl *device)
{
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Cannot open list file %s", qPrintable(fn));
        return false;
    }
    const QByteArray listContents = f.readAll();
    f.close();

    const QByteArrayList files = listContents.split('\n');
    for (const QByteArray &file : files) {
        const QByteArray tfile = file.trimmed();
        if (!tfile.isEmpty()) {
            if (!processFile(QFileInfo(tfile).absoluteFilePath(), device))
                return false;
        }
    }

    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser cmdLineParser;
    const QString appDesc = QString::asprintf("Qt Lightmap Denoiser %s (using OpenImageDenoise %d.%d.%d)",
                                              qVersion(),
                                              OIDN_VERSION_MAJOR,
                                              OIDN_VERSION_MINOR,
                                              OIDN_VERSION_PATCH);
    cmdLineParser.setApplicationDescription(appDesc);
    app.setApplicationVersion(QLatin1String(QT_VERSION_STR));
    cmdLineParser.addHelpOption();
    cmdLineParser.addVersionOption();
    cmdLineParser.addPositionalArgument(QLatin1String("file"), QObject::tr(".exr file or .txt with list of files"), QObject::tr("file"));
    cmdLineParser.process(app);

    if (cmdLineParser.positionalArguments().isEmpty()) {
        cmdLineParser.showHelp();
        return EXIT_SUCCESS;
    }

    OIDNDeviceImpl *device = oidnNewDevice(OIDN_DEVICE_TYPE_CPU);
    oidnCommitDevice(device);

    for (const QString &fn : cmdLineParser.positionalArguments()) {
        const QFileInfo fi(fn);
        if (fi.suffix() == QLatin1String("txt")) {
            if (!processListFile(fi.absoluteFilePath(), device))
                return EXIT_FAILURE;
        } else {
            if (!processFile(fi.absoluteFilePath(), device))
                return EXIT_FAILURE;
        }
    }

    oidnReleaseDevice(device);
    return EXIT_SUCCESS;
}
