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
#include "pxr/imaging/glf/ptexMipmapTextureLoader.h"
#include "pxr/base/arch/fileSystem.h"

#include <Ptexture.h>
#include <vector>
#include <list>
#include <algorithm>
#include <cstring>
#include <cassert>

PXR_NAMESPACE_OPEN_SCOPE


// sample neighbor pixels and populate around blocks
void
GlfPtexMipmapTextureLoader::Block::guttering(
    GlfPtexMipmapTextureLoader *loader,
    PtexTexture *ptex, int level,
    int wid, int hei,
    unsigned char *pptr, int bpp,
    int stride)
{
    int lineBufferSize = std::max(wid, hei) * bpp;
    unsigned char * lineBuffer = new unsigned char[lineBufferSize];

    for (int edge = 0; edge < 4; edge++) {
        int len = (edge == 0 || edge == 2) ? wid : hei;
        loader->sampleNeighbor(lineBuffer, this->index, edge, len, bpp);

        unsigned char *s = lineBuffer, *d;
        for (int j = 0; j < len; ++j) {
            d = pptr;
            switch (edge) {
            case Ptex::e_bottom:
                d += bpp * (j + 1);
                break;
            case Ptex::e_right:
                d += stride * (j + 1) + bpp * (wid+1);
                break;
            case Ptex::e_top:
                d += stride * (hei+1) + bpp*(len-j);
                break;
            case Ptex::e_left:
                d += stride * (len-j);
                break;
            }
            for (int k = 0; k < bpp; k++)
                 *d++ = *s++;
        }
    }
    delete[] lineBuffer;

    // fix corner pixels
    int numchannels = ptex->numChannels();
    float *accumPixel = new float[numchannels];
    int uv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};


    for (int edge = 0; edge < 4; edge++) {
        int du = uv[edge][0];
        int dv = uv[edge][1];

        /*  There are 3 cases when filling a corner pixel on gutter.

            case 1: Regular 4 valence
                    We already have correct 'B' and 'C' pixels by edge
                    resampling above.
                    so here only one more pixel 'D' is needed,
                    and it will be placed on the gutter corner.
               +-----+-----+
               |     |     |<-current
               |    B|A    |
               +-----*-----+
               |    D|C    |
               |     |     |
               +-----+-----+

            case 2: T-vertex case (note that this doesn't mean 3 valence)
                    If the current face comes from non-quad root face, there
                    could be a T-vertex on its corner. Just like case 1,
                    need to fill border corner with pixel 'D'.
               +-----+-----+
               |     |     |<-current
               |    B|A    |
               |     *-----+
               |    D|C    |
               |     |     |
               +-----+-----+

            case 3: Other than 4 valence case
                        (everything else, including boundary)
                    Since guttering pixels are placed on the border of each
                    ptex faces, it's not possible to store more than 4 pixels
                    at a coner for a reasonable interpolation.
                    In this case, we need to average all corner pixels and
                    overwrite with an averaged value, so that every face
                    vertex picks the same value.
               +---+---+
               |   |   |<-current
               |  B|A  |
               +---*---|
               | D/E\C |
               | /   \ |
               |/     \|
               +-------+
         */

        // seamless mipmap only works with square faces.
        if (loader->getCornerPixel(accumPixel, numchannels,
                                   this->index, edge, (int8_t)(this->ulog2-level))) {
            // case1, case 2
            if (edge == 1 || edge == 2) du += wid;
            if (edge == 2 || edge == 3) dv += hei;
            unsigned char *d = pptr + dv*stride + du*bpp;
            Ptex::ConvertFromFloat(d, accumPixel,
                                   ptex->dataType(), numchannels);
        } else {
            // case 3, set accumPixel to the corner 4 pixels
            if (edge == 1 || edge == 2) du += wid - 1;
            if (edge == 2 || edge == 3) dv += hei - 1;
            for (int x = 0; x < 2; ++x) {
                for (int y = 0; y < 2; ++y) {
                    unsigned char *d = pptr + (dv+x)*stride + (du+y)*bpp;
                    Ptex::ConvertFromFloat(d, accumPixel,
                                           ptex->dataType(), numchannels);
                }
            }
        }
    }
    delete[] accumPixel;
}

