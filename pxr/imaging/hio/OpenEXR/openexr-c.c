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

#include "pxr/base/arch/pragmas.h"

// Not all functions in the OpenEXR library are used by Hio, and the OpenEXR
// symbols themselves are declared static for inclusion within Hio.
// Therefore, the unused function warning is suppressed as the messages are
// not useful for development, as it is expected that many functions are
// defined but not referenced or exported.
ARCH_PRAGMA_UNUSED_FUNCTION

#include "OpenEXRCoreUnity.h"

#include <ctype.h>
#include <math.h>

// re-export the statically hidden exr_ functions as required
// for visibility from C++

exr_result_t nanoexr_get_attribute_by_index(
    exr_const_context_t     ctxt,
    int                     part_index,
    int                     i,
    const exr_attribute_t** outattr)
{
    return exr_get_attribute_by_index(ctxt, part_index, 
                                      EXR_ATTR_LIST_SORTED_ORDER, i, outattr);
}

int nanoexr_get_attribute_count(exr_const_context_t ctxt, int part_index) {
    int count = 0;
    exr_get_attribute_count(ctxt, part_index, &count);
    return count;
}

void nanoexr_attr_set_string(exr_context_t ctxt, int part_index, 
                             const char* name, const char* s) {
    exr_attr_set_string(ctxt, part_index, name, s);
}

void nanoexr_attr_set_int(exr_context_t ctxt, int part_index, 
                          const char* name, int v) {
    exr_attr_set_int(ctxt, part_index, name, v);
}

void nanoexr_attr_set_float(exr_context_t ctxt, int part_index, 
                            const char* name, float v) {
    exr_attr_set_float(ctxt, part_index, name, v);
}

void nanoexr_attr_set_double(exr_context_t ctxt, int part_index, 
                             const char* name, double v) {
    exr_attr_set_double(ctxt, part_index, name, v);
}

void nanoexr_attr_set_m44f(exr_context_t ctxt, int part_index, 
                           const char* name, const float* v) {
    exr_attr_set_m44f(ctxt, part_index, name, (exr_attr_m44f_t*) v);
}

void nanoexr_attr_set_m44d(exr_context_t ctxt, int part_index, 
                           const char* name, const double* v) {
    exr_attr_set_m44d(ctxt, part_index, name, (exr_attr_m44d_t*) v);
}

const char* nanoexr_get_error_code_as_string (exr_result_t code)
{
    return exr_get_error_code_as_string(code);
}

static float integrate_gaussian(float x, float sigma)
{
  float p1 = erf((x - 0.5f) / sigma * sqrtf(0.5f));
  float p2 = erf((x + 0.5f) / sigma * sqrtf(0.5f));
  return (p2-p1) * 0.5f;
}

bool nanoexr_Gaussian_resample(const nanoexr_ImageData_t* src,
                               nanoexr_ImageData_t* dst)
{
    if (src->pixelType != EXR_PIXEL_FLOAT && dst->pixelType != EXR_PIXEL_FLOAT)
        return false;
    if (src->channelCount != dst->channelCount)
        return false;
    
    const int srcWidth  = src->width;
    const int dstWidth  = dst->width;
    const int srcHeight = src->height;
    const int dstHeight = dst->height;
    if (srcWidth == dstWidth && srcHeight == dstHeight) {
        memcpy(dst->data, src->data, 
               src->channelCount * srcWidth * srcHeight * sizeof(float));
        return true;
    }
    
    float* srcData = (float*)src->data;
    float* dstData = (float*)dst->data;

    // two pass image resize using a Gaussian filter per:
    // https://bartwronski.com/2021/10/31/practical-gaussian-filter-binomial-filter-and-small-sigma-gaussians
    // chose sigma to suppress high frequencies that can't be represented 
    // in the downsampled image
    const float ratio_w = (float)dstWidth / (float)srcWidth;
    const float ratio_h = (float)dstHeight / (float)srcHeight;
    const float sigma_w = 1.f / 2.f * ratio_w;
    const float sigma_h = 1.f / 2.f * ratio_h;
    const float support = 0.995f;
    float radius = ceilf(sqrtf(-2.0f * sigma_w * sigma_w * logf(1.0f - support)));
    int filterSize_w = (int)radius;
    if (!filterSize_w)
        return false;
    
    float* filter_w = (float*) malloc(sizeof(float) * (filterSize_w + 1) * 2);
    float sum = 0.0f;
    for (int i = 0; i <= filterSize_w; i++) {
        int idx = i + filterSize_w;
        filter_w[idx] = integrate_gaussian((float) i, sigma_w);
        if (i > 0)
            sum += 2 * filter_w[idx];
        else
            sum = filter_w[idx];
    }
    for (int i = 0; i <= filterSize_w; ++i) {
        filter_w[i + filterSize_w] /= sum;
    }
    for (int i = 0; i < filterSize_w; ++i) {
        filter_w[filterSize_w - i - 1] = filter_w[i + filterSize_w + 1];
    }
    int fullFilterSize_w = filterSize_w * 2 + 1;

    // again for height
    radius = ceilf(sqrtf(-2.0f * sigma_h * sigma_h * logf(1.0f - support)));
    int filterSize_h = (int)radius;
    if (!filterSize_h)
        return false;
    
    float* filter_h = (float*) malloc(sizeof(float) * (1 + filterSize_h) * 2);
    sum = 0.0f;
    for (int i = 0; i <= filterSize_h; i++) {
        int idx = i + filterSize_h;
        filter_h[idx] = integrate_gaussian((float) i, sigma_h);
        if (i > 0)
            sum += 2 * filter_h[idx];
        else
            sum = filter_h[idx];
    }
    for (int i = 0; i <= filterSize_h; ++i) {
        filter_h[i + filterSize_h] /= sum;
    }
    for (int i = 0; i < filterSize_h; ++i) {
        filter_h[filterSize_h - i - 1] = filter_h[i + filterSize_h + 1];
    }
    int fullFilterSize_h = filterSize_h * 2 + 1;
    
    // first pass: resize horizontally
    int srcFloatsPerLine = src->channelCount * srcWidth;
    int dstFloatsPerLine = src->channelCount * dstWidth;
    float* firstPass = (float*)malloc(dstWidth * src->channelCount * srcHeight * sizeof(float));
    for (int y = 0; y < srcHeight; ++y) {
        for (int x = 0; x < dstWidth; ++x) {
            for (int c = 0; c < src->channelCount; ++c) {
                float sum = 0.0f;
                for (int i = 0; i < fullFilterSize_w; ++i) {
                    int srcX = (int)((x + 0.5f) / ratio_w - 0.5f) + i - filterSize_w;
                    if (srcX < 0 || srcX >= srcWidth)
                        continue;
                    int idx = y * srcFloatsPerLine + (srcX * src->channelCount) + c;
                    sum += srcData[idx] * filter_w[i];
                }
                firstPass[y * dstFloatsPerLine + (x * src->channelCount) + c] = sum;
            }
        }
    }

    // second pass: resize vertically
    float* secondPass = dstData;
    for (int y = 0; y < dstHeight; ++y) {
        for (int x = 0; x < dstWidth; ++x) {
            for (int c = 0; c < src->channelCount; ++c) {
                float sum = 0.0f;
                for (int i = 0; i < fullFilterSize_h; ++i) {
                    int srcY = (int)((y + 0.5f) / ratio_h - 0.5f) + i - filterSize_h;
                    if (srcY < 0 || srcY >= srcHeight)
                        continue;
                    int idx = src->channelCount * srcY * dstWidth + (x * src->channelCount) + c;
                    sum += firstPass[idx] * filter_h[i];
                }
                secondPass[dst->channelCount * y * dstWidth + (x * dst->channelCount) + c] = sum;
            }
        }
    }
    free(filter_h);
    free(filter_w);
    free(firstPass);
    return true;
}

