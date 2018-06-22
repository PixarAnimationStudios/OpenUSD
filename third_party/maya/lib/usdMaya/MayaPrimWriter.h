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
#ifndef PXRUSDMAYA_MAYAPRIMWRITER_H
#define PXRUSDMAYA_MAYAPRIMWRITER_H

#include "pxr/pxr.h"

#include "usdMaya/api.h"
#include "usdMaya/jobArgs.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUtils/sparseValueWriter.h"

#include <maya/MDagPath.h>

PXR_NAMESPACE_OPEN_SCOPE

class usdWriteJobCtx;
struct PxrUsdMayaJobExportArgs;
class UsdGeomImageable;
class UsdTimeCode;
class UsdUtilsSparseValueWriter;

/// Base class for all built-in and user-defined prim writers. Translates Maya
/// node data into USD prim(s).
class MayaPrimWriter
{
public:
    PXRUSDMAYA_API
    MayaPrimWriter(
            const MDagPath& iDag,
            const SdfPath& uPath,
            usdWriteJobCtx& jobCtx);

    PXRUSDMAYA_API
    virtual ~MayaPrimWriter();

    /// Main export function that runs when the traversal hits the node.
    virtual void Write(const UsdTimeCode &usdTime) = 0;

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
    
    /// Whether this prim writer add references on the USD stage.
    ///
    /// Base implementation returns \c false.
    PXRUSDMAYA_API
    virtual bool ExportsReferences() const;

    /// Whether the traversal routine using this prim writer should skip all of
    /// the Maya node's descendants when continuing traversal.
    ///
    /// Base implementation returns \c false; prim writers that handle export
    /// for their entire subtree should override.
    PXRUSDMAYA_API
    virtual bool ShouldPruneChildren() const;

    /// Whether visibility can be exported for this prim; overrides settings
    /// from the export args.
    PXRUSDMAYA_API
    bool GetExportVisibility() const;

    /// Sets whether visibility can be exported for this prim. If \c true,
    /// then uses the setting from the export args. If \c false, then will
    /// never export visibility on this prim.
    PXRUSDMAYA_API
    void SetExportVisibility(bool exportVis);

    /// Gets all of the prim paths that this prim writer has created.
    /// The base implementation just gets the single generated prim's path.
    /// Prim writers that generate more than one USD prim from a single Maya
    /// node should override this function to indicate all the prims they
    /// create.
    /// Implementations should add to outPaths instead of replacing. The return
    /// value should indicate whether any items were added to outPaths.
    PXRUSDMAYA_API
    virtual bool GetAllAuthoredUsdPaths(SdfPathVector* outPaths) const;

    /// The source Maya DAG path that we are consuming.
    PXRUSDMAYA_API
    const MDagPath& GetDagPath() const;

    /// The path of the destination USD prim to which we are writing.
    PXRUSDMAYA_API
    const SdfPath& GetUsdPath() const;

    /// The destination USD prim to which we are writing.
    PXRUSDMAYA_API
    const UsdPrim& GetUsdPrim() const;

    /// Gets the USD stage that we're writing to.
    PXRUSDMAYA_API
    const UsdStageRefPtr& GetUsdStage() const;

    /// Whether this prim writer is valid or not.
    /// Invalid prim writers shouldn't be used, and they shouldn't do anything.
    PXRUSDMAYA_API
    bool IsValid() const;

protected:
    /// Whether there is shape (not transform) animation.
    /// XXX This is really a helper method that needs to be moved
    /// elsewhere.
    virtual bool _IsShapeAnimated() const = 0;

    /// Sets the path on the USD stage where this prim writer should define its
    /// output prim.
    /// \sa GetUsdPath()
    PXRUSDMAYA_API
    void _SetUsdPath(const SdfPath& newPath);

    /// Sets whether this prim writer is valid or not.
    /// \sa IsValid()
    PXRUSDMAYA_API
    void _SetValid(bool isValid);

    /// Writes the attributes that are common to all UsdGeomImageable prims.
    /// Subclasses should almost always invoke _WriteImageableAttrs somewhere
    /// in their Write() function.
    /// The \p transformDagPath is useful only if merging shapes and transforms
    /// and this prim writer's source node is a shape, in which case
    /// \p transformDagPath should be the parent transform path. Otherwise,
    /// \p transformDagPath can just be the empty DAG path.
    PXRUSDMAYA_API
    bool _WriteImageableAttrs(
            const MDagPath& transformDagPath,
            const UsdTimeCode& usdTime,
            UsdGeomImageable& primSchema);

    /// Gets the current global export args in effect.
    PXRUSDMAYA_API
    const PxrUsdMayaJobExportArgs& _GetExportArgs() const;

    /// Sets the value of \p attr to \p value at \p time with value 
    /// compression. When this method is used to write attribute values, 
    /// any redundant authoring of the default value or of time-samples 
    /// are avoided (by using the utility class UsdUtilsSparseValueWriter).
    template <typename T>
    bool _SetAttribute(const UsdAttribute &attr, 
                       const T &value, 
                       const UsdTimeCode time=UsdTimeCode::Default()) {
        VtValue val(value);
        return _valueWriter.SetAttribute(attr, &val, time);
    }

    /// \overload
    /// This overload takes the value by pointer and hence avoids a copy 
    /// of the value.
    /// However, it swaps out the value held in \p value for efficiency, 
    /// leaving it in default-constructed state (value-initialized).
    template <typename T>
    bool _SetAttribute(const UsdAttribute &attr, 
                       T *value, 
                       const UsdTimeCode time=UsdTimeCode::Default()) {
        return _valueWriter.SetAttribute(attr, VtValue::Take(*value), time);
    }

    /// Get the attribute value-writer object to be used when writing 
    /// attributes. Access to this is provided so that attribute authoring 
    /// happening inside non-member functions can make use of it.
    PXRUSDMAYA_API
    UsdUtilsSparseValueWriter *_GetSparseValueWriter();

    UsdPrim _usdPrim;
    usdWriteJobCtx& _writeJobCtx;

private:
    MDagPath _dagPath;
    SdfPath _usdPath;

    UsdUtilsSparseValueWriter _valueWriter;

    bool _isValid;
    bool _exportVisibility;
};

typedef std::shared_ptr<MayaPrimWriter> MayaPrimWriterPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_MAYAPRIMWRITER_H
