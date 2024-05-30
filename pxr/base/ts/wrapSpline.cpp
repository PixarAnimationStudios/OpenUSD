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

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"

#include "pxr/base/ts/keyFrameMap.h"
#include "pxr/base/ts/loopParams.h"
#include "pxr/base/ts/wrapUtils.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/valueFromPython.h"

#include <boost/python.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;
using std::string;
using std::vector;

static string
_GetRepr( const TsSpline & val )
{
    string repr = TF_PY_REPR_PREFIX + "Spline(";
    std::pair<TsExtrapolationType, TsExtrapolationType> extrapolation =
        val.GetExtrapolation();
    TsLoopParams loopParams = val.GetLoopParams();
    size_t counter = val.size();

    bool empty = (counter == 0)
        && (extrapolation.first == TsExtrapolationHeld)
        && (extrapolation.second == TsExtrapolationHeld)
        && (!loopParams.IsValid());

    if (!empty) {
        repr += "[";
        TF_FOR_ALL(it, val) {
            repr += TfPyRepr(*it);
            counter--;
            repr += counter > 0 ? ", " : "";
        }
        repr += "]";
        repr += string(", ") + TfPyRepr( extrapolation.first );
        repr += string(", ") + TfPyRepr( extrapolation.second );
        repr += string(", ") + TfPyRepr( loopParams );
    }

    repr += ")";
    return repr;
}

static TsSpline*
_ConstructFromKeyFrameDict( boost::python::object & kfDict, 
                            TsKnotType knotType )
{
    // Given an iterable python object, produces a C++ object suitable
    // for a C++11-style for-loop.
    class PyIterableWrapper {
    public:
        using Iterator = stl_input_iterator<object>;

        PyIterableWrapper(const object &iterable) : _iterable(iterable) { }

        Iterator begin() {
            return Iterator(_iterable);
        }
        Iterator end() {
            return Iterator();
        }
    private:
        object _iterable;
    };

    object items = kfDict.attr("items")();

    vector<TsKeyFrame> keyframes;
    for (const object &item : PyIterableWrapper(items))
    {
        object key = item[0];
        extract<TsTime> time(key);
        if (!time.check()) {
            TfPyThrowTypeError("expected time for keyframe in dict");
        }

        extract<VtValue> value( item[1] );

        // VtValue can hold any python object, so this always suceeds.
        // And if this fails in the future, boost.python will throw a
        // C++ exception, which will be converted to a Python exception.

        keyframes.push_back( TsKeyFrame( time(), value(), knotType ) );
    }

    return new TsSpline( keyframes );
}

static vector<VtValue>
_EvalMultipleTimes( const TsSpline & val,
                    const vector<TsTime> & times )
{
    vector<VtValue> result;
    result.reserve( times.size() );
    TF_FOR_ALL( it, times )
        result.push_back( val.Eval(*it) );

    return result;
}

static vector<VtValue>
_GetRange( const TsSpline & val,
           TsTime startTime, TsTime endTime)
{
    std::pair<VtValue, VtValue> range = val.GetRange(startTime, endTime);
    vector<VtValue> result;
    result.reserve( 2 );
    result.push_back( range.first );
    result.push_back( range.second );

    return result;
}

////////////////////////////////////////////////////////////////////////

static boost::python::object
_GetClosestKeyFrame( const TsSpline & val, TsTime t )
{
    std::optional<TsKeyFrame> kf = val.GetClosestKeyFrame(t);
    return kf ? object(*kf) : object();
}

static boost::python::object
_GetClosestKeyFrameBefore( const TsSpline & val, TsTime t )
{
    std::optional<TsKeyFrame> kf = val.GetClosestKeyFrameBefore(t);
    return kf ? object(*kf) : object();
}

static boost::python::object
_GetClosestKeyFrameAfter( const TsSpline & val, TsTime t )
{
    std::optional<TsKeyFrame> kf = val.GetClosestKeyFrameAfter(t);
    return kf ? object(*kf) : object();
}

////////////////////////////////////////////////////////////////////////

