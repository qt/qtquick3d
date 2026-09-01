// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Regenerates the fixtures in tests/auto/exr/data with OpenEXR. Not part of
// the build, see README.md.

#include <ImfHeader.h>
#include <ImfOutputFile.h>
#include <ImfTiledOutputFile.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfTileDescription.h>
#include <ImathBox.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace Imf;
using namespace Imath;

// Exactly representable in both half and float.
static float sample(int x, int y, int c)
{
    return (x * 7 + y * 13 + c * 29) / 256.0f;
}

struct Plane
{
    std::vector<float> f;
    std::vector<half> h;
};

static void fillPlanes(std::vector<Plane> &planes, int w, int h, size_t channelCount)
{
    planes.resize(channelCount);
    for (size_t c = 0; c < channelCount; ++c) {
        planes[c].f.assign(size_t(w) * h, 0.0f);
        planes[c].h.assign(size_t(w) * h, half(0.0f));
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const float v = sample(x, y, int(c));
                planes[c].f[size_t(y) * w + x] = v;
                planes[c].h[size_t(y) * w + x] = half(v);
            }
        }
    }
}

static void addSlices(FrameBuffer &fb, const std::vector<std::string> &names, std::vector<Plane> &planes, PixelType pt, int w, int xOrigin, int yOrigin)
{
    for (size_t c = 0; c < names.size(); ++c) {
        char *base = pt == FLOAT ? (char *)planes[c].f.data() : (char *)planes[c].h.data();
        const size_t elementSize = pt == FLOAT ? sizeof(float) : sizeof(half);
        // Slice addresses absolute pixel coordinates, so bias the base pointer.
        base -= (ptrdiff_t(yOrigin) * w + xOrigin) * ptrdiff_t(elementSize);
        fb.insert(names[c], Slice(pt, base, elementSize, elementSize * size_t(w)));
    }
}

static void writeScanline(const std::string &path,
                          const std::vector<std::string> &names,
                          int w,
                          int h,
                          Compression compression,
                          PixelType pt,
                          int xOrigin = 0,
                          int yOrigin = 0,
                          bool decreasingY = false)
{
    Header header(Box2i(V2i(0, 0), V2i(xOrigin + w - 1, yOrigin + h - 1)),
                  Box2i(V2i(xOrigin, yOrigin), V2i(xOrigin + w - 1, yOrigin + h - 1)));
    header.compression() = compression;
    if (decreasingY)
        header.lineOrder() = DECREASING_Y;
    for (const auto &name : names)
        header.channels().insert(name, Channel(pt));

    std::vector<Plane> planes;
    fillPlanes(planes, w, h, names.size());
    FrameBuffer fb;
    addSlices(fb, names, planes, pt, w, xOrigin, yOrigin);

    OutputFile out(path.c_str(), header);
    out.setFrameBuffer(fb);
    out.writePixels(h);
    printf("  %s\n", path.c_str());
}

static void writeTiled(const std::string &path,
                       const std::vector<std::string> &names,
                       int w,
                       int h,
                       Compression compression,
                       PixelType pt,
                       int tileWidth,
                       int tileHeight,
                       LevelMode levelMode = ONE_LEVEL)
{
    Header header(w, h);
    header.compression() = compression;
    for (const auto &name : names)
        header.channels().insert(name, Channel(pt));
    header.setTileDescription(TileDescription(tileWidth, tileHeight, levelMode));

    std::vector<Plane> planes;
    fillPlanes(planes, w, h, names.size());
    FrameBuffer fb;
    addSlices(fb, names, planes, pt, w, 0, 0);

    TiledOutputFile out(path.c_str(), header);
    out.setFrameBuffer(fb);
    for (int ly = 0; ly < out.numYLevels(); ++ly) {
        for (int lx = 0; lx < out.numXLevels(); ++lx) {
            if (levelMode == RIPMAP_LEVELS || lx == ly)
                out.writeTiles(0, out.numXTiles(lx) - 1, 0, out.numYTiles(ly) - 1, lx, ly);
        }
    }
    printf("  %s\n", path.c_str());
}