static void
err_cb (exr_const_context_t f, int code, const char* msg)
{
    fprintf(stderr, "err_cb ERROR %d: %s\n", code, msg);
}

void nanoexr_set_defaults(const char* filename, nanoexr_Reader_t* reader) {
    exr_get_library_version (&reader->exrSDKVersionMajor,
                             &reader->exrSDKVersionMinor,
                             &reader->exrSDKVersionPatch,
                             &reader->exrSDKExtraInfo);

    reader->filename = strdup(filename);
    reader->width = 0;
    reader->height = 0;
    reader->channelCount = 0;
    reader->pixelType = EXR_PIXEL_LAST_TYPE;
    reader->partIndex = 0;
    reader->numMipLevels = 0;
    reader->isScanline = false;
    reader->tileLevelCount = 0;
    reader->wrapMode = nanoexr_WrapModeClampToEdge;
}


void nanoexr_free_storage(nanoexr_Reader_t* reader) {
    if (!reader)
        return;
    free(reader->filename);
}

int nanoexr_read_header(nanoexr_Reader_t* reader, exr_read_func_ptr_t readFn,
                        nanoexr_attrRead attrRead, void* callback_userData,
                        int partIndex) {
    if (!reader)
        return EXR_ERR_INVALID_ARGUMENT;
    
    exr_context_t exr;
    exr_context_initializer_t init = EXR_DEFAULT_CONTEXT_INITIALIZER;
    init.read_fn = readFn;
    init.user_data = callback_userData;
    int rv = exr_start_read(&exr, reader->filename, &init);
    if (rv != EXR_ERR_SUCCESS) {
        exr_finish(&exr);
        return rv;
    }

    exr_attr_box2i_t datawin;
    rv = exr_get_data_window(exr, partIndex, &datawin);
    if (rv != EXR_ERR_SUCCESS) {
        exr_finish(&exr);
        return rv;
    }
    reader->partIndex = partIndex;
    reader->width = datawin.max.x - datawin.min.x + 1;
    reader->height = datawin.max.y - datawin.min.y + 1;

    exr_storage_t storage;
    rv = exr_get_storage(exr, partIndex, &storage);
    if (rv != EXR_ERR_SUCCESS) {
        exr_finish(&exr);
        return rv;
    }
    reader->isScanline = (storage == EXR_STORAGE_SCANLINE);

    int numMipLevelsX = 1, numMipLevelsY = 1;
    if (reader->isScanline) {
        numMipLevelsX = 1;
        numMipLevelsY = 1;
    } else {
        rv = exr_get_tile_levels(exr, partIndex, &numMipLevelsX, &numMipLevelsY);
        if (rv != EXR_ERR_SUCCESS) {
            exr_finish(&exr);
            return rv;
        }
    }
    if (numMipLevelsX != numMipLevelsY) {
        // current only supporting mip levels uniformly in both directions
        numMipLevelsX = 1;
        numMipLevelsY = 1;
    }
    reader->numMipLevels = numMipLevelsX;

    const exr_attr_chlist_t* chlist = NULL;
    rv = exr_get_channels(exr, partIndex, &chlist);
    if (rv != EXR_ERR_SUCCESS) {
        exr_finish(&exr);
        return rv;
    }
    reader->channelCount = chlist->num_channels;
    reader->pixelType = chlist->entries[0].pixel_type;

    const exr_attribute_t* attr = NULL;
    exr_result_t wrap_rv = exr_get_attribute_by_name(exr, partIndex, 
                                                     "wrapmodes", &attr);
    if (wrap_rv == EXR_ERR_SUCCESS && attr != NULL) {
        if (!strncmp("black", attr->string->str, 5))
            reader->wrapMode = nanoexr_WrapModeClampToBorderColor;
        else if (!strncmp("clamp", attr->string->str, 5))
            reader->wrapMode = nanoexr_WrapModeClampToEdge;
        else if (!strncmp("periodic", attr->string->str, 8))
            reader->wrapMode = nanoexr_WrapModeRepeat;
        else if (!strncmp("mirror", attr->string->str, 6))
            reader->wrapMode = nanoexr_WrapModeMirrorRepeat;
    }

    if (attrRead)
        attrRead(callback_userData, exr);

    exr_finish(&exr);
    return rv;
}

