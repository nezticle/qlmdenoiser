// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DEFAULTLIGHTMAPDENOISER_H
#define DEFAULTLIGHTMAPDENOISER_H

#include <string>

class DefaultLightmapDenoiser {

public:
    DefaultLightmapDenoiser();
    ~DefaultLightmapDenoiser();

    bool process(const std::string &fileName);

protected:
    bool denoise(const std::string &fileName);

private:
    bool processListFile(const std::string &fn);
};

#endif // DEFAULTLIGHTMAPDENOISER_H
