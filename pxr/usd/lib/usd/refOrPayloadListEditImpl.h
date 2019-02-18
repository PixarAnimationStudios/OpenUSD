//
// Copyright 2019 Pixar
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
#ifndef USD_REFORPAYLOADLISTEDITIMPL_H
#define USD_REFORPAYLOADLISTEDITIMPL_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/valueUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

// Templated implementation of the edit operations provided by UsdRefereneces
// and UsdPayloads. Editing payloads and references is identical outside of 
// their type.
template <class RefsOrPayloadsEditorType, class RefsOrPayloadsProxyType>
struct Usd_RefOrPayloadListEditImpl 
{
    using RefOrPayloadType = 
        typename RefsOrPayloadsProxyType::value_type;
    using RefOrPayloadVector 
        = typename RefsOrPayloadsProxyType::value_vector_type;
    
    static bool Add(const RefsOrPayloadsEditorType &editor, 
                    const RefOrPayloadType& refOrPayloadIn, 
                    UsdListPosition position)
    {
        const UsdPrim &prim = editor.GetPrim();

        if (!prim) {
            TF_CODING_ERROR("Invalid prim");
            return false;
        }

        RefOrPayloadType refOrPayload = refOrPayloadIn;
        if (!_TranslatePath(&refOrPayload, prim.GetStage()->GetEditTarget())) {
            return false;
        }

        SdfChangeBlock block;
        bool success = false;
        {
            TfErrorMark mark;
            if (RefsOrPayloadsProxyType listEditor = _GetListEditor(prim)) {
                Usd_InsertListItem(listEditor, refOrPayload, position);
                // mark *should* contain only errors from adding the payload or 
                // reference, NOT any recomposition errors, because the 
                // SdfChangeBlock handily defers composition till after we 
                // leave this scope.
                success = mark.IsClean();
            }
        }
        return success;
    }
    
    static bool Add(const RefsOrPayloadsEditorType &editor,
                    const std::string &assetPath,
                    const SdfPath &primPath,
                    const SdfLayerOffset &layerOffset,
                    UsdListPosition position)
    {
        return Add(editor, 
            RefOrPayloadType(assetPath, primPath, layerOffset), position);
    }

    static bool Add(const RefsOrPayloadsEditorType &editor,
                    const std::string &assetPath,
                    const SdfLayerOffset &layerOffset,
                    UsdListPosition position)
    {
        return Add(editor, assetPath, SdfPath(), layerOffset, position);
    }

    static bool AddInternal(const RefsOrPayloadsEditorType &editor,
                            const SdfPath &primPath,
                            const SdfLayerOffset &layerOffset,
                            UsdListPosition position)
    {
        return Add(editor, std::string(), primPath, layerOffset, position);
    }

    static bool Remove(const RefsOrPayloadsEditorType &editor, 
                       const RefOrPayloadType& refOrPayloadIn)
    {
        const UsdPrim &prim = editor.GetPrim();

        if (!prim) {
            TF_CODING_ERROR("Invalid prim");
            return false;
        }

        RefOrPayloadType refOrPayload = refOrPayloadIn;
        if (!_TranslatePath(&refOrPayload, prim.GetStage()->GetEditTarget())) {
            return false;
        }

        SdfChangeBlock block;
        TfErrorMark mark;
        bool success = false;

        if (RefsOrPayloadsProxyType listEditor = _GetListEditor(prim)) {
            listEditor.Remove(refOrPayload);
            success = mark.IsClean();
        }
        mark.Clear();
        return success;
    }

    static bool Clear(const RefsOrPayloadsEditorType &editor)
    {
        const UsdPrim &prim = editor.GetPrim();

        if (!prim) {
            TF_CODING_ERROR("Invalid prim");
            return false;
        }

        SdfChangeBlock block;
        TfErrorMark mark;
        bool success = false;

        if (RefsOrPayloadsProxyType listEditor = _GetListEditor(prim)) {
            success = listEditor.ClearEdits() && mark.IsClean();
        }
        mark.Clear();
        return success;
    }

    static bool Set(const RefsOrPayloadsEditorType &editor, 
                    const RefOrPayloadVector& itemsIn)
    {
        const UsdPrim &prim = editor.GetPrim();

        if (!prim) {
            TF_CODING_ERROR("Invalid prim");
            return false;
        }

        const UsdEditTarget& editTarget = prim.GetStage()->GetEditTarget();

        TfErrorMark mark;

        RefOrPayloadVector items;
        items.reserve(itemsIn.size());
        for (RefOrPayloadType item : itemsIn) {
            if (_TranslatePath(&item, editTarget)) {
                items.push_back(item);
            }
        }

        if (!mark.IsClean()) {
            return false;
        }

        SdfChangeBlock block;
        if (RefsOrPayloadsProxyType listEditor = _GetListEditor(prim)) {
            listEditor.GetExplicitItems() = items;
        }

        bool success = mark.IsClean();
        mark.Clear();
        return success;
    }        

private:
    static bool
    _TranslatePath(RefOrPayloadType* refOrPayload, 
                   const UsdEditTarget& editTarget)
    {
        // We do not map prim paths across the edit target for non-internal 
        // references or payloads, as these paths are supposed to be in the 
        // namespace of the layer stack.
        if (!refOrPayload->GetAssetPath().empty()) {
            return true;
        }

        // Non-sub-root payloads aren't expected to be mappable across 
        // non-local edit targets, so we can just use the given reference or 
        // payload as-is.
        if (refOrPayload->GetPrimPath().IsEmpty() ||
            refOrPayload->GetPrimPath().IsRootPrimPath()) {
            return true;
        }

        const SdfPath mappedPath = editTarget.MapToSpecPath(
            refOrPayload->GetPrimPath());
        if (mappedPath.IsEmpty()) {
            TF_CODING_ERROR(
                "Cannot map <%s> to current edit target.", 
                refOrPayload->GetPrimPath().GetText());
            return false;
        }

        // If the edit target points inside a variant, the mapped path may 
        // contain a variant selection. We need to strip this out, since
        // inherit paths may not contain variant selections.
        refOrPayload->SetPrimPath(mappedPath.StripAllVariantSelections());
        return true;
    }

    static RefsOrPayloadsProxyType
    _GetListEditor(const UsdPrim &prim)
    {
        if (!TF_VERIFY(prim)) {
            return RefsOrPayloadsProxyType();
        }

        SdfPrimSpecHandle spec = 
            prim.GetStage()->_CreatePrimSpecForEditing(prim);

        if (!spec) {
            return RefsOrPayloadsProxyType();
        }

        return _GetListEditorForSpec(spec);
    }

    // This is purposefully not defined here as this is needs to be specialized
    // by UsdPayloads and UsdReferences. The implementation is defined in their
    // cpp files.
    static RefsOrPayloadsProxyType
    _GetListEditorForSpec(const SdfPrimSpecHandle &spec);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_REFORPAYLOADLISTEDITIMPL_H
