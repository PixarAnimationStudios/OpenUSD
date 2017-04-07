//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_JOBARGS_H
#define PXRUSDMAYA_JOBARGS_H

/// \file JobArgs.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include <maya/MString.h>
#include <maya/MStringArray.h>

PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_TRANSLATOR_TOKENS \
    ((UsdFileExtensionDefault, "usd")) \
    ((UsdFileExtensionASCII, "usda")) \
    ((UsdFileExtensionCrate, "usdc")) \
    ((UsdFileFilter, "*.usd *.usda *.usdc"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaTranslatorTokens,
        PXRUSDMAYA_TRANSLATOR_TOKENS);

#define PXRUSDMAYA_JOBARGS_TOKENS \
    (Full) \
    (Collapsed) \
    (Uniform) \
    (defaultLayer) \
    (currentLayer) \
    (modelingVariant)

TF_DECLARE_PUBLIC_TOKENS(PxUsdExportJobArgsTokens, 
        PXRUSDMAYA_JOBARGS_TOKENS);

struct JobSharedArgs
{
    JobSharedArgs();

    // Templating since we know at compile time which subclass we have
    template<typename JobArgClass>
    void parseOptionsString(const MString &optionsString)
    {
        if ( optionsString.length() > 0 ) {
            MStringArray optionList;
            MStringArray theOption;
            optionsString.split(';', optionList);
            for (unsigned int i = 0; i < optionList.length(); ++i) {
                theOption.clear();
                optionList[i].split('=', theOption);
                static_cast<JobArgClass*>(this)->parseSingleOption(theOption);
            }
        }
    }

    bool parseSharedOption(const MStringArray& theOption);

    void setDefaultMeshScheme(const MString& stringVal);

    std::string fileName;
    TfToken shadingMode;
    TfToken defaultMeshScheme;
    double startTime;
    double endTime;

};

struct JobExportArgs : JobSharedArgs
{
    PXRUSDMAYA_API
    JobExportArgs();

    inline void parseExportOptions(const MString &optionsString)
    {
        parseOptionsString<JobExportArgs>(optionsString);
    }

    void parseSingleOption(const MStringArray& theOption);

    std::set<double> frameSamples;

    bool exportRefsAsInstanceable;
    bool exportDisplayColor;
    
    bool mergeTransformAndShape;

    bool exportAnimation;
    bool excludeInvisible;
    bool exportDefaultCameras;

    bool exportMeshUVs;
    bool normalizeMeshUVs;
    
    bool normalizeNurbs;
    bool exportNurbsExplicitUV;
    TfToken nurbsExplicitUVType;
    
    bool exportColorSets;

    TfToken renderLayerMode;
    
    bool exportVisibility;

    std::string melPerFrameCallback;
    std::string melPostCallback;
    std::string pythonPerFrameCallback;
    std::string pythonPostCallback;

    PxrUsdMayaUtil::ShapeSet dagPaths;

    std::vector<std::string> chaserNames;
    typedef std::map<std::string, std::string> ChaserArgs;
    std::map< std::string, ChaserArgs > allChaserArgs;
    
    // This path is provided when dealing with variants
    // where a _BaseModel_ root path is used instead of
    // the model path. This to allow a proper internal reference
    SdfPath usdModelRootOverridePath;

    TfToken rootKind;
};

struct JobImportArgs : JobSharedArgs
{
    PXRUSDMAYA_API
    JobImportArgs();

    inline void parseImportOptions(const MString &optionsString)
    {
        parseOptionsString<JobImportArgs>(optionsString);
    }

    void parseSingleOption(const MStringArray& theOption);

    std::string primPath;
    TfToken assemblyRep;
    bool readAnimData;
    bool useCustomFrameRange;
    bool importWithProxyShapes;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_JOBARGS_H