void
GlfPtexMipmapTextureLoader::Block::Generate(
    GlfPtexMipmapTextureLoader *loader,
    PtexTexture *ptex,
    unsigned char *destination,
    int bpp, int wid, int maxLevels)
{
    const Ptex::FaceInfo &faceInfo = ptex->getFaceInfo(index);
    int stride = bpp * wid;

    int ulog2_ = this->ulog2;
    int vlog2_ = this->vlog2;

    int level = 0;
    int uofs = u, vofs = v;

    // The minimum size of non-subface is 4x4, so that it matches with adjacent
    // 2x2 subfaces.
    int limit = faceInfo.isSubface() ? 1 : 2;

    // but if the base size is already less than limit, we'd like to pick it
    // instead of nothing.
    limit = std::min(std::min(limit, ulog2_), vlog2_);

    while (ulog2_ >= limit && vlog2_ >= limit
           && (maxLevels == -1 || level <= maxLevels)) {
        if (level % 2 == 1)
            uofs += (1<<(ulog2_+1))+2;
        if ((level > 0) && (level % 2 == 0))
            vofs += (1<<(vlog2_+1)) + 2;

        unsigned char *dst = destination + vofs * stride + uofs * bpp;
        unsigned char *dstData = destination
            + (vofs + 1) * stride
            + (uofs + 1) * bpp;
        ptex->getData(index, dstData, stride, Ptex::Res(ulog2_, vlog2_));

        guttering(loader, ptex, level, 1<<ulog2_, 1<<vlog2_, dst, bpp, stride);

        --ulog2_;
        --vlog2_;
        ++level;
    }
    nMipmaps = level;
}

void
GlfPtexMipmapTextureLoader::Block::SetSize(unsigned char ulog2_,
                                           unsigned char vlog2_, bool mipmap)
{
    ulog2 = ulog2_;
    vlog2 = vlog2_;

    int w = 1 << ulog2;
    int h = 1 << vlog2;

    // includes mipmap
    if (mipmap) {
        w = w + w/2 + 4;
        h = h + 2;
    }

    width = (int16_t)w;
    height = (int16_t)h;
}

// ---------------------------------------------------------------------------

struct GlfPtexMipmapTextureLoader::Page
{
    struct Slot
    {
        Slot(uint16_t u_, uint16_t v_,
             uint16_t w_, uint16_t h_) :
            u(u_), v(v_), width(w_), height(h_) { }

        uint16_t u, v, width, height;

        // returns true if a block can fit in this slot
        bool Fits(const Block *block) {
            return (block->width <= width) && (block->height <= height);
        }
    };

    typedef std::list<Block *> BlockList;

    Page(uint16_t width, uint16_t height) {
        _slots.push_back(Slot(0, 0, width, height));
    }

    bool IsFull() const {
        return _slots.empty();
    }

    // true when the block "b" is successfully added to this  page :
    //
    //  |--------------------------|       |------------|-------------|
    //  |                          |       |............|             |
    //  |                          |       |............|             |
    //  |                          |       |.... B .....| Right Slot  |
    //  |                          |       |............|             |
    //  |                          |       |............|             |
    //  |                          |       |------------|-------------|
    //  |      Original Slot       |  ==>  |                          |
    //  |                          |       |                          |
    //  |                          |       |       Bottom Slot        |
    //  |                          |       |                          |
    //  |                          |       |                          |
    //  |--------------------------|       |--------------------------|
    //
    bool AddBlock(Block *block) {
        for (SlotList::iterator it = _slots.begin(); it != _slots.end(); ++it) {
            if (it->Fits(block)) {
                _blocks.push_back(block);

                block->u = it->u;
                block->v = it->v;

                // add new slot to the right
                if (it->width > block->width) {
                    _slots.push_front(Slot(it->u + block->width,
                                           it->v,
                                           it->width - block->width,
                                           block->height));
                }
                // add new slot to the bottom
                if (it->height > block->height) {
                    _slots.push_back(Slot(it->u,
                                          it->v + block->height,
                                          it->width,
                                          it->height - block->height));
                }
                _slots.erase(it);
                return true;
            }
        }
        return false;
    }

