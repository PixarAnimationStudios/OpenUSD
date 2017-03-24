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
#ifndef PXRUSDMAYA_PRIMWRITERERARGS_H
#define PXRUSDMAYA_PRIMWRITERERARGS_H

/// \file primWriterArgs.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec3f.h"

#include <maya/MDagPath.h>
#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE


/// \class PxrUsdMayaPrimWriterArgs
/// \brief This class holds read-only arguments that are passed into the writer
/// plugins for the usdMaya library.  This mostly contains functions to get data
/// from the maya scene and helpers to retrieve values from maya and prepare
/// them to author into usd.
///
/// \sa PxrUsdMayaPrimWriterContext
class PxrUsdMayaPrimWriterArgs
{
public:
    PXRUSDMAYA_API
    PxrUsdMayaPrimWriterArgs(
            const MDagPath& dagPath,
            const bool exportRefsAsInstanceable);

    /// \brief returns the MObject that should be exported. 
    PXRUSDMAYA_API
    MObject GetMObject() const;

    PXRUSDMAYA_API
    const MDagPath& GetMDagPath() const;

    PXRUSDMAYA_API
    bool GetExportRefsAsInstanceable() const;

    /// helper functions to get data from attribute named \p from the current
    /// MObject.  
    /// \{
    PXRUSDMAYA_API
    bool ReadAttribute(const std::string& name, std::string* val) const;
    PXRUSDMAYA_API
    bool ReadAttribute(const std::string& name, VtIntArray* val) const;
    PXRUSDMAYA_API
    bool ReadAttribute(const std::string& name, VtFloatArray* val) const;
    PXRUSDMAYA_API
    bool ReadAttribute(const std::string& name, VtVec3fArray* val) const;
    /// \}

private:
    MDagPath _dagPath;
    bool _exportRefsAsInstanceable;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_PRIMWRITERERARGS_H

