// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QDEFAULTLIGHTMAPMDENOISER_H
#define QDEFAULTLIGHTMAPMDENOISER_H

#include <QString>

QT_BEGIN_NAMESPACE

class QDefaultLightmapDenoiser {

public:
    QDefaultLightmapDenoiser();
    ~QDefaultLightmapDenoiser();

    bool process(const QString &fileName);

protected:
    bool denoise(const QString& fileName);

private:
    bool processListFile(const QString &fn);
};

QT_END_NAMESPACE

#endif // QDEFAULTLIGHTMAPMDENOISER_H