exr_result_t nanoexr_write_exr(
    const char* filename,
    nanoexr_attrsAdd attrsAdd, void* attrsAdd_userData,
    int width, int height, bool flipped,
    exr_pixel_type_t pixel_type,
    uint8_t* red,   int32_t redPixelStride,   int32_t redLineStride,
    uint8_t* green, int32_t greenPixelStride, int32_t greenLineStride,
    uint8_t* blue,  int32_t bluePixelStride,  int32_t blueLineStride,
    uint8_t* alpha, int32_t alphaPixelStride, int32_t alphaLineStride)
{

    int channelCount = red ? 1 : 0;
    channelCount += blue ? 1 : 0;
    channelCount += green ? 1 : 0;
    channelCount += alpha ? 1 : 0;
    if (!channelCount) {
        return EXR_ERR_INVALID_ARGUMENT;
    }

    int partidx = 0;
    exr_context_t exr;
    exr_context_initializer_t init = EXR_DEFAULT_CONTEXT_INITIALIZER;

    // switch to write mode after everything is set up
    /// XXX improvement: use EXR_INTERMEDIATE_TEMP_FILE
    exr_result_t result = exr_start_write(
                                &exr, filename, EXR_WRITE_FILE_DIRECTLY, &init);
    if (result != EXR_ERR_SUCCESS) {
        return result;
    }

    result = exr_add_part(exr, "beauty", EXR_STORAGE_SCANLINE, &partidx);
    if (result != EXR_ERR_SUCCESS) {
        return result;
    }

    // modern exr should support long names
    exr_set_longname_support(exr, 1);

    /// XXX In the future Hio may be able to specify compression levels
    result = exr_set_zip_compression_level(exr, 0, 4);
    if (result != EXR_ERR_SUCCESS) {
        return result;
    }

    exr_attr_box2i_t dataw = {0, 0, width - 1, height - 1};
    exr_attr_box2i_t dispw = dataw;
    exr_attr_v2f_t   swc   = {0.5f, 0.5f}; // center of the screen window
    result = exr_initialize_required_attr (
        exr,
        partidx,
        &dataw,
        &dispw,
        1.f,    // pixel aspect ratio
        &swc,
        1.f,    // screen window width corresponding to swc
        EXR_LINEORDER_INCREASING_Y,
        EXR_COMPRESSION_ZIPS); // one line per chunk, ZIP is 16
    if (result != EXR_ERR_SUCCESS) {
        return result;
    }

    if (red) {
        result = exr_add_channel(
                     exr,
                     partidx,
                     "R",
                     pixel_type,
                     EXR_PERCEPTUALLY_LOGARITHMIC, // hint that data is an image
                     1, 1); // x & y sampling rate
        if (result != EXR_ERR_SUCCESS) {
            return result;
        }
    }

    if (green) {
        result = exr_add_channel(
                     exr,
                     partidx,
                     "G",
                     pixel_type,
                     EXR_PERCEPTUALLY_LOGARITHMIC,
                     1, 1); // x & y sampling rate
        if (result != EXR_ERR_SUCCESS) {
            return result;
        }
    }

    if (blue) {
        result = exr_add_channel(
                     exr,
                     partidx,
                     "B",
                     pixel_type,
                     EXR_PERCEPTUALLY_LOGARITHMIC,
                     1, 1); // x & y sampling rate
        if (result != EXR_ERR_SUCCESS) {
            return result;
        }
    }

    if (alpha) {
        result = exr_add_channel(
                     exr,
                     partidx,
                     "A",
                     pixel_type,
                     EXR_PERCEPTUALLY_LOGARITHMIC,
                     1, 1); // x & y sampling rate
        if (result != EXR_ERR_SUCCESS) {
            return result;
        }
    }

    result = exr_set_version(exr, partidx, 1); // 1 is the latest version

    // set chromaticities to Rec. ITU-R BT.709-3
    exr_attr_chromaticities_t chroma = {
        0.6400f, 0.3300f,  // red
        0.3000f, 0.6000f,  // green
        0.1500f, 0.0600f,  // blue
        0.3127f, 0.3290f}; // white
    result = exr_attr_set_chromaticities(exr, partidx, "chromaticities", &chroma);
    if (result != EXR_ERR_SUCCESS) {
        return result;
    }

    if (attrsAdd) {
        attrsAdd(attrsAdd_userData, exr);
    }

    result = exr_write_header(exr);
    if (result != EXR_ERR_SUCCESS) {
        return result;
    }
    
    exr_encode_pipeline_t encoder;
    exr_chunk_info_t cinfo;
    int32_t               scansperchunk = 0;
    exr_get_scanlines_per_chunk(exr, partidx, &scansperchunk);
    bool                  first = true;

    uint8_t* pRed;
    uint8_t* pGreen;
    uint8_t* pBlue;
    uint8_t* pAlpha;

    if (flipped) {
        pRed =   red + (height - 1) * redLineStride;
        pGreen = green + (height - 1) * greenLineStride;
        pBlue =  blue + (height - 1) * blueLineStride;
        pAlpha = alpha + (height - 1) * alphaLineStride;
    }
    else {
        pRed =   red;
        pGreen = green;
        pBlue =  blue;
        pAlpha = alpha;
    }
    
    int chunkInfoIndex = 0;
    for (int y = dataw.min.y; y <= dataw.max.y; y += scansperchunk, ++chunkInfoIndex) {
        result = exr_write_scanline_chunk_info(exr, partidx, y, &cinfo);
        if (result != EXR_ERR_SUCCESS) {
            return result;
        }

        if (first)
        {
            result = exr_encoding_initialize(exr, partidx, &cinfo, &encoder);
            if (result != EXR_ERR_SUCCESS) {
                return result;
            }
        }
        else
        {
            result = exr_encoding_update(exr, partidx, &cinfo, &encoder);
        }
        
        int c = 0;
        encoder.channel_count = channelCount;
        if (red) {
            encoder.channels[c].user_pixel_stride = redPixelStride;
            encoder.channels[c].user_line_stride  = redLineStride;
            encoder.channels[c].encode_from_ptr   = pRed;
            encoder.channels[c].height            = scansperchunk; // chunk height
            encoder.channels[c].width             = dataw.max.x - dataw.min.y + 1;
            ++c;
        }
        if (green) {
            encoder.channels[c].user_pixel_stride = greenPixelStride;
            encoder.channels[c].user_line_stride  = greenLineStride;
            encoder.channels[c].height            = scansperchunk; // chunk height
            encoder.channels[c].width             = dataw.max.x - dataw.min.y + 1;
            encoder.channels[c].encode_from_ptr   = pGreen;
            ++c;
        }
        if (blue) {
            encoder.channels[c].user_pixel_stride = bluePixelStride;
            encoder.channels[c].user_line_stride  = blueLineStride;
            encoder.channels[c].height            = scansperchunk; // chunk height
            encoder.channels[c].encode_from_ptr   = pBlue;
            ++c;
        }
        if (alpha) {
            encoder.channels[c].user_pixel_stride = alphaPixelStride;
            encoder.channels[c].user_line_stride  = alphaLineStride;
            encoder.channels[c].height            = scansperchunk; // chunk height
            encoder.channels[c].width             = dataw.max.x - dataw.min.y + 1;
            encoder.channels[c].encode_from_ptr   = pAlpha;
        }

        if (first) {
            result = exr_encoding_choose_default_routines(exr, partidx, &encoder);
            if (result != EXR_ERR_SUCCESS) {
                return result;
            }
        }

        result = exr_encoding_run(exr, partidx, &encoder);
        if (result != EXR_ERR_SUCCESS) {
            return result;
        }

        first = false;
        if (flipped) {
            pRed -= redLineStride;
            pGreen -= greenLineStride;
            pBlue -= blueLineStride;
            pAlpha -= alphaLineStride;
        }
        else {
            pRed += redLineStride;
            pGreen += greenLineStride;
            pBlue += blueLineStride;
            pAlpha += alphaLineStride;
        }
    }

    result = exr_encoding_destroy(exr, &encoder);
    if (result != EXR_ERR_SUCCESS) {
        return result;
    }

    result = exr_finish(&exr);
    return result;
}


