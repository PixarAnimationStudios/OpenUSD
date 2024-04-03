//
// Copyright 2023 Pixar
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

#ifndef PXR_BASE_TS_KEY_FRAME_MAP_H
#define PXR_BASE_TS_KEY_FRAME_MAP_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/keyFrame.h"
#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class TsKeyFrameMap
///
/// \brief An ordered sequence of keyframes with STL-compliant API
/// for finding, inserting, and erasing keyframes while maintaining order.
///
/// We use this instead of a map or set of keyframes because it allows
/// the keyframes to be stored with fewer heap allocation and better
/// data locality.
///
/// For the sake of efficiency, this class makes two assumptions:
///     The keyframes are always ordered
///     There is never more than one key frame at a given time.
///
/// The client (TsSpline) is responsible for maintaining these
/// preconditions.
class TsKeyFrameMap {

public:
    typedef std::vector<TsKeyFrame>::iterator iterator;
    typedef std::vector<TsKeyFrame>::const_iterator const_iterator;
    typedef std::vector<TsKeyFrame>::reverse_iterator reverse_iterator;
    typedef std::vector<TsKeyFrame>::const_reverse_iterator const_reverse_iterator;

    TS_API
    TsKeyFrameMap() {
    }

    TS_API
    TsKeyFrameMap(TsKeyFrameMap const& other) :
        _data(other._data) {
    }

    TS_API
    TsKeyFrameMap& operator=(TsKeyFrameMap const& other) {
        _data = other._data;
        return *this;
    }

    TS_API
    iterator begin() {
        return _data.begin();
    }

    TS_API
    const_iterator begin() const {
        return _data.begin();
    }

    TS_API
    iterator end() {
        return _data.end();
    }

    TS_API
    const_iterator end() const {
        return _data.end();
    }

    TS_API
    reverse_iterator rbegin() {
        return _data.rbegin();
    }

    TS_API
    const_reverse_iterator rbegin() const {
        return _data.rbegin();
    }

    TS_API
    reverse_iterator rend() {
        return _data.rend();
    }

    TS_API
    const_reverse_iterator rend() const {
        return _data.rend();
    }

    TS_API
    size_t size() const {
        return _data.size();
    }

    TS_API
    size_t max_size() const {
        return _data.max_size();
    }

    TS_API
    bool empty() const {
        return _data.empty();
    }

    TS_API
    void reserve(size_t size)  {
        _data.reserve(size);
    }

    TS_API
    void clear() {
        _data.clear();
    }

    TS_API
    void swap(TsKeyFrameMap& other) {
        other._data.swap(_data);
    }

    TS_API
    void swap(std::vector<TsKeyFrame>& other) {
        other.swap(_data);
    }

    TS_API
    void erase(iterator first, iterator last) {
        _data.erase(first,last);
    }

    TS_API
    void erase(iterator i) {
        _data.erase(i);
    }

    TS_API
    bool operator==(const TsKeyFrameMap& other) const {
        return (_data == other._data);
    }

    TS_API
    bool operator!=(const TsKeyFrameMap& other) const {
        return (_data != other._data);
    }

    TS_API
    iterator lower_bound(TsTime t);

    TS_API
    const_iterator lower_bound(TsTime t) const;

    TS_API
    iterator upper_bound(TsTime t);

    TS_API
    const_iterator upper_bound(TsTime t) const;

    TS_API
    iterator find(const TsTime &t) {
        iterator i = lower_bound(t);
        if (i != _data.end() && i->GetTime() == t) {
            return i;
        }
        return _data.end();
    }

    TS_API
    const_iterator find(const TsTime &t) const {
        const_iterator i = lower_bound(t);
        if (i != _data.end() && i->GetTime() == t) {
            return i;
        }
        return _data.end();
    }
    
    TS_API
    iterator insert(TsKeyFrame const& value) {
        // If the inserted value comes at the end, then avoid doing the
        // lower_bound and just insert there.
        iterator i = end();
        if (!empty() && value.GetTime() <= _data.back().GetTime()) {
            i = lower_bound(value.GetTime());
        }
        
        return _data.insert(i,value);
    }

    template <class Iter>
    void insert(Iter const& first, Iter const& last) {
        for(Iter i = first; i != last; i++) {
            insert(*i);
        }
    }

    TS_API
    void erase(TsTime const& t) {
        iterator i = find(t);
        if (i != _data.end()) {
            erase(i);
        }
    }

    TS_API
    TsKeyFrame& operator[](const TsTime& t) {
        iterator i = lower_bound(t);
        if (i != _data.end() && i->GetTime() == t) {
            return *i;
        }
	TsKeyFrame &k = *(_data.insert(i,TsKeyFrame()));
	k.SetTime(t);
	return k;
    }

private:
    std::vector<TsKeyFrame> _data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
