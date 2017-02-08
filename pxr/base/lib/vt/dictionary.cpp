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

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/base/tf/atomicOfstreamWrapper.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/base/tracelite/trace.h"

#include <boost/python/object.hpp>
#include <boost/python/dict.hpp>

#include <fstream>
#include <sstream>

#include <utility>
#include <vector>

using std::make_pair;
using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<VtDictionary>();
}

TF_MAKE_STATIC_DATA(VtDictionary, _emptyDictionary) {
    *_emptyDictionary = VtDictionary();
}

VtDictionary::VtDictionary(VtDictionary const& other) {
    if (other._dictMap)
        _dictMap.reset(new _Map(*other._dictMap));
}

VtDictionary::VtDictionary(std::initializer_list<value_type> init)
    : _dictMap(new _Map(init.begin(), init.end()))
{
}
   
VtDictionary& VtDictionary::operator=(VtDictionary const& other) {
    if (this != &other)
        _dictMap.reset(other._dictMap ? new _Map(*other._dictMap) : 0);
    return *this;
}

VtValue& VtDictionary::operator[](const string& key) {
    TfAutoMallocTag2 tag("Vt", "VtDictionary::operator[]");
    _CreateDictIfNeeded();
    return (*_dictMap)[key];
}

VtDictionary::size_type VtDictionary::count(const string& key) const {
    return _dictMap ? _dictMap->count(key) : 0;
}
    
VtDictionary::size_type VtDictionary::erase(const string& key) {
    return _dictMap ? _dictMap->erase(key) : 0;
}

void VtDictionary::erase(iterator it) {
    _dictMap->erase(it.GetUnderlyingIterator(_dictMap.get()));
}

void VtDictionary::erase(iterator f, iterator l) {
    if (!_dictMap)
        return;
    _dictMap->erase(f.GetUnderlyingIterator(_dictMap.get()),
        l.GetUnderlyingIterator(_dictMap.get()));
}

void VtDictionary::clear() {
    if (_dictMap)
        _dictMap->clear();
}

VtDictionary::iterator VtDictionary::find(const string& key) {
    return _dictMap ? iterator(_dictMap.get(), _dictMap->find(key))
        : iterator(); 
}

VtDictionary::const_iterator VtDictionary::find(const string& key) const {
    return _dictMap ? const_iterator(_dictMap.get(), _dictMap->find(key))
        : const_iterator(); 
}

VtDictionary::iterator VtDictionary::begin() {
    return _dictMap ? iterator(_dictMap.get(), _dictMap->begin())
        : iterator(); 
}

VtDictionary::const_iterator VtDictionary::begin() const {
    return _dictMap ? const_iterator(_dictMap.get(), _dictMap->begin())
        : const_iterator(); 
}

VtDictionary::iterator VtDictionary::end() {
    return iterator();
}

VtDictionary::const_iterator VtDictionary::end() const {
    return const_iterator();
}

VtDictionary::size_type VtDictionary::size() const {
    return _dictMap ? _dictMap->size() : 0;
}

bool VtDictionary::empty() const {
    return _dictMap ? _dictMap->empty() : true;
}

void VtDictionary::swap(VtDictionary& dict) {
    _dictMap.swap(dict._dictMap);
}

std::pair<VtDictionary::iterator, bool>
VtDictionary::insert(const value_type& obj) {
    TfAutoMallocTag2 tag("Vt", "VtDictionary::insert");
    _CreateDictIfNeeded();
    std::pair<_Map::iterator, bool> inserted = _dictMap->insert(obj);
    return std::pair<iterator, bool>(
        iterator(_dictMap.get(), inserted.first), inserted.second);
}

VtValue const *
VtDictionary::GetValueAtPath(string const &keyPath,
                             char const *delimiters) const
{
    return GetValueAtPath(TfStringSplit(keyPath, delimiters));
}

