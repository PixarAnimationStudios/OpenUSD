//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_CONFIG_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_CONFIG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"

PXR_NAMESPACE_OPEN_SCOPE

// NOTE: types here restricted to bool/int/string, as also used for
// TF_DEFINE_ENV_SETTING
constexpr int HdEmbreeDefaultSamplesToConvergence = 100;
constexpr int HdEmbreeDefaultTileSize = 8;
constexpr int HdEmbreeDefaultAmbientOcclusionSamples = 16;
constexpr bool HdEmbreeDefaultJitterCamera = true;
constexpr bool HdEmbreeDefaultUseFaceColors = true;
constexpr int HdEmbreeDefaultCameraLightIntensity = 300;
constexpr int HdEmbreeDefaultRandomNumberSeed = -1;

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

    /// How many samples do we need before a pixel is considered
    /// converged?
    ///
    /// Override with *HDEMBREE_SAMPLES_TO_CONVERGENCE*.
    unsigned int samplesToConvergence = HdEmbreeDefaultSamplesToConvergence;

    /// How many pixels are in an atomic unit of parallel work?
    /// A work item is a square of size [tileSize x tileSize] pixels.
    ///
    /// Override with *HDEMBREE_TILE_SIZE*.
    unsigned int tileSize = HdEmbreeDefaultTileSize;

    /// How many ambient occlusion rays should we generate per
    /// camera ray?
    ///
    /// Override with *HDEMBREE_AMBIENT_OCCLUSION_SAMPLES*.
    unsigned int ambientOcclusionSamples = HdEmbreeDefaultAmbientOcclusionSamples;

    /// Should the renderpass jitter camera rays for antialiasing?
    ///
    /// Override with *HDEMBREE_JITTER_CAMERA*. The case-insensitive strings
    /// "true", "yes", "on", and "1" are considered true; an empty value uses
    /// the default, and all other values are false.
    bool jitterCamera = HdEmbreeDefaultJitterCamera;

    /// Should the renderpass use the color primvar, or flat white colors?
    /// (Flat white shows off ambient occlusion better).
    ///
    /// Override with *HDEMBREE_USE_FACE_COLORS*.  The case-insensitive strings
    /// "true", "yes", "on", and "1" are considered true; an empty value uses
    /// the default, and all other values are false.
    bool useFaceColors = HdEmbreeDefaultUseFaceColors;

    /// What should the intensity of the camera light be, specified as a
    /// percent of <1, 1, 1>.  For example, 300 would be <3, 3, 3>.
    ///
    /// Override with *HDEMBREE_CAMERA_LIGHT_INTENSITY*.
    float cameraLightIntensity = HdEmbreeDefaultCameraLightIntensity;

    /// Seed to give to the random number generator. A value of anything other
    /// than -1, combined with setting PXR_WORK_THREAD_LIMIT=1, should give
    /// deterministic / repeatable results. A value of -1 (the default) will
    /// allow the implementation to set a value that varies from invocation to
    /// invocation and thread to thread.
    ///
    /// Override with *HDEMBREE_RANDOM_NUMBER_SEED*.
    int randomNumberSeed = HdEmbreeDefaultRandomNumberSeed;

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

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_CONFIG_H