    void Generate(GlfPtexMipmapTextureLoader *loader, PtexTexture *ptex,
                  unsigned char *destination,
                  int bpp, int width, int maxLevels) {
        for (BlockList::iterator it = _blocks.begin();
             it != _blocks.end(); ++it) {
            (*it)->Generate(loader, ptex, destination, bpp, width, maxLevels);
        }
    }

    const BlockList &GetBlocks() const {
        return _blocks;
    }

    void Dump() const {
        for (BlockList::const_iterator it = _blocks.begin();
             it != _blocks.end(); ++it) {
            printf(" (%d, %d)  %d x %d\n",
                   (*it)->u, (*it)->v, (*it)->width, (*it)->height);
        }
    }

private:
    BlockList _blocks;

    typedef std::list<Slot> SlotList;
    SlotList _slots;
};

// ---------------------------------------------------------------------------

// Utility class for Ptex corner iteration
class GlfPtexMipmapTextureLoader::CornerIterator
{
public:
    CornerIterator(PtexTexture *ptex, int face, int edge, int8_t reslog2) :
        _ptex(ptex),
        _startFace(face), _startEdge(edge),
        _currentFace(face), _currentEdge(edge), _reslog2(reslog2),
        _clockWise(true), _mid(false), _done(false), _isBoundary(true) {

        _numChannels = _ptex->numChannels();
        _currentInfo = _ptex->getFaceInfo(_currentFace);
        if (_currentInfo.isSubface()) ++_reslog2;
    }

    int GetCurrentFace() const {
        return _currentFace;
    }

    void GetPixel(float *resultPixel) {
        int8_t r = (int8_t)(_currentInfo.isSubface() ? _reslog2 - 1 : _reslog2);

        // limit to the maximum ptex resolution
        r = std::min(std::min(r, _currentInfo.res.ulog2),
                     _currentInfo.res.vlog2);
        Ptex::Res res(r, r);
        int uv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        int u = uv[_currentEdge][0] * (res.u()-1);
        int v = uv[_currentEdge][1] * (res.v()-1);

        _ptex->getPixel(_currentFace, u, v, resultPixel, 0, _numChannels, res);
    }

    bool IsDone() const {
        return _done;
    }

    bool IsSubface() const {
        return _currentInfo.isSubface();
    }

    bool IsBoundary() const {
        return _isBoundary;
    }

    void Next() {
        if (_done) return;

        // next face
        Ptex::FaceInfo info = _ptex->getFaceInfo(_currentFace);

        if (_clockWise) {
            _currentFace = info.adjface(_currentEdge);
            if (_mid) {
                _currentFace = _ptex->getFaceInfo(_currentFace).adjface(2);
                _currentEdge = 1;
                _mid = false;
            } else if (info.isSubface() && 
                (!_ptex->getFaceInfo(_currentFace).isSubface()) &&
                _currentEdge == 3) {
                _mid = true;
                _currentEdge = info.adjedge(_currentEdge);
            } else {
                _mid = false;
                _currentEdge = info.adjedge(_currentEdge);
                _currentEdge = (_currentEdge+1)%4;
            }
        } else {
            _currentFace = info.adjface((_currentEdge+3)%4);
            _currentEdge = info.adjedge((_currentEdge+3)%4);
        }

        if (_currentFace == -1) {
            // border case.
            if (_clockWise) {
                // reset position and restart counter clock wise
                Ptex::FaceInfo sinfo = _ptex->getFaceInfo(_startFace);
                _currentFace = sinfo.adjface((_startEdge+3)%4);
                _currentEdge = sinfo.adjedge((_startEdge+3)%4);
                _clockWise = false;
            } else {
                // end
                _done = true;
                return;
            }
        }
        Ptex::FaceInfo nextFaceInfo = _ptex->getFaceInfo(_currentFace);
        if ((!_clockWise) && 
            (!info.isSubface()) && (nextFaceInfo.isSubface())) {
             // needs tricky traverse for boundary subface...
             if (_currentEdge == 3) {
                 _currentFace = nextFaceInfo.adjface(2);
                 _currentEdge = 0;
             }
        }

        if (_currentFace == -1) {
            _done = true;
            return;
        }

        if (_currentFace == _startFace) {
            _done = true;
            _isBoundary = false;
            return;
        }

        _currentInfo = _ptex->getFaceInfo(_currentFace);
    }

private:
    PtexTexture *_ptex;
    int _numChannels;
    int _startFace, _startEdge;
    int _currentFace, _currentEdge;
    int8_t _reslog2;
    bool _clockWise;
    bool _mid;
    bool _done;
    bool _isBoundary;
    Ptex::FaceInfo _currentInfo;
};

