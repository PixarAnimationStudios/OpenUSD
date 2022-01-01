#ifndef MATH_H
#define MATH_H

#include "nurbs.h"

namespace adsk
{
    //! This definition allows control over whether 32 or 64 bit time precision 
    // should be used.
#ifdef MAYA_64BIT_TIME_PRECISION
    long kTicksPerSecondInTicks = 141120000;
#else
    long kTicksPerSecondInTicks = 6000;
#endif

    const double kPI 					= 3.141592653589793;
    const double kSlerpThreshold 		= 0.00001;
    const double kDblEpsilonSqr 		= 1.0e-20;
    const double kDblEpsilon 			= 1.0e-10;
    const double kOneThird 				= 1.0 / 3.0;

    struct T4DblVector;
    struct T4dDblMatrix;
    struct TrotateXYZ;
    struct Tquaternion;

    bool lessThan(double x, double y, double epsilon = kDblEpsilon);
    bool lessEqual(double x, double y, double epsilon = kDblEpsilon);
    bool greaterThan(double x, double y, double epsilon = kDblEpsilon);
    bool equivalent(double a, double b, double epsilon = kDblEpsilon);
    
    template <typename T> T clamp(T v, T min, T max);
    template <typename T> int sign(T val);

    inline bool lessThan(double x, double y, double epsilon)
    {
        return (x <= (y - epsilon));
    }

    inline bool lessEqual(double x, double y, double epsilon)
    {
        return (x <= (y + epsilon));
    }

    inline bool greaterThan(double x, double y, double epsilon)
    {
        return (x >= (y + epsilon));
    }

    inline bool equivalent(double a, double b, double epsilon)
    {
        return (std::abs(a - b) <= epsilon);
    }

    // Convert double time into ticks and back to double to 
    // mimic Maya's loss of precision when time is involved
    double toTickDoubleTime(double time)
    {
        using Ticks = long;
        Ticks ticks = (Ticks)llround(
            std::max(
                std::min((double)std::numeric_limits<Ticks>::max(), (double)(time * kTicksPerSecondInTicks)),
                (double)std::numeric_limits<Ticks>::min()
            ));
        return (double)ticks / (double)kTicksPerSecondInTicks;
    }

    template <typename T>
    T clamp(T v, T min, T max)
    {
        return std::min(std::max(v, min), max);
    }

    template <typename T>
    int sign(T val)
    {
        return (T(0) < val) - (val < T(0));
    }

	template <typename T>
	int signNoZero(T val)
	{
		return (T(0) < val) ? -1 : 1;
	}

    template <typename T>
    T lerp(T f, T a, T b)
    {
        return a + f * (b - a);
    }
}

#include "T4DblVector.h"
#include "T4DblMatrix.h"
#include "TrotateXYZ.h"
#include "Tquaternion.h"
#include "Tbezier.h"

#endif