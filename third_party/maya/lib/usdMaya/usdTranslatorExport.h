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
/**
 * \class usdTranslatorExport
 * \brief file translator for USD files
 */

#ifndef __PX_USDTRANSLATOREXPORT_H__
#define __PX_USDTRANSLATOREXPORT_H__

#include <maya/MPxFileTranslator.h>

#include <maya/MFnMesh.h>


const char* const usdTranslatorExportDefaults = 
        "shadingMode=GPrim Colors;"
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
        "animation=0;"
        "startTime=1;"
        "endTime=1";

class usdTranslatorExport : public MPxFileTranslator {

    public:

        /**
         * method to create usdTranslatorExport file translator
         */
        static void * creator();

        MStatus writer(
            const MFileObject& file, 
            const MString& optionsString,
            MPxFileTranslator::FileAccessMode mode);
        
        bool haveReadMethod() const { return false; }
        bool haveWriteMethod() const { return true; }
        
        MFileKind identifyFile(
            const MFileObject&,
            const char*, 
            short) const;

        MString defaultExtension() const { return "usda"; }
        MString filter() const { return "*.usda"; }
      
    protected:

    private:

        usdTranslatorExport();
        usdTranslatorExport(
            const usdTranslatorExport&);
        ~usdTranslatorExport();
        usdTranslatorExport& operator=(const usdTranslatorExport&);
};

#endif /* usdTranslatorExport */
