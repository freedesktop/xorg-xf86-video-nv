/*
 * Copyright (c) 2007 NVIDIA, Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xatom.h>

#include "g80_type.h"
#include "g80_display.h"
#include "g80_output.h"

static void
G80SorSetPClk(xf86OutputPtr output, int pclk)
{
    G80Ptr pNv = G80PTR(output->scrn);
    G80OutputPrivPtr pPriv = output->driver_private;
    const int orOff = 0x800 * pPriv->or;

    pNv->reg[(0x00614300+orOff)/4] = (pclk > 165000) ? 0x101 : 0;
}

static void
G80SorDPMSSet(xf86OutputPtr output, int mode)
{
    ErrorF("SOR dpms unimplemented\n");
}

static void
G80SorModeSet(xf86OutputPtr output, DisplayModePtr mode,
              DisplayModePtr adjusted_mode)
{
    ScrnInfoPtr pScrn = output->scrn;
    G80OutputPrivPtr pPriv = output->driver_private;
    const int sorOff = 0x40 * pPriv->or;

    if(adjusted_mode)
        ErrorF("SOR%i mode %s -> HEAD%i\n", pPriv->or, adjusted_mode->name, G80CrtcGetHead(output->crtc));

    if(!adjusted_mode) {
        /* Disconnect the SOR */
        C(0x00000600 + sorOff, 0);
        return;
    }

    C(0x00000600 + sorOff,
        (G80CrtcGetHead(output->crtc) == HEAD0 ? 1 : 2) |
        (mode->SynthClock > 165000 ? 0x500 : 0x100) |
        ((mode->Flags & V_NHSYNC) ? 0x1000 : 0) |
        ((mode->Flags & V_NVSYNC) ? 0x2000 : 0));
}

static xf86OutputStatus
G80SorDetect(xf86OutputPtr output)
{
    return XF86OutputStatusUnknown;

#if 0
    DisplayModePtr modes = output->funcs->get_modes(output);
    xf86OutputStatus status;

    if(modes)
        status = XF86OutputStatusConnected;
    else
        status = XF86OutputStatusDisconnected;
    xfree(modes);

    return status;
#endif
}

static void
G80SorDestroy(xf86OutputPtr output)
{
    G80OutputDestroy(output);

    xfree(output->driver_private);
    output->driver_private = NULL;
}

static const xf86OutputFuncsRec G80SorOutputFuncs = {
    .dpms = G80SorDPMSSet,
    .save = NULL,
    .restore = NULL,
    .mode_valid = G80OutputModeValid,
    .mode_fixup = G80OutputModeFixup,
    .prepare = G80OutputPrepare,
    .commit = G80OutputCommit,
    .mode_set = G80SorModeSet,
    .detect = G80SorDetect,
    .get_modes = G80OutputGetDDCModes,
    .destroy = G80SorDestroy,
};

xf86OutputPtr
G80CreateSor(ScrnInfoPtr pScrn, ORNum or, int i2cPort)
{
    G80OutputPrivPtr pPriv = xnfcalloc(sizeof(*pPriv), 1);
    xf86OutputPtr output;
    char orName[5];

    if(!pPriv)
        return FALSE;

    snprintf(orName, 5, "DVI%i", or);
    output = xf86OutputCreate(pScrn, &G80SorOutputFuncs, orName);

    pPriv->type = SOR;
    pPriv->or = or;
    pPriv->set_pclk = G80SorSetPClk;
    output->driver_private = pPriv;
    output->interlaceAllowed = TRUE;
    output->doubleScanAllowed = TRUE;

    /* Create an I2C object */
    G80I2CInit(output, i2cPort);

    return output;
}