// ---------------------------------------------------------------------------

GlfPtexMipmapTextureLoader::GlfPtexMipmapTextureLoader(PtexTexture *ptex,
                                                       int maxNumPages,
                                                       int maxLevels,
                                                       size_t targetMemory,
                                                       bool seamlessMipmap) :
    _ptex(ptex), _maxLevels(maxLevels), _bpp(0),
    _pageWidth(0), _pageHeight(0), _texelBuffer(NULL), _layoutBuffer(NULL),
    _memoryUsage(0)
{
    // byte per pixel
    _bpp = ptex->numChannels() * Ptex::DataSize(ptex->dataType());

    int numFaces = ptex->numFaces();
    _blocks.resize(numFaces);

    for (int i = 0; i < numFaces; ++i) {
        const Ptex::FaceInfo &faceInfo = ptex->getFaceInfo(i);
        _blocks[i].index = i;
        if (seamlessMipmap) {
            // need to squarize ptex face
            unsigned char s = std::min(faceInfo.res.ulog2, faceInfo.res.vlog2);
            _blocks[i].SetSize(s, s, _maxLevels != 0);
        } else {
            _blocks[i].SetSize(faceInfo.res.ulog2,
                               faceInfo.res.vlog2,
                               _maxLevels != 0);
        }
    }

    optimizePacking(maxNumPages, targetMemory);
    generateBuffers();
}

GlfPtexMipmapTextureLoader::~GlfPtexMipmapTextureLoader()
{
    for (size_t i = 0; i < _pages.size(); ++i) {
        delete _pages[i];
    }
    delete _texelBuffer;
    delete _layoutBuffer;
}

// resample border texels for guttering
//
int
GlfPtexMipmapTextureLoader::resampleBorder(int face, int edgeId,
                                           unsigned char *result,
                                           int dstLength, int bpp,
                                           float srcStart, float srcEnd)
{
    Ptex::Res res(_blocks[face].ulog2, _blocks[face].vlog2);

    int edgeLength = (edgeId == 0 || edgeId == 2) ? res.u() : res.v();
    int srcOffset = (int)(srcStart*edgeLength);
    int srcLength = (int)((srcEnd-srcStart)*edgeLength);

    if (dstLength >= srcLength) {
        // copy or up sampling (nearest)
        PtexFaceData * data = _ptex->getData(face, res);
        unsigned char *border = new unsigned char[bpp*srcLength];

        // order of the result will be flipped to match adjacent pixel order
        for (int i = 0; i < srcLength; ++i) {
            int u = 0, v = 0;
            if (edgeId == Ptex::e_bottom) {
                u = edgeLength-1-(i+srcOffset);
                v = 0;
            } else if (edgeId == Ptex::e_right) {
                u = res.u()-1;
                v = edgeLength-1-(i+srcOffset);
            } else if (edgeId == Ptex::e_top) {
                u = i+srcOffset;
                v = res.v()-1;
            } else if (edgeId == Ptex::e_left) {
                u = 0;
                v = i+srcOffset;
            }
            data->getPixel(u, v, &border[i*bpp]);
        }

        // nearest resample to fit dstLength
        for (int i = 0; i < dstLength; ++i) {
            for (int j = 0; j < bpp; j++) {
                result[i*bpp+j] = border[(i*srcLength/dstLength)*bpp+j];
            }
        }
        data->release();
        delete[] border;
    } else {
        // down sampling
        while (srcLength > dstLength && res.ulog2 && res.vlog2) {
            --res.ulog2;
            --res.vlog2;
            srcLength /= 2;
        }

        PtexFaceData * data = _ptex->getData(face, res);
        unsigned char *border = new unsigned char[bpp*srcLength];
        edgeLength = (edgeId == 0 || edgeId == 2) ? res.u() : res.v();
        srcOffset = (int)(srcStart*edgeLength);

        for (int i = 0; i < dstLength; ++i) {
            int u = 0, v = 0;
            if (edgeId == Ptex::e_bottom) {
                u = edgeLength-1-(i+srcOffset);
                v = 0;
            } else if (edgeId == Ptex::e_right) {
                u = res.u() - 1;
                v = edgeLength-1-(i+srcOffset);
            } else if (edgeId == Ptex::e_top) {
                u = i+srcOffset;
                v = res.v() - 1;
            } else if (edgeId == Ptex::e_left) {
                u = 0;
                v = i+srcOffset;
            }
            data->getPixel(u, v, &border[i*bpp]);

            for (int j = 0; j < bpp; ++j) {
                result[i*bpp+j] = border[i*bpp+j];
            }
        }

        data->release();
        delete[] border;
    }

    return srcLength;
}

