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
#ifndef PXRUSDTRANSLATORS_LOCATOR_WRITER_H
#define PXRUSDTRANSLATORS_LOCATOR_WRITER_H

/// \file locatorWriter.h

#include "pxr/pxr.h"

#include "usdMaya/primWriter.h"
#include "usdMaya/usdWriteJobCtx.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MDagPath.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// A simple USD prim writer for Maya locator shape nodes.
///
/// Having this dedicated prim writer for locators ensures that we get the
/// correct resulting USD whether mergeTransformAndShape is turned on or off.
///
/// Note that there is currently no "Locator" type in USD and that Maya locator
/// nodes are exported as UsdGeomXform prims. This means that locators will not
/// currently round-trip out of Maya to USD and back because the importer is
/// not able to differentiate between Xform prims that were the result of
/// exporting Maya "transform" type nodes and those that were the result of
/// exporting Maya "locator" type nodes.
class PxrUsdTranslators_LocatorWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_LocatorWriter(
            const MDagPath& iDag,
            const SdfPath& uPath,
            usdWriteJobCtx& jobCtx);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