VtValue const *
VtDictionary::GetValueAtPath(vector<string> const &keyElems) const
{
    // Search for keyElems in dictionary.  All elements but the last in
    // keyElems must identify sub-dictionaries.
    if (keyElems.empty())
        return NULL;

    // Walk up to the last element.
    vector<string>::const_iterator
        start = keyElems.begin(), last = keyElems.end() - 1;

    // Descend dictionaries according to the key path elements.  If we fail
    // to find a dictionary element at any point, we can bail out.
    VtDictionary const *dict = this;
    for (vector<string>::const_iterator i = start; i != last; ++i) {
        const_iterator j = dict->find(*i);
        if (j == dict->end() || !j->second.IsHolding<VtDictionary>())
            return NULL;
        dict = &j->second.UncheckedGet<VtDictionary>();
    }

    // Now look for the last key path element which may or may not be a
    // dictionary.
    const_iterator j = dict->find(*last);
    return j != dict->end() ? &j->second : NULL;
}

void
VtDictionary::_SetValueAtPathImpl(vector<string>::const_iterator curKeyElem,
                                  vector<string>::const_iterator keyElemEnd,
                                  VtValue const &value)
{
    // Look ahead to see if we're on the last path element.  If so, we can set
    // the final value in place and return.
    vector<string>::const_iterator nextKeyElem = curKeyElem;
    ++nextKeyElem;
    if (nextKeyElem == keyElemEnd) {
        (*this)[*curKeyElem] = value;
        return;
    }

    // Otherwise we'll create a new or modify an existing subdictionary at key
    // *curKeyElem.  Look up an existing value or insert a new dictionary.
    iterator i = insert(make_pair(*curKeyElem, VtValue(VtDictionary()))).first;

    // Swap the value at curKeyElem with newDict.  In case a new dictionary was
    // inserted above, this is a noop swap.  In case the existing element is not
    // a dictionary, this replaces it with the empty newDict and leaves newDict
    // empty.  In case the existing element is a dictionary, this swaps it into
    // newDict to be modified.
    VtDictionary newDict;
    i->second.Swap(newDict);

    // Recurse with next path element.
    newDict._SetValueAtPathImpl(nextKeyElem, keyElemEnd, value);

    // Now store the modified dictionary back in the outer dictionary.
    i->second.Swap(newDict);
}

void
VtDictionary::SetValueAtPath(
    string const &keyPath, VtValue const &value, char const *delimiters)
{
    vector<string> keyElems = TfStringSplit(keyPath, delimiters);
    if (keyElems.empty())
        return;
    _SetValueAtPathImpl(keyElems.begin(), keyElems.end(), value);
}

void
VtDictionary::SetValueAtPath(
    vector<string> const &keyPath, VtValue const &value)
{
    if (keyPath.empty())
        return;
    _SetValueAtPathImpl(keyPath.begin(), keyPath.end(), value);
}

void
VtDictionary::_EraseValueAtPathImpl(vector<string>::const_iterator curKeyElem,
                                    vector<string>::const_iterator keyElemEnd)
{
    // Look ahead to see if we're on the last path element.  If so we can kill
    // the element at this path and return.
    vector<string>::const_iterator nextKeyElem = curKeyElem;
    ++nextKeyElem;
    if (nextKeyElem == keyElemEnd) {
        erase(*curKeyElem);
        return;
    }

    // Otherwise we'll descend into an existing subdictionary at key *curKeyElem
    // if one exists.
    iterator i = find(*curKeyElem);
    if (i != end() && i->second.IsHolding<VtDictionary>()) {
        VtDictionary newDict;
        i->second.Swap(newDict);
        newDict._EraseValueAtPathImpl(nextKeyElem, keyElemEnd);
        // store back potentially modified dict.
        if (newDict.empty()) {
            erase(i);
        } else {
            i->second.Swap(newDict);
        }
    }
}

void
VtDictionary::EraseValueAtPath(
    string const &keyPath, char const *delimiters)
{
    vector<string> keyElems = TfStringSplit(keyPath, delimiters);
    if (keyElems.empty())
        return;

    _EraseValueAtPathImpl(keyElems.begin(), keyElems.end());
}

void
VtDictionary::EraseValueAtPath(vector<string> const &keyPath)
{
    if (keyPath.empty())
        return;

    _EraseValueAtPathImpl(keyPath.begin(), keyPath.end());
}


void VtDictionary::_CreateDictIfNeeded() {
    if (!_dictMap) {
        TfAutoMallocTag2 tag("Vt", "VtDictionary::_CreateDictIfNeeded");
        _dictMap.reset(new _Map());
    }
}


VtDictionary const &VtGetEmptyDictionary() {
    return *_emptyDictionary;
}

Vt_DefaultGenerator VtDefault;

VtDictionary
VtDictionaryOver(const VtDictionary &strong, const VtDictionary &weak,
                 bool coerceToWeakerOpinionType)
{
    VtDictionary result = strong;
    VtDictionaryOver(&result, weak, coerceToWeakerOpinionType);
    return result;
}

void
VtDictionaryOver(VtDictionary *strong, const VtDictionary &weak,
                 bool coerceToWeakerOpinionType)
{
    if (!strong) {
        TF_CODING_ERROR("VtDictionaryOver: NULL dictionary pointer.");
        return;
    }
    strong->insert(weak.begin(), weak.end());

    if (coerceToWeakerOpinionType) {
        TF_FOR_ALL(i, *strong) {
            VtDictionary::const_iterator j = weak.find(i->first);
            if (j != weak.end()) {
                i->second.CastToTypeOf(j->second);
            }
        }
    }
}

void
VtDictionaryOver(const VtDictionary &strong, VtDictionary *weak,
                 bool coerceToWeakerOpinionType)
{
    if (!weak) {
        TF_CODING_ERROR("VtDictionaryOver: NULL dictionary pointer");
        return;
    }
    if (coerceToWeakerOpinionType) {
        TF_FOR_ALL(it, strong) {
            VtDictionary::iterator j = weak->find(it->first);
            if (j == weak->end()) {
                weak->insert(*it);
            }
            else {
                j->second = VtValue::CastToTypeOf(it->second, j->second);
            }
        }
    }
    else {
        // Can't use map::insert here, because that doesn't overwrite
        // values for keys in strong that are already in weak.
        TF_FOR_ALL(it, strong) {
            (*weak)[it->first] = it->second;
        }
    }
}

VtDictionary
VtDictionaryOverRecursive(const VtDictionary &strong, const VtDictionary &weak,
                          bool coerceToWeakerOpinionType)
{
    VtDictionary result = strong;
    VtDictionaryOverRecursive(&result, weak, coerceToWeakerOpinionType);
    return result;
}

void
VtDictionaryOverRecursive(VtDictionary *strong, const VtDictionary &weak,
                          bool coerceToWeakerOpinionType)
{
    if (!strong) {
        TF_CODING_ERROR("VtDictionaryOverRecursive: NULL dictionary pointer.");
        return;
    }

    TF_FOR_ALL(it, weak) {
        // If both dictionaries have values that are in turn dictionaries, 
        // recurse:
        if (VtDictionaryIsHolding<VtDictionary>(*strong, it->first) &&
            VtDictionaryIsHolding<VtDictionary>(weak, it->first)) {

            const VtDictionary &weakSubDict =
                VtDictionaryGet<VtDictionary>(weak, it->first);

            // Swap out the stored dictionary, mutate it, then swap it back in
            // place.  This avoids expensive copying.  There may still be a copy
            // if the VtValue storage is shared.
            VtDictionary::iterator i = strong->find(it->first);
            VtDictionary strongSubDict;
            i->second.Swap(strongSubDict);
            // Modify the extracted dict.
            VtDictionaryOverRecursive(&strongSubDict, weakSubDict);
            // Swap the modified dict back into place.
            i->second.Swap(strongSubDict);

        } else {
            // Insert will set strong with value from weak only if 
            // strong does not already have a value for that key.
            std::pair<VtDictionary::iterator, bool> result =strong->insert(*it);
            if (!result.second && coerceToWeakerOpinionType) {
                result.first->second.CastToTypeOf(it->second);
            }
        }
    }
}

