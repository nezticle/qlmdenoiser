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
#include <OpenImageDenoise/oidn.h>

int main(int argc, char **argv)
{
    int ret = EXIT_SUCCESS;
    OIDNDeviceImpl *device = oidnNewDevice(OIDN_DEVICE_TYPE_CPU);
    oidnCommitDevice(device);
    OIDNFilter filter = oidnNewFilter(device, "RTLightmap");

    // oidnSetSharedFilterImage(filter, "color", (void *)p_floats, OIDN_FORMAT_FLOAT3, p_width, p_height, 0, 0, 0);
    // oidnSetSharedFilterImage(filter, "output", (void *)p_floats, OIDN_FORMAT_FLOAT3, p_width, p_height, 0, 0, 0);
    // oidnSetFilter1b(filter, "hdr", true);
    // oidnCommitFilter(filter);
    // oidnExecuteFilter(filter);

    const char *msg;
    if (oidnGetDeviceError(device, &msg) != OIDN_ERROR_NONE) {
        fprintf(stderr, "Error from denoiser: %s\n", msg);
        ret = EXIT_FAILURE;
    }

    oidnReleaseFilter(filter);
    oidnReleaseDevice(device);
    return ret;
}
