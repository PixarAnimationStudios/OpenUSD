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

#ifndef PXRUSDMAYA_EXPORT_TRANSLATOR_H
#define PXRUSDMAYA_EXPORT_TRANSLATOR_H

/// \file usdMaya/exportTranslator.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/jobArgs.h"

#include <maya/MFileObject.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE


class UsdMayaExportTranslator : public MPxFileTranslator
{
    public:

        /**
         * method to create UsdMayaExportTranslator file translator
         */
        PXRUSDMAYA_API
        static void* creator();

        PXRUSDMAYA_API
        MStatus writer(
                const MFileObject& file, 
                const MString& optionsString,
                MPxFileTranslator::FileAccessMode mode) override;

        bool haveReadMethod() const override { return false; }
        bool haveWriteMethod() const override { return true; }

        PXRUSDMAYA_API
        MFileKind identifyFile(
                const MFileObject& file,
                const char* buffer,
                short size) const override;

        MString defaultExtension() const override {
            return UsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText();
        }
        MString filter() const override {
            return UsdMayaTranslatorTokens->UsdFileFilter.GetText();
        }

        PXRUSDMAYA_API
        static const std::string& GetDefaultOptions();

    private:

        UsdMayaExportTranslator();
        UsdMayaExportTranslator(const UsdMayaExportTranslator&);
        ~UsdMayaExportTranslator() override;
        UsdMayaExportTranslator& operator=(const UsdMayaExportTranslator&);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