// flip order of pixel buffer
static void
flipBuffer(unsigned char *buffer, int length, int bpp)
{
    for (int i = 0; i < length/2; ++i) {
        for (int j = 0; j < bpp; j++) {
            std::swap(buffer[i*bpp+j], buffer[(length-1-i)*bpp+j]);
        }
    }
}

// sample neighbor face's edge
void
GlfPtexMipmapTextureLoader::sampleNeighbor(unsigned char *border, int face,
                                           int edge, int length, int bpp)
{
    const Ptex::FaceInfo &fi = _ptex->getFaceInfo(face);

    // copy adjacent borders
    int adjface = fi.adjface(edge);
    if (adjface != -1) {
        int ae = fi.adjedge(edge);
        if (!fi.isSubface() && _ptex->getFaceInfo(adjface).isSubface()) {
            /* nonsubface -> subface (1:0.5)  see http://ptex.us/adjdata.html for more detail
              +------------------+
              |       face       |
              +--------edge------+
              | adj face |       |
              +----------+-------+
            */
            resampleBorder(adjface, ae, border, length/2, bpp);
            const Ptex::FaceInfo &sfi1 = _ptex->getFaceInfo(adjface);
            adjface = sfi1.adjface((ae+3)%4);
            ae = (sfi1.adjedge((ae+3)%4)+3)%4;
            resampleBorder(adjface, ae, border+(length/2*bpp),
                           length/2, bpp);

        } else if (fi.isSubface() && !_ptex->getFaceInfo(adjface).isSubface()) {
            /* subface -> nonsubface (0.5:1).   two possible configuration
                     case 1                    case 2
              +----------+----------+  +----------+----------+--------+
              |   face   |    B     |  |          |  face    |   B    |
              +---edge---+----------+  +----------+--edge----+--------+
              |0.0      0.5      1.0|  |0.0      0.5      1.0|
              |       adj face      |  |       adj face      |
              +---------------------+  +---------------------+
            */
            int Bf = fi.adjface((edge+1)%4);
            int Be = fi.adjedge((edge+1)%4);
            int f = _ptex->getFaceInfo(Bf).adjface((Be+1)%4);
            int e = _ptex->getFaceInfo(Bf).adjedge((Be+1)%4);
            if (f == adjface && e == ae)  // case 1
                resampleBorder(adjface, ae, border,
                               length, bpp, 0.0, 0.5);
            else  // case 2
                resampleBorder(adjface, ae, border,
                               length, bpp, 0.5, 1.0);

        } else {
            /*  ordinary case (1:1 match)
                +------------------+
                |       face       |
                +--------edge------+
                |    adj face      |
                +----------+-------+
            */
            resampleBorder(adjface, ae, border, length, bpp);
        }
    } else {
        /* border edge. duplicate itself
           +-----------------+
           |       face      |
           +-------edge------+
        */
        resampleBorder(face, edge, border, length, bpp);
        flipBuffer(border, length, bpp);
    }
}

