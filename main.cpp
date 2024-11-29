// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <iostream>
#include <string>
#include <vector>
#include <OpenImageDenoise/oidn.h>
#include "defaultlightmapdenoiser.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <getopt.h>
#endif

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

#ifdef _WIN32
std::vector<std::string> getCommandLineArgs()
{
    std::vector<std::string> args;
    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 0; i < argc; ++i) {
        char buffer[256];
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, buffer, 256, NULL, NULL);
        args.push_back(buffer);
    }
    LocalFree(argv);
    return args;
}
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::vector<std::string> args = getCommandLineArgs();
    std::string appName = args[0];
#else
    std::string appName = argv[0];
#endif

#ifdef _WIN32
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-h" || args[i] == "--help") {
            showHelp(appName);
            return EXIT_SUCCESS;
        } else if (args[i] == "-v" || args[i] == "--version") {
            showVersion();
            return EXIT_SUCCESS;
        }
    }
#else
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
#endif

    std::vector<std::string> positionalArguments;
#ifdef _WIN32
    for (size_t i = 1; i < args.size(); ++i) {
        positionalArguments.emplace_back(args[i]);
    }
#else
    for (int i = optind; i < argc; i++) {
        positionalArguments.emplace_back(argv[i]);
    }
#endif

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
