//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/usd/ar/resolver.h"
#include <array>
#include <mutex>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

const int w = 256;
const int h = 256;

const std::vector<uint8_t>& GetGrey8Values()
{
    // create a checkerboard pattern, with a stride of 32 pixels.
    static std::once_flag _once;
    static std::vector<uint8_t> _grey8Values(w * h);
    std::call_once(_once, []() {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int xsnap = x & 0xE0; // mask off bottom 5 bits
                int ysnap = y & 0xE0;
                uint8_t value = (xsnap + ysnap) & 0xff;
                int index = y * w + x;
                int checkIndex = (y/32 * w + x/32);
                if (checkIndex & 1) {
                    _grey8Values[index] = value;
                }
                else {
                    _grey8Values[index] = 255 - value;
                }
            }
        }
    });
    return _grey8Values;
}

const std::vector<uint8_t>& GetRgb8Values()
{
    static std::once_flag _once;
    static std::vector<uint8_t> _rgb8Values(w * h * 3);
    std::call_once(_once, []() {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int index = 3 * (y * w + x);
                _rgb8Values[index + 0] = x & 0xff;
                _rgb8Values[index + 1] = y & 0xff;
                _rgb8Values[index + 2] = (x + y) & 0xff;
            }
        }
    });
    return _rgb8Values;
}

const std::vector<float>& GetRgbFloatValues()
{
    static std::once_flag _once;
    static std::vector<float> _rgbFloatValues(w * h * 3);
    std::call_once(_once, []() {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int index = 3 * (y * w + x);
                _rgbFloatValues[index + 0] = (x & 0xff) / 255.0f;
                _rgbFloatValues[index + 1] = (y & 0xff) / 255.0f;
                _rgbFloatValues[index + 2] = ((x + y) & 0xff) / 255.0f;
            }
        }
    });
    return _rgbFloatValues;
}

