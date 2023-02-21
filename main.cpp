// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QCommandLineParser>

#include <OpenImageDenoise/oidn.h>

#include "qdefaultlightmapdenoiser.h"

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

    QDefaultLightmapDenoiser denoiser;

    for (const QString &fn : cmdLineParser.positionalArguments())
        if (!denoiser.process(fn))
            return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
