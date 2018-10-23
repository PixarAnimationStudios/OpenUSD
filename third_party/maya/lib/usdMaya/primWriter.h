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
#ifndef PXRUSDMAYA_PRIM_WRITER_H
#define PXRUSDMAYA_PRIM_WRITER_H

/// \file usdMaya/primWriter.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "usdMaya/jobArgs.h"
#include "usdMaya/util.h"

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdUtils/sparseValueWriter.h"

#include <maya/MDagPath.h>
#include <maya/MObject.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


class UsdMayaWriteJobContext;


/// Base class for all built-in and user-defined prim writers. Translates Maya
/// node data into USD prim(s).
///
/// Note that this class can be used to write USD prims for both DG and DAG
/// Maya nodes. For DAG nodes, an MDagPath is required to uniquely identify
/// instances in the DAG, so the writer should be constructed using an
/// MDagPath. For DG nodes, the MObject constructor must be used.
class UsdMayaPrimWriter
{
public:
    /// Constructs a prim writer for writing a Maya DAG node.
    PXRUSDMAYA_API
    UsdMayaPrimWriter(
            const MDagPath& dagPath,
            const SdfPath& usdPath,
            UsdMayaWriteJobContext& jobCtx);

    /// Constructs a prim writer for writing a Maya DG node.
    PXRUSDMAYA_API
    UsdMayaPrimWriter(
            const MObject& dgNode,
            const SdfPath& usdPath,
            UsdMayaWriteJobContext& jobCtx);

    PXRUSDMAYA_API
    virtual ~UsdMayaPrimWriter();

    /// Main export function that runs when the traversal hits the node.
    /// The default implementation writes attributes for the UsdGeomImageable
    /// and UsdGeomGprim schemas if the prim conforms to one or both; in most
    /// cases, subclasses will want to invoke the base class Write() method
    /// when overriding.
    PXRUSDMAYA_API
    virtual void Write(const UsdTimeCode& usdTime);

    /// Post export function that runs before saving the stage.
    ///
    /// Base implementation does nothing.
    PXRUSDMAYA_API
    virtual void PostExport();

    /// Whether this prim writer directly create one or more gprims on the
    /// current model on the USD stage. (Excludes cases where the prim writer
    /// introduces gprims via a reference or by adding a sub-model, such as in
    /// a point instancer.)
    ///
    /// Base implementation returns \c false; prim writers exporting
    /// gprim (shape) classes should override.
    PXRUSDMAYA_API
    virtual bool ExportsGprims() const;

    /// Whether the traversal routine using this prim writer should skip all of
    /// the Maya node's descendants when continuing traversal.
    /// If you override this to return \c true, you may also want to override
    /// GetDagToUsdPathMapping() if you handle export of descendant nodes
    /// (though that is not required).
    ///
    /// Base implementation returns \c false; prim writers that handle export
    /// for their entire subtree should override.
    PXRUSDMAYA_API
    virtual bool ShouldPruneChildren() const;

    /// Whether visibility can be exported for this prim.
    /// By default, this is based off of the export visibility setting in the
    /// export args.
    PXRUSDMAYA_API
    bool GetExportVisibility() const;

    /// Sets whether visibility can be exported for this prim.
    /// This will override the export args.
    PXRUSDMAYA_API
    void SetExportVisibility(const bool exportVis);

    /// Gets all of the exported prim paths that are potentially models, i.e.
    /// the prims on which this prim writer has authored kind metadata or
    /// otherwise expects kind metadata to exist (e.g. via reference).
    ///
    /// The USD export process will attempt to "fix-up" kind metadata to ensure
    /// contiguous model hierarchy for any potential model prims.
    ///
    /// The base implementation returns an empty vector.
    PXRUSDMAYA_API
    virtual const SdfPathVector& GetModelPaths() const;

