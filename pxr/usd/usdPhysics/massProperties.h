//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_MASS_PROPERTIES_H
#define PXR_USD_USD_MASS_PROPERTIES_H

#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/api.h"
#include "pxr/usd/usd/common.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4f.h"

PXR_NAMESPACE_OPEN_SCOPE

GfQuatf UsdPhysicsIndexedRotation(uint32_t axis, float s, float c)
{
    float v[3] = { 0, 0, 0 };
    v[axis] = s;
    return GfQuatf(c, v[0], v[1], v[2]);
}

uint32_t UsdPhysicsGetNextIndex3(uint32_t i)
{
    return (i + 1 + (i >> 1)) & 3;
}


GfVec3f UsdPhysicsDiagonalize(const GfMatrix3f& m, GfQuatf& massFrame)
{
    const uint32_t MAX_ITERS = 24;

    GfQuatf q = GfQuatf(1.0);

    GfMatrix3f d;
    for (uint32_t i = 0; i < MAX_ITERS; i++)
    {
        GfMatrix3f axes(q);
        d = axes * m * axes.GetTranspose();

        float d0 = fabs(d[1][2]), d1 = fabs(d[0][2]), d2 = fabs(d[0][1]);
        uint32_t a = uint32_t(d0 > d1 && d0 > d2 ? 0 : d1 > d2 ? 1 : 2); // rotation axis index, from largest
                                                                         // off-diagonal
                                                                         // element

        uint32_t a1 = UsdPhysicsGetNextIndex3(a), a2 = UsdPhysicsGetNextIndex3(a1);
        if (d[a1][a2] == 0.0f || fabs(d[a1][a1] - d[a2][a2]) > 2e6 * fabs(2.0 * d[a1][a2]))
            break;

        float w = (d[a1][a1] - d[a2][a2]) / (2.0f * d[a1][a2]); // cot(2 * phi), where phi is the rotation angle
        float absw = fabs(w);

        GfQuatf r;
        if (absw > 1000)
            r = UsdPhysicsIndexedRotation(a, 1 / (4 * w), 1.0f); // h will be very close to 1, so use small angle approx instead
        else
        {
            float t = 1 / (absw + sqrt(w * w + 1)); // absolute value of tan phi
            float h = 1 / sqrt(t * t + 1); // absolute value of cos phi

            r = UsdPhysicsIndexedRotation(a, sqrt((1 - h) / 2) * ((w >= 0.0f) ? 1.0f : -1.0f), sqrt((1 + h) / 2));
        }

        q = (q * r).GetNormalized();
    }

    massFrame = q;
    return GfVec3f(d.GetColumn(0)[0], d.GetColumn(1)[1], d.GetColumn(2)[2]);
}

// -------------------------------------------------------------------------- //
// MASSPROPERTIES                                                             //
// -------------------------------------------------------------------------- //

/// \class UsdPhysicsMassProperties
///
/// Mass properties computation class. Used to combine together individual mass 
/// properties and produce final one.
///
class UsdPhysicsMassProperties
{
public:

    /// Construct a MassProperties
    USDPHYSICS_API UsdPhysicsMassProperties() : _inertiaTensor(0.0f), _centerOfMass(0.0f), _mass(1.0f)
    {
        _inertiaTensor[0][0] = 1.0;
        _inertiaTensor[1][1] = 1.0;
        _inertiaTensor[2][2] = 1.0;
    }

    /// Construct from individual elements.
    USDPHYSICS_API UsdPhysicsMassProperties(const float m, const GfMatrix3f& inertiaT, const GfVec3f& com)
        : _inertiaTensor(inertiaT), _centerOfMass(com), _mass(m)
    {
    }

    /// Scale mass properties.
    /// \p scale The linear scaling factor to apply to the mass properties.
    /// \return The scaled mass properties.    
    USDPHYSICS_API UsdPhysicsMassProperties operator*(const float scale) const
    {
        return UsdPhysicsMassProperties(_mass * scale, _inertiaTensor * scale, _centerOfMass);
    }

    /// Translate the center of mass by a given vector and adjust the inertia tensor accordingly.
    /// \p t The translation vector for the center of mass.    
    USDPHYSICS_API void Translate(const GfVec3f& t)
    {
        _inertiaTensor = TranslateInertia(_inertiaTensor, _mass, t);
        _centerOfMass += t;
    }

