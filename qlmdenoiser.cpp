/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

static inline QString tempFilePath(const QTemporaryDir &tempDir, const QString &filename)
{
    return tempDir.path() + QLatin1String("/") + filename;
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

static bool processFile(const QString &fn, OIDNDeviceImpl *device, QTemporaryDir &tempDir)
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

    const QString outFilePath = tempFilePath(tempDir, fn);
    const QByteArray outFn = outFilePath.toUtf8();
    qDebug("Saving");
    if (SaveEXR(reinterpret_cast<const float *>(rgba.constData()), width, height, 4, false, outFn.constData(), &err) < 0) {
        printError("Failed to save EXR image: %s", err);
        return false;
    }

    if (!QDir().remove(fn)) {
        printError("Failed to remove source file");
        return false;
    }
    if (!QDir().rename(outFilePath, fn)) {
        printError("Rename failed");
        return false;
    }

    qDebug("Done %s", inFilename.constData());
    return true;
}

static bool processListFile(const QString &fn, OIDNDeviceImpl *device, QTemporaryDir &tempDir)
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
            if (!processFile(tfile, device, tempDir))
                return false;
        }
    }

    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser cmdLineParser;
    const QString appDesc = QString::asprintf("Qt Lightmap Denoiser (using OpenImageDenoise)", qVersion());
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

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        printError("Failed to create temporary directory");
        return EXIT_FAILURE;
    }

    OIDNDeviceImpl *device = oidnNewDevice(OIDN_DEVICE_TYPE_CPU);
    oidnCommitDevice(device);

    for (const QString &fn : cmdLineParser.positionalArguments()) {
        if (QFileInfo(fn).suffix() == QLatin1String("txt")) {
            if (!processListFile(fn, device, tempDir))
                return EXIT_FAILURE;
        } else {
            if (!processFile(fn, device, tempDir))
                return EXIT_FAILURE;
        }
    }

    oidnReleaseDevice(device);
    return EXIT_SUCCESS;
}