    /// Gets a mapping from MDagPaths to exported prim paths.
    /// Useful only for DAG prim writers that override ShouldPruneChildren() to
    /// \c true but still want the export process to know about the Maya-to-USD
    /// correspondence for their descendants, e.g., for material binding
    /// purposes.
    /// The result vector should only include paths for which there is a true,
    /// one-to-one correspondence between the Maya node and USD prim; don't
    /// include any mappings where the mapped value is an invalid path.
    ///
    /// The base implementation for DAG prim writers simply maps GetDagPath()
    /// to GetUsdPath(). For DG prim writers, an empty map is returned.
    PXRUSDMAYA_API
    virtual const UsdMayaUtil::MDagPathMap<SdfPath>&
            GetDagToUsdPathMapping() const;

    /// The source Maya DAG path that we are consuming.
    ///
    /// If this prim writer is for a Maya DG node and not a DAG node, this will
    /// return an invalid MDagPath.
    PXRUSDMAYA_API
    const MDagPath& GetDagPath() const;

    /// The MObject for the Maya node being written by this writer.
    PXRUSDMAYA_API
    const MObject& GetMayaObject() const;

    /// The path of the destination USD prim to which we are writing.
    PXRUSDMAYA_API
    const SdfPath& GetUsdPath() const;

    /// The destination USD prim to which we are writing.
    PXRUSDMAYA_API
    const UsdPrim& GetUsdPrim() const;

    /// Gets the USD stage that we're writing to.
    PXRUSDMAYA_API
    const UsdStageRefPtr& GetUsdStage() const;

protected:
    /// Helper function for determining whether the current node has input
    /// animation curves.
    PXRUSDMAYA_API
    virtual bool _HasAnimCurves() const;

    /// Gets the current global export args in effect.
    PXRUSDMAYA_API
    const UsdMayaJobExportArgs& _GetExportArgs() const;

    /// Sets the value of \p attr to \p value at \p time with value
    /// compression. When this method is used to write attribute values,
    /// any redundant authoring of the default value or of time-samples
    /// are avoided (by using the utility class UsdUtilsSparseValueWriter).
    template <typename T>
    bool _SetAttribute(
            const UsdAttribute& attr,
            const T& value,
            const UsdTimeCode time = UsdTimeCode::Default()) {
        VtValue val(value);
        return _valueWriter.SetAttribute(attr, &val, time);
    }

    /// \overload
    /// This overload takes the value by pointer and hence avoids a copy
    /// of the value.
    /// However, it swaps out the value held in \p value for efficiency,
    /// leaving it in default-constructed state (value-initialized).
    template <typename T>
    bool _SetAttribute(
            const UsdAttribute& attr,
            T* value,
            const UsdTimeCode time = UsdTimeCode::Default()) {
        return _valueWriter.SetAttribute(attr, VtValue::Take(*value), time);
    }

    /// Get the attribute value-writer object to be used when writing
    /// attributes. Access to this is provided so that attribute authoring
    /// happening inside non-member functions can make use of it.
    PXRUSDMAYA_API
    UsdUtilsSparseValueWriter* _GetSparseValueWriter();

    UsdPrim _usdPrim;
    UsdMayaWriteJobContext& _writeJobCtx;

private:
    /// Whether this prim writer represents the transform portion of a merged
    /// shape and transform.
    bool _IsMergedTransform() const;

    /// Whether this prim writer represents the shape portion of a merged shape
    /// and transform.
    bool _IsMergedShape() const;

    /// The MDagPath for the Maya node being written, valid only when the prim
    /// writer is constructed with an MDagPath.
    const MDagPath _dagPath;

    /// The MObject for the Maya node being written, valid for both DAG and DG
    /// node prim writers.
    const MObject _mayaObject;

    const SdfPath _usdPath;
    const UsdMayaUtil::MDagPathMap<SdfPath> _baseDagToUsdPaths;

    UsdUtilsSparseValueWriter _valueWriter;

    bool _exportVisibility;
    bool _hasAnimCurves;
};

typedef std::shared_ptr<UsdMayaPrimWriter> UsdMayaPrimWriterSharedPtr;


PXR_NAMESPACE_CLOSE_SCOPE


#endif
