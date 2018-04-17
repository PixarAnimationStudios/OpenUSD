//
// Copyright 2017 Pixar
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
#ifndef _usdExport_MayaInstancerWriter_h_
#define _usdExport_MayaInstancerWriter_h_

#include "pxr/pxr.h"
#include "usdMaya/MayaTransformWriter.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE


class UsdGeomPointInstancer;

/// Used internally by MayaInstancerWriter to keep track of the
/// instancerTranslate xformOp for compensating Maya's instancer position
/// behavior.
struct MayaInstancerWriter_TranslateOpData {
    MDagPath mayaPath;
    UsdGeomXformOp op;
    AnimChannelSampleType sampleType;

    MayaInstancerWriter_TranslateOpData(
            const MDagPath& mayaPath,
            const UsdGeomXformOp& op,
            AnimChannelSampleType sampleType)
            : mayaPath(mayaPath), op(op), sampleType(sampleType)
    {
    }
};

/// \brief Exporter for Maya particle instancer nodes (MFnInstancer).
/// The instancer node is used in both nParticles and MASH networks.
///
/// The MayaInstancerWriter exports instancers to UsdGeomPointInstancers.
/// It collects all of the prototypes used by the instancer (the "instanced
/// objects" or "input hierarchies") and places them underneath a new
/// "Prototypes" prim that lives underneath the UsdGeomPointInstancer.
///
/// Prototypes may thus be exported twice if they are included in the
/// selection of nodes to export -- once at their original location in the
/// hierarchy, and another time as a prototype of the UsdGeomPointInstancer.
class MayaInstancerWriter : public MayaTransformWriter
{
    /// Number of prototypes that have been set up so far.
    int _numPrototypes;
    /// All valid prim writers for all prototypes. The size of this will most
    /// likely be larger than _numPrototypes.
    std::vector<MayaPrimWriterPtr> _prototypeWriters;
    /// Data used to write the instancerTranslate xformOp on prototypes that
    /// need it. There is at most one instancerTranslate op for each prototype.
    std::vector<MayaInstancerWriter_TranslateOpData> _instancerTranslateOps;

public:
    MayaInstancerWriter(const MDagPath & iDag,
            const SdfPath& uPath,
            bool instanceSource,
            usdWriteJobCtx& jobCtx);
    virtual ~MayaInstancerWriter() {};
    
    PXRUSDMAYA_API
    virtual void write(const UsdTimeCode &usdTime) override;
    PXRUSDMAYA_API
    virtual void postExport() override;
    PXRUSDMAYA_API
    virtual bool exportsReferences() const override;
    PXRUSDMAYA_API
    virtual bool shouldPruneChildren() const override;
    PXRUSDMAYA_API
    virtual bool getAllAuthoredUsdPaths(SdfPathVector* outPaths) const override;

protected:
    virtual bool writeInstancerAttrs(
            const UsdTimeCode& usdTime, const UsdGeomPointInstancer& instancer);

private:
    AnimChannelSampleType _GetInstancerTranslateSampleType(
            const MDagPath& prototypeDagPath) const;
    void _ExportPrototype(
            const MDagPath& prototypeDagPath,
            const SdfPath& prototypeUsdPath,
            std::vector<MayaPrimWriterPtr>* validPrimWritersOut);
};

typedef std::shared_ptr<MayaInstancerWriter> MayaInstancerWriterPtr;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // _usdExport_MayaInstancerWriter_h_
