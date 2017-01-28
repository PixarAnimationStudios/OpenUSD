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
#ifndef SDF_MAPEDITOR_H
#define SDF_MAPEDITOR_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/spec.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

/// \class Sdf_MapEditor
///
/// Interface for private implementations used by SdfMapEditProxy.
///
template <class MapType>
class Sdf_MapEditor {
public:
    typedef typename MapType::key_type    key_type;
    typedef typename MapType::mapped_type mapped_type;
    typedef typename MapType::value_type  value_type;
    typedef typename MapType::iterator    iterator;

    virtual ~Sdf_MapEditor();

    /// Returns a string describing the location of the map being edited.
    /// This is used for debugging and error messages.
    virtual std::string GetLocation() const = 0;

    /// Returns owner of the map being edited.
    virtual SdfSpecHandle GetOwner() const = 0;

    /// Returns true if the map being edited is expired, false otherwise.
    virtual bool IsExpired() const = 0;

    /// Returns const pointer to map being edited.
    virtual const MapType* GetData() const = 0;

    /// Returns non-const pointer to map being edited. 
    /// All edits to the map should be done using the editing functions below.
    /// This function is primarily here for convenience. Ideally, only the
    /// const version of this function would exist.
    virtual MapType* GetData() = 0;
    
    /// \name Editing Operations
    /// @{

    virtual void Copy(const MapType& other) = 0;
    virtual void Set(const key_type& key, const mapped_type& other) = 0;
    virtual std::pair<iterator, bool> Insert(const value_type& value) = 0;
    virtual bool Erase(const key_type& key) = 0;

    virtual SdfAllowed IsValidKey(const key_type& key) const = 0;
    virtual SdfAllowed IsValidValue(const mapped_type& value) const = 0;

    /// @}

protected:
    Sdf_MapEditor();

};

template <class T>
boost::shared_ptr<Sdf_MapEditor<T> > 
Sdf_CreateMapEditor(const SdfSpecHandle& owner, const TfToken& field);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_MAPEDITOR_H