// get corner pixel by traversing all adjacent faces around vertex
//
bool
GlfPtexMipmapTextureLoader::getCornerPixel(float *resultPixel, int numchannels,
                                           int face, int edge,
                                           int8_t reslog2)
{
    const Ptex::FaceInfo &fi = _ptex->getFaceInfo(face);

    /*
       see http://ptex.us/adjdata.html Figure 2 for the reason of conditions edge==1 and 3
    */

    if (fi.isSubface() && edge == 3) {
        /*
          in T-vertex case, this function sets 'D' pixel value to *resultPixel and returns false
                gutter line
                |
          +------+-------+
          |      |       |
          |     D|C      |<-- gutter line
          |      *-------+
          |     B|A [2]  |
          |      |[3] [1]|
          |      |  [0]  |
          +------+-------+
        */
        int adjface = fi.adjface(edge);
        if (adjface != -1 && !_ptex->getFaceInfo(adjface).isSubface()) {
            int adjedge = fi.adjedge(edge);

            Ptex::Res res(std::min((int)_blocks[adjface].ulog2, reslog2+1),
                          std::min((int)_blocks[adjface].vlog2, reslog2+1));

            int uv[2] = {0, 0};
            if (adjedge == 0) {
                uv[0] = res.u()/2;
                uv[1] = 0;
            } else if (adjedge == 1) {
                uv[0] = res.u()-1;
                uv[1] = res.v()/2;
            } else if (adjedge == 2) {
                uv[0] = res.u()/2-1;
                uv[1] = res.v()-1;
            } else {
                uv[0] = 0;
                uv[1] = res.v()/2-1;
            }

            _ptex->getPixel(adjface, uv[0], uv[1],
                            resultPixel, 0, numchannels, res);
            return true;
        }
    }
    if (fi.isSubface() && edge == 1) {
        /*      gutter line
                |
          +------+-------+
          |      |  [3]  |
          |      |[0] [2]|
          |     B|A [1]  |
          |      *-------+
          |     D|C      |<-- gutter line
          |      |       |
          +------+-------+

             note: here we're focusing on vertex A which corresponds to the edge 1,
                   but the edge 0 is an adjacent edge to get D pixel.
        */
        int adjface = fi.adjface(0);
        if (adjface != -1 && !_ptex->getFaceInfo(adjface).isSubface()) {
            int adjedge = fi.adjedge(0);
            Ptex::Res res(std::min((int)_blocks[adjface].ulog2, reslog2+1),
                          std::min((int)_blocks[adjface].vlog2, reslog2+1));

            int uv[2] = {0, 0};
            if (adjedge == 0) {
                uv[0] = res.u()/2-1;
                uv[1] = 0;
            } else if (adjedge == 1) {
                uv[0] = res.u()-1;
                uv[1] = res.v()/2-1;
            } else if (adjedge == 2) {
                uv[0] = res.u()/2;
                uv[1] = res.v()-1;
            } else {
                uv[0] = 0;
                uv[1] = res.v()/2;
            }

            _ptex->getPixel(adjface, uv[0], uv[1],
                            resultPixel, 0, numchannels, res);
            return true;
        }
    }

    float *pixel = (float*)alloca(sizeof(float)*numchannels);
    float *accumPixel = (float*)alloca(sizeof(float)*numchannels);
    // clear accum pixel
    memset(accumPixel, 0, sizeof(float)*numchannels);

    // iterate faces around the vertex
    int numFaces = 0;
    CornerIterator it(_ptex, face, edge, reslog2);
    for (; !it.IsDone(); it.Next(), ++numFaces) {
        it.GetPixel(pixel);

        // accumulate pixel value
        for (int j = 0; j < numchannels; ++j) {
            accumPixel[j] += pixel[j];
            if (numFaces == 2) {
                // also save the diagonal pixel for regular corner case
                resultPixel[j] = pixel[j];
            }
        }
    }
    // if regular corner, returns diagonal pixel without averaging
    if (numFaces == 4 && (!it.IsBoundary())) {
        return true;
    }

    // non-4 valence. let's average and return false;
    for (int j = 0; j < numchannels; ++j) {
        resultPixel[j] = accumPixel[j]/numFaces;
    }
    return false;
}

