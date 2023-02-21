// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qdefaultlightmapdenoiser.h"

#include <QDir>
#include <OpenImageDenoise/oidn.h>

#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 1
#define TINYEXR_USE_THREAD 1
#include "tinyexr.h"

struct Data {
    OIDNDeviceImpl *device = nullptr;
} d;

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

QDefaultLightmapDenoiser::QDefaultLightmapDenoiser()
{
    d.device = oidnNewDevice(OIDN_DEVICE_TYPE_CPU);
    oidnCommitDevice(d.device);
}

QDefaultLightmapDenoiser::~QDefaultLightmapDenoiser()
{
    oidnReleaseDevice(d.device);
}

bool QDefaultLightmapDenoiser::process(const QString &fileName)
{
    const QFileInfo fi(fileName);
    if (fi.suffix() == QLatin1String("txt"))
        return processListFile(fi.absoluteFilePath());
    else
        return denoise(fi.absoluteFilePath());
}

bool QDefaultLightmapDenoiser::denoise(const QString &fileName)
{
    const QByteArray inFilename = fileName.toUtf8();
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
    OIDNFilter filter = oidnNewFilter(d.device, "RTLightmap");
    oidnSetSharedFilterImage(filter, "color", inData.data(), OIDN_FORMAT_FLOAT3, width, height, 0, 0, 0);
    oidnSetSharedFilterImage(filter, "output", outData.data(), OIDN_FORMAT_FLOAT3, width, height, 0, 0, 0);
    oidnSetFilter1b(filter, "hdr", true);
    oidnCommitFilter(filter);
    oidnExecuteFilter(filter);

    const char *msg;
    if (oidnGetDeviceError(d.device, &msg) != OIDN_ERROR_NONE) {
        fprintf(stderr, "Error from denoiser: %s\n", msg);
        return false;
    }

    oidnReleaseFilter(filter);

    QByteArray rgba(width * height * 4 * sizeof(float), Qt::Uninitialized);
    combineRGBAndAlpha(outData.constData(), alpha.constData(), &rgba, width, height);

    const QString tempFn = QDir::tempPath() + QLatin1String("/") + QFileInfo(fileName).fileName();
    qDebug("Saving");
    if (SaveEXR(reinterpret_cast<const float *>(rgba.constData()), width, height, 4, false, tempFn.toUtf8().constData(), &err) < 0) {
        printError("Failed to save EXR image: %s", err);
        return false;
    }

    if (!QDir().remove(fileName)) {
        printError("Failed to remove source file");
        return false;
    }
    if (!QDir().rename(tempFn, fileName)) {
        printError("Rename failed");
        return false;
    }


    qDebug("Done %s", inFilename.constData());
    return true;
}

bool QDefaultLightmapDenoiser::processListFile(const QString &fn)
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
            if (!denoise(QFileInfo(tfile).absoluteFilePath()))
                return false;
        }
    }

    return true;
}
