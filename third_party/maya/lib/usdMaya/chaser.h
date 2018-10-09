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
#ifndef PXRUSDMAYA_CHASER_H
#define PXRUSDMAYA_CHASER_H

/// \file usdMaya/chaser.h

#include "usdMaya/api.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"

#include "pxr/usd/usd/timeCode.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_REF_PTRS(UsdMayaChaser);

/// \brief base class for plugin chasers which are plugins that run after the
/// core usdExport out of maya.
///
/// Chaser objects will be constructed after the initial "unvarying" export.
/// Chasers should save off necessary data when they are constructed.
/// Afterwards, the chasers will be invoked to export Defaults.  For each frame,
/// after the core process the given frame, all the chasers will be invoked to
/// process that frame.
///
/// The key difference between these and the mel/python postScripts is that a
/// chaser can have direct access to the core usdExport context.
///
/// Chasers need to be very careful as to not modify the structure of the usd
/// file.  This should ideally be used to make small changes or to add
/// attributes in a non-destructive way.
class UsdMayaChaser : public TfRefBase
{
public:
    ~UsdMayaChaser() override { }

    /// Do custom processing after UsdMaya has exported data at the default
    /// time.
    /// The stage will be incomplete; any animated data will not have
    /// been exported yet.
    /// Returning false will terminate the whole export.
    PXRUSDMAYA_API
    virtual bool ExportDefault();

    /// Do custom processing after UsdMaya has exported data at \p time.
    /// The stage will be incomplete; any future animated frames will not
    /// have been exported yet.
    /// Returning false will terminate the whole export.
    PXRUSDMAYA_API
    virtual bool ExportFrame(const UsdTimeCode& time);

    /// Do custom post-processing that needs to run after the main UsdMaya
    /// export loop.
    /// At this point, all data has been authored to the stage (except for
    /// any custom data that you'll author in this step).
    /// Returning false will terminate the whole export.
    PXRUSDMAYA_API
    virtual bool PostExport();
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
