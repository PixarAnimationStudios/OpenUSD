//
// Copyright 2017 Pixar
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
#ifndef HDEMBREE_CONFIG_H
#define HDEMBREE_CONFIG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdEmbreeConfig
///
/// This class is a singleton, holding configuration parameters for HdEmbree.
/// Everything is provided with a default, but can be overridden using
/// environment variables before launching a hydra process.
///
/// Many of the parameters can be used to control quality/performance
/// tradeoffs, or to alter how HdEmbree takes advantage of parallelism.
///
/// At startup, this class will print config parameters if
/// *HDEMBREE_PRINT_CONFIGURATION* is true. Integer values greater than zero
/// are considered "true".
///
class HdEmbreeConfig {
public:
    /// \brief Return the configuration singleton.
    static const HdEmbreeConfig &GetInstance();

    /// How many samples does each pixel get per frame?
    ///
    /// Override with *HDEMBREE_SAMPLES_PER_FRAME*.
    unsigned int samplesPerFrame;

    /// How many samples do we need before a pixel is considered
    /// converged?
    ///
    /// Override with *HDEMBREE_SAMPLES_TO_CONVERGENCE*.
    unsigned int samplesToConvergence;

    /// How many pixels are in an atomic unit of parallel work?
    /// A work item is a square of size [tileSize x tileSize] pixels.
    ///
    /// Override with *HDEMBREE_TILE_SIZE*.
    unsigned int tileSize;

    /// How many ambient occlusion rays should we generate per
    /// camera ray?
    ///
    /// Override with *HDEMBREE_AMBIENT_OCCLUSION_SAMPLES*.
    unsigned int ambientOcclusionSamples;

    /// How many bytes should we allocate for the embree subdivision
    /// surface cache?
    ///
    /// Override with *HDEMBREE_SUBDIVISION_CACHE*.
    unsigned int subdivisionCache;

    /// Should the renderpass's sampling functions use a fixed random seed?
    /// (Helpful for things like unit tests, to get consistent results).
    ///
    /// Override with *HDEMBREE_FIX_RANDOM_SEED*. Integer values greater than
    /// zero are considered "true".
    bool fixRandomSeed;

private:
    // The constructor initializes the config variables with their
    // default or environment-provided override, and optionally prints
    // them.
    HdEmbreeConfig();
    ~HdEmbreeConfig() = default;

    HdEmbreeConfig(const HdEmbreeConfig&) = delete;
    HdEmbreeConfig& operator=(const HdEmbreeConfig&) = delete;

    friend class TfSingleton<HdEmbreeConfig>;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDEMBREE_CONFIG_H
