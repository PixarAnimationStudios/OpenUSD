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
#include "usdMaya/primWriterArgs.h"
#include "usdMaya/writeUtil.h"

#include <maya/MFnDependencyNode.h>

PxrUsdMayaPrimWriterArgs::PxrUsdMayaPrimWriterArgs(
        const MDagPath& dagPath,
        const bool exportRefsAsInstanceable) :
    _dagPath(dagPath),
    _exportRefsAsInstanceable(exportRefsAsInstanceable)
{
}

MObject
PxrUsdMayaPrimWriterArgs::GetMObject() const
{
    return _dagPath.node();
}

const MDagPath&
PxrUsdMayaPrimWriterArgs::GetMDagPath() const
{
    return _dagPath;
}

bool
PxrUsdMayaPrimWriterArgs::GetExportRefsAsInstanceable() const
{
    return _exportRefsAsInstanceable;
}

bool
PxrUsdMayaPrimWriterArgs::ReadAttribute(
        const std::string& name,
        std::string* val) const
{
    return PxrUsdMayaWriteUtil::ReadMayaAttribute(
            MFnDependencyNode(GetMObject()), 
            MString(name.c_str()), val);
}

bool
PxrUsdMayaPrimWriterArgs::ReadAttribute(
        const std::string& name,
        VtIntArray* val) const
{
    return PxrUsdMayaWriteUtil::ReadMayaAttribute(
            MFnDependencyNode(GetMObject()), 
            MString(name.c_str()), val);
}

bool
PxrUsdMayaPrimWriterArgs::ReadAttribute(
        const std::string& name,
        VtFloatArray* val) const
{
    return PxrUsdMayaWriteUtil::ReadMayaAttribute(
            MFnDependencyNode(GetMObject()), 
            MString(name.c_str()), val);
}

bool
PxrUsdMayaPrimWriterArgs::ReadAttribute(
        const std::string& name,
        VtVec3fArray* val) const
{
    return PxrUsdMayaWriteUtil::ReadMayaAttribute(
            MFnDependencyNode(GetMObject()), 
            MString(name.c_str()), val);
}

