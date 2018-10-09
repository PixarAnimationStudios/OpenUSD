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
#ifndef PXRUSDMAYA_WRITE_JOB_CONTEXT_H
#define PXRUSDMAYA_WRITE_JOB_CONTEXT_H

/// \file usdMaya/writeJobContext.h

#include "usdMaya/api.h"
#include "usdMaya/jobArgs.h"
#include "usdMaya/primWriter.h"
#include "usdMaya/primWriterRegistry.h"

#include "pxr/pxr.h"

#include "pxr/usd/sdf/path.h"

#include <maya/MDagPath.h>
#include <maya/MObjectHandle.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class UsdMaya_SkelBindingsProcessor;
class UsdMaya_WriteJob;

/// \class UsdMayaWriteJobContext
/// \brief Provides basic functionality and access to shared data for UsdMayaPrimWriters.
///
/// The main purpose of this class is to handle source prim creation for instancing,
/// and to avoid storing the UsdMayaJobExportArgs and UsdStage on each prim writer.
///
class UsdMayaWriteJobContext {
protected:
    friend class UsdMaya_WriteJob;

    PXRUSDMAYA_API
    UsdMayaWriteJobContext(const UsdMayaJobExportArgs& args);

    PXRUSDMAYA_API
    ~UsdMayaWriteJobContext();

public:
    const UsdMayaJobExportArgs& GetArgs() const;
    const UsdStageRefPtr& GetUsdStage() const;

    /// Whether we will merge the transform at \p path with its single
    /// exportable child shape, given its hierarchy and the current path
    /// translation rules. (This always returns false if the export args
    /// don't specify merge transform/shape.)
    PXRUSDMAYA_API
    bool IsMergedTransform(const MDagPath& path) const;

    /// Convert DAG paths to USD paths, taking into account the current path
    /// translation rules (such as merge transform/shape, strip namespaces,
    /// visibility, etc).
    /// Note that this does *not* take into account instancing; the returned
    /// path is translated as if \p dagPath were un-instanced.
    PXRUSDMAYA_API
    SdfPath ConvertDagToUsdPath(const MDagPath& dagPath) const;

    /// Creates a prim writer that writes the Maya node at \p curDag, excluding
    /// its descendants, to the given \p usdPath.
    /// If \usdPath is the empty path, then the USD path will be inferred from
    /// the Maya DAG path.
    /// If \p forceUninstance is \c true, then the node will be un-instanced
    /// during export, even if the export args have instancing enabled.
    /// Note that you must call UsdMayaPrimWriter::Write() on the returned prim
    /// writer in order to author its USD attributes.
    PXRUSDMAYA_API
    UsdMayaPrimWriterSharedPtr CreatePrimWriter(
            const MDagPath& curDag,
            const SdfPath& usdPath = SdfPath(),
            const bool forceUninstance = false);

    /// Creates all prim writers necessary for writing the Maya node hierarchy
    /// rooted at \p rootDag to the USD namespace hierarchy rooted at
    /// \p rootUsdPath.
    /// If \p rootUsdPath is the empty path, then the USD path will be inferred
    /// from the root Maya DAG path.
    /// \p forceUninstance controls whether the root node will be un-instanced;
    /// nodes further down in the hierarchy will _never_ be un-instanced if
    /// the export args have instancing enabled.
    /// \p exportRootVisibility controls whether visibility is allowed to be
    /// exported for the rootmost node of the hierarchy; this is only useful
    /// for Maya instancers, which have special behavior on prototype roots.
    /// \p primWritersOut must be non-null; all of the valid prim writers
    /// for this prototype's hierarchy will be appended to the vector.
    /// Note that you must call UsdMayaPrimWriter::Write() on all the returned prim
    /// writers in order to author their USD attributes.
    PXRUSDMAYA_API
    void CreatePrimWriterHierarchy(
        const MDagPath& rootDag,
        const SdfPath& rootUsdPath,
        const bool forceUninstance,
        const bool exportRootVisibility,
        std::vector<UsdMayaPrimWriterSharedPtr>* primWritersOut);

