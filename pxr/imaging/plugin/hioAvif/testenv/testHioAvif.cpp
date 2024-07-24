//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hio/image.h"
#include <array>
#include <mutex>
#include <fstream>

PXR_NAMESPACE_USING_DIRECTIVE
int
main(int argc, char *argv[])
{
    // check existence of the png and avif plugins which are required for this
    // test.
    {
        TF_AXIOM(HioImage::IsSupportedImageFile("dummy.png"));
        TF_AXIOM(HioImage::IsSupportedImageFile("dummy.avif"));
    }
    
    // To enable running the test within xcode, where a cwd cannot be set, for
    // running, we allow an optional argument to specify the root path for the 
    // test assets. If there is an argument, use it as the root path for the 
    // test assets.
    std::string rootPath = argc > 1 ? argv[1] : ".";
    // if the argument doesn't end with a slash, add one
    if (rootPath.back() != '/') {
        rootPath += "/";
    }
    
    std::string csGrayPng = rootPath + "cs-gray-7f7f7f.png";
    std::string csGrayAvif = rootPath + "cs-gray-7f7f7f.avif";

    // test that the files exist at the specified paths
    {
        std::ifstream pngFile(csGrayPng);
        TF_AXIOM(pngFile.good());
        std::ifstream avifFile(csGrayAvif);
        TF_AXIOM(avifFile.good());
    }

    int width = 0;
    int height = 0;
    HioImage::StorageSpec pngSpec;
    std::vector<unsigned char> pngReadback;
    {
        // fetch basic information about the png to be compared with the avif
        std::string path = csGrayPng;
        HioImageSharedPtr image = HioImage::OpenForReading(path);
        if (!TF_VERIFY(image)) {
            return 1;
        }

        width = image->GetWidth();
        height = image->GetHeight();
        TF_VERIFY(width > 0 && height > 0);
        pngReadback.resize(width * height * 4);
        pngSpec.width = width;
        pngSpec.height = height;
        pngSpec.format = image->GetFormat();
        pngSpec.flipped = false;
        pngSpec.data = pngReadback.data();
        TF_VERIFY(image->Read(pngSpec));

        // this write back is for a visual check, not directly used by the test
        std::string filename = "pngTestWriteback.png";
        HioImageSharedPtr writePngImage = HioImage::OpenForWriting(filename);
        TF_VERIFY(writePngImage);
        writePngImage->Write(pngSpec);
    }

    {
        // fetch basic information about the avif
        std::string path = csGrayAvif;
        HioImageSharedPtr image = HioImage::OpenForReading(path);
        TF_VERIFY(image);
        TF_VERIFY(image->GetWidth() == width);
        TF_VERIFY(image->GetHeight() == height);
        TF_VERIFY(image->GetFormat() == HioFormatFloat16Vec4);

        HioImage::StorageSpec avifSpec;
        avifSpec.width = width;
        avifSpec.height = height;
        avifSpec.format = image->GetFormat(); // f16v4 is native
        std::vector<unsigned char> readback(width * height * sizeof(uint16_t) * 4);
        avifSpec.data = readback.data();
        TF_VERIFY(image->Read(avifSpec));
        {
            // this write back is for a visual check, not directly used by the test
            std::string filename = "avifTestWriteback16.exr";
            HioImageSharedPtr exrimage = HioImage::OpenForWriting(filename);
            TF_AXIOM(exrimage);
            TF_AXIOM(exrimage->Write(avifSpec));
        }

        HioImage::StorageSpec avifSpecF32;
        avifSpecF32.width = width;
        avifSpecF32.height = height;
        avifSpecF32.format = HioFormatFloat32Vec4;
        std::vector<unsigned char> readbackf32(width * height * sizeof(float) * 4);
        avifSpecF32.data = readbackf32.data();
        TF_VERIFY(image->Read(avifSpecF32));
        {
            // this write back is for a visual check, not directly used by the test
            std::string filename = "avifTestWriteback32.exr";
            HioImageSharedPtr exrimage = HioImage::OpenForWriting(filename);
            TF_AXIOM(exrimage);
            TF_AXIOM(exrimage->Write(avifSpecF32));
        }

        // compare the pixel data of the read avif image and the reference png.
        uint8_t* pngData = reinterpret_cast<uint8_t*>(pngSpec.data);
        GfHalf* avifData = reinterpret_cast<GfHalf*>(avifSpec.data);
        float* avif32Data = reinterpret_cast<float*>(readbackf32.data());
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // fairly loose tolerance as this test is using a 2.2 gamma
                // approximation for the srgb transfer function, which is good
                // enough to know whether the png and avif data match or not.
                const float tol = 0.01f;
                float pngValueR = pow(pngData[(y*width+x) * 3 + 0] / 255.0f, 2.2f);
                float pngValueG = pow(pngData[(y*width+x) * 3 + 1] / 255.0f, 2.2f);
                float pngValueB = pow(pngData[(y*width+x) * 3 + 2] / 255.0f, 2.2f);
                float avifValueR = avifData[(y*width+x) * 4 + 0];
                float avifValueG = avifData[(y*width+x) * 4 + 1];
                float avifValueB = avifData[(y*width+x) * 4 + 2];
                float avif32ValueR = avif32Data[(y*width+x) * 4 + 0];
                float avif32ValueG = avif32Data[(y*width+x) * 4 + 1];
                float avif32ValueB = avif32Data[(y*width+x) * 4 + 2];
                //printf("(%d, %d): %f %f %f / %f %f %f\n", x, y,
                //       pngValueR, pngValueG, pngValueB,
                //       avifValueR, avifValueG, avifValueB);
                TF_AXIOM(fabs(pngValueR - avifValueR) < tol);
                TF_AXIOM(fabs(pngValueG - avifValueG) < tol);
                TF_AXIOM(fabs(pngValueB - avifValueB) < tol);
                TF_AXIOM(fabs(pngValueR - avif32ValueR) < tol);
                TF_AXIOM(fabs(pngValueG - avif32ValueG) < tol);
                TF_AXIOM(fabs(pngValueB - avif32ValueB) < tol);
            }
        }
    }

    printf("OK\n");
    return 0;
}
