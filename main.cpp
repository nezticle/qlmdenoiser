// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <iostream>
#include <string>
#include <vector>
#include <getopt.h>
#include <OpenImageDenoise/oidn.h>
#include "defaultlightmapdenoiser.h"

void showHelp(const std::string &appName)
{
    std::cout << "Usage: " << appName << " [options] <file>\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help      Show this help message\n";
    std::cout << "  -v, --version   Show version information\n";
    std::cout << "Arguments:\n";
    std::cout << "  file            .exr file or .txt with list of files\n";
}

void showVersion()
{
    std::cout << "Lightmap Denoiser (using OpenImageDenoise " << OIDN_VERSION_MAJOR
              << "." << OIDN_VERSION_MINOR << "." << OIDN_VERSION_PATCH << ")\n";
}

int main(int argc, char **argv)
{
    std::string appName = argv[0];

    static struct option long_options[] = {
        {"help",    no_argument,       nullptr, 'h'},
        {"version", no_argument,       nullptr, 'v'},
        {nullptr,   0,                 nullptr,  0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hv", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'h':
                showHelp(appName);
                return EXIT_SUCCESS;
            case 'v':
                showVersion();
                return EXIT_SUCCESS;
            default:
                showHelp(appName);
                return EXIT_FAILURE;
        }
    }

    std::vector<std::string> positionalArguments;
    for (int i = optind; i < argc; i++) {
        positionalArguments.emplace_back(argv[i]);
    }

    if (positionalArguments.empty()) {
        showHelp(appName);
        return EXIT_SUCCESS;
    }

    DefaultLightmapDenoiser denoiser;

    for (const std::string &fn : positionalArguments) {
        if (!denoiser.process(fn)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
