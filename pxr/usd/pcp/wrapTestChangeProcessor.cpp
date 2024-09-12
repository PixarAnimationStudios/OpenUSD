//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/changes.h"

#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/notice.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakBase.h"
#include <boost/noncopyable.hpp>
#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

class Pcp_PyTestChangeProcessor
    : public TfWeakBase
    , public boost::noncopyable
{
public:
    Pcp_PyTestChangeProcessor(const PcpCache* cache)
        : _cache(cache)
    {
    }

    void Enter()
    {
        _layerChangedNoticeKey = TfNotice::Register(
            TfCreateWeakPtr(this),
            &Pcp_PyTestChangeProcessor::_HandleLayerDidChange);
    }

    void Exit(const object&, const object&, const object&)
    {
        TfNotice::Revoke(_layerChangedNoticeKey);
        _changes = PcpChanges();
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
        _changes.DidChange(_cache, n.GetChangeListVec());
        _changes.Apply();
    }

private:
    const PcpCache* _cache;
    TfNotice::Key _layerChangedNoticeKey;
    PcpChanges _changes;
};

} // anonymous namespace 

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
