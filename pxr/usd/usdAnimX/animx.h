#ifndef ANIM_X
#define ANIM_X

#ifdef WIN32
    #ifdef MAYA_ANIMATION_DLL_EXPORT
        #define DLL_EXPORT extern "C" __declspec(dllexport)
	#elif defined MAYA_ANIMATION_DLL_IMPORT
		#define DLL_EXPORT extern "C" __declspec(dllimport)
    #else
        #define DLL_EXPORT extern "C"
    #endif
#else
    #define DLL_EXPORT extern "C"
#endif

namespace adsk
{
#ifdef MAYA_64BIT_TIME_PRECISION
    typedef double seconds;
#else
    typedef float  seconds;
#endif

    //! Defines span interpolation method determined by the tangents of boundary keys TODO: difference between this and the next enum
    enum class SpanInterpolationMethod
    {
        Bezier,     //!< Bezier
        Linear,     //!< Linear
        Step,       //!< Step
        StepNext    //!< StepNext
    };

    //! Defines interpolation function within curve segments for non-rotation curves
    enum class CurveInterpolatorMethod
    {
        Bezier,     //!< Bezier
        Hermite,    //!< Hermite
        Sine,       //!< Sine
        Parabolic,  //!< Parabolic
        TangentLog  //!< Log
    };

    //! Defines interpolation mode for the rotation curves
    enum class CurveRotationInterpolationMethod
    {
        None,       //!< Non-rotational curves or non-sync Euler. This is the behaviour for pre-Maya 4.0
        Euler,      //!< Sync rotation curves & use Euler angles
        Slerp,      //!< Use quaternion cubic interpolation
        Quaternion, //!< Use quaternion tangent dependent interpolation
        Squad       //!< Use quaternion spherical interpolation
    };

    //! Defines the type of the tangent
    enum class TangentType
    {
        Global,     //!< Global
        Fixed,      //!< Fixed
        Linear,     //!< Linear
        Flat,       //!< Flat
        Step,       //!< Step
        Slow,       //!< Slow
        Fast,       //!< Fast
        Smooth,     //!< Smooth
        Clamped,    //!< Clamped
        Auto,       //!< Auto

        Sine,       //!< Sine
        Parabolic,  //!< Parabolic
        Log,        //!< Log

        Plateau,    //!< Plateau
        StepNext    //!< StepNext
    };

    //! Defines the type of the infinity
    /*!
        The infinity controls the shape of the animation curve in the
        regions before the first key and after the last key.
    */
    enum class InfinityType
    {
        Constant,       //!< Constant
        Linear,         //!< Linear
        Cycle,          //!< Cycle
        CycleRelative,  //!< CycleRelative
        Oscillate       //!< Oscillate
    };

    //! Defines type of infinity
    enum class Infinity
    {
        Pre,    //!< Pre-infinity
        Post    //!< Post-infinity
    };

    //! Single in- or out- tangent of a key
    struct Tangent
    {
        TangentType type;   //!< Tangent type
        seconds x;          //!< Tangent x value
        seconds y;          //!< Tangent y value
    };

    //! Quaternion
    struct Quaternion
    {
        double x, y, z, w;
    };

    struct KeyTimeValue
    {
        double time;                //!< Time
        double value;               //!< Value
    };

    //! Keyframe
    struct Keyframe : public KeyTimeValue
    {
        int index;                  //!< Sequential index of a key in a curve        
        Tangent tanIn;              //!< In-tangent
        Tangent tanOut;             //!< Out-tangent
        double quaternionW;         //!< W component of a quaternion if rotation curve
        bool linearInterpolation;   //!< Should curve be linearly interpolated? True if tangents of this key and the neighboring one are both linear.

        SpanInterpolationMethod spanInterpolationMethod() const
        {
            if (linearInterpolation)
                return SpanInterpolationMethod::Linear;
            if (tanOut.type == TangentType::Step)
                return SpanInterpolationMethod::Step;
            if (tanOut.type == TangentType::StepNext)
                return SpanInterpolationMethod::StepNext;
            return SpanInterpolationMethod::Bezier;
        }

        CurveInterpolatorMethod curveInterpolationMethod(bool isWeighted) const
        {
            CurveInterpolatorMethod method = isWeighted ? CurveInterpolatorMethod::Bezier : CurveInterpolatorMethod::Hermite;

            switch (tanOut.type)
            {
            case TangentType::Sine:
                method = CurveInterpolatorMethod::Sine;
                break;
            case TangentType::Parabolic:
                method = CurveInterpolatorMethod::Parabolic;
                break;
            case TangentType::Log:
                method = CurveInterpolatorMethod::TangentLog;
                break;
            default:
                break;
            }

            return method;
        }
    };

    /*!
        Adapter abstract class for a curve.

        Instance of a derived class of this adapter serves as an accessor for various basic
        curve information like its key frames or infinity types. This is to avoid introducing a new
        curve type the clients would have to convert their data to before invoking this library.

        The assumption about the curves are:
        - keys are stored sequentially, with indexes between [0 .. keyframeCount()-1]
        - each key has a broken in/out tangents
    */
    class ICurve
    {
    public:
        /*!
            Returns a key at a particular index, if valid. False otherwise.
        */
        virtual bool keyframeAtIndex(int index, Keyframe &key) const = 0;

        /*!
            Returns closest key at or after the specified time, or the last key if time is
            beyond the end of the curve.
        */
        virtual bool keyframe(double time, Keyframe &key) const = 0;

        /*!
            Returns the first key.
        */
        virtual bool first(Keyframe &key) const = 0;

        /*!
            Returns the last key
        */
        virtual bool last(Keyframe &key) const = 0;

        /*!
            Returns the pre infinity type.
        */
        virtual InfinityType preInfinityType() const = 0;