    /// Mark \p path as containing bindings utilizing the skeleton
    /// at \p skelPath.
    /// Bindings are marked so that SkelRoots may be post-processed.
    /// Valid values for \p config are:
    /// - UsdMayaJobExportArgsTokens->explicit_: search for an existing SkelRoot
    /// - UsdMayaJobExportArgsTokens->auto_: create a SkelRoot if needed
    /// UsdMayaJobExportArgsTokens->none is not valid for \p config; it will
    /// mark an invalid binding.
    PXRUSDMAYA_API
    void MarkSkelBindings(
            const SdfPath& path,
            const SdfPath& skelPath,
            const TfToken& config);

protected:
    /// Opens the stage with the given \p filename for writing.
    /// If \p append is \c true, the file must already exist.
    PXRUSDMAYA_API
    bool _OpenFile(const std::string& filename, bool append);

    /// Whether the current export options should traverse \p curDag and its
    /// descendants.
    PXRUSDMAYA_API
    bool _NeedToTraverse(const MDagPath& curDag) const;

    /// Perform any necessary cleanup; call this before you save the stage.
    PXRUSDMAYA_API
    bool _PostProcess();

    UsdMayaJobExportArgs mArgs;
    // List of the primitive writers to iterate over
    std::vector<UsdMayaPrimWriterSharedPtr> mMayaPrimWriterList;
    // Stage used to write out USD file
    UsdStageRefPtr mStage;

private:
    /// A pair of paths, the first being the "export path", or where the
    /// master is authored on the stage, and the second being the "reference
    /// path", or the path that you should reference from any instances.
    /// They might be the same path.
    typedef std::pair<SdfPath, SdfPath> _ExportAndRefPaths;

    /// Gets the export path and reference path for an instance master of the
    /// given DAG path.
    /// In most cases, the two paths are the same, but is \p instancePath
    /// represents a directly-instanced gprim, the two paths may be different.
    /// The reference path is _always_ a prefix of the export path.
    _ExportAndRefPaths _GetInstanceMasterPaths(
            const MDagPath& instancePath) const;

    /// If the instance master for \p instancePath already exists, returns its
    /// USD path pair. Otherwise, creates the instance master (including its
    /// descendants) and returns the new USD path pair.
    _ExportAndRefPaths _FindOrCreateInstanceMaster(
            const MDagPath& instancePath);

    /// Gets the existing prim writers for the instance master of
    /// \p instancePath if that instance master has already been created.
    /// If successful, returns \c true and populates the iterators; the
    /// requested prim writers are in the range [\p begin, \p end).
    /// Otherwise, returns \c false and does nothing with the iterators.
    bool _GetInstanceMasterPrimWriters(
            const MDagPath& instancePath,
            std::vector<UsdMayaPrimWriterSharedPtr>::const_iterator* begin,
            std::vector<UsdMayaPrimWriterSharedPtr>::const_iterator* end) const;

    /// Prim writer search with ancestor type resolution behavior.
    UsdMayaPrimWriterRegistry::WriterFactoryFn _FindWriter(
            const std::string& mayaNodeType);

    struct MObjectHandleComp {
        bool operator()(const MObjectHandle& rhs, const MObjectHandle& lhs) const {
            return rhs.hashCode() < lhs.hashCode();
        }
    };

    /// Mapping of Maya object handles to the corresponding instance master's
    /// USD export path and reference path. A pair of empty USD paths means that
    /// we previously tried, but failed, to create the instance master.
    std::map<MObjectHandle, _ExportAndRefPaths, MObjectHandleComp>
            _objectsToMasterPaths;

    // Mapping of Maya object handles to the indices of the instance master's
    // prim writers in mMayaPrimWriterList. An instance master has a prim writer
    // for each node in its hierarchy; thus, the value represents an interval
    // of indices [first, last) in mMayaPrimWriterList. This avoids having to
    // manage two containers of shared pointers.
    std::map<MObjectHandle, std::pair<size_t, size_t>, MObjectHandleComp>
            _objectsToMasterWriters;

    UsdPrim mInstancesPrim;
    SdfPath mParentScopePath;

    std::unique_ptr<UsdMaya_SkelBindingsProcessor> _skelBindingsProcessor;

    // Cache of node type names mapped to their "resolved" writer factory,
    // taking into account Maya's type hierarchy (note that this means that
    // some types not resolved by the UsdMayaPrimWriterRegistry will get
    // resolved in this map).
    std::map<std::string, UsdMayaPrimWriterRegistry::WriterFactoryFn>
            mWriterFactoryCache;

    // UsdMaya_InstancedNodeWriter is in a separate file, but functions as
    // an internal helper for UsdMayaWriteJobContext.
    friend class UsdMaya_InstancedNodeWriter;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
