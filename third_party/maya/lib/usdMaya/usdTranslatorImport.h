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
 * \class usdTranslatorImport
 * \brief file translator for USD files
 */

#ifndef PXRUSDMAYA_TRANSLATOR_IMPORT_H
#define PXRUSDMAYA_TRANSLATOR_IMPORT_H

#include "usdMaya/api.h"
#include <maya/MPxFileTranslator.h>

#include <string>

const char* const usdTranslatorImportDefaults =
        "shadingMode=GPrim Colors;"
        "readAnimData=0;"
        "assemblyRep=Collapsed";

class usdTranslatorImport : public MPxFileTranslator {

    public:

        /**
         * method to create usdTranslatorImport file translator
         */
        USDMAYA_API
        static void * creator(const std::string& assemblyTypeName,
                              const std::string& proxyShapeTypeName);

        MStatus reader(
            const MFileObject& file,
            const MString& optionsString,
            FileAccessMode mode);

        bool haveReadMethod() const { return true; }
        bool haveWriteMethod() const { return false; }

        MFileKind identifyFile(
            const MFileObject&,
            const char*,
            short) const;

        MString defaultExtension() const { return "usda"; }
        MString filter() const { return "*.usd*"; }

    protected:

    private:

        usdTranslatorImport(const std::string& assemblyTypeName,
                            const std::string& proxyShapeTypeName);
        usdTranslatorImport(
            const usdTranslatorImport&);
        ~usdTranslatorImport();
        usdTranslatorImport& operator=(const usdTranslatorImport&);

        const std::string _assemblyTypeName;
        const std::string _proxyShapeTypeName;
};

#endif // PXRUSDMAYA_TRANSLATOR_IMPORT_H