int main(int argc, char **argv)
{
    const std::string dir = argc > 1 ? argv[1] : ".";
    const std::vector<std::string> rgb = { "B", "G", "R" };
    const std::vector<std::string> rgba = { "A", "B", "G", "R" };
    const std::vector<std::string> single = { "R" };
    // Z and foo must not shift the planes of R, G and B.
    const std::vector<std::string> extra = { "A", "B", "G", "R", "Z", "foo" };

    printf("PIZ, which the test cannot encode itself:\n");
    // 32 + 8 rows for PIZ and 16 + 16 + 8 for ZIP, so both end on a short block.
    writeScanline(dir + "/piz_half_scanline_rgb_16x40.exr", rgb, 16, 40, PIZ_COMPRESSION, HALF);
    writeScanline(dir + "/piz_half_scanline_extra_16x24.exr", extra, 16, 24, PIZ_COMPRESSION, HALF);
    // 33x33 makes the wavelet take both its odd column and odd row passes.
    writeScanline(dir + "/piz_half_scanline_rgb_33x33.exr", rgb, 33, 33, PIZ_COMPRESSION, HALF);
    writeScanline(dir + "/piz_float_scanline_r_1x1.exr", single, 1, 1, PIZ_COMPRESSION, FLOAT);
    writeTiled(dir + "/piz_float_tiled_rgba_17x9_t8.exr", rgba, 17, 9, PIZ_COMPRESSION, FLOAT, 8, 8);

    printf("tiled layouts:\n");
    // 17x9 over 8x8 tiles has a partial right column and a partial bottom row.
    writeTiled(dir + "/zip_half_tiled_rgba_17x9_t8.exr", rgba, 17, 9, ZIP_COMPRESSION, HALF, 8, 8);
    writeTiled(dir + "/zip_half_tiled_rgb_9x5_t1.exr", rgb, 9, 5, ZIP_COMPRESSION, HALF, 1, 1);
    writeTiled(dir + "/zip_half_tiled_rgb_9x5_t64.exr", rgb, 9, 5, ZIP_COMPRESSION, HALF, 64, 64);
    writeTiled(dir + "/zip_half_mipmap_rgb_16x16_t8.exr", rgb, 16, 16, ZIP_COMPRESSION, HALF, 8, 8, MIPMAP_LEVELS);
    writeTiled(dir + "/zip_half_ripmap_rgb_16x8_t8.exr", rgb, 16, 8, ZIP_COMPRESSION, HALF, 8, 8, RIPMAP_LEVELS);

    printf("reference encodings of the codecs the test also builds itself:\n");
    writeScanline(dir + "/rle_half_scanline_rgb_16x24.exr", rgb, 16, 24, RLE_COMPRESSION, HALF);
    writeScanline(dir + "/zips_half_scanline_rgb_16x24.exr", rgb, 16, 24, ZIPS_COMPRESSION, HALF);
    writeScanline(dir + "/zip_float_scanline_rgba_16x8.exr", rgba, 16, 8, ZIP_COMPRESSION, FLOAT);
    writeScanline(dir + "/none_half_scanline_r_16x8.exr", single, 16, 8, NO_COMPRESSION, HALF);

    printf("geometry:\n");
    // A data window off the origin, smaller than the display window.
    writeScanline(dir + "/piz_half_scanline_offset_16x8.exr", rgb, 16, 8, PIZ_COMPRESSION, HALF, 7, 3);
    writeScanline(dir + "/zip_half_scanline_decreasing_16x40.exr", rgb, 16, 40, ZIP_COMPRESSION, HALF, 0, 0, true);

    printf("codecs we reject:\n");
    writeScanline(dir + "/unsupported_pxr24.exr", rgb, 8, 8, PXR24_COMPRESSION, FLOAT);
    writeScanline(dir + "/unsupported_b44.exr", rgb, 8, 8, B44_COMPRESSION, HALF);
    writeScanline(dir + "/unsupported_b44a.exr", rgb, 8, 8, B44A_COMPRESSION, HALF);
    writeScanline(dir + "/unsupported_dwaa.exr", rgb, 8, 8, DWAA_COMPRESSION, HALF);
    writeScanline(dir + "/unsupported_dwab.exr", rgb, 8, 8, DWAB_COMPRESSION, HALF);

    printf("special float values:\n");
    {
        Header header(8, 8);
        header.compression() = ZIP_COMPRESSION;
        header.channels().insert("R", Channel(FLOAT));
        // Cycles through the values a decoder is most likely to mishandle.
        const float specials[8] = { 0.0f, -0.0f, 1.17549435e-38f, 65504.0f, 1e30f, -1e30f, INFINITY, -INFINITY };
        std::vector<float> v(64);
        for (int i = 0; i < 64; ++i)
            v[i] = specials[i % 8];
        FrameBuffer fb;
        fb.insert("R", Slice(FLOAT, (char *)v.data(), sizeof(float), sizeof(float) * 8));
        const std::string path = dir + "/zip_float_scanline_specials_8x8.exr";
        OutputFile out(path.c_str(), header);
        out.setFrameBuffer(fb);
        out.writePixels(8);
        printf("  %s\n", path.c_str());
    }

    return 0;
}