int nanoexr_getPixelTypeSize(exr_pixel_type_t t)
{
    switch (t) {
        case EXR_PIXEL_HALF: return 2;
        case EXR_PIXEL_UINT: return 4;
        case EXR_PIXEL_FLOAT: return 4;
        default: return 0;
    }
}

static bool strIsRed(const char* layerName, const char* str) {
    if (layerName && (strncmp(layerName, str, strlen(layerName)) != 0))
        return false;

    // check if the case folded string is R or RED, or ends in .R or .RED
    char* folded = strdup(str);
    for (int i = 0; folded[i]; ++i) {
        folded[i] = tolower(folded[i]);
    }
    if (strcmp(folded, "r") == 0 || strcmp(folded, "red") == 0)
        return true;
    size_t l = strlen(folded);
    if ((l > 2) && (folded[l - 2] == '.') && (folded[l - 1] == 'r'))
        return true;
    if (l < 4)
        return false;
    return strcmp(folded + l - 4, ".red");
}

static bool strIsGreen(const char* layerName, const char* str) {
    if (layerName && (strncmp(layerName, str, strlen(layerName)) != 0))
        return false;

    // check if the case folded string is G or GREEN, or ends in .G or .GREEN
    char* folded = strdup(str);
    for (int i = 0; folded[i]; ++i) {
        folded[i] = tolower(folded[i]);
    }
    if (strcmp(folded, "g") == 0 || strcmp(folded, "green") == 0)
        return true;
    size_t l = strlen(folded);
    if ((l > 2) && (folded[l - 2] == '.') && (folded[l - 1] == 'g'))
        return true;
    if (l < 6)
        return false;
    return strcmp(folded + l - 6, ".green");
}

