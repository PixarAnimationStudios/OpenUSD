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
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/changes.h"

#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/notice.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakBase.h"
#include <boost/noncopyable.hpp>
#include <boost/python.hpp>

using namespace boost::python;

class Pcp_PyTestChangeProcessor
    : public TfWeakBase
    , public boost::noncopyable
{
public:
    Pcp_PyTestChangeProcessor(PcpCache* cache)
        : _cache(cache)
    {
        _layerChangedNoticeKey = TfNotice::Register(
            TfCreateWeakPtr(this),
            &Pcp_PyTestChangeProcessor::_HandleLayerDidChange);
    }

    void Enter()
    {
    }

    void Exit(const object&, const object&, const object&)
    {
        TfNotice::Revoke(_layerChangedNoticeKey);
    }

    SdfPathVector GetSignificantChanges() const
    {
        TF_FOR_ALL(it, _changes.GetCacheChanges()) {
            if (it->first == _cache) {
                return SdfPathVector(
                    it->second.didChangeSignificantly.begin(), 
                    it->second.didChangeSignificantly.end());
            }
        }
        return SdfPathVector();
    }

    SdfPathVector GetSpecChanges() const
    {
        TF_FOR_ALL(it, _changes.GetCacheChanges()) {
            if (it->first == _cache) {
                return SdfPathVector(
                    it->second.didChangeSpecs.begin(), 
                    it->second.didChangeSpecs.end());
            }
        }
        return SdfPathVector();
    }

    SdfPathVector GetPrimChanges() const
    {
        TF_FOR_ALL(it, _changes.GetCacheChanges()) {
            if (it->first == _cache) {
                return SdfPathVector(
                    it->second.didChangePrims.begin(), 
                    it->second.didChangePrims.end());
            }
        }
        return SdfPathVector();
    }

private:
    void _HandleLayerDidChange(const SdfNotice::LayersDidChange& n)
    {
        _changes.DidChange(
            std::vector<PcpCache*>(1, _cache), n.GetChangeListMap());
        _changes.Apply();
    }

private:
    PcpCache* _cache;
    TfNotice::Key _layerChangedNoticeKey;
    PcpChanges _changes;
};

void
wrapTestChangeProcessor()
{
    typedef Pcp_PyTestChangeProcessor This;
    typedef TfWeakPtr<Pcp_PyTestChangeProcessor> ThisPtr;

    class_<This, ThisPtr, boost::noncopyable>
        ("_TestChangeProcessor", init<PcpCache*>())
        .def("__enter__", &This::Enter, return_self<>())
        .def("__exit__", &This::Exit)

        .def("GetSignificantChanges", 
            &This::GetSignificantChanges,
            return_value_policy<TfPySequenceToList>())
        .def("GetSpecChanges", 
            &This::GetSpecChanges,
            return_value_policy<TfPySequenceToList>())
        .def("GetPrimChanges", 
            &This::GetPrimChanges,
            return_value_policy<TfPySequenceToList>())
        ;
}
