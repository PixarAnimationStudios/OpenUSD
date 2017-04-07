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

#ifndef PXRUSDMAYA_TRANSLATOR_H
#define PXRUSDMAYA_TRANSLATOR_H

/// \file usdTranslator.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/JobArgs.h"

#include <maya/MFileObject.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE



const char* const usdTranslatorDefaults =
        // Shared options
        "shadingMode=GPrim Colors;"
        "defaultMeshScheme=CatmullClark SDiv;"

        // import options
        "readAnimData=1;"
        "useCustomFrameRange=0;"
        "assemblyRep=Collapsed;"

        // export options
        "animation=0;"
        "exportRefsAsInstanceable=0;"
        "exportUVs=1;"
        "normalizeUVs=0;"
        "exportColorSets=1;"
        "renderableOnly=0;"
        "allCameras=0;"
        "renderLayerMode=Use Default Layer;"
        "mergeXForm=1;"
        "defaultMeshScheme=CatmullClark SDiv;"
        "exportVisibility=1;"
        "startTime=1;"
        "endTime=1";


class usdTranslator : public MPxFileTranslator
{
    public:

        /**
         * method to create usdTranslator file translator
         */
        PXRUSDMAYA_API
        static void* creator(const std::string& assemblyTypeName,
                             const std::string& proxyShapeTypeName);

        PXRUSDMAYA_API
        MStatus reader(
                const MFileObject& file,
                const MString& optionsString,
                MPxFileTranslator::FileAccessMode mode);

        PXRUSDMAYA_API
        MStatus writer(
                const MFileObject& file,
                const MString& optionsString,
                MPxFileTranslator::FileAccessMode mode);

        bool haveReadMethod() const { return true; }
        bool haveWriteMethod() const { return true; }

        PXRUSDMAYA_API
        MFileKind identifyFile(
                const MFileObject& file,
                const char* buffer,
                short size) const;

        MString defaultExtension() const {
            return PxrUsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText();
        }
        MString filter() const {
            return PxrUsdMayaTranslatorTokens->UsdFileFilter.GetText();
        }

    private:

        usdTranslator(
                const std::string& assemblyTypeName,
                const std::string& proxyShapeTypeName);
        usdTranslator(const usdTranslator&);
        ~usdTranslator();
        usdTranslator& operator=(const usdTranslator&);

        const std::string _assemblyTypeName;
        const std::string _proxyShapeTypeName;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_TRANSLATOR_H
