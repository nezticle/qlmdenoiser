// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "defaultlightmapdenoiser.h"
#include <OpenImageDenoise/oidn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <vector>
#include <string>

// TinyEXR related defines
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

static void toRGBAndAlpha(const float *rgba, std::vector<float> &rgb, std::vector<float> &alpha, int width, int height)
{
    const float *inP = rgba;
    float *outP = rgb.data();
    float *alphaP = alpha.data();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            *outP++ = *inP++;
            *outP++ = *inP++;
            *outP++ = *inP++;
            *alphaP++ = *inP++;
        }
    }
}

static void combineRGBAndAlpha(const float *rgb, const float *alpha, std::vector<float> &rgba, int width, int height)
{
    const float *inP = rgb;
    const float *alphaP = alpha;
    float *outP = rgba.data();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            *outP++ = *inP++;
            *outP++ = *inP++;
            *outP++ = *inP++;
            *outP++ = *alphaP++;
        }
    }
}

DefaultLightmapDenoiser::DefaultLightmapDenoiser()
{
    d.device = oidnNewDevice(OIDN_DEVICE_TYPE_CPU);
    oidnCommitDevice(d.device);
}

DefaultLightmapDenoiser::~DefaultLightmapDenoiser()
{
    oidnReleaseDevice(d.device);
}

bool DefaultLightmapDenoiser::process(const std::string &fileName)
{
    std::filesystem::path filePath(fileName);
    if (filePath.extension() == ".txt")
        return processListFile(filePath.string());
    else
        return denoise(filePath.string());
}

bool DefaultLightmapDenoiser::denoise(const std::string &fileName)
{
    float *inOrigData = nullptr;
    int width = 0;
    int height = 0;
    const char *err = nullptr;

    // Resolve the file name to an absolute path
    std::filesystem::path absFilePath = std::filesystem::absolute(fileName);
    std::string absFileName = absFilePath.string();

    std::cout << "Loading EXR image " << absFileName << std::endl;

    if (LoadEXR(&inOrigData, &width, &height, absFileName.c_str(), &err) < 0) {
        printError("Failed to load EXR image: %s", err);
        return false;
    }

    std::vector<float> inData(width * height * 3);
    std::vector<float> alpha(width * height);
    toRGBAndAlpha(inOrigData, inData, alpha, width, height);
    free(inOrigData);

    std::vector<float> outData(width * height * 3);

    std::cout << "Denoising" << std::endl;
    OIDNFilter filter = oidnNewFilter(d.device, "RTLightmap");
    oidnSetSharedFilterImage(filter, "color", inData.data(), OIDN_FORMAT_FLOAT3, width, height, 0, 0, 0);
    oidnSetSharedFilterImage(filter, "output", outData.data(), OIDN_FORMAT_FLOAT3, width, height, 0, 0, 0);
    oidnSetFilter1b(filter, "hdr", true);
    oidnCommitFilter(filter);
    oidnExecuteFilter(filter);

    const char *msg;
    if (oidnGetDeviceError(d.device, &msg) != OIDN_ERROR_NONE) {
        std::cerr << "Error from denoiser: " << msg << std::endl;
        return false;
    }

    oidnReleaseFilter(filter);

    std::vector<float> rgba(width * height * 4);
    combineRGBAndAlpha(outData.data(), alpha.data(), rgba, width, height);

    std::filesystem::path tempFn = std::filesystem::temp_directory_path() / absFilePath.filename();
    std::cout << "Saving" << std::endl;
    if (SaveEXR(rgba.data(), width, height, 4, false, tempFn.string().c_str(), &err) < 0) {
        printError("Failed to save EXR image: %s", err);
        return false;
    }

    // Replace the original file
    if (!std::filesystem::remove(absFileName)) {
        printError("Failed to remove source file");
        return false;
    }
    std::filesystem::rename(tempFn, absFileName);

    std::cout << "Done " << absFileName << std::endl;
    return true;
}

bool DefaultLightmapDenoiser::processListFile(const std::string &fn)
{
    std::ifstream f(fn);
    if (!f.is_open()) {
        std::cerr << "Cannot open list file " << fn << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) {
            std::filesystem::path filePath(line);
            if (!denoise(filePath.string())) {
                return false;
            }
        }
    }

    return true;
}