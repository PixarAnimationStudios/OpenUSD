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
#ifndef SDF_LIST_EDITOR_H
#define SDF_LIST_EDITOR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/spec.h"

// XXX: Including this here may not be great, as this header leaks out
//      of Sdf and causes downstream code to pick up the tracelite
//      versions of TRACE_FUNCTION and TRACE_SCOPE instead of the
//      versions in Common/Trace. Functionally, they should be equivalent,
//      but could there be unwanted overhead involved?
// #include "pxr/base/tracelite/trace.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);
SDF_DECLARE_HANDLES(SdfSpec);

/// \class Sdf_ListEditor
///
/// Base class for list editor implementations in which list editing operations 
/// are stored in data field(s) associated with an owning spec.
///
template <class TypePolicy>
class Sdf_ListEditor
    : public boost::noncopyable 
{
private:
    typedef Sdf_ListEditor<TypePolicy> This;

public:
    typedef typename TypePolicy::value_type  value_type;
    typedef std::vector<value_type>          value_vector_type;

    SdfLayerHandle GetLayer() const
    {
        return _owner ? _owner->GetLayer() : SdfLayerHandle();
    }

    SdfPath GetPath() const
    {
        return _owner ? _owner->GetPath() : SdfPath();
    }

    bool IsValid() const
    {
        return (!IsExpired() && !IsNullEditor());
    }

    virtual bool IsExpired() const
    {
        return !_owner;
    }

    virtual bool IsNullEditor() const
    {
        return false;
    }

    bool HasKeys() const
    {
        if (IsExplicit()) {
            return true;
        }
        else if (IsOrderedOnly()) {
            return !_GetOperations(SdfListOpTypeOrdered).empty();
        }
        else {
            return (!_GetOperations(SdfListOpTypeAdded).empty()   ||
                    !_GetOperations(SdfListOpTypePrepended).empty() ||
                    !_GetOperations(SdfListOpTypeAppended).empty() ||
                    !_GetOperations(SdfListOpTypeDeleted).empty() ||
                    !_GetOperations(SdfListOpTypeOrdered).empty());
        }
    }

    virtual bool IsExplicit() const = 0;
    virtual bool IsOrderedOnly() const = 0;

    virtual SdfAllowed PermissionToEdit(SdfListOpType op) const
    {
        if (!_owner) {
            return SdfAllowed("List editor is expired");
        }

        if (!_owner->PermissionToEdit()) {
            return SdfAllowed("Permission denied");
        }

        return true;
    }

    virtual bool CopyEdits(const Sdf_ListEditor& rhs) = 0;
    virtual bool ClearEdits() = 0;
    virtual bool ClearEditsAndMakeExplicit() = 0;

    typedef boost::function<
                boost::optional<value_type>(const value_type&)
            >
        ModifyCallback;

    /// Modifies the operations stored in all operation lists.
    /// \p callback is called for every key.  If the returned key is
    /// invalid then the key is removed, otherwise it's replaced with the
    /// returned key.
    virtual void ModifyItemEdits(const ModifyCallback& cb) = 0;

    typedef boost::function<
                boost::optional<value_type>(SdfListOpType, const value_type&)
            >
        ApplyCallback;

    /// Apply the list operations represented by this interface to the given
    /// vector of values \p vec. If \p callback is valid then it's called
    /// for every key in the editor before applying it to \p vec.  If the
    /// returned key is empty then the key will not be applied.  Otherwise
    /// the returned key is applied, allowing callbacks to perform key
    /// translation.  Note that this means list editors can't meaningfully
    /// hold the empty key.
    virtual void ApplyEdits(value_vector_type* vec, 
                            const ApplyCallback& cb = ApplyCallback()) = 0;

    /// Returns the number of elements in the specified list of operations.
    size_t GetSize(SdfListOpType op) const
    {
        return _GetOperations(op).size();
    }

    /// Returns the \p i'th value in the specified list of operations.
    value_type Get(SdfListOpType op, size_t i) const
    {
        return _GetOperations(op)[i];
    }

    /// Returns the specified list of operations.
    value_vector_type GetVector(SdfListOpType op) const
    {
        return _GetOperations(op);
    }

    /// Returns the number of occurrences of \p val in the specified list of 
    /// operations.
    size_t Count(SdfListOpType op, const value_type& val) const
    {
        const value_vector_type& ops = _GetOperations(op);
        return std::count(ops.begin(), ops.end(), _typePolicy.Canonicalize(val));
    }

    /// Returns the index of \p val in the specified list of operations, -1
    /// if \p val is not found.
    size_t Find(SdfListOpType op, const value_type& val) const
    {
        const value_vector_type& vec = _GetOperations(op);
        typename value_vector_type::const_iterator findIt = 
            std::find(vec.begin(), vec.end(), _typePolicy.Canonicalize(val));
        if (findIt != vec.end()) {
            return std::distance(vec.begin(), findIt);
        }

        return size_t(-1);
    }

    /// Replaces the operations in the specified list of operations in range
    /// [index, index + n) with the given \p elems. 
    virtual bool ReplaceEdits(
        SdfListOpType op, size_t index, size_t n, 
        const value_vector_type& elems) = 0;

    /// Applies a \p rhs opinions about a given operation list to this one.
    virtual void ApplyList(SdfListOpType op, const Sdf_ListEditor& rhs) = 0;

protected:
    Sdf_ListEditor()
    {
    }

    Sdf_ListEditor(const SdfSpecHandle& owner, const TfToken& field,
                   const TypePolicy& typePolicy)
        : _owner(owner),
          _field(field),
          _typePolicy(typePolicy)
    {
    }

    virtual ~Sdf_ListEditor() = default;

    const SdfSpecHandle& _GetOwner() const
    {
        return _owner;
    }

    const TfToken& _GetField() const
    {
        return _field;
    }

    const TypePolicy& _GetTypePolicy() const
    {
        return _typePolicy;
    }

    virtual bool _ValidateEdit(SdfListOpType op, 
                               const value_vector_type& oldValues,
                               const value_vector_type& newValues) const
    {
        // See XXX comment for pxr/base/tracelite above.
        // TRACE_FUNCTION();

        // Disallow duplicate items from being stored in the new list
        // editor values. This is O(n^2), but we expect the number of elements
        // stored to be small enough that this won't matter.
        //
        // XXX: 
        // We assume that duplicate data items are never allowed to be
        // authored. For full generality, this information ought to come from
        // the layer schema.
        for (int i = 0; i < newValues.size(); ++i) {
            for (int j = i + 1; j < newValues.size(); ++j) {
                if (newValues[i] == newValues[j]) {
                    TF_CODING_ERROR("Duplicate item '%s' not allowed for "
                                    "field '%s' on <%s>",
                                    TfStringify(newValues[i]).c_str(),
                                    _field.GetText(),
                                    this->GetPath().GetText());
                    return false;
                }
            }
        }

        // Ensure that all new values are valid for this field.
        const SdfSchema::FieldDefinition* fieldDef = 
            _owner->GetSchema().GetFieldDefinition(_field);
        if (!fieldDef) {
            TF_CODING_ERROR("No field definition for field '%s'", 
                            _field.GetText());
        }
        else {
            TF_FOR_ALL(v, newValues) {
                if (SdfAllowed isValid = fieldDef->IsValidListValue(*v)) { }
                else {
                    TF_CODING_ERROR("%s", isValid.GetWhyNot().c_str());
                    return false;
                }
            }
        }

        return true;
    }

    virtual void _OnEdit(SdfListOpType op,
                         const value_vector_type& oldValues,
                         const value_vector_type& newValues) const
    {
    }

    virtual const value_vector_type& _GetOperations(SdfListOpType op) const = 0;

private:
    SdfSpecHandle _owner;
    TfToken _field;
    TypePolicy _typePolicy;

};