// Helper function to find begin/end iterators for times in slice.
// Throws exceptions for invalid slices.
static void
_SliceKeyframes( const boost::python::slice & index,
                 const TsKeyFrameMap & keyframes,
                 TsKeyFrameMap::const_iterator *begin,
                 TsKeyFrameMap::const_iterator *end )
{
    // Prohibit use of 'step'
    if (!TfPyIsNone(index.step()))
        TfPyThrowValueError("cannot use 'step' when indexing keyframes");

    // Find begin & end iterators, based on slice bounds
    if (!TfPyIsNone(index.start())) {
        boost::python::extract< TsTime > startExtractor( index.start() );
        if (!startExtractor.check())
            TfPyThrowValueError("expected time in keyframe slice");
        TsTime startVal = startExtractor();
        *begin = keyframes.lower_bound( startVal );
    } else {
        *begin = keyframes.begin();
    }
    if (!TfPyIsNone(index.stop())) {
        boost::python::extract< TsTime > stopExtractor( index.stop() );
        if (!stopExtractor.check())
            TfPyThrowValueError("expected time in keyframe slice");
        TsTime stopVal = stopExtractor();
        *end = keyframes.lower_bound( stopVal );
    } else {
        *end = keyframes.end();
    }
}

static size_t
_GetSize( const TsSpline & val )
{
    return val.GetKeyFrames().size();
}

static vector<TsKeyFrame>
_GetValues( const TsSpline & val )
{
    const TsKeyFrameMap &vec = val.GetKeyFrames();
    return vector<TsKeyFrame>(vec.begin(),vec.end());
}

static vector<TsTime>
_GetKeys( const TsSpline & val )
{
    const TsKeyFrameMap & kf = val.GetKeyFrames();

    vector<TsTime> result;
    result.reserve(kf.size());
    TF_FOR_ALL(it, kf)
        result.push_back( it->GetTime() );

    return result;
}

static TsKeyFrame
_GetItemByKey( const TsSpline & val, const TsTime & key )
{
    const TsKeyFrameMap & kf = val.GetKeyFrames();

    TsKeyFrameMap::const_iterator it = kf.find( key );

    if (it == kf.end())
        TfPyThrowIndexError(TfStringPrintf("no keyframe at time"));

    return *it;
}

static vector<TsKeyFrame>
_GetSlice( const TsSpline & val, const boost::python::slice & index )
{
    using boost::python::slice;

    const TsKeyFrameMap & kf = val.GetKeyFrames();
    TsKeyFrameMap::const_iterator begin;
    TsKeyFrameMap::const_iterator end;
    _SliceKeyframes( index, kf, &begin, &end );

    // Grab elements in slice bounds
    vector<TsKeyFrame> result;
    TsKeyFrameMap::const_iterator it;
    for (it = begin; it != end && it != kf.end(); ++it) {
        result.push_back( *it );
    }

    return result;
}

static bool
_ContainsItemWithKey( const TsSpline & val, const TsTime & key )
{
    return val.GetKeyFrames().find(key) != val.GetKeyFrames().end();
}

static void
_DelItemByKey( TsSpline & val, const TsTime & key )
{
    val.RemoveKeyFrame( key );
}

static void
_DelSlice( TsSpline & val, const boost::python::slice & index )
{
    vector<TsKeyFrame> keyframesToDelete = _GetSlice(val, index);

    TF_FOR_ALL(it, keyframesToDelete)
        val.RemoveKeyFrame(it->GetTime());
}

// For __iter__, we copy the current keyframes into a list, and return
// an iterator for the copy.  This provides the desired semantics of
// being able to iterate over the keyframes, modifying them, without
// worrying about missing any.
static PyObject*
_Iter( const TsSpline & val )
{
    vector<TsKeyFrame> kf = _GetValues(val);

    // Convert our vector to a Python list.
    PyObject *kf_list =
        TfPySequenceToList::apply< vector<TsKeyFrame> >::type()(kf);

    // Return a native Python iterator over the list.
    // The iterator will INCREF the list, and free it when iteration is
    // complete.
    PyObject *iter = PyObject_GetIter(kf_list);

    Py_DECREF(kf_list);

    return iter;
}

static Ts_AnnotatedBoolResult
_CanSetKeyFrame(TsSpline &self, const TsKeyFrame &kf)
{
    std::string reason;
    const bool canSet = self.CanSetKeyFrame(kf, &reason);
    return Ts_AnnotatedBoolResult(canSet, reason);
}

static void
_SetKeyFrame( TsSpline & self, const TsKeyFrame & kf )
{
    // Wrapper to discard optional intervalAffected argument.
    self.SetKeyFrame(kf);
}

static void
_SetKeyFrames( TsSpline & self,
               const std::vector<TsKeyFrame> & kf )
{
    // XXX: pixar-ism
    //
    // We need to save and restore the spline's non-key frame state.
    // The specification for Clear says that Clear should not affect this
    // other state, but several implementations of TsSplineInterface
    // fail to meet this specification.

    TsLoopParams params = self.GetLoopParams();
    std::pair<TsExtrapolationType, TsExtrapolationType> extrapolation;
    extrapolation = self.GetExtrapolation();
    self.Clear();
    self.SetExtrapolation(extrapolation.first, extrapolation.second);
    self.SetLoopParams(params);
    
    TF_FOR_ALL(it, kf) {
        self.SetKeyFrame(*it);
    }
}

