//
// Copyright 2023 Pixar
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

// openexr_c.h and openexr_c.c
// are a "single file" standalone version of the OpenEXRCore library.
// It's not part of the regular OpenEXR build. These two files are meant
// to be added to your project, either as a library with just the single
// standalone files, or added directly to your project.
// note that the prefix "nanoexr" is a proposal and subject to change

#ifndef openexr_c_h
#define openexr_c_h

#include "OpenEXRCore/openexr.h"

#include "OpenEXRCore/openexr_attr.h"
#include "OpenEXRCore/openexr_context.h"
#include "OpenEXRCore/openexr_part.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int nanoexr_get_attribute_count(exr_const_context_t, int part_index);
void nanoexr_attr_set_string(
    exr_context_t ctxt, int part_index, const char* name, const char* s);
void nanoexr_attr_set_int(
    exr_context_t ctxt, int part_index, const char* name, int v);
void nanoexr_attr_set_float(
    exr_context_t ctxt, int part_index, const char* name, float v);
void nanoexr_attr_set_double(
    exr_context_t ctxt, int part_index, const char* name, double v);
void nanoexr_attr_set_m44f(
    exr_context_t ctxt, int part_index, const char* name, const float* v);
void nanoexr_attr_set_m44d(
    exr_context_t ctxt, int part_index, const char* name, const double* v);

exr_result_t nanoexr_get_attribute_by_index(
    exr_const_context_t     ctxt,
    int                     part_index,
    int                     i,
    const exr_attribute_t** outattr);

// structure to hold image data that is read from an EXR file
typedef struct {
    uint8_t* data;
    size_t dataSize;
    exr_pixel_type_t pixelType;
    int channelCount; // 1 for luminance, 3 for RGB, 4 for RGBA
    int width, height;
    int dataWindowMinY, dataWindowMaxY;
} nanoexr_ImageData_t;

typedef enum {
    nanoexr_WrapModeClampToEdge = 0,
    nanoexr_WrapModeMirrorClampToEdge,
    nanoexr_WrapModeRepeat,
    nanoexr_WrapModeMirrorRepeat,
    nanoexr_WrapModeClampToBorderColor
} nanoexr_WrapMode;

typedef struct {
    char* filename;
    bool isScanline;
    int partIndex;
    exr_pixel_type_t pixelType;
    int channelCount;
    int width, height;
    int tileLevelCount;
    nanoexr_WrapMode wrapMode;
    int numMipLevels;
    int exrSDKVersionMajor;
    int exrSDKVersionMinor;
    int exrSDKVersionPatch;
    const char* exrSDKExtraInfo;
} nanoexr_Reader_t;

// given a filename and a reader, set up defaults in the reader
void nanoexr_set_defaults(const char* filename, nanoexr_Reader_t* reader);

// free any memory allocated by the reader, but not the reader itself
void nanoexr_free_storage(nanoexr_Reader_t* reader);

const char* nanoexr_get_error_code_as_string(exr_result_t code);
int         nanoexr_getPixelTypeSize(exr_pixel_type_t t);

// reads an entire tiled image into memory
// returns any exr_result_t error code encountered upon reading
// if no error, returns EXR_ERR_SUCCESS
//
// imageData is a pointer to a nanoexr_ImageData_t struct supplied
// by the caller.  The data pointer in this struct will be set to
// point to the image data, and the dataSize field will be set to
// the size of the data in bytes.  The caller is responsible for
// freeing the data pointer when it is no longer needed.

// callback to allow a user to process attributes as desired at a
// point when the context is available during header reading
typedef void (*nanoexr_attrRead)(void*, exr_context_t);

exr_result_t nanoexr_read_header(nanoexr_Reader_t* reader, 
                                 exr_read_func_ptr_t,
                                 nanoexr_attrRead, void* callback_userData,
                                 int partIndex);

exr_result_t nanoexr_read_exr(const char* filename,
                              exr_read_func_ptr_t readfn,
                              void* callback_userData,
                              nanoexr_ImageData_t* img,
                              const char* layerName,
                              int numChannelsToRead,
                              int partIndex,
                              int level);

// callback to allow a user to add attributes to a context as desired
typedef void (*nanoexr_attrsAdd)(void*, exr_context_t);

// simplified write for the most basic case of a single part file containing
// rgb data in half format.
exr_result_t nanoexr_write_exr(
               const char* filename,
               nanoexr_attrsAdd, void* attrsAdd_userData,
               int width, int height, bool flipped,
               exr_pixel_type_t pixel_type,
               uint8_t* red,   int32_t redPixelStride,   int32_t redLineStride,
               uint8_t* green, int32_t greenPixelStride, int32_t greenLineStride,
               uint8_t* blue,  int32_t bluePixelStride,  int32_t blueLineStride,
               uint8_t* alpha, int32_t alphaPixelStride, int32_t alphaLineStride);

void nanoexr_release_image_data(nanoexr_ImageData_t* imageData);

bool nanoexr_Gaussian_resample(const nanoexr_ImageData_t* src,
                               nanoexr_ImageData_t* dst);

#ifdef __cplusplus
}
#endif

#endif
