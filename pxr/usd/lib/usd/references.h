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
#ifndef USD_REFERENCES_H
#define USD_REFERENCES_H

#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/base/vt/value.h"

SDF_DECLARE_HANDLES(SdfPrimSpec);

/// \class UsdReferences
///
/// UsdReferences provides an interface to authoring and introspecting
/// references in Usd.
///
/// References are the primary operator for "encapsulated aggregation" of
/// scene description. \em aggregation means that references allow us to
/// build up rich scenes by instancing scene description recorded in a
/// (most often) different file.  When we say "instancing" here, we mean
/// that we can reference the same file many times at different locations
/// in a scene's namespace.  Referenced scene description can be overridden
/// in the referencing (or stronger) layers, allowing each instance to be
/// directly specialized. \em Encapsulated means that regardless of how much
/// scene description is in the referenced file, only the scene description
/// under the targetted prim will be composed into the aggregate scene.
///
/// Some important facts about references and use of the UsdReferences API:
/// \li Any prim can host zero, one or multiple references 
/// \li References are \em list \em editable, that is, they compose differently
/// than ordinary properties and metadata.  In any given LayerStack, each 
/// authored reference operation at the same SdfPath location in each layer 
/// (i.e. on the same prim) will compose into an aggregate result by
/// adding to, removing from, or stomping "weaker" references.
/// \li References can only target <b>root prims</b> in referenced files; this
/// is validated at authoring-time.  Note that references may omit the root prim
/// path if they target layers that have the 'defaultPrim' layer metadata set.
/// See SdfLayer::GetDefaultPrim() and SdfLayer::SetDefaultPrim().  In this
/// case, the reference targets the 'defaultPrim' in the referenced layer.
/// \li References can target the same layer in which they are authored,
/// as long as doing so does not introduce a cycle in the composition graph
/// \li The \c identifier component of a reference in the provided API
/// can be a resolvable asset-path to some external file, empty, in which case 
/// the reference targets the root layer of the LayerStack containing the 
/// referencing layer, or the identifer of an existing anonymous, in-memory-only
/// SdfLayer.  Care should be exercised in the latter case: calling Export() on 
/// an anonymous layer to serialize it to a file will not attempt to replace 
/// any references to anonymous layers with references to file-backed layers.
/// \li Opinions brought in by reference on an ancestor prim are weaker than
/// opinions brought in by references on a descendant prim.
///
/// \anchor Usd_DefaultPrim_References
/// <b>Expressing references without prim paths.</b>
///
/// References may omit the target prim path if the referenced layer has the
/// 'defaultPrim' metadata set.  See SdfLayer::GetDefaultPrim(),
/// SdfLayer::SetDefaultPrim().  In that case, such a reference targets the
/// layer's specified default prim.
///
/// \anchor Usd_Internal_References
/// <b>Expressing references to the containing LayerStack.</b>
///
/// References may omit the identifier specifying the referenced layer.  This
/// creates an "internal" reference. During composition, the referenced layer 
/// will be resolved to the root layer of the LayerStack containing the 
/// layer where the reference was authored.
///
/// \anchor Usd_Failing_References
/// <b>Reasons why adding a reference may fail, why adding a reference may
/// succeed but still generate errors, and what it all means.</b>
///
/// Add() and SetItems() can each fail for a number of reasons.  If one of
/// the specified prim targets for one of the references is not a root prim,
/// we will generate an error, fail to author any scene description, and return
/// \c false.  If anything goes wrong in attempting to write the reference,
/// we also return false, and the reference will also remain unauthored.
/// Otherwise, if the reference was successfully authored, we will return true.
/// <b>A succesful reference authoring operation may still generate composition
/// errors!</b>  Just because the reference you specified was syntactically
/// correct and therefore successfully authored, does not imply it was
/// meaningful. If you wish to ensure that the reference you are about to
/// author will be meaningfully consumable by your stage, you are strongly
/// encouraged to <b>ensure it will resolve to an actual file by using
/// UsdStage::ResolveIdentifierToEditTarget() before authoring the reference.</b>
///
/// Immediately upon successful authoring of the reference (before returning
/// from Add(), Remove(), Clear(), or SetItems()), the UsdStage on which the
/// reference was authored will recompose the subtree rooted at the prim
/// hosting the reference.  If the provided identifer does not resolve to a
/// layer that is already opened or that can be opened in the usd format,
/// \em or if the provided primPath is not an actual root prim in that layer,
/// the stage's recomposition will fail, and pass on composition errors to the
/// client.
class UsdReferences {
    friend class UsdPrim;

    explicit UsdReferences(const UsdPrim& prim) : _prim(prim) {}

public:

    // XXX: should we hide SdfReference here? it seems helpful for
    // Sd/Mf compatibility

    /// Add a reference to the reference listOp at the current EditTarget.
    /// \sa \ref Usd_Failing_References "Why adding references may fail" for
    /// explanation of expectations on \p ref and what return values and errors
    /// to expect, and \ref Usd_OM_ListOps for details on list editing and
    /// composition of listOps.
    USD_API
    bool Add(const SdfReference& ref);

    /// \overload 
    USD_API
    bool Add(const std::string &identifier,
             const SdfPath &primPath,
             const SdfLayerOffset &layerOffset = SdfLayerOffset());

    /// \overload
    /// \sa \ref Usd_DefaultPrim_References "References Without Prim Paths"
    USD_API
    bool Add(const std::string &identifier,
             const SdfLayerOffset &layerOffset = SdfLayerOffset());

    /// Add an internal reference to the specified prim.
    /// \sa \ref Usd_Internal_References "Internal References"
    USD_API
    bool AddInternal(const SdfPath &primPath,
                     const SdfLayerOffset &layerOffset = SdfLayerOffset());

    /// Removes the specified reference from the references listOp at the
    /// current EditTarget.  This does not necessarily eliminate the 
    /// reference completely, as it may be added or set in another layer in
    /// the same LayerStack as the current EditTarget.
    /// \sa \ref Usd_OM_ListOps 
    USD_API
    bool Remove(const SdfReference& ref);

    /// Removes the authored reference listOp edits at the current EditTarget.
    /// The same caveats for Remove() apply to Clear().  In fact, Clear() may
    /// actually increase the number of composed references, if the listOp
    /// being cleared contained the "remove" operator.
    /// \sa \ref Usd_OM_ListOps 
    USD_API
    bool Clear();

    /// Explicitly set the references, potentially blocking weaker opinions
    /// that add or remove items.
    /// \sa \ref Usd_Failing_References "Why adding references may fail" for
    /// explanation of expectations on \p ref and what return values and errors
    /// to expect, and \ref Usd_OM_ListOps for details on list editing and
    /// composition of listOps.
    USD_API
    bool SetItems(const SdfReferenceVector& items);

    /// Return the prim this object is bound to.
    const UsdPrim &GetPrim() const { return _prim; }

    // XXX Is this actually needed?
    /// \overload
    UsdPrim GetPrim() { return _prim; }

    // TODO: use safe bool idiom
    operator bool() { return bool(_prim); }


    // ---------------------------------------------------------------------- //
    // Private Methods and Members
    // ---------------------------------------------------------------------- //
private:

    SdfPrimSpecHandle _CreatePrimSpecForEditing();
    UsdPrim _prim;
};

#endif //USD_REFERENCES_H