// Simple breakdown.
static object
_Breakdown1( TsSpline & self, double x, TsKnotType type,
            bool flatTangents, double tangentLength,
            const VtValue &v = VtValue())
{
    // Wrapper to discard optional intervalAffected argument.
    std::optional<TsKeyFrame> kf =
        self.Breakdown(x, type, flatTangents, tangentLength, v);

    return kf ? object(*kf) : object();
}

BOOST_PYTHON_FUNCTION_OVERLOADS(_Breakdown1_overloads, _Breakdown1, 5, 6);

// Vectorized breakdown.
static std::map<TsTime,TsKeyFrame>
_Breakdown2( TsSpline & self, std::set<double> times,
             TsKnotType type, bool flatTangents, double tangentLength)
{
    // Wrapper to discard optional intervalAffected argument.
    TsKeyFrameMap vec;
    self.Breakdown(times, type, flatTangents, tangentLength, VtValue(), 
        NULL, &vec);
    std::map<TsTime,TsKeyFrame> map;
    TF_FOR_ALL(i, vec) {
        map.insert(std::make_pair(i->GetTime(),*i));
    }
    return map;
}

// Vectorized breakdown with multiple values
static std::map<TsTime,TsKeyFrame>
_Breakdown3( TsSpline & self, std::vector<double> times,
             TsKnotType type, bool flatTangents, double tangentLength,
             std::vector<VtValue> values)
{
    // Wrapper to discard optional intervalAffected argument.
    TsKeyFrameMap vec;
    self.Breakdown(times, type, flatTangents, tangentLength, values,
        NULL, &vec);
    std::map<TsTime,TsKeyFrame> map;
    TF_FOR_ALL(i, vec) {
        map.insert(std::make_pair(i->GetTime(),*i));
    }
    return map;
}

// Vectorized breakdown with multiple values and knot types
static std::map<TsTime,TsKeyFrame>
_Breakdown4( TsSpline & self, std::vector<double> times,
             std::vector<TsKnotType> types, 
             bool flatTangents, double tangentLength,
             std::vector<VtValue> values)
{
    // Wrapper to discard optional intervalAffected argument.
    TsKeyFrameMap vec;
    self.Breakdown(times, types, flatTangents, tangentLength, values,
        NULL, &vec);
    std::map<TsTime,TsKeyFrame> map;
    TF_FOR_ALL(i, vec) {
        map.insert(std::make_pair(i->GetTime(),*i));
    }
    return map;
}

static void
_SetExtrapolation(
    TsSpline & self,
    const std::pair<TsExtrapolationType, TsExtrapolationType>& x )
{
    self.SetExtrapolation(x.first, x.second);
}


// We implement these operators in the Python sense -- value equality --
// without just implementing and wrapping C++ operator==(), since
// Python also has the 'is' operator, but in C++ we don't want to
// gloss over the difference between value and identity comparison.
static bool
_Eq( const TsSpline & lhs, const TsSpline & rhs )
{
    return (&lhs == &rhs) ||
           (lhs.GetKeyFrames() == rhs.GetKeyFrames() &&
            lhs.GetExtrapolation() == rhs.GetExtrapolation() &&
            lhs.GetLoopParams() == rhs.GetLoopParams());
}
static bool
_Ne( const TsSpline & lhs, const TsSpline & rhs )
{
    return !_Eq(lhs, rhs);
}