int
GlfPtexMipmapTextureLoader::getLevelDiff(int face, int edge)
{
    // returns the highest mipmap level difference around the vertex
    // at face/edge
    Ptex::FaceInfo faceInfo = _ptex->getFaceInfo(face);

    // note: seamless interpolation only works for square tex faces.
    int8_t baseRes = _blocks[face].ulog2;
    if (faceInfo.isSubface()) ++baseRes;

    int maxDiff = 0;
    CornerIterator it(_ptex, face, edge, baseRes);
    for (; !it.IsDone(); it.Next()) {
        int res = _blocks[it.GetCurrentFace()].ulog2;
        if (it.IsSubface()) ++res;
        maxDiff = std::max(maxDiff, baseRes - res);
    }
    return maxDiff;
}

void
GlfPtexMipmapTextureLoader::optimizePacking(int maxNumPages,
                                            size_t targetMemory)
{
    size_t numTexels = 0;

    // prepare a list of pointers
    typedef std::vector<Block> BlockArray;
    typedef std::list<Block *> BlockPtrList;
    BlockPtrList blocks;
    for (BlockArray::iterator it = _blocks.begin(); it != _blocks.end(); ++it) {
        blocks.push_back(&(*it));
        numTexels += it->GetNumTexels();
    }

    // sort blocks by height-width order
    blocks.sort(Block::sort);

    // try to fit into the target memory size if specified
    if (targetMemory != 0 && _bpp * numTexels > targetMemory) {
        size_t numTargetTexels = targetMemory / _bpp;
        while (numTexels > numTargetTexels) {
            Block *block = blocks.front();

            if (block->ulog2 < 2 || block->vlog2 < 2) break;

            // pick a smaller mipmap
            numTexels -= block->GetNumTexels();
            block->SetSize((unsigned char)(block->ulog2-1),
                           (unsigned char)(block->vlog2-1), _maxLevels != 0);
            numTexels += block->GetNumTexels();

            // move to the last
            blocks.pop_front();
            blocks.push_back(block);
        }
    }

    // compute page size ---------------------------------------------
    {
        // page size is set to the largest edge of the largest block :
        // this is the smallest possible page size, which should minimize
        // the texels wasted on the "last page" when the smallest blocks are
        // being packed.
        int w = 0, h = 0;
        for (BlockPtrList::iterator it = blocks.begin();
             it != blocks.end(); ++it) {
            w = std::max(w, (int)(*it)->width);
            h = std::max(h, (int)(*it)->height);
        }

        // grow the pagesize to make sure the optimization will not exceed
        // the maximum number of pages allowed
        int minPageSize = 512;
        int maxPageSize = 4096;  // XXX:should be configurable.

        // use minPageSize if too small
        if (w < minPageSize) w = w*(minPageSize/w + 1);
        if (h < minPageSize) h = h*(minPageSize/h + 1);

        // rough estimate of num pages
        int estimatedNumPages = (int)numTexels/w/h;

        // if expecting too many pages, increase page size
        int pageLimit = std::max(1, maxNumPages/2);
        if (estimatedNumPages > pageLimit) {
            w = std::min(w*(estimatedNumPages/pageLimit), maxPageSize);
            estimatedNumPages = (int)numTexels/w/h;
        }
        if (estimatedNumPages > pageLimit) {
            h = std::min(h*(estimatedNumPages/pageLimit), maxPageSize);
        }

        _pageWidth = w;
        _pageHeight = h;
    }

    // pack blocks into slots ----------------------------------------
    size_t firstslot = 0;
    for (BlockPtrList::iterator it = blocks.begin();
         it != blocks.end(); ++it) {
        Block *block = *it;

        // traverse existing pages for a suitable slot ---------------
        bool added = false;
        for (size_t p = firstslot; p < _pages.size(); ++p) {
            if ((added = _pages[p]->AddBlock(block)) == true) {
                break;
            }
        }
        // if none of page was found : start new page
        if (!added) {
            Page *page = new Page(_pageWidth, _pageHeight);
            added = page->AddBlock(block);
            // XXX -- Should not use assert().
            assert(added);
            _pages.push_back(page);
        }

        // adjust the page flag to the first page with open slots
        if (_pages.size() > (firstslot+1) && 
            _pages[firstslot+1]->IsFull()) ++firstslot;
    }

    // set corner pixel mipmap factors
    for (BlockArray::iterator it = _blocks.begin(); it != _blocks.end(); ++it) {
        int face = it->index;
        uint16_t adjSizeDiffs = 0;
        for (int edge = 0; edge < 4; ++edge) {
            int levelDiff = getLevelDiff(face, edge);
            adjSizeDiffs <<= 4;
            adjSizeDiffs |= (uint16_t)levelDiff;
        }
        it->adjSizeDiffs = adjSizeDiffs;
        // printf("Block %d, %08x\n", it->index, adjSizeDiffs);
    }

#if 0
    for (size_t i = 0; i < _pages.size(); ++i) {
        printf("Page %ld : \n", i);
        _pages[i]->Dump();
    }
#endif
}

