/***************************************************************************
 *   Copyright (C) 2019 PCSX-Redux authors                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#pragma once

#include "core/gpu.h"
#include "gpu/soft/externals.h"
#include "gpu/soft/prim.h"

namespace PCSX {

class GUI;

namespace SoftGPU {

class impl final : public GPU {
    int32_t initBackend(GUI *) override;
    int32_t shutdown() override;
    void stopDump() override;
    void readDataMem(uint32_t *pMem, int iSize) override;
    uint32_t readStatusInternal() override;
    void writeDataMem(uint32_t *pMem, int iSize) override;
    void writeStatusInternal(uint32_t gdata) override;
    int32_t dmaChain(uint32_t *baseAddrL, uint32_t addr) override;
    void vblank() override;
    bool configure() override;
    void debug() override;

    void setDither(int setting) override { m_softPrim.m_useDither = setting; }
    void clearVRAM() override;
    void reset() override {
        clearVRAM();
        m_display.reset();
    }
    GLuint getVRAMTexture() override { return m_vramTexture16; }
    void setLinearFiltering() override;

    void restoreStatus(uint32_t status) override;

    void updateDisplay();
    void initDisplay();
    void doBufferSwap();

    Slice getVRAM() override {
        Slice ret;
        ret.borrow(psxVuw, 1024 * 512 * 2);
        return ret;
    }

    void partialUpdateVRAM(int x, int y, int w, int h, const uint16_t *pixels) override {
        auto ptr = psxVuw;
        ptr += y * 1024 + x;
        for (int i = 0; i < h; i++) {
            std::memcpy(ptr, pixels, w * sizeof(uint16_t));
            ptr += 1024;
            pixels += w;
        }
    }

    ScreenShot takeScreenShot() override {
        ScreenShot ss;
        auto startX = PSXDisplay.DisplayPosition.x;
        auto startY = PSXDisplay.DisplayPosition.y;
        auto width = PSXDisplay.DisplayEnd.x - PSXDisplay.DisplayPosition.x;
        auto height = PSXDisplay.DisplayEnd.y - PSXDisplay.DisplayPosition.y;
        ss.width = width;
        ss.height = height;
        unsigned factor = PSXDisplay.RGB24 ? 3 : 2;
        ss.bpp = PSXDisplay.RGB24 ? ScreenShot::BPP_24 : ScreenShot::BPP_16;
        unsigned size = width * height * factor;
        char *pixels = reinterpret_cast<char *>(malloc(size));
        ss.data.acquire(pixels, size);
        if (PSXDisplay.RGB24) {
            auto ptr = psxVSecure;
            ptr += (startY * 1024 + startX) * 3;
            for (int i = 0; i < height; i++) {
                std::memcpy(pixels, ptr, width * 3);
                ptr += 1024 * 3;
                pixels += width * 3;
            }
        } else {
            auto ptr = psxVuw;
            ptr += startY * 1024 + startX;
            for (int i = 0; i < height; i++) {
                std::memcpy(pixels, ptr, width * sizeof(uint16_t));
                ptr += 1024;
                pixels += width * 2;
            }
        }

        return ss;
    }
    SoftPrim m_softPrim;
    void *m_dumpFile = nullptr;
    GLuint m_vramTexture16;
    GLuint m_vramTexture24;

    GUI *m_gui;

    ////////////////////////////////////////////////////////////////////////
    // memory image of the PSX vram
    ////////////////////////////////////////////////////////////////////////

    //    unsigned char *psxVSecure;
    //    unsigned char *psxVub;
    //    signed char *psxVsb;
    //    uint16_t *psxVuw;
    //    uint16_t *psxVuw_eom;
    //    int16_t *psxVsw;
    //    uint32_t *psxVul;
    //    int32_t *psxVsl;

    ////////////////////////////////////////////////////////////////////////
    // GPU globals
    ////////////////////////////////////////////////////////////////////////

    int32_t lGPUdataRet;
    //    int32_t lGPUstatusRet;
    //    char szDispBuf[64];
    //    char szMenuBuf[36];
    //    char szDebugText[512];
    //    uint32_t ulStatusControl[256];

    //    uint32_t gpuDataM[256];
    //    unsigned char gpuCommand = 0;
    //    int32_t gpuDataC = 0;
    //    int32_t gpuDataP = 0;

    //    VRAMLoad_t VRAMWrite;
    //    VRAMLoad_t VRAMRead;
    //    DATAREGISTERMODES DataWriteMode;
    //    DATAREGISTERMODES DataReadMode;

    //    bool bSkipNextFrame = false;
    //    DWORD dwLaceCnt = 0;
    //    int iColDepth;
    //    int iWindowMode;
    //    int16_t sDispWidths[8] = {256, 320, 512, 640, 368, 384, 512, 640};
    //    PSXDisplay_t PSXDisplay;
    //    PSXDisplay_t PreviousPSXDisplay;
    //    int32_t lSelectedSlot = 0;
    //    bool bChangeWinMode = false;
    //    bool bDoLazyUpdate = false;
    //    uint32_t lGPUInfoVals[16];
    //    int iFakePrimBusy = 0;
    //    int iRumbleVal = 0;
    //    int iRumbleTime = 0;

    void write0(FastFill *) override;

    template <Shading shading, Shape shape, Textured textured, Blend blend, Modulation modulation>
    void polyExec(Poly<shading, shape, textured, blend, modulation> *);
    void write0(Poly<Shading::Flat, Shape::Tri, Textured::No, Blend::Off, Modulation::Off> *) override;
    void write0(Poly<Shading::Flat, Shape::Tri, Textured::No, Blend::Off, Modulation::On> *) override;
    void write0(Poly<Shading::Flat, Shape::Tri, Textured::No, Blend::Semi, Modulation::Off> *) override;
    void write0(Poly<Shading::Flat, Shape::Tri, Textured::No, Blend::Semi, Modulation::On> *) override;
    void write0(Poly<Shading::Flat, Shape::Tri, Textured::Yes, Blend::Off, Modulation::Off> *) override;
    void write0(Poly<Shading::Flat, Shape::Tri, Textured::Yes, Blend::Off, Modulation::On> *) override;
    void write0(Poly<Shading::Flat, Shape::Tri, Textured::Yes, Blend::Semi, Modulation::Off> *) override;
    void write0(Poly<Shading::Flat, Shape::Tri, Textured::Yes, Blend::Semi, Modulation::On> *) override;
    void write0(Poly<Shading::Flat, Shape::Quad, Textured::No, Blend::Off, Modulation::Off> *) override;
    void write0(Poly<Shading::Flat, Shape::Quad, Textured::No, Blend::Off, Modulation::On> *) override;
    void write0(Poly<Shading::Flat, Shape::Quad, Textured::No, Blend::Semi, Modulation::Off> *) override;
    void write0(Poly<Shading::Flat, Shape::Quad, Textured::No, Blend::Semi, Modulation::On> *) override;
    void write0(Poly<Shading::Flat, Shape::Quad, Textured::Yes, Blend::Off, Modulation::Off> *) override;
    void write0(Poly<Shading::Flat, Shape::Quad, Textured::Yes, Blend::Off, Modulation::On> *) override;
    void write0(Poly<Shading::Flat, Shape::Quad, Textured::Yes, Blend::Semi, Modulation::Off> *) override;
    void write0(Poly<Shading::Flat, Shape::Quad, Textured::Yes, Blend::Semi, Modulation::On> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Tri, Textured::No, Blend::Off, Modulation::Off> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Tri, Textured::No, Blend::Off, Modulation::On> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Tri, Textured::No, Blend::Semi, Modulation::Off> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Tri, Textured::No, Blend::Semi, Modulation::On> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Tri, Textured::Yes, Blend::Off, Modulation::Off> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Tri, Textured::Yes, Blend::Off, Modulation::On> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Tri, Textured::Yes, Blend::Semi, Modulation::Off> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Tri, Textured::Yes, Blend::Semi, Modulation::On> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Quad, Textured::No, Blend::Off, Modulation::Off> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Quad, Textured::No, Blend::Off, Modulation::On> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Quad, Textured::No, Blend::Semi, Modulation::Off> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Quad, Textured::No, Blend::Semi, Modulation::On> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Quad, Textured::Yes, Blend::Off, Modulation::Off> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Quad, Textured::Yes, Blend::Off, Modulation::On> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Quad, Textured::Yes, Blend::Semi, Modulation::Off> *) override;
    void write0(Poly<Shading::Gouraud, Shape::Quad, Textured::Yes, Blend::Semi, Modulation::On> *) override;

    template <Shading shading, LineType lineType, Blend blend>
    void lineExec(Line<shading, lineType, blend> *);
    void write0(Line<Shading::Flat, LineType::Simple, Blend::Off> *) override;
    void write0(Line<Shading::Flat, LineType::Simple, Blend::Semi> *) override;
    void write0(Line<Shading::Flat, LineType::Poly, Blend::Off> *) override;
    void write0(Line<Shading::Flat, LineType::Poly, Blend::Semi> *) override;
    void write0(Line<Shading::Gouraud, LineType::Simple, Blend::Off> *) override;
    void write0(Line<Shading::Gouraud, LineType::Simple, Blend::Semi> *) override;
    void write0(Line<Shading::Gouraud, LineType::Poly, Blend::Off> *) override;
    void write0(Line<Shading::Gouraud, LineType::Poly, Blend::Semi> *) override;

    template <Size size, Textured textured, Blend blend, Modulation modulation>
    void rectExec(Rect<size, textured, blend, modulation> *);
    void write0(Rect<Size::Variable, Textured::No, Blend::Off, Modulation::Off> *) override;
    void write0(Rect<Size::Variable, Textured::No, Blend::Semi, Modulation::Off> *) override;
    void write0(Rect<Size::Variable, Textured::Yes, Blend::Off, Modulation::Off> *) override;
    void write0(Rect<Size::Variable, Textured::Yes, Blend::Semi, Modulation::Off> *) override;
    void write0(Rect<Size::S1, Textured::No, Blend::Off, Modulation::Off> *) override;
    void write0(Rect<Size::S1, Textured::No, Blend::Semi, Modulation::Off> *) override;
    void write0(Rect<Size::S1, Textured::Yes, Blend::Off, Modulation::Off> *) override;
    void write0(Rect<Size::S1, Textured::Yes, Blend::Semi, Modulation::Off> *) override;
    void write0(Rect<Size::S8, Textured::No, Blend::Off, Modulation::Off> *) override;
    void write0(Rect<Size::S8, Textured::No, Blend::Semi, Modulation::Off> *) override;
    void write0(Rect<Size::S8, Textured::Yes, Blend::Off, Modulation::Off> *) override;
    void write0(Rect<Size::S8, Textured::Yes, Blend::Semi, Modulation::Off> *) override;
    void write0(Rect<Size::S16, Textured::No, Blend::Off, Modulation::Off> *) override;
    void write0(Rect<Size::S16, Textured::No, Blend::Semi, Modulation::Off> *) override;
    void write0(Rect<Size::S16, Textured::Yes, Blend::Off, Modulation::Off> *) override;
    void write0(Rect<Size::S16, Textured::Yes, Blend::Semi, Modulation::Off> *) override;
    void write0(Rect<Size::Variable, Textured::No, Blend::Off, Modulation::On> *) override;
    void write0(Rect<Size::Variable, Textured::No, Blend::Semi, Modulation::On> *) override;
    void write0(Rect<Size::Variable, Textured::Yes, Blend::Off, Modulation::On> *) override;
    void write0(Rect<Size::Variable, Textured::Yes, Blend::Semi, Modulation::On> *) override;
    void write0(Rect<Size::S1, Textured::No, Blend::Off, Modulation::On> *) override;
    void write0(Rect<Size::S1, Textured::No, Blend::Semi, Modulation::On> *) override;
    void write0(Rect<Size::S1, Textured::Yes, Blend::Off, Modulation::On> *) override;
    void write0(Rect<Size::S1, Textured::Yes, Blend::Semi, Modulation::On> *) override;
    void write0(Rect<Size::S8, Textured::No, Blend::Off, Modulation::On> *) override;
    void write0(Rect<Size::S8, Textured::No, Blend::Semi, Modulation::On> *) override;
    void write0(Rect<Size::S8, Textured::Yes, Blend::Off, Modulation::On> *) override;
    void write0(Rect<Size::S8, Textured::Yes, Blend::Semi, Modulation::On> *) override;
    void write0(Rect<Size::S16, Textured::No, Blend::Off, Modulation::On> *) override;
    void write0(Rect<Size::S16, Textured::No, Blend::Semi, Modulation::On> *) override;
    void write0(Rect<Size::S16, Textured::Yes, Blend::Off, Modulation::On> *) override;
    void write0(Rect<Size::S16, Textured::Yes, Blend::Semi, Modulation::On> *) override;

    void write0(BlitVramVram *) override;

    void write0(TPage *) override;
    void write0(TWindow *) override;
    void write0(DrawingAreaStart *) override;
    void write0(DrawingAreaEnd *) override;
    void write0(DrawingOffset *) override;
    void write0(MaskBit *) override;
};

}  // namespace SoftGPU

}  // namespace PCSX
