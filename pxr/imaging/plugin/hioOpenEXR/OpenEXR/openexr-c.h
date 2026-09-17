//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

int32_t nanoexr_get_attribute_count(exr_const_context_t, int32_t part_index);
void nanoexr_attr_set_string(
    exr_context_t ctxt, int32_t part_index, const char* name, const char* s);
void nanoexr_attr_set_int(
    exr_context_t ctxt, int32_t part_index, const char* name, int32_t v);
void nanoexr_attr_set_float(
    exr_context_t ctxt, int32_t part_index, const char* name, float v);
void nanoexr_attr_set_double(
    exr_context_t ctxt, int32_t part_index, const char* name, double v);
void nanoexr_attr_set_m44f(
    exr_context_t ctxt, int32_t part_index, const char* name, const float* v);
void nanoexr_attr_set_m44d(
    exr_context_t ctxt, int32_t part_index, const char* name, const double* v);

exr_result_t nanoexr_get_attribute_by_index(
    exr_const_context_t     ctxt,
    int32_t                 part_index,
    int32_t                 i,
    const exr_attribute_t** outattr);

// structure to hold image data that is read from an EXR file
typedef struct {
    uint8_t* data;
    size_t dataSize;
    exr_pixel_type_t pixelType;
    int32_t channelCount; // 1 for luminance, 3 for RGB, 4 for RGBA
    int32_t width, height;
    int32_t dataWindowMinY, dataWindowMaxY;
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
    int32_t partIndex;
    exr_pixel_type_t pixelType;
    int32_t channelCount;
    int32_t width, height;
    int32_t tileLevelCount;
    nanoexr_WrapMode wrapMode;
    int32_t numMipLevels;
    int32_t exrSDKVersionMajor;
    int32_t exrSDKVersionMinor;
    int32_t exrSDKVersionPatch;
    const char* exrSDKExtraInfo;
} nanoexr_Reader_t;

typedef enum {
    NANOEXR_READ_HEADER,
    NANOEXR_START_READ,
    NANOEXR_GET_DATA_WINDOW,
    NANOEXR_GET_STORAGE,
    NANOEXR_GET_TILE_LEVELS,
    NANOEXR_GET_CHANNELS,

    NANOEXR_START_WRITE,
    NANOEXR_ADD_CHANNEL,
    NANOEXR_ADD_PART,
    NANOEXR_SET_COMPRESSION,
    NANOEXR_ADD_ATTR,
    NANOEXR_ADD_HEADER,
    NANOEXR_ENCODE,
} nanoexr_AuxCode_t;

typedef struct {
    nanoexr_AuxCode_t nanoexrAuxCode;
    exr_error_code_t  exrErrorCode;
} nanoexr_ErrorCode_t;

const char* nanoexr_get_default_aux_message(nanoexr_AuxCode_t);

// given a filename and a reader, set up defaults in the reader
void nanoexr_set_defaults(const char* filename, nanoexr_Reader_t* reader);

// free any memory allocated by the reader, but not the reader itself
void nanoexr_free_storage(nanoexr_Reader_t* reader);

const char* nanoexr_get_error_code_as_string(exr_result_t code);
const char* nanoexr_get_default_error_message(exr_result_t code);
int32_t     nanoexr_getPixelTypeSize(exr_pixel_type_t t);

// callback to allow a user to process attributes as desired at a
// point when the context is available during header reading
typedef void (*nanoexr_attrRead)(void*, exr_context_t);

// Read an OpenEXR file hader, and process any attributes
// encountered in the header.

nanoexr_ErrorCode_t 
nanoexr_read_header(nanoexr_Reader_t* reader, 
                    exr_read_func_ptr_t,
                    nanoexr_attrRead, void* callback_userData,
                    int32_t partIndex);


// reads an entire tiled image into memory
// returns any exr_result_t error code encountered upon reading
// if no error, returns EXR_ERR_SUCCESS
//
// imageData is a pointer to a nanoexr_ImageData_t struct supplied
// by the caller.  The data pointer in this struct will be set to
// point to the image data, and the dataSize field will be set to
// the size of the data in bytes.  The caller is responsible for
// freeing the data pointer when it is no longer needed.

exr_result_t nanoexr_read_exr(const char* filename,
                              exr_read_func_ptr_t readfn,
                              void* callback_userData,
                              nanoexr_ImageData_t* img,
                              const char* layerName,
                              int32_t numChannelsToRead,
                              int32_t partIndex,
                              int32_t level);

// callback to allow a user to add attributes to a context as desired
typedef void (*nanoexr_attrsAdd)(void*, exr_context_t);

// simplified write for the most basic case of a single part file containing
// rgb data in half format.
nanoexr_ErrorCode_t nanoexr_write_exr(
               const char* filename,
               nanoexr_attrsAdd, void* attrsAdd_userData,
               int32_t width, int32_t height, bool flipped,
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