int
main(int argc, char *argv[])
{
    {
        // verify that the hioOpenEXR plugin exists
        bool found = false;
        for (PlugPluginPtr plug : PlugRegistry::GetInstance().GetAllPlugins()) {
            if (plug->GetName() == "hioOpenEXR") {
                found = true;
                break;
            }
        }
        TF_AXIOM(found);
    }
    {
        // validate that the Ar plugin loaded by asking the default resolver to
        // tell the filename extension; Ar is going to be used by Hio to load
        // images via the Asset API.
        const TfToken fileExtension(
                TfStringToLowerAscii(ArGetResolver().GetExtension("test.exr")));
        TF_AXIOM(fileExtension.GetString() == "exr");
    }
    {
        // get the TfType for HioImage
        TfType hioImageType = TfType::Find<HioImage>();
        int stockPlugins = 0;
        // validate that the HioImage subclass types are registered
        std::vector<TfType> hioImageTypes =
            PlugRegistry::GetInstance().GetDirectlyDerivedTypes(hioImageType);
        for (auto t : hioImageTypes) {
            auto tn = t.GetTypeName();
            if (tn == "Hio_OpenEXRImage")
                ++stockPlugins;
        }
        // the exr plugin is available.
        TF_AXIOM(stockPlugins == 1);
    }

    // check that IsSupportedImageFile recognizes a filename
    {
        TF_AXIOM(HioImage::IsSupportedImageFile("dummy.exr"));
    }

    // do a lossless comparison for exr and float32
    {
        const std::vector<float>& rgbFloatValues = GetRgbFloatValues();
        std::string filename = "test.exr";
        HioImageSharedPtr image = HioImage::OpenForWriting(filename);
        TF_AXIOM(image);

        // create storage spec
        HioImage::StorageSpec storageSpec;
        storageSpec.width = w;
        storageSpec.height = h;
        storageSpec.format = HioFormatFloat32Vec3;
        storageSpec.flipped = false;
        storageSpec.data = (void*) rgbFloatValues.data();

        TF_AXIOM(image->Write(storageSpec));
        image.reset();

        image = HioImage::OpenForReading(filename);
        TF_AXIOM(image);
        TF_AXIOM(image->GetWidth() == w);
        TF_AXIOM(image->GetHeight() == h);
        TF_AXIOM(image->GetFormat() == HioFormatFloat32Vec3);
        TF_AXIOM(image->GetBytesPerPixel() == sizeof(float) * 3);
        std::vector<float> readback(w * h * 3);
        auto readSpec = storageSpec;
        readSpec.data = readback.data();
        TF_AXIOM(image->Read(readSpec));
        TF_AXIOM(rgbFloatValues == readback);
    }
    
    // test.exr now exists; read it requesting a half scale resize
    {
        const std::vector<float>& rgbFloatValues = GetRgbFloatValues();
        HioImageSharedPtr image = HioImage::OpenForReading("test.exr");
        TF_AXIOM(image);
        TF_AXIOM(image->GetWidth() == w);
        TF_AXIOM(image->GetHeight() == h);
        TF_AXIOM(image->GetFormat() == HioFormatFloat32Vec3);
        TF_AXIOM(image->GetBytesPerPixel() == sizeof(float) * 3);

        int w2 = w/2;
        int h2 = h/2;
        std::vector<float> readback(w2 * h2 * 3);

        HioImage::StorageSpec readSpec;
        readSpec.width = w2;
        readSpec.height = h2;
        readSpec.format = HioFormatFloat32Vec3;
        readSpec.data = readback.data();
        TF_AXIOM(image->Read(readSpec));
        // verify that the pixel values are approximately the same
        for (int y = 0; y < h2; y++) {
            for (int x = 0; x < w2; x++) {
                int index = 3 * (y * w2 + x);
                int index2 = 3 * (y * 2 * w + x * 2);
                // a loose comparison is fine for this test
                TF_AXIOM(fabsf(readback[index + 0] - rgbFloatValues[index2 + 0]) < 16.f/255.f);
                TF_AXIOM(fabsf(readback[index + 1] - rgbFloatValues[index2 + 1]) < 16.f/255.f);
                TF_AXIOM(fabsf(readback[index + 2] - rgbFloatValues[index2 + 2]) < 16.f/255.f);
            }
        }
    }

    // read the exr file as float32 rgba, and verify that the pixels are the
    // same and that the alpha channel is full of ones.
    {
        HioImageSharedPtr image = HioImage::OpenForReading("test.exr");
        TF_AXIOM(image);
        TF_AXIOM(image->GetWidth() == w);
        TF_AXIOM(image->GetHeight() == h);
        TF_AXIOM(image->GetFormat() == HioFormatFloat32Vec3);
        TF_AXIOM(image->GetBytesPerPixel() == sizeof(float) * 3);
        std::vector<float> readback(w * h * 4);
        HioImage::StorageSpec readSpec;
        readSpec.width = w;
        readSpec.height = h;
        readSpec.format = HioFormatFloat32Vec4;
        readSpec.data = readback.data();
        TF_AXIOM(image->Read(readSpec));
        // verify that the pixel values are the same
        const std::vector<float>& rgbFloatValues = GetRgbFloatValues();
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int index = 4 * (y * w + x);
                int index3 = 3 * (y * w + x);
                TF_AXIOM(readback[index + 0] == rgbFloatValues[index3 + 0]);
                TF_AXIOM(readback[index + 1] == rgbFloatValues[index3 + 1]);
                TF_AXIOM(readback[index + 2] == rgbFloatValues[index3 + 2]);
                TF_AXIOM(readback[index + 3] == 1.0f);
            }
        }
    }

    // read the exr file as uint8_t rgba, verify this is not be supported.
    {
        HioImageSharedPtr image = HioImage::OpenForReading("test.exr");
        TF_AXIOM(image);
        TF_AXIOM(image->GetWidth() == w);
        TF_AXIOM(image->GetHeight() == h);
        TF_AXIOM(image->GetFormat() == HioFormatFloat32Vec3);
        TF_AXIOM(image->GetBytesPerPixel() == sizeof(float) * 3);
        std::vector<uint8_t> readback(w * h * 4);
        HioImage::StorageSpec readSpec;
        readSpec.width = w;
        readSpec.height = h;
        readSpec.format = HioFormatUNorm8Vec4srgb;
        readSpec.data = readback.data();
        TF_AXIOM(!image->Read(readSpec));
    }

    printf("OK\n");
    return 0;
}