        /*!
            Returns the post infinity type.
        */
        virtual InfinityType postInfinityType() const = 0;

        /*!
            Returns whether a curve has weighted tangents.
        */
        virtual bool isWeighted() const = 0;

        /*!
            Returns the total number of key frames.
        */
        virtual unsigned int keyframeCount() const = 0;

        /*!
            Returns whether a curve is static (has a constant value).
        */
        virtual bool isStatic() const = 0;
    };



    /*!
        Evaluate a single curve.

        \param[in] time Time at which to evaluate the curve.
        \param[in] curve Curve accessor to operate on.

        \return
        Evaluated double value of a curve.
    */
    DLL_EXPORT double evaluateCurve(
        double time,
        const ICurve &curve);

    /*!
        Evaluate an individual curve segment.

        \param[in] interpolationMethod How should the segment be interpolated
        \param[in] curveInterpolatorMethod If span method is Bezier, choose desired evaluation model
        \param[in] time Time to evaluate segment at (startX <= time <= endX)
        \param[in] startX Time of the first key
        \param[in] startY Value of the first key
        \param[in] x1 Coordinate X of first control point
        \param[in] y1 Coordinate Y of first control point
        \param[in] x2 Coordinate X of second control point
        \param[in] y2 Coordinate Y of second control point
        \param[in] endX Time of the second key
        \param[in] endY Value of the second key

        \return
        Evaluated double value of a segment.
    */
    DLL_EXPORT double evaluateCurveSegment(
        SpanInterpolationMethod interpolationMethod,
        CurveInterpolatorMethod curveInterpolatorMethod,
        double time,
        double startX, double startY,
        double x1, double y1,
        double x2, double y2,
        double endX, double endY);

    /*!
        Evaluate rotation infinities using quaternion interpolation.

        \param[in] time Time to evaluate rotation curve's infinity (time <= first OR time >= last)
        \param[in] firstTime Time of the first key in the curve
        \param[in] firstValue Quaternion value of the first key
        \param[in] lastTime Time of the last key in the curve
        \param[in] lastValue Quaternion value of the last key       
        \param[in] preInfinityType Pre-infinity type of the curve
        \param[in] postInfinityType Post-infinity type of the curve
        \param[out] qOffset The rotation offset to add to the final quaternion evaluation
        \param[out] qStart First keyed rotation of the sequence, used in final quaternion evaluation to offset back to identity quaternion
        \param[out] inverse Depending on the infinity case, should the quaternion evaluation be inversed

        \return
        Boolean whether the resolved value needs post-processing. See more in evaluateQuaternionCurve() implementation
    */
    DLL_EXPORT bool evaluateQuaternionInfinity(
        double &time,
        double firstTime, Quaternion firstValue,
        double lastTime, Quaternion lastValue,
        InfinityType preInfinityType,
        InfinityType postInfinityType,
        Quaternion &qOffset,
        Quaternion &qStart,
        bool &inverse);

    /*!
        Evaluate infinities of a single curve.

        \param[in] time Time to evaluate infinity at
        \param[in] curve Curve accessor to operate on
        \param[in] infinity Evaluating pre or post infinity

        \return
        Evaluated double value of the infinity.
    */
    DLL_EXPORT double evaluateInfinity(
        double time,
        const ICurve &curve,
        Infinity infinity);

    /*!
        Evaluate an individual rotation curve segment using quaternion interpolation.

        \param[in] time Time to evaluation rotation at
        \param[in] interpolationMethod Rotation interpolation method
        \param[in] spanInterpolationMethod Interpolation mode of the segment
        \param[in] startTime Time of the key at/before given time
        \param[in] startValue Quaternion value of the start key
        \param[in] endTime Time of the key after given time
        \param[in] endValue Quaternion value of the end key
        \param[in] tangentType Out-tangent type of the start key        
        \param[in] prevValue Quaternion value of the prev key
        \param[in] nextValue Quaternion value of the next key                       

        \return
        Evaluated quaternion value of a rotation segment.
    */
    DLL_EXPORT Quaternion evaluateQuaternion(
        seconds time,
        CurveRotationInterpolationMethod interpolationMethod,
        SpanInterpolationMethod spanInterpolationMethod,
        seconds startTime, Quaternion startValue,
        seconds endTime, Quaternion endValue,
        TangentType tangentType,
        Quaternion prevValue,
        Quaternion nextValue);

    /*!
        Evaluate rotation curves using quaternion interpolation.

        \param[in] time Time to evaluate at
        \param[in] pcX Curve accessor for rotation X
        \param[in] pcY Curve accessor for rotation Y
        \param[in] pcZ Curve accessor for rotation Z
        \param[in] interpolationMethod Rotation interpolation method

        \return
        Evaluated quaternion value of a rotation curve.
    */
    DLL_EXPORT Quaternion evaluateQuaternionCurve(
        double time,
        const ICurve &pcX, const ICurve &pcY, const ICurve &pcZ,
        CurveRotationInterpolationMethod interpolationMethod);

    /*!
        Compute tangent values for a key with Auto tangent type.

        \param[in] calculateInTangent True when calculating "in" tangent. False if "out"
        \param[in] key Key tangent of we are calculating
        \param[in] prevKey Previous key, if present
        \param[in] nextKey Next key, if present
        \param[in] tanX Output tangent X value
        \param[in] tanY Output tangent Y value
    */
    DLL_EXPORT void autoTangent(
        bool calculateInTangent, 
        KeyTimeValue key, 
        KeyTimeValue *prevKey, 
        KeyTimeValue *nextKey, 
        CurveInterpolatorMethod curveInterpolationMethod,
        seconds &tanX,
        seconds &tanY);
}

#endif