    /// Get the entries of the diagonalized inertia tensor and the corresponding reference rotation.
    /// \p inertia The inertia tensor to diagonalize.
    /// \p massFrame The frame the diagonalized tensor refers to.
    /// \return The entries of the diagonalized inertia tensor.    
    USDPHYSICS_API static GfVec3f GetMassSpaceInertia(const GfMatrix3f& inertia, GfQuatf& massFrame)
    {

        GfVec3f diagT = UsdPhysicsDiagonalize(inertia, massFrame);
        return diagT;
    }

    /// Translate an inertia tensor using the parallel axis theorem
    /// \p inertia The inertia tensor to translate.
    /// \p mass The mass of the object.
    /// \p t The relative frame to translate the inertia tensor to.
    /// \return The translated inertia tensor.
    USDPHYSICS_API static GfMatrix3f TranslateInertia(const GfMatrix3f& inertia, const float mass, const GfVec3f& t)
    {
        GfMatrix3f s;
        s.SetColumn(0, GfVec3f(0, t[2], -t[1]));
        s.SetColumn(1, GfVec3f(-t[2], 0, t[0]));
        s.SetColumn(2, GfVec3f(t[1], -t[0], 0));

        GfMatrix3f translatedIT = s * s.GetTranspose() * mass + inertia;
        return translatedIT;
    }


    /// Rotate an inertia tensor around the center of mass
    /// \p inertia The inertia tensor to rotate.
    /// \p q The rotation to apply to the inertia tensor.
    /// \return The rotated inertia tensor.    
    USDPHYSICS_API static GfMatrix3f RotateInertia(const GfMatrix3f& inertia, const GfQuatf& q)
    {
        GfMatrix3f m(q);
        GfMatrix3f rotatedIT = m.GetTranspose() * inertia * m;
        return rotatedIT;
    }

    /// Sum up individual mass properties.
    /// \p props Array of mass properties to sum up.
    /// \p transforms Reference transforms for each mass properties entry.
    /// \p count The number of mass properties to sum up.
    /// \return The summed up mass properties.    
    USDPHYSICS_API static UsdPhysicsMassProperties Sum(const UsdPhysicsMassProperties* props, const GfMatrix4f* transforms, const uint32_t count)
    {
        float combinedMass = 0.0f;
        GfVec3f combinedCoM(0.0f);
        GfMatrix3f combinedInertiaT = GfMatrix3f(0.0f);

        for (uint32_t i = 0; i < count; i++)
        {
            combinedMass += props[i]._mass;
            const GfVec3f comTm = transforms[i].Transform(props[i]._centerOfMass);
            combinedCoM += comTm * props[i]._mass;
        }

        if (combinedMass > 0.f)
            combinedCoM /= combinedMass;

        for (uint32_t i = 0; i < count; i++)
        {
            const GfVec3f comTm = transforms[i].Transform(props[i]._centerOfMass);
            combinedInertiaT += TranslateInertia(
                RotateInertia(props[i]._inertiaTensor, GfQuatf(transforms[i].ExtractRotation().GetQuat())),
                props[i]._mass, combinedCoM - comTm);
        }

        return UsdPhysicsMassProperties(combinedMass, combinedInertiaT, combinedCoM);
    }

    /// Get inertia tensor
    /// \return Inertia tensor
    const GfMatrix3f& GetInertiaTensor() const
    {
        return _inertiaTensor;
    }

    /// Set inertia tensor
    /// \p inTensor New inertia tensor.
    void SetInertiaTensor(const GfMatrix3f& inTensor)
    {
        _inertiaTensor = inTensor;
    }

    /// Get center of mass
    /// \return Center of mass
    const GfVec3f& GetCenterOfMass() const
    {
        return _centerOfMass;
    }

    /// Get mass
    /// \return Mass
    float GetMass() const
    {
        return _mass;
    }

    /// Set mass    
    /// \p inMass New mass.
    void SetMass(float inMass)
    {
        _mass = inMass;
    }    

private:
    GfMatrix3f _inertiaTensor; //!< The inertia tensor of the object.
    GfVec3f    _centerOfMass; //!< The center of mass of the object.
    float           _mass; //!< The mass of the object.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
