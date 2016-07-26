#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/tf/pyNoticeWrapper.h"
#include "pxr/base/tf/pyLock.h"

#include <boost/noncopyable.hpp>
#include <boost/python/class.hpp>

using namespace boost::python;

TF_INSTANTIATE_NOTICE_WRAPPER(
    TfStopBackgroundThreadsNotice, TfNotice);

static void
_StopBackgroundThreads()
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();
    TfThreadDispatcher::StopBackgroundThreads();
}

void wrapThreadDispatcher() {
    typedef TfThreadDispatcher This;

    class_<This, boost::noncopyable>("ThreadDispatcher", no_init)
        .def("StopBackgroundThreads", &::_StopBackgroundThreads)
        .staticmethod("StopBackgroundThreads")
        .add_static_property("physicalThreadLimit",
                             &This::GetPhysicalThreadLimit,
                             &This::SetPhysicalThreadLimit);

    TfPyNoticeWrapper<TfStopBackgroundThreadsNotice, TfNotice>
        ::Wrap("StopBackgroundThreadsNotice");
}