void
VtDictionaryOverRecursive(const VtDictionary &strong, VtDictionary *weak,
                          bool coerceToWeakerOpinionType)
{
    if (!weak) {
        TF_CODING_ERROR("VtDictionaryOverRecursive: NULL dictionary pointer.");
        return;
    }

    TF_FOR_ALL(it, strong) {
        // If both dictionaries have values that are in turn dictionaries, 
        // recurse:
        if (VtDictionaryIsHolding<VtDictionary>(strong, it->first) &&
            VtDictionaryIsHolding<VtDictionary>(*weak, it->first)) {

            VtDictionary const &strongSubDict =
                VtDictionaryGet<VtDictionary>(strong, it->first);

            // Swap out the stored dictionary, mutate it, then swap it back in
            // place.  This avoids expensive copying.  There may still be a copy
            // if the VtValue storage is shared.
            VtDictionary::iterator i = weak->find(it->first);
            VtDictionary weakSubDict;
            i->second.Swap(weakSubDict);
            // Modify the extracted dict.
            VtDictionaryOverRecursive(strongSubDict, &weakSubDict);
            // Swap the modified dict back into place.
            i->second.Swap(weakSubDict);

        } else if (coerceToWeakerOpinionType) {
            // Else stomp over weak with strong but with type coersion.
            VtDictionary::iterator j = weak->find(it->first);
            if (j == weak->end()) {
                weak->insert(*it);
            } else {
                j->second = VtValue::CastToTypeOf(it->second, j->second);
            }
        } else {
            // Else stomp over weak with strong
            (*weak)[it->first] = it->second;
        }
    }
}


bool operator==(VtDictionary const &lhs, VtDictionary const &rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    // Iterate over all key-value pairs in the left-hand side dictionary
    // and check if they match up with the content of the right-hand
    // side dictionary.
    TF_FOR_ALL(it, lhs){
        VtDictionary::const_iterator it2 = rhs.find(it->first);
        if (it2 == rhs.end()) {
            return false;
        }
        if (it->second != it2->second) {
            return false;
        }
    }
    return true;
}

bool operator!=(VtDictionary const &lhs, VtDictionary const &rhs)
{
    return !(lhs == rhs);
}

std::ostream &
operator<<(std::ostream &stream, VtDictionary const &dict)
{
    bool first = true;
    stream << '{';
    TF_FOR_ALL(i, dict) {
        if (first)
            first = false;
        else
            stream << ", ";
        stream << '\'' << i->first << "': " << i->second;
    }
    stream << '}';
    return stream;
}

static void
_PrettyPrintVtValueToStream(const VtValue& vtval,
                            std::ostream& ostr,
                            size_t indentLevel);

template<class T>
static void
_PrettyPrintPyObjectToStream(const T& val,
                             std::ostream& ostr,
                             size_t indentLevel)
{
    TfPyInitialize();

    ostr << TfPyRepr(val);
}

static void
_PrettyPrintVtDictionaryToStream(const VtDictionary& vtdict,
                                 std::ostream& ostr,
                                 size_t indentLevel)
{
    if (vtdict.empty()) {
        ostr << "{}";
        return;
    }

    const static size_t shiftWidth = 2;
    const string outerShift(shiftWidth * indentLevel, ' ');

    ostr << "{\n";

    TF_FOR_ALL(i, vtdict) {

        const string& key = i->first;
        const VtValue& val = i->second;

        ostr << outerShift << "  ";

        _PrettyPrintPyObjectToStream(key, ostr, indentLevel + 1);

        ostr << ": ";

        _PrettyPrintVtValueToStream(val, ostr, indentLevel + 1);

        if (i.GetNext()) {
            ostr << ",";
        }

        ostr << "\n";
    }

    ostr << outerShift << "}";
}