static bool strIsBlue(const char* layerName, const char* str) {
    if (layerName && (strncmp(layerName, str, strlen(layerName)) != 0))
        return false;

    // check if the case folded string is B or BLUE, or ends in .B or .BLUE
    char* folded = strdup(str);
    for (int i = 0; folded[i]; ++i) {
        folded[i] = tolower(folded[i]);
    }
    if (strcmp(folded, "b") == 0 || strcmp(folded, "blue") == 0)
        return true;
    size_t l = strlen(folded);
    if ((l > 2) && (folded[l - 2] == '.') && (folded[l - 1] == 'b'))
        return true;
    if (l < 5)
        return false;
    return strcmp(folded + l - 5, ".blue");
}

static bool strIsAlpha(const char* layerName, const char* str) {
    if (layerName && (strncmp(layerName, str, strlen(layerName)) != 0))
        return false;

    // check if the case folded string is A or ALPHA, or ends in .A or .ALPHA
    char* folded = strdup(str);
    for (int i = 0; folded[i]; ++i) {
        folded[i] = tolower(folded[i]);
    }
    if (strcmp(folded, "a") == 0 || strcmp(folded, "alpha") == 0)
        return true;
    size_t l = strlen(folded);
    if ((l > 2) && (folded[l - 2] == '.') && (folded[l - 1] == 'a'))
        return true;
    if (l < 6)
        return false;
    return strcmp(folded + l - 6, ".alpha");
}

void nanoexr_release_image_data(nanoexr_ImageData_t* imageData)
{
    if (imageData->data) {
        free(imageData->data);
        imageData->data = NULL;
    }
}

static void nanoexr_cleanup(exr_context_t exr, 
                             exr_decode_pipeline_t* decoder)
{
    if (exr && decoder) {
        exr_decoding_destroy(exr, decoder);
    }
}

static void
tiled_exr_err_cb (exr_const_context_t f, int code, const char* msg)
{
    fprintf(stderr, "err_cb ERROR %d: %s\n", code, msg);
}

static exr_result_t _nanoexr_rgba_decoding_initialize(
    exr_context_t exr,
    nanoexr_ImageData_t* img,
    const char* layerName,
    int partIndex, exr_chunk_info_t* cinfo, exr_decode_pipeline_t* decoder,
    int* rgba)
{
    exr_result_t rv = EXR_ERR_SUCCESS;
    rv = exr_decoding_initialize(exr, partIndex, cinfo, decoder);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "exr_decoding_initialize failed: %s\n", 
                exr_get_default_error_message(rv));
        return rv;
    }
    int bytesPerChannel = nanoexr_getPixelTypeSize(img->pixelType);
    for (int i = 0; i < img->channelCount; ++i) {
        rgba[i] = -1;
    }
    for (int c = 0; c < decoder->channel_count; ++c) {
        int channelIndex = -1;
        if (strIsRed(layerName, decoder->channels[c].channel_name)) {
            rgba[0] = c;
            channelIndex = 0;
        }
        else if (strIsGreen(layerName, decoder->channels[c].channel_name)) {
            rgba[1] = c;
            channelIndex = 1;
        }
        else if (strIsBlue(layerName, decoder->channels[c].channel_name)) {
            rgba[2] = c;
            channelIndex = 2;
        }
        else if (strIsAlpha(layerName, decoder->channels[c].channel_name)) {
            rgba[3] = c;
            channelIndex = 3;
        }
        if (channelIndex == -1 || channelIndex >= img->channelCount) {
            decoder->channels[c].decode_to_ptr = 0;
            continue;
        }
        // precompute pixel channel byte offset
        decoder->channels[c].decode_to_ptr = (uint8_t*) (ptrdiff_t) (channelIndex * bytesPerChannel);
    }
    return rv;
}