void
GlfPtexMipmapTextureLoader::generateBuffers()
{
    // ptex layout struct
    // struct Layout {
    //     uint16_t page;
    //     uint16_t nMipmap;
    //     uint16_t u;
    //     uint16_t v;
    //     uint16_t adjSizeDiffs; //(4:4:4:4)
    //     uint8_t  width log2;
    //     uint8_t  height log2;
    // };

    int numFaces = (int)_blocks.size();
    int numPages = (int)_pages.size();

    // populate the texels
    int pageStride = _bpp * _pageWidth * _pageHeight;

    _texelBuffer = new unsigned char[pageStride * numPages];
    _memoryUsage = pageStride * numPages;
    memset(_texelBuffer, 0, pageStride * numPages);

    for (int i = 0; i < numPages; ++i) {
        _pages[i]->Generate(this, _ptex, _texelBuffer + pageStride * i,
                            _bpp, _pageWidth, _maxLevels);
    }

    // populate the layout texture buffer
    _layoutBuffer = new unsigned char[numFaces * sizeof(uint16_t) * 6];
    _memoryUsage += numFaces * sizeof(uint16_t) * 6;
    for (int i = 0; i < numPages; ++i) {
        Page *page = _pages[i];
        for (Page::BlockList::const_iterator it = page->GetBlocks().begin();
             it != page->GetBlocks().end(); ++it) {
            int ptexIndex = (*it)->index;
            uint16_t *p = (uint16_t*)(_layoutBuffer
                                      + sizeof(uint16_t)*6*ptexIndex);
            *p++ = (uint16_t)i;  // page
            *p++ = (uint16_t)((*it)->nMipmaps-1);
            *p++ = (uint16_t)((*it)->u+1);
            *p++ = (uint16_t)((*it)->v+1);
            *p++ = (*it)->adjSizeDiffs;
            *p++ = (uint16_t)(((*it)->ulog2 << 8) | (*it)->vlog2);
        }
    }

#if 0
    // debug
    FILE *fp = ArchOpenFile("out.ppm", "w");
    fprintf(fp, "P3\n");
    fprintf(fp, "%d %d\n", _pageWidth, _pageHeight * numPages);
    fprintf(fp, "255\n");
    unsigned char *p = _texelBuffer;
    for (int i = 0; i < numPages; ++i) {
        for (int y = 0; y < _pageHeight; ++y) {
            for (int x = 0; x < _pageWidth; ++x) {
                fprintf(fp, "%d %d %d ", (int)p[0], (int)p[1], (int)p[2]);
                p += _bpp;
            }
            fprintf(fp, "\n");
        }
    }
    fclose(fp);
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE

