//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_FRAMEBUFFER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_FRAMEBUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"

#include <vector>
#include <mutex>

class RixContext;

PXR_NAMESPACE_OPEN_SCOPE

/// A simple framebuffer used to receive display-driver output from PRMan.
/// This lives in a separate small library so it can be accessible to
/// both the hdPrman hydra plgin at the d_hydra display driver plugin,
/// without requiring either to know about the other.
class HdPrmanFramebuffer 
{
public:
    enum HdPrmanAccumulationRule {
        k_accumulationRuleFilter,
        k_accumulationRuleAverage,
        k_accumulationRuleMin,
        k_accumulationRuleMax,
        k_accumulationRuleZmin,
        k_accumulationRuleZmax,
        k_accumulationRuleSum
    };

    struct AovDesc
    {
        TfToken name;
        HdFormat format;
        VtValue clearValue;
        HdPrmanAccumulationRule rule;

        bool ShouldNormalizeBySampleCount() const
        {
            return format != HdFormatInt32 && rule != k_accumulationRuleMin &&
                   rule != k_accumulationRuleMax && rule != k_accumulationRuleZmin &&
                   rule != k_accumulationRuleZmax;
        }
    };

    struct AovBuffer
    {
        AovDesc desc;
        std::vector<uint32_t> pixels;
    };

    using AovDescVector = std::vector<AovDesc>;
    using AovBufferVector = std::vector<AovBuffer>;

    HdPrmanFramebuffer();
    ~HdPrmanFramebuffer();

    /// Find a buffer instance with the given ID.
    /// The expectation is that the buffer will exist, so
    /// this raises a runtime error if the ID is not found.
    static HdPrmanFramebuffer* GetByID(int32_t id);
    static void Register(RixContext*);

    /// Convert the accumulation rule string to the HdPrmanAccumulationRule enum
    static HdPrmanAccumulationRule ToAccumulationRule(RtUString name);

    /// (Re-)Creates Aov buffers without allocating pixel storage
    /// (allocated through Resize).
    void CreateAovBuffers(const AovDescVector &aovDescs);

    /// Resize the buffer.
    void Resize(int width, int height,
                int cropXMin=0, int cropYMin=0,
                int cropWidth=0, int cropHeight=0);

    void Clear();

    std::mutex mutex;
    AovBufferVector aovBuffers;

    int w, h;
    int cropOrigin[2];
    int cropRes[2];
    int32_t id;

    // Projection matrix (for the depth output).
    GfMatrix4d proj;

    // Clear functionality.
    bool pendingClear;

    std::atomic<bool> newData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_FRAMEBUFFER_H