template <class TypePolicy>
std::ostream& 
operator<<(std::ostream& s, const Sdf_ListEditor<TypePolicy>& x)
{
    struct Util {
        typedef typename Sdf_ListEditor<TypePolicy>::value_vector_type 
            value_vector_type;

        static void _Write(std::ostream& s, const value_vector_type& v)
        {
            s << '[';
            for (size_t i = 0, n = v.size(); i < n; ++i) {
                if (i != 0) {
                    s << ", ";
                }
                s << v[i];
            }
            s << ']';
        }
    };

    if (!x.IsValid()) {
        return s;
    }
    else if (x.IsExplicit()) {
        Util::_Write(s, x.GetVector(SdfListOpTypeExplicit));
        return s;
    }
    else {
        s << "{ ";
        if (!x.IsOrderedOnly()) {
            s << "'added': ";
            Util::_Write(s, x.GetVector(SdfListOpTypeAdded));
            s << "'prepended': ";
            Util::_Write(s, x.GetVector(SdfListOpTypePrepended));
            s << "'appended': ";
            Util::_Write(s, x.GetVector(SdfListOpTypeAppended));
            s << ", 'deleted': ";
            Util::_Write(s, x.GetVector(SdfListOpTypeDeleted));
            s << ", ";
        }
        s << "'ordered': ";
        Util::_Write(s, x.GetVector(SdfListOpTypeOrdered));
        return s << " }";
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_LIST_EDITOR_H
