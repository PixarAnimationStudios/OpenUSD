//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/config.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Instantiate the config singleton.
TF_INSTANTIATE_SINGLETON(HdEmbreeConfig);

// Each configuration variable has an associated environment variable.
// The environment variable macro takes the variable name, a default value,
// and a description...
TF_DEFINE_ENV_SETTING(
    HDEMBREE_SAMPLES_TO_CONVERGENCE,
    HdEmbreeDefaultSamplesToConvergence,
    "Samples per pixel before we stop rendering (must be >= 1)");

TF_DEFINE_ENV_SETTING(
    HDEMBREE_TILE_SIZE,
    HdEmbreeDefaultTileSize,
    "Size (per axis) of threading work units (must be >= 1)");

TF_DEFINE_ENV_SETTING(
    HDEMBREE_AMBIENT_OCCLUSION_SAMPLES,
    HdEmbreeDefaultAmbientOcclusionSamples,
    "Ambient occlusion samples per camera ray (must be >= 0;"
    " a value of 0 disables ambient occlusion)");

TF_DEFINE_ENV_SETTING(
    HDEMBREE_JITTER_CAMERA,
    HdEmbreeDefaultJitterCamera,
    "Should HdEmbree jitter camera rays while rendering?");

TF_DEFINE_ENV_SETTING(
    HDEMBREE_USE_FACE_COLORS,
    HdEmbreeDefaultUseFaceColors,
    "Should HdEmbree use face colors while rendering?");

TF_DEFINE_ENV_SETTING(
    HDEMBREE_CAMERA_LIGHT_INTENSITY,
    HdEmbreeDefaultCameraLightIntensity,
    "Intensity of the camera light, specified as a percentage of <1,1,1>.");

TF_DEFINE_ENV_SETTING(
    HDEMBREE_RANDOM_NUMBER_SEED,
    HdEmbreeDefaultRandomNumberSeed,
    "Seed to give to the random number generator. A value of anything other"
        " than -1, combined with setting PXR_WORK_THREAD_LIMIT=1, should"
        " give deterministic / repeatable results. A value of -1 (the"
        " default) will allow the implementation to set a value that varies"
        " from invocation to invocation and thread to thread.");

TF_DEFINE_ENV_SETTING(HDEMBREE_PRINT_CONFIGURATION,
    false,
    "Should HdEmbree print configuration on startup?");

HdEmbreeConfig::HdEmbreeConfig()
{
    // Read in values from the environment, clamping them to valid ranges.
    samplesToConvergence = std::max(1,
            TfGetEnvSetting(HDEMBREE_SAMPLES_TO_CONVERGENCE));
    tileSize = std::max(1,
            TfGetEnvSetting(HDEMBREE_TILE_SIZE));
    ambientOcclusionSamples = std::max(0,
            TfGetEnvSetting(HDEMBREE_AMBIENT_OCCLUSION_SAMPLES));
    jitterCamera = (TfGetEnvSetting(HDEMBREE_JITTER_CAMERA));
    useFaceColors = (TfGetEnvSetting(HDEMBREE_USE_FACE_COLORS));
    cameraLightIntensity = (std::max(100,
            TfGetEnvSetting(HDEMBREE_CAMERA_LIGHT_INTENSITY)) / 100.0f);
    randomNumberSeed = TfGetEnvSetting(HDEMBREE_RANDOM_NUMBER_SEED);

    if (TfGetEnvSetting(HDEMBREE_PRINT_CONFIGURATION)) {
        std::cout
            << "HdEmbree Configuration: \n"
            << "  samplesToConvergence       = "
            <<    samplesToConvergence    << "\n"
            << "  tileSize                   = "
            <<    tileSize                << "\n"
            << "  ambientOcclusionSamples    = "
            <<    ambientOcclusionSamples << "\n"
            << "  jitterCamera               = "
            <<    jitterCamera            << "\n"
            << "  useFaceColors              = "
            <<    useFaceColors           << "\n"
            << "  cameraLightIntensity      = "
            <<    cameraLightIntensity    << "\n"
            << "  randomNumberSeed          = "
            <<    randomNumberSeed        << "\n"
            ;
    }
}

/*static*/
const HdEmbreeConfig&
HdEmbreeConfig::GetInstance()
{
    return TfSingleton<HdEmbreeConfig>::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