exr_result_t nanoexr_read_tiled_exr(exr_context_t exr,
                                    nanoexr_ImageData_t* img,
                                    const char* layerName,
                                    int partIndex,
                                    int mipLevel,
                                    int* rgbaIndex)
{
    exr_decode_pipeline_t decoder = EXR_DECODE_PIPELINE_INITIALIZER;
    exr_result_t rv = EXR_ERR_SUCCESS;
    do {
        int bytesPerChannel = nanoexr_getPixelTypeSize(img->pixelType);
        uint32_t tilew = 0, tileh = 0;
        exr_tile_level_mode_t levelMode;
        exr_tile_round_mode_t roundMode;
        rv = exr_get_tile_descriptor(exr, partIndex, &tilew, &tileh, &levelMode, &roundMode);
        if (rv != EXR_ERR_SUCCESS)
            break;

        int mipLevelsX = 0, mipLvelsY = 0;
        rv = exr_get_tile_levels(exr, partIndex, &mipLevelsX, &mipLvelsY);
        if (rv != EXR_ERR_SUCCESS)
            break;

        int levelWidth = 0, levelHeight = 0;
        rv = exr_get_level_sizes(exr, partIndex, mipLevel, mipLevel, &levelWidth, &levelHeight);
        if (rv != EXR_ERR_SUCCESS)
            break;

        int32_t xTiles = (img->width + tilew - 1) / tilew;
        int32_t yTiles = (img->height + tileh - 1) / tileh;
        int pixelStride = img->channelCount * bytesPerChannel;
        int imageYStride = img->width * pixelStride;
        
        memset(&decoder, 0, sizeof(decoder));
        for (int tileY = 0; tileY < yTiles; ++tileY) {
            for (int tileX = 0; tileX < xTiles; ++tileX) {
                exr_chunk_info_t cinfo;
                rv = exr_read_tile_chunk_info(exr, partIndex, 
                                              tileX, tileY, mipLevel, mipLevel, &cinfo);
                if (rv != EXR_ERR_SUCCESS)
                    break;
                if (decoder.channels == NULL) {
                    rv = _nanoexr_rgba_decoding_initialize(exr, img,
                                                           layerName, partIndex, &cinfo, &decoder, rgbaIndex);
                    if (rv != EXR_ERR_SUCCESS)
                        break;

                    rv = exr_decoding_choose_default_routines(exr, partIndex, &decoder);
                    if (rv != EXR_ERR_SUCCESS)
                        break;
                }
                else {
                    // Reuse existing pipeline
                    rv = exr_decoding_update(exr, partIndex, &cinfo, &decoder);
                    if (rv != EXR_ERR_SUCCESS)
                        break;
                }
                
                int x = tileX * tilew;
                int y = tileY * tileh;
                uint8_t* curtilestart = img->data;
                curtilestart += y * imageYStride;
                curtilestart += x * pixelStride;
                for (int c = 0; c < decoder.channel_count; ++c) {
                    if (rgbaIndex[c] >= 0) {
                        decoder.channels[c].decode_to_ptr = curtilestart + rgbaIndex[c] * bytesPerChannel;
                    }
                    else {
                        decoder.channels[c].decode_to_ptr = NULL;
                    }
                    decoder.channels[c].user_pixel_stride = pixelStride;
                    decoder.channels[c].user_line_stride = imageYStride;
                    decoder.channels[c].user_bytes_per_element = bytesPerChannel;
                }
                
                rv = exr_decoding_run(exr, partIndex, &decoder);
                if (rv != EXR_ERR_SUCCESS)
                    break;
            }
        }
    } while(false);
    
    if (rv != EXR_ERR_SUCCESS)
        fprintf(stderr, "nanoexr error: %s\n", exr_get_default_error_message(rv));

    nanoexr_cleanup(exr, &decoder);
    return rv;
}

exr_result_t nanoexr_read_scanline_exr(exr_context_t exr,
                                       nanoexr_ImageData_t* img,
                                       const char* layerName,
                                       int partIndex,
                                       int* rgbaIndex)
{
    exr_decode_pipeline_t decoder = EXR_DECODE_PIPELINE_INITIALIZER;
    exr_result_t rv = EXR_ERR_SUCCESS;

    // use a do/while(false) to allow error handling via a break and
    // check at the end.
    do {
        int scanLinesPerChunk;
        rv = exr_get_scanlines_per_chunk(exr, partIndex, &scanLinesPerChunk);
        if (rv != EXR_ERR_SUCCESS)
            break;
        
        int bytesPerChannel = nanoexr_getPixelTypeSize(img->pixelType);
        int pixelbytes = bytesPerChannel * img->channelCount;
        
        for (int chunky = img->dataWindowMinY; chunky < img->dataWindowMaxY; chunky += scanLinesPerChunk) {
            exr_chunk_info_t cinfo;
            rv = exr_read_scanline_chunk_info(exr, partIndex, chunky, &cinfo);
            if (rv != EXR_ERR_SUCCESS)
                break;

            if (decoder.channels == NULL) {
                rv = _nanoexr_rgba_decoding_initialize(exr, img, layerName,
                                                       partIndex, &cinfo, &decoder, rgbaIndex);
                if (rv != EXR_ERR_SUCCESS)
                    break;

                if (decoder.channels == NULL) {
                    rv = EXR_ERR_INCORRECT_CHUNK;
                    break;
                }
                bytesPerChannel = decoder.channels[0].bytes_per_element;
                pixelbytes = bytesPerChannel * img->channelCount;
                
                for (int c = 0; c < decoder.channel_count; ++c) {                
                    decoder.channels[c].user_pixel_stride = img->channelCount * bytesPerChannel;
                    decoder.channels[c].user_line_stride = decoder.channels[c].user_pixel_stride * img->width;
                    decoder.channels[c].user_bytes_per_element = bytesPerChannel;
                }
                
                rv = exr_decoding_choose_default_routines(exr, partIndex, &decoder);
                if (rv != EXR_ERR_SUCCESS)
                    break;
            }
            else {
                // Reuse existing pipeline
                rv = exr_decoding_update(exr, partIndex, &cinfo, &decoder);
                if (rv != EXR_ERR_SUCCESS)
                    break;
            }
            uint8_t* start = img->data + (chunky - img->dataWindowMinY) * img->width * pixelbytes;
            for (int c = 0; c < decoder.channel_count; ++c) {                
                decoder.channels[c].decode_to_ptr = start + rgbaIndex[c] * bytesPerChannel;
            }
            
            rv = exr_decoding_run(exr, partIndex, &decoder);
            if (rv != EXR_ERR_SUCCESS)
                break;
        }
    }
    while (false);
    
    if (rv != EXR_ERR_SUCCESS)
        fprintf(stderr, "nanoexr error: %s\n", exr_get_default_error_message(rv));

    nanoexr_cleanup(exr, &decoder);
    return rv;
}

