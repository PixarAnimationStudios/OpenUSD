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
#ifndef PXR_USD_USD_LIST_EDIT_IMPL_H
#define PXR_USD_USD_LIST_EDIT_IMPL_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/valueUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

// Non templated base class to namespace the overloading of _TranslatePath on 
// the list item type.
class Usd_ListEditImplBase
{
protected:
    // Generic path translation for the list edit types.
    static bool
    _TranslatePath(SdfPath *path, const UsdEditTarget& editTarget)
    {
        if (path->IsEmpty()) {
            TF_CODING_ERROR("Invalid empty path");
            return false;
        }

        // Root prim paths for all list edit types aren't expected to be 
        // mappable across non-local edit targets, so we can just use the given
        // path as-is.
        if (path->IsRootPrimPath()) {
            return true;
        }

        const SdfPath mappedPath = editTarget.MapToSpecPath(*path);
        if (mappedPath.IsEmpty()) {
            TF_CODING_ERROR(
                "Cannot map <%s> to current edit target.", path->GetText());
            return false;
        }

        // If the edit target points inside a variant, the mapped path may 
        // contain a variant selection. We need to strip this out, since
        // paths for these purposes may not contain variant selections.
        *path = mappedPath.StripAllVariantSelections();
        return true;
    }

    // Special path translation for references and payloads
    template <typename RefOrPayloadType>
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

        // The generic _TranslatePath errors for empty paths as those are 
        // invalid for specializes and inherits. However an empty prim path
        // is find for references and payloads.
        SdfPath path = refOrPayload->GetPrimPath();
        if (path.IsEmpty()) {
            return true;
        }

        // Translate the path and update the reference or payload.
        if (!_TranslatePath(&path, editTarget)) {
            return false;
        }
        refOrPayload->SetPrimPath(path);
        return true;
    }
};

// Templated implementation of the edit operations provided by UsdRefereneces
// and UsdPayloads. Editing payloads and references is identical outside of 
// their type.
template <class UsdListEditorType, class ListOpProxyType>
struct Usd_ListEditImpl : public Usd_ListEditImplBase
{
    using ListOpValueType = typename ListOpProxyType::value_type;
    using ListOpValueVector = typename ListOpProxyType::value_vector_type;
    
    static bool Add(const UsdListEditorType &editor, 
                    const ListOpValueType& itemIn, 
                    UsdListPosition position)
    {
        const UsdPrim &prim = editor.GetPrim();

        if (!prim) {
            TF_CODING_ERROR("Invalid prim");
            return false;
        }

        ListOpValueType item = itemIn;
        if (!_TranslatePath(&item, prim.GetStage()->GetEditTarget())) {
            return false;
        }

        SdfChangeBlock block;
        bool success = false;
        {
            TfErrorMark mark;
            if (ListOpProxyType listEditor = _GetListEditor(prim)) {
                Usd_InsertListItem(listEditor, item, position);
                // mark *should* contain only errors from adding the item, 
                // NOT any recomposition errors, because the 
                // SdfChangeBlock handily defers composition till after we 
                // leave this scope.
                success = mark.IsClean();
            }
        }
        return success;
    }
    
    static bool Remove(const UsdListEditorType &editor, 
                       const ListOpValueType& itemIn)
    {
        const UsdPrim &prim = editor.GetPrim();

        if (!prim) {
            TF_CODING_ERROR("Invalid prim");
            return false;
        }

        ListOpValueType item = itemIn;
        if (!_TranslatePath(&item, prim.GetStage()->GetEditTarget())) {
            return false;
        }

        SdfChangeBlock block;
        TfErrorMark mark;
        bool success = false;

        if (ListOpProxyType listEditor = _GetListEditor(prim)) {
            listEditor.Remove(item);
            success = mark.IsClean();
        }
        mark.Clear();
        return success;
    }

    static bool Clear(const UsdListEditorType &editor)
    {
        const UsdPrim &prim = editor.GetPrim();

        if (!prim) {
            TF_CODING_ERROR("Invalid prim");
            return false;
        }

        SdfChangeBlock block;
        TfErrorMark mark;
        bool success = false;

        if (ListOpProxyType listEditor = _GetListEditor(prim)) {
            success = listEditor.ClearEdits() && mark.IsClean();
        }
        mark.Clear();
        return success;
    }

    static bool Set(const UsdListEditorType &editor, 
                    const ListOpValueVector& itemsIn)
    {
        const UsdPrim &prim = editor.GetPrim();

        if (!prim) {
            TF_CODING_ERROR("Invalid prim");
            return false;
        }

        const UsdEditTarget& editTarget = prim.GetStage()->GetEditTarget();

        TfErrorMark mark;

        ListOpValueVector items;
        items.reserve(itemsIn.size());
        for (ListOpValueType item : itemsIn) {
            if (_TranslatePath(&item, editTarget)) {
                items.push_back(item);
            }
        }

        if (!mark.IsClean()) {
            return false;
        }

        SdfChangeBlock block;
        if (ListOpProxyType listEditor = _GetListEditor(prim)) {
            // There's a specific semantic meaning to setting the list op to 
            // an empty list which is to make the list explicitly
            // empty. We have to handle this case specifically as setting the
            // the list edit proxy's explicit items to an empty vector is a 
            // no-op when the list op is not currently explicit.
            if (items.empty()) {
                listEditor.ClearEditsAndMakeExplicit();
            } else {
                listEditor.GetExplicitItems() = items;
            }
        }

        bool success = mark.IsClean();
        mark.Clear();
        return success;
    }        

private:
    static ListOpProxyType
    _GetListEditor(const UsdPrim &prim)
    {
        if (!TF_VERIFY(prim)) {
            return ListOpProxyType();
        }

        SdfPrimSpecHandle spec = 
            prim.GetStage()->_CreatePrimSpecForEditing(prim);

        if (!spec) {
            return ListOpProxyType();
        }

        return _GetListEditorForSpec(spec);
    }

    // This is purposefully not defined here as this is needs to be specialized
    // by UsdPayloads, UsdReferences, etc. The implementation is defined in 
    // their cpp files.
    static ListOpProxyType
    _GetListEditorForSpec(const SdfPrimSpecHandle &spec);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_LIST_EDIT_IMPL_H
