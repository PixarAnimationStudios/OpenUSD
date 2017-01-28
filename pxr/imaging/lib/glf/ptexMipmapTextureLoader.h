//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef GLF_OPENSUBDIV_EXAMPLES_PTEX_MIPMAP_TEXTURE_LOADER_H
#define GLF_OPENSUBDIV_EXAMPLES_PTEX_MIPMAP_TEXTURE_LOADER_H

#include "pxr/pxr.h"
#include <Ptexture.h>
#include <stdlib.h>
#include <stdint.h>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class GlfPtexMipmapTextureLoader {
public:
    GlfPtexMipmapTextureLoader(PtexTexture *ptex,
                               int maxNumPages,
                               int maxLevels = -1,
                               size_t targetMemory = 0,
                               bool seamlessMipmap = true);

    ~GlfPtexMipmapTextureLoader();

    const unsigned char * GetLayoutBuffer() const {
        return _layoutBuffer;
    }
    const unsigned char * GetTexelBuffer() const {
        return _texelBuffer;
    }
    int GetNumFaces() const {
        return (int)_blocks.size();
    }
    int GetNumPages() const {
        return (int)_pages.size();
    }
    int GetPageWidth() const {
        return _pageWidth;
    }
    int GetPageHeight() const {
        return _pageHeight;
    }
    size_t GetMemoryUsage() const {
        return _memoryUsage;
    }

/*
  block : atomic texture unit
  XXX: face of 128x128 or more (64kb~) texels should be considered separately
       using ARB_sparse_texture...?

  . : per-face texels for each mipmap level
  x : guttering pixel

  xxxxxxxxxxxxxx
  x........xx..x 2x2
  x........xx..x
  x........xxxxx
  x..8x8...xxxxxxx
  x........xx....x
  x........xx....x 4x4
  x........xx....x
  x........xx....x
  xxxxxxxxxxxxxxxx

  For each face (w*h), texels with guttering and mipmap is stored into
  (w+2+w/2+2)*(h+2) area as above.

 */

/*
  Ptex loader

  Texels buffer : the packed texels

 */

private:
    struct Block {
        int index;                 // ptex index
        int nMipmaps;
        uint16_t u, v;             // top-left texel offset
        uint16_t width, height;    // texel dimension (includes mipmap)
        uint16_t adjSizeDiffs;     // maximum tile size difference around each vertices
        int8_t   ulog2, vlog2;     // texel dimension log2 (original tile)

        void Generate(GlfPtexMipmapTextureLoader *loader, PtexTexture *ptex,
                      unsigned char *destination,
                      int bpp, int width, int maxLevels);

        void SetSize(unsigned char ulog2_, unsigned char vlog2_, bool mipmap);

        int GetNumTexels() const {
            return width*height;
        }

        void guttering(GlfPtexMipmapTextureLoader *loader, PtexTexture *ptex,
                       int level, int width, int height,
                       unsigned char *pptr, int bpp, int stride);

        static bool sort(const Block *a, const Block *b) {
            return (a->height > b->height) ||
                   ((a->height == b->height) && (a->width > b->width));
        }
    };

    struct Page;
    class CornerIterator;

    void generateBuffers();
    void optimizePacking(int maxNumPages, size_t targetMemory);
    int  getLevelDiff(int face, int edge);
    bool getCornerPixel(float *resultPixel, int numchannels,
                        int face, int edge, int8_t res);
    void sampleNeighbor(unsigned char *border,
                        int face, int edge, int length, int bpp);
    int  resampleBorder(int face, int edgeId, unsigned char *result,
                        int dstLength, int bpp,
                        float srcStart = 0.0f, float srcEnd = 1.0f);

    std::vector<Block> _blocks;
    std::vector<Page *> _pages;

    PtexTexture *_ptex;
    int _maxLevels;
    int _bpp;
    int _pageWidth, _pageHeight;

    unsigned char *_texelBuffer;
    unsigned char *_layoutBuffer;

    size_t _memoryUsage;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // GLF_OPENSUBDIV_EXAMPLES_PTEX_MIPMAP_TEXTURE_LOADER_H