void fill_channel_u16(nanoexr_ImageData_t* img, int channel, uint16_t value) {
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            uint8_t* curpixel = img->data + 
                y * img->width * img->channelCount * 2 + 
                x * img->channelCount * 2 + channel * 2;
            *(uint16_t*) curpixel = value;
        }
    }
}

void fill_channel_u32(nanoexr_ImageData_t* img, int channel, uint32_t value) {
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            uint8_t* curpixel = img->data + 
                y * img->width * img->channelCount * 4 + 
                x * img->channelCount * 4 + channel * 4;
            *(uint32_t*) curpixel = value;
        }
    }
}

void fill_channel_float(nanoexr_ImageData_t* img, int channel, float value) {
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            uint8_t* curpixel = img->data + 
                y * img->width * img->channelCount * 4 + 
                x * img->channelCount * 4 + channel * 4;
            *(float*) curpixel = value;
        }
    }
}

void copy_channel_u16(nanoexr_ImageData_t* img, int from_channel, int to_channel) {
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            uint8_t* curpixel = img->data + 
                y * img->width * img->channelCount * 2 + 
                x * img->channelCount * 2 + from_channel * 2;
            uint8_t* topixel = img->data + 
                y * img->width * img->channelCount * 2 + 
                x * img->channelCount * 2 + to_channel * 2;
            *(uint16_t*) topixel = *(uint16_t*) curpixel;
        }
    }
}

void copy_channel_u32(nanoexr_ImageData_t* img, int from_channel, int to_channel) {
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            uint8_t* curpixel = img->data + 
                y * img->width * img->channelCount * 4 + 
                x * img->channelCount * 4 + from_channel * 4;
            uint8_t* topixel = img->data + 
                y * img->width * img->channelCount * 4 + 
                x * img->channelCount * 4 + to_channel * 4;
            *(uint32_t*) topixel = *(uint32_t*) curpixel;
        }
    }
}

void copy_channel_float(nanoexr_ImageData_t* img, int from_channel, int to_channel) {
    for (int y = 0; y < img->height; ++y) {
        for (int x = 0; x < img->width; ++x) {
            uint8_t* curpixel = img->data + 
                y * img->width * img->channelCount * 4 + 
                x * img->channelCount * 4 + from_channel * 4;
            uint8_t* topixel = img->data + 
                y * img->width * img->channelCount * 4 + 
                x * img->channelCount * 4 + to_channel * 4;
            *(float*) topixel = *(float*) curpixel;
        }
    }
}

