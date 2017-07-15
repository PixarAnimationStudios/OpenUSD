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
#include "pxr/imaging/hdEmbree/config.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// Instantiate the config singleton.
TF_INSTANTIATE_SINGLETON(HdEmbreeConfig);

// Each configuration variable has an associated environment variable.
// The environment variable macro takes the variable name, a default value,
// and a description...
TF_DEFINE_ENV_SETTING(HDEMBREE_SAMPLES_PER_FRAME, 1,
        "Raytraced samples per pixel per frame (must be >= 1)");

TF_DEFINE_ENV_SETTING(HDEMBREE_SAMPLES_TO_CONVERGENCE, 100,
        "Samples per pixel before we stop rendering (must be >= 1)");

TF_DEFINE_ENV_SETTING(HDEMBREE_TILE_SIZE, 8,
        "Squared size of threading work units (must be >= 1)");

TF_DEFINE_ENV_SETTING(HDEMBREE_AMBIENT_OCCLUSION_SAMPLES, 16,
        "Ambient occlusion samples per camera ray (must be >= 0; a value of 0 disables ambient occlusion)");

TF_DEFINE_ENV_SETTING(HDEMBREE_SUBDIVISION_CACHE, 128*1024*1024,
        "Number of bytes to allocate for the embree subdivision surface cache (must be >= 128MB)");

TF_DEFINE_ENV_SETTING(HDEMBREE_FIX_RANDOM_SEED, 0,
        "Should HdEmbree sampling use a fixed random seed? (values > 0 are true)");

TF_DEFINE_ENV_SETTING(HDEMBREE_PRINT_CONFIGURATION, 0,
        "Should HdEmbree print configuration on startup? (values > 0 are true)");

HdEmbreeConfig::HdEmbreeConfig()
{
    // Read in values from the environment, clamping them to valid ranges.
    samplesPerFrame = std::max(1,
            TfGetEnvSetting(HDEMBREE_SAMPLES_PER_FRAME));
    samplesToConvergence = std::max(1,
            TfGetEnvSetting(HDEMBREE_SAMPLES_TO_CONVERGENCE));
    tileSize = std::max(1,
            TfGetEnvSetting(HDEMBREE_TILE_SIZE));
    ambientOcclusionSamples = std::max(0,
            TfGetEnvSetting(HDEMBREE_AMBIENT_OCCLUSION_SAMPLES));
    subdivisionCache = std::max(128*1024*1024,
            TfGetEnvSetting(HDEMBREE_SUBDIVISION_CACHE));
    fixRandomSeed = (TfGetEnvSetting(HDEMBREE_FIX_RANDOM_SEED) > 0);

    if (TfGetEnvSetting(HDEMBREE_PRINT_CONFIGURATION) > 0) {
        std::cout
            << "HdEmbree Configuration: \n"
            << "  samplesPerFrame            = "
            <<    samplesPerFrame         << "\n"
            << "  samplesToConvergence       = "
            <<    samplesToConvergence    << "\n"
            << "  tileSize                   = "
            <<    tileSize                << "\n"
            << "  ambientOcclusionSamples    = "
            <<    ambientOcclusionSamples << "\n"
            << "  subdivisionCache           = "
            <<    subdivisionCache        << "\n"
            << "  fixRandomSeed              = "
            <<    fixRandomSeed           << "\n"
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