// Functions for inspecting redundancy of key frames.
static bool
_IsKeyFrameRedundant( const TsSpline & self, const TsTime & key,
                      const VtValue & defaultValue = VtValue() ) 
{
    return self.IsKeyFrameRedundant(_GetItemByKey(self, key), defaultValue);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(_IsKeyFrameRedundant_overloads, 
                                _IsKeyFrameRedundant, 2, 3);

static bool
_IsKeyFrameRedundant_2( const TsSpline & self, 
                        const TsKeyFrame & kf,
                        const VtValue & defaultValue = VtValue() )
{
    return self.IsKeyFrameRedundant(kf, defaultValue);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(_IsKeyFrameRedundant_2_overloads,
                                _IsKeyFrameRedundant_2, 2, 3);

static bool
_IsSegmentFlat( const TsSpline & self, const TsTime & lkey,
                                                  const TsTime & rkey )
{
    return self.IsSegmentFlat(_GetItemByKey(self, lkey),
                              _GetItemByKey(self, rkey));
}

static bool
_IsSegmentFlat_2( const TsSpline & self, const TsKeyFrame & lkf,
                                                    const TsKeyFrame & rkf )
{
    return self.IsSegmentFlat(lkf, rkf);
}

static bool
_IsSegmentValueMonotonic( const TsSpline & self, 
                          const TsTime & lkey,
                          const TsTime & rkey )
{
    return self.IsSegmentValueMonotonic(_GetItemByKey(self, lkey),
                                        _GetItemByKey(self, rkey));
}

static bool
_IsSegmentValueMonotonic_2( const TsSpline & self, 
                            const TsKeyFrame & lkf,
                            const TsKeyFrame & rkf )
{
    return self.IsSegmentValueMonotonic(lkf, rkf);
}

void wrapSpline()
{
    using This = TsSpline;

    class_<This>("Spline", no_init)
        .def( init<>() )
        .def( init<const TsSpline &>() )
        .def( init<const vector< TsKeyFrame > &,
                   optional<TsExtrapolationType,
                            TsExtrapolationType,
                            const TsLoopParams &> >())
        .def( "__init__", make_constructor(&::_ConstructFromKeyFrameDict) )

        .def("__repr__", &::_GetRepr)
        .def("IsLinear", &This::IsLinear)
        .def("ClearRedundantKeyFrames", &This::ClearRedundantKeyFrames,
             "Clears all redundant key frames from the spline\n\n",
             ( arg("defaultValue") = VtValue(),
               arg("intervals") = GfMultiInterval(GfInterval(
                                   -std::numeric_limits<double>::infinity(),
                                   std::numeric_limits<double>::infinity()))))

        .add_property("typeName", &This::GetTypeName, 
                      "Returns the typename of the value type for "
                      "keyframes in this TsSpline. If no keyframes have been "
                      "set, returns None.")

        .def("SetKeyFrames", &::_SetKeyFrames, 
             "SetKeyFrames(keyFrames)\n\n"
             "keyFrames : sequence<TsKeyFrame>\n\n"
             "Replaces all of the specified keyframes. Keyframes may be "
             "specified using any type of Python sequence, such as a list or tuple.")
        .def("SetKeyFrame", &::_SetKeyFrame)
        .def("CanSetKeyFrame", &::_CanSetKeyFrame,
            "CanSetKeyFrame(kf) -> bool\n\n"
             "kf : TsKeyFrame\n\n"
             "Returns true if the given keyframe can be set on this "
             "spline. If it returns false, it also returns the reason why not. "
             "The reason can be accessed like this: "
             "anim.CanSetKeyFrame(kf).reasonWhyNot.")

        .def("Breakdown", _Breakdown1, _Breakdown1_overloads())
        .def("Breakdown", _Breakdown2, return_value_policy<TfPyMapToDictionary>())
        .def("Breakdown", _Breakdown3, return_value_policy<TfPyMapToDictionary>())
        .def("Breakdown", _Breakdown4, return_value_policy<TfPyMapToDictionary>())

        .add_property("extrapolation",
            make_function(&This::GetExtrapolation,
                return_value_policy< TfPyPairToTuple >()),
            make_function(&::_SetExtrapolation))

        .add_property("loopParams",
            make_function(&This::GetLoopParams,
                return_value_policy<return_by_value>()),
                &This::SetLoopParams)

        .def("IsTimeLooped", &This::IsTimeLooped,
              "True if the given time is in the unrolled region of a spline "
              "that is looping; i.e. not in the master region")
        .def("BakeSplineLoops", &This::BakeSplineLoops)
        .def("Eval", &This::Eval,
             ( arg("time"),
               arg("side") = TsRight ) )
        .def("EvalHeld", &This::EvalHeld,
             ( arg("side") = TsRight ) )
        .def("EvalDerivative", &This::EvalDerivative,
             ( arg("side") = TsRight ) )
        .def("Eval", &::_EvalMultipleTimes,
             return_value_policy< TfPySequenceToTuple >(),
             "Eval(times) -> sequence<VtValue>\n\n"
             "times : tuple<Time>\n\n"
             "Evaluates this spline at a tuple or list of times, "
             "returning a tuple of results.")
        .def("DoSidesDiffer", &This::DoSidesDiffer,
             (arg("time")))

        .def("Sample", &This::Sample,
             return_value_policy< TfPySequenceToTuple >() )

        .def("Range", &::_GetRange,
             return_value_policy< TfPySequenceToTuple >(),
             "Range(startTime, endTime) -> tuple<VtValue>\n\n"
             "startTime : Time\n"
             "endTime : Time\n\n"
             "The minimum and maximum of this spline returned as a "
             "tuple pair over the given time domain.")

        .add_property("empty", &This::IsEmpty)

        .add_property("frames",
            make_function(&::_GetKeys,
                return_value_policy< TfPySequenceToList >()),
            "A list of the frames for which keyframes exist.")

        .add_property("frameRange",
            &This::GetFrameRange,
            "A list with the first and last frames as elements.")

        .def("GetKeyFramesInMultiInterval", &This::GetKeyFramesInMultiInterval,
            return_value_policy<TfPySequenceToList>())

        // Keyframes dictionary interface
        .def("__len__", &::_GetSize)
        .def("__getitem__", &::_GetItemByKey)
        .def("__getitem__", &::_GetSlice,
             return_value_policy<TfPySequenceToList>())
        .def("has_key", &::_ContainsItemWithKey)
        .def("__contains__", &::_ContainsItemWithKey)
        .def("keys", &::_GetKeys,
             return_value_policy<TfPySequenceToList>())
        .def("values", &::_GetValues,
             return_value_policy<TfPySequenceToList>())
        .def("clear", &This::Clear)
        .def("__delitem__", &::_DelItemByKey)
        .def("__delitem__", &::_DelSlice)
        .def("__iter__", &::_Iter)

        .def("__eq__", &::_Eq)
        .def("__ne__", &::_Ne)

        .def("ClosestKeyFrame", &::_GetClosestKeyFrame, 
             "ClosestKeyFrame(time) -> TsKeyFrame\n\n"
             "time : Time\n\n"
             "Finds the keyframe closest to the given time. "
             "Returns None if there are no keyframes.")
        .def("ClosestKeyFrameBefore", &::_GetClosestKeyFrameBefore, 
             "ClosestKeyFrameBefore(time) -> TsKeyFrame\n\n"
             "time : Time\n\n"
             "Finds the closest keyframe before the given time. Returns None if "
             "no such keyframe exists.")
        .def("ClosestKeyFrameAfter", &::_GetClosestKeyFrameAfter, 
             "ClosestKeyFrameAfter(time) -> TsKeyFrame\n\n"
             "time : Time\n\n"
             "Finds the closest keyframe after the given time. Returns None if "
             "no such keyframe exists.")

         .def("IsKeyFrameRedundant", _IsKeyFrameRedundant, 
              _IsKeyFrameRedundant_overloads(
              "True if the key frame at the provided time is redundant.\n\n"
              "If a second parameter is provided it is used as a default value"
              "for the spline, so that the last knot on a spline can be marked"
              "redundant if it is equal to the default value.\n"
              "If the TsTime provided does not refer to a frame that has a"
              "knot, an exception will be thrown."))
          .def("IsKeyFrameRedundant", _IsKeyFrameRedundant_2, 
              _IsKeyFrameRedundant_2_overloads(
              "True if the key frame is redundant.\n\n"
              "If a second parameter is provided it is used as a default value"
              "for the spline, so that the last knot on a spline can be marked"
              "redundant if it is equal to the default value."))
         .def("HasRedundantKeyFrames", &This::HasRedundantKeyFrames,
              "True if any key frames are redundant.\n\n",
             ( arg("defaultValue") = VtValue() ) )
         .def("IsSegmentFlat", &::_IsSegmentFlat,
              "True if the segment between the two provided TsTimes is flat."
              "\n\n"
              "If either TsTime does not refer to a knot then an exception"
              " is thrown.")
         .def("IsSegmentFlat", &::_IsSegmentFlat_2,
              "True if the segment between the two provided TsKeyFrames is"
              "flat.")
         .def("IsSegmentValueMonotonic", &::_IsSegmentValueMonotonic,
              "True if the segment between the two provided TsTimes is"
              " monotonically increasing or monotonically decreasing, i.e. no"
              " extremes are present"
              "\n\n"
              "If either TsTime does not refer to a knot then an exception"
              " is thrown.")
         .def("IsSegmentValueMonotonic", &::_IsSegmentValueMonotonic_2,
              "True if the segment between the two provided TsKeyFrames is"
              " monotonically increasing or monotonically decreasing, i.e. no"
              " extremes are present")
         .def("IsVarying", &This::IsVarying,
              "True if the value of the spline changes over time, "
              "whether due to differing values among keyframes, "
              "knot sides, or non-flat tangents")
         .def("IsVaryingSignificantly", &This::IsVaryingSignificantly,
              "True if the value of the spline changes over time, "
              "more than a tiny amount, whether due to differing values among "
              "keyframes, knot sides, or non-flat tangents")
        ;

    VtValueFromPython<TsSpline>();
}