exr_result_t nanoexr_read_exr(const char* filename, 
                              exr_read_func_ptr_t readfn,
                              void* callback_userData,
                              nanoexr_ImageData_t* img,
                              const char* layerName,
                              int numChannelsToRead,
                              int partIndex,
                              int mipLevel) {
    exr_context_t exr = NULL;
    exr_result_t rv = EXR_ERR_SUCCESS;
    exr_context_initializer_t cinit = EXR_DEFAULT_CONTEXT_INITIALIZER;
    cinit.error_handler_fn = tiled_exr_err_cb;
    cinit.read_fn = readfn;
    cinit.user_data = callback_userData;
    rv = exr_test_file_header(filename, &cinit);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr header error: %s\n", 
                exr_get_default_error_message(rv));
        return rv;
    }

    rv = exr_start_read(&exr, filename, &cinit);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr start error: %s\n", 
               exr_get_default_error_message(rv));
        exr_finish(&exr);
        return rv;
    }
    exr_storage_t storage;
    rv = exr_get_storage(exr, partIndex, &storage);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr storage error: %s\n", 
                exr_get_default_error_message(rv));
        exr_finish(&exr);
        return rv;
    }

    int num_sub_images = 0;
    rv = exr_get_count(exr, &num_sub_images);
    if (rv != EXR_ERR_SUCCESS || partIndex >= num_sub_images) {
        fprintf(stderr, "nanoexr error: part index %d out of range\n", partIndex);
        exr_finish(&exr);
        return rv;
    }

    // check that compression type is understood
    exr_compression_t compression;
    rv = exr_get_compression(exr, partIndex, &compression);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr compression error: %s\n", 
                exr_get_default_error_message(rv));
        exr_finish(&exr);
        return rv;
    }

    exr_attr_box2i_t datawin;
    exr_attr_box2i_t displaywin;
    rv = exr_get_data_window(exr, partIndex, &datawin);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr data window error: %s\n", 
                exr_get_default_error_message(rv));
        exr_finish(&exr);
        return rv;
    }
    rv = exr_get_display_window(exr, partIndex, &displaywin);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr display window error: %s\n",
                exr_get_default_error_message(rv));
        exr_finish(&exr);
        return rv;
    }

    int width = datawin.max.x - datawin.min.x + 1;
    int height = datawin.max.y - datawin.min.y + 1;

    const exr_attr_chlist_t* chlist = NULL;
    rv = exr_get_channels(exr, partIndex, &chlist);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr channels error: %s\n", 
                exr_get_default_error_message(rv));
        exr_finish(&exr);
        return rv;
    }

    exr_pixel_type_t pixelType = chlist->entries[0].pixel_type;
    int bytesPerChannel = nanoexr_getPixelTypeSize(pixelType);
    if (bytesPerChannel == 0) {
        fprintf(stderr, "nanoexr error: unsupported pixel type\n");
        exr_finish(&exr);
        return rv;
    }

    img->channelCount = numChannelsToRead;
    img->width = width;
    img->height = height;
    img->dataSize = width * height * img->channelCount * bytesPerChannel;
    img->pixelType = pixelType;
    img->dataWindowMinY = datawin.min.y;
    img->dataWindowMaxY = datawin.max.y;
    img->data = (unsigned char*) malloc(img->dataSize);
    if (img->data == NULL) {
        fprintf(stderr, "nanoexr error: could not allocate memory for image data\n");
        exr_finish(&exr);
        return rv;
    }

    int rgbaIndex[4] = {-1, -1, -1, -1};

    if (storage == EXR_STORAGE_TILED) {
        rv = nanoexr_read_tiled_exr(exr, img, layerName, partIndex, mipLevel, rgbaIndex);
    }
    else {
        // n ote - scanline images do not support mip levels
        rv = nanoexr_read_scanline_exr(exr, img, layerName, partIndex, rgbaIndex);
    }
    
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr: failed to read image\n");
        free(img->data);
        img->data = NULL;
        return rv;
    }

    uint16_t oneValue = float_to_half(1.0f);
    uint16_t zeroValue = float_to_half(0.0f);

    // if the image is rgba, and any of the channels are missing, fill them in
    // by propagating the channel to the left if possible. If not, fill with
    // zero or one. Alpha is always filled with one.
    if (img->channelCount == 4) {
        if (rgbaIndex[3] == -1) {
            // fill the alpha channel with 1.0
            if (img->pixelType == EXR_PIXEL_HALF) {
                fill_channel_u16(img, 3, oneValue);
            }
            else if (img->pixelType == EXR_PIXEL_FLOAT) {
                fill_channel_float(img, 3, 1.0f);
            }
            else if (img->pixelType == EXR_PIXEL_UINT) {
                // We're treating uint data as data, not rgba, so fill with zero
                fill_channel_u32(img, 3, 0);
            }
        }
        if (rgbaIndex[2] == -1) {
            // if G exists, propagate it, else if R exists, propagate it, else fill with zero
            int srcChannel = rgbaIndex[1] >= 0 ? 1 : (rgbaIndex[0] >= 0 ? 0 : -1);
            if (srcChannel >= 0) {
                if (img->pixelType == EXR_PIXEL_HALF) {
                    copy_channel_u16(img, srcChannel, 2);
                }
                else if (img->pixelType == EXR_PIXEL_FLOAT) {
                    copy_channel_float(img, srcChannel, 2);
                }
                else if (img->pixelType == EXR_PIXEL_UINT) {
                    copy_channel_u32(img, srcChannel, 2);
                }
            }
            else {
                if (img->pixelType == EXR_PIXEL_HALF) {
                    fill_channel_u16(img, 2, zeroValue);
                }
                else if (img->pixelType == EXR_PIXEL_FLOAT) {
                    fill_channel_float(img, 2, 0.0f);
                }
                else if (img->pixelType == EXR_PIXEL_UINT) {
                    fill_channel_u32(img, 2, 0);
                }
            }
        }
        if (rgbaIndex[1] == -1) {
            // if R exists, propagate it, else fill with zero
            int srcChannel = rgbaIndex[0] >= 0 ? 0 : -1;
            if (srcChannel >= 0) {
                if (img->pixelType == EXR_PIXEL_HALF) {
                    copy_channel_u16(img, srcChannel, 1);
                }
                else if (img->pixelType == EXR_PIXEL_FLOAT) {
                    copy_channel_float(img, srcChannel, 1);
                }
                else if (img->pixelType == EXR_PIXEL_UINT) {
                    copy_channel_u32(img, srcChannel, 1);
                }
            }
            else {
                if (img->pixelType == EXR_PIXEL_HALF) {
                    fill_channel_u16(img, 1, zeroValue);
                }
                else if (img->pixelType == EXR_PIXEL_FLOAT) {
                    fill_channel_float(img, 1, 0.0f);
                }
                else if (img->pixelType == EXR_PIXEL_UINT) {
                    fill_channel_u32(img, 1, 0);
                }
            }
        }
        if (rgbaIndex[0] == -1) {
            // fill with zero
            if (img->pixelType == EXR_PIXEL_HALF) {
                fill_channel_u16(img, 0, zeroValue);
            }
            else if (img->pixelType == EXR_PIXEL_FLOAT) {
                fill_channel_float(img, 0, 0.0f);
            }
            else if (img->pixelType == EXR_PIXEL_UINT) {
                fill_channel_u32(img, 0, 0);
            }
        }
    }

    rv = exr_finish(&exr);
    if (rv != EXR_ERR_SUCCESS) {
        fprintf(stderr, "nanoexr finish error: %s\n", 
                exr_get_default_error_message(rv));
    }
    return rv;
}
