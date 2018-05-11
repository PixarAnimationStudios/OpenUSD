//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_MAYA_LOCATOR_WRITER_H
#define PXRUSDMAYA_MAYA_LOCATOR_WRITER_H

/// \file MayaLocatorWriter.h

#include "pxr/pxr.h"

#include "usdMaya/MayaTransformWriter.h"
#include "usdMaya/usdWriteJobCtx.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MDagPath.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// A simple USD prim writer for Maya locator shape nodes.
///
/// Having this dedicated prim writer for locators ensures that we get the
/// correct resulting USD whether mergeTransformAndShape is turned on or off,
/// and it avoids further complicating the logic for node collapsing and
/// exporting transforms in the MayaTransformWriter.
///
/// Note that there is currently no "Locator" type in USD and that Maya locator
/// nodes are exported as UsdGeomXform prims. This means that locators will not
/// currently round-trip out of Maya to USD and back because the importer is
/// not able to differentiate between Xform prims that were the result of
/// exporting Maya "transform" type nodes and those that were the result of
/// exporting Maya "locator" type nodes.
class MayaLocatorWriter : public MayaTransformWriter
{
    public:
        MayaLocatorWriter(
                const MDagPath& iDag,
                const SdfPath& uPath,
                const bool instanceSource,
                usdWriteJobCtx& jobCtx);

        virtual ~MayaLocatorWriter() override;

        virtual void write(const UsdTimeCode& usdTime) override;
};

typedef std::shared_ptr<MayaLocatorWriter> MayaLocatorWriterPtr;


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYA_MAYA_LOCATOR_WRITER_H