static void
_PrettyPrintStdVectorToStream(const std::vector<VtValue>& stdvec,
                              std::ostream& ostr,
                              size_t indentLevel)
{
    if (stdvec.empty()) {
        ostr << "[]";
        return;
    }

    const static size_t shiftWidth = 2;
    const string outerShift(shiftWidth * indentLevel, ' ');

    ostr << "[\n";

    TF_FOR_ALL(i, stdvec) {

        const VtValue& val = *i;

        ostr << outerShift << "  ";

        _PrettyPrintVtValueToStream(val, ostr, indentLevel + 1);

        if (i.GetNext()) {
            ostr << ",";
        }

        ostr << "\n";
    }

    ostr << outerShift << "]";
}

static void
_PrettyPrintVtValueToStream(const VtValue& vtval,
                            std::ostream& ostr,
                            size_t indentLevel)
{
    if (vtval.IsHolding<VtDictionary>()) {
        _PrettyPrintVtDictionaryToStream(vtval.UncheckedGet<VtDictionary>(),
                                         ostr, indentLevel);
        return;
    }

    if (vtval.IsHolding<std::vector<VtValue> >()) {
        _PrettyPrintStdVectorToStream(
            vtval.UncheckedGet< std::vector<VtValue> >(), ostr, indentLevel);
        return;
    }

    _PrettyPrintPyObjectToStream(vtval, ostr, indentLevel);
}

VtDictionary
VtDictionaryFromPythonString(
    const string& content)
{
    if (content.empty()) {
        TF_CODING_ERROR("Cannot create VtDictionary from empty string.");
        return VtDictionary();
    }

    VtDictionary dict;
    if (!VtDictionaryFromPythonString(content, &dict)) {
        TF_RUNTIME_ERROR("Failed to extract VtDictionary from input: '%s'",
                         content.c_str());
        return VtDictionary();
    }

    return dict;
}

bool VtDictionaryFromPythonString(
    const string& content, 
    VtDictionary* dict)
{
    return TfPyEvaluateAndExtract(content, dict);
}


string
VtDictionaryPrettyPrint(
    const VtDictionary& vtdict)
{
    TRACE_FUNCTION();

    std::stringstream ss;

    _PrettyPrintVtDictionaryToStream(vtdict, ss, 0);

    return ss.str();
}

std::ostream&
VtDictionaryPrettyPrint(
    const VtDictionary& vtdict,
    std::ostream& ostream)
{
    TRACE_FUNCTION();

    _PrettyPrintVtDictionaryToStream(vtdict, ostream, 0);

    return ostream;
}

VtDictionary
VtDictionaryFromFile(const string& fpath)
{
    if (!fpath.empty() && TfIsFile(fpath, /* resolveSymlinks */ true)) {
        std::ifstream ifs(fpath.c_str());
        if (ifs.good()) {
            std::stringstream sstream;
            sstream << ifs.rdbuf();
            return VtDictionaryFromPythonString(sstream.str());
        }
    }

    return VtDictionary();
}

bool
VtDictionaryPrettyPrintToFile(const VtDictionary& vtdict,
                              const string& fpath)
{
    TRACE_FUNCTION();

    if (fpath.empty()) {
        return false;
    }

    TfAtomicOfstreamWrapper wrapper(fpath);

    string reason;
    if (!wrapper.Open(&reason)) {
        TF_RUNTIME_ERROR(reason);
        return false;
    }

    _PrettyPrintVtDictionaryToStream(vtdict, wrapper.GetStream(), 0);

    bool ok = wrapper.GetStream().good();

    if (ok) {
        if (!wrapper.Commit(&reason)) {
            TF_RUNTIME_ERROR(reason);
            ok = false;
        }
    }

    return ok;
}

PXR_NAMESPACE_CLOSE_SCOPE

