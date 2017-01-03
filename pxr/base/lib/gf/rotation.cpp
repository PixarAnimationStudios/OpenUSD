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
#include "pxr/base/gf/rotation.h"

#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/ostreamHelpers.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/type.h"

#include <iostream>

// CODE_COVERAGE_OFF_GCOV_BUG
TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfRotation>();
}
// CODE_COVERAGE_ON_GCOV_BUG

GfRotation &
GfRotation::SetQuat(const GfQuatd &quat)
{
    double len = quat.GetImaginary().GetLength();
    if (len > GF_MIN_VECTOR_LENGTH) {
        // Pass through the public API which normalizes axis.
        // Otherwise, it would be possible to create GfRotations using
        // SetQuaternion which cannot be re-created via SetAxisAngle().
        double x = acos(GfClamp(quat.GetReal(), -1.0, 1.0));
        SetAxisAngle(quat.GetImaginary() / len, 2.0 * GfRadiansToDegrees(x));
    }
    else
        SetIdentity();

    return *this;
}

GfRotation &
GfRotation::SetRotateInto(const GfVec3d &rotateFrom, const GfVec3d &rotateTo)
{
    GfVec3d from = rotateFrom.GetNormalized();
    GfVec3d to   = rotateTo.GetNormalized();

    double cos = GfDot(from, to);

    // If vectors are close enough to parallel, use the identity
    // rotation
    if (cos > 0.9999999)
        return SetIdentity();

    // If vectors are opposite, rotate by 180 degrees around an axis
    // vector perpendicular to the original axis.
    if (cos < -0.9999999) {
        // Try cross product with X axis first.  If that's too close
        // to the original axis, use the Y axis
        GfVec3d tmp = GfCross(from, GfVec3d(1.0, 0.0, 0.0));
        if (tmp.GetLength() < 0.00001)
            tmp = GfCross(from, GfVec3d(0.0, 1.0, 0.0));
        return SetAxisAngle(tmp.GetNormalized(), 180.0);
    }

    // Generic case: compute the rotation to bring the vectors
    // together.
    GfVec3d axis = GfCross(rotateFrom, rotateTo).GetNormalized();
    return SetAxisAngle(axis, GfRadiansToDegrees(acos(cos)));
}

GfQuatd
GfRotation::GetQuat() const
{
    double radians = GfDegreesToRadians(_angle) / 2.0;
    double sinR, cosR;
    GfSinCos(radians, &sinR, &cosR);
    GfVec3d axis = _axis * sinR;
    return GfQuatd(cosR, axis).GetNormalized();
}

// helper function for Decompose and DecomposeRotation
static double _GetEpsilon() { 
    return 1e-6;
}

GfVec3d 
GfRotation::Decompose( const GfVec3d &axis0,
                       const GfVec3d &axis1,
                       const GfVec3d &axis2 ) const
{
    GfMatrix4d mat;
    mat.SetRotate( *this );

    // Build the axes tensors
    GfVec3d nAxis0 = axis0.GetNormalized();
    GfVec3d nAxis1 = axis1.GetNormalized();
    GfVec3d nAxis2 = axis2.GetNormalized();

    // Use GF_MIN_ORTHO_TOLERANCE to match OrthogonalizeBasis().
    // XXX Should add GfAreOrthogonal(v0, v1, v2) (which also
    //     GfMatrix4d::HasOrthogonalRows3() could use).
    if (!(GfIsClose( GfDot( nAxis0, nAxis1 ), 0, GF_MIN_ORTHO_TOLERANCE ) &&
          GfIsClose( GfDot( nAxis0, nAxis2 ), 0, GF_MIN_ORTHO_TOLERANCE ) &&
          GfIsClose( GfDot( nAxis1, nAxis2 ), 0, GF_MIN_ORTHO_TOLERANCE )))
    {
        TF_WARN("Rotation axes are not orthogonal.");
    }

    GfMatrix4d axes( nAxis0[0], nAxis1[0], nAxis2[0], 0,
                     nAxis0[1], nAxis1[1], nAxis2[1], 0,
                     nAxis0[2], nAxis1[2], nAxis2[2], 0,
                     0, 0, 0, 1 );

    // get a transformation that takes the given axis into a coordinate
    // frame that has those axis aligned with the x,y,z axis.
    GfMatrix4d m = axes.GetTranspose() * mat * axes;

    // Decompose to the 3 rotations around the major axes.
    // The following code was taken from Graphic Gems 4 p 222. 
    // Euler Angle Conversion by Ken Shoemake.
    int i = 0, j = 1, k = 2;
    double r0, r1, r2;
    double cy = sqrt(m[i][i]*m[i][i] + m[j][i]*m[j][i]);
    if (cy > _GetEpsilon()) {
        r0 = atan2(m[k][j], m[k][k]);
        r1 = atan2(-m[k][i], cy);
        r2 = atan2(m[j][i], m[i][i]);
    } else {
        r0 = atan2(-m[j][k], m[j][j]);
        r1 = atan2(-m[k][i], cy);
        r2 = 0;
    }

    // Check handedness of matrix
    GfVec3d axisCross = GfCross( nAxis0, nAxis1 );
    double axisHand = GfDot(axisCross, nAxis2 );
    if (axisHand >= 0.0) {
        r0 = -r0;
        r1 = -r1;
        r2 = -r2;
    }

    return GfVec3d( GfRadiansToDegrees( r0 ), 
                    GfRadiansToDegrees( r1 ), 
                    GfRadiansToDegrees( r2 ) );
}

// Brought over to ExtMover 
// from //depot/main/tools/src/menv/lib/gpt/util.h [10/16/06] 
//
//  CfgAffineMapd -> GfMatrix4d
//    ::Rotation(org, axis, *theta) ->  SetRotate(GfRotation(axis, *theta)) +  set position
//    ::Apply(CfgAffineMapd)  ->    mx4a.Apply(mx4b)    -> Compose(*mx4a, mx4b) -> mx4b * mx4a.  I think.
//  CfgVectord -> GfVec3d
//    ::Dot -> GfDot
//    ::DualCross -> GfCross  ?
//  CfgPointd -> GfVec3d

GfRotation
GfRotation::RotateOntoProjected(const GfVec3d &v1,
                                const GfVec3d &v2,
                                const GfVec3d &axisParam)
{
    GfVec3d axis = axisParam.GetNormalized();
    
    GfVec3d v1Proj = v1 - GfDot(v1, axis) * axis;
    GfVec3d v2Proj = v2 - GfDot(v2, axis) * axis;
    v1Proj.Normalize();
    v2Proj.Normalize();
    GfVec3d crossAxis = GfCross(v1Proj, v2Proj);
    double sinTheta = GfDot(crossAxis, axis);
    double cosTheta = GfDot(v1Proj, v2Proj);
    double theta = 0;
    if (!(fabs(sinTheta) < _GetEpsilon() && fabs(cosTheta) < _GetEpsilon()))
        theta = atan2(sinTheta, cosTheta);

    const double toDeg = (180.0)/M_PI;
    return GfRotation(axis, theta * toDeg); // GfRotation takes angle in degrees
}

// helper function for DecomposeRotation: Gets the rotation as a matrix and
// returns theta in radians instead of degrees.
static GfMatrix4d
_RotateOntoProjected(const GfVec3d &v1,
                    const GfVec3d &v2,
                    const GfVec3d &axisParam,
                    double *thetaInRadians)
{
    GfMatrix4d mat;
    GfRotation r = GfRotation::RotateOntoProjected(v1, v2, axisParam);
    mat.SetRotate(r);
    if (thetaInRadians) {
        const double toDeg = (180.0)/M_PI;
        *thetaInRadians = r.GetAngle() / toDeg;
    }

    return mat;
}

// helper function for DecomposeRotation
// Given a vector of hint euler angles, alter the desired attempt values such 
// that each is the closest multiple of itself of 2pi to its respective hint.
static GfVec4d _PiShift(
    const GfVec4d &hint, const GfVec4d &attempt, double mul=2*M_PI) 
{
    GfVec4d result(attempt);
    for (int i = 0; i < 4; i++)
    {
        double      mod1 = fmod(attempt[i], mul);
        double      mod2 = fmod(hint[i], mul);
        result[i] = (hint[i]-mod2)+mod1;
        if (fabs(hint[i]-result[i])>mul/2.0) 
            result[i]+=(hint[i]<0?-mul:mul);
    }
    return result;
}

// Another helper function to readjust the first and last angles of a three
// euler anlge solution when the middle angle collapses first and last angles'
// axes onto each other.
static void _ShiftGimbalLock(
    double middleAngle, double *firstAngle, double *lastAngle)
{
    // If the middle angle is PI or -PI, we flipped the axes so use the
    // difference of the two angles.
    if (fabs(fabs(middleAngle) - M_PI) < _GetEpsilon()) {
        double diff = *lastAngle - *firstAngle;
        *lastAngle = diff/2;
        *firstAngle = -diff/2;
    }

    // If the middle angle is 0, then the two axes have the same effect so use
    // the sum of the angles.
    if (fabs(middleAngle) < _GetEpsilon() ) {
        double sum = *lastAngle + *firstAngle;
        *lastAngle = sum/2;
        *firstAngle = sum/2;
    }
}

void 
GfRotation::DecomposeRotation(const GfMatrix4d &rot,
                           const GfVec3d &TwAxis,
                           const GfVec3d &FBAxis,
                           const GfVec3d &LRAxis,
                           double handedness,
                           double *thetaTw,
                           double *thetaFB,
                           double *thetaLR,
                           double *thetaSw,
                           bool    useHint,
                           const double *swShift)
{
    // Enum of which angle is being zeroed out when decomposing the roatation.
    // This is determined by which angle output (if any) is NULL.
    enum _ZeroAngle {
        ZERO_NONE = 0,
        ZERO_TW,
        ZERO_FB,
        ZERO_LR,
        ZERO_SW
    };
    _ZeroAngle zeroAngle = ZERO_NONE;

    double angleStandin = 0.0f, hintTw=0.0f, hintFB=0.0f, hintLR=0.0f,hintSw=0.0f;
    if (thetaTw == NULL) {
        zeroAngle = ZERO_TW;
        thetaTw = &angleStandin;
    }
    if (thetaFB == NULL) {
        if (zeroAngle != ZERO_NONE) {
            TF_CODING_ERROR("Need three angles to correctly decompose rotation");
            return;
        }
        zeroAngle = ZERO_FB;
        thetaFB = &angleStandin;
    }
    if (thetaLR == NULL) {
        if (zeroAngle != ZERO_NONE) {
            TF_CODING_ERROR("Need three angles to correctly decompose rotation");
            return;
        }
        zeroAngle = ZERO_LR;
        thetaLR = &angleStandin;
    }
    if (thetaSw == NULL) {
        if (zeroAngle != ZERO_NONE) {
            TF_CODING_ERROR("Need three angles to correctly decompose rotation");
            return;
        }
        zeroAngle = ZERO_SW;
        thetaSw = &angleStandin;
    }

    if (swShift && zeroAngle != ZERO_NONE) {
        TF_WARN("A swing shift was provided but we're not decomposing into"
                " four angles.  The swing shift will be ignored.");
    }

    // Update hint values if we're using them as hints.
    if (useHint)
    {
        if (thetaTw) hintTw = *thetaTw ;
        if (thetaFB) hintFB = *thetaFB ;
        if (thetaLR) hintLR = *thetaLR ;
        if (thetaSw) hintSw = *thetaSw ;
    }

    // Apply the matrix to the axes.
    GfVec3d FBAxisR = rot.TransformDir(FBAxis);
    GfVec3d TwAxisR = rot.TransformDir(TwAxis);

    // do three rotates about the euler axes, in reverse order, that bring
    // the transformed axes back onto the originals.  The resulting rotation 
    // is the inverse of rot, and the angles are the negatives of the euler 
    // angles.
    GfMatrix4d r(1);

    // The angles used and what order we rotate axes is determined by which
    // angle we're not decomposing into.
    switch (zeroAngle)
    {
    case ZERO_SW:
    case ZERO_NONE:
        r = r * _RotateOntoProjected
            (r.TransformDir(TwAxisR), TwAxis, LRAxis, thetaLR);
        r = r * _RotateOntoProjected
            (r.TransformDir(TwAxisR), TwAxis, FBAxis, thetaFB);
        r = r * _RotateOntoProjected
            (r.TransformDir(FBAxisR), FBAxis, TwAxis, thetaTw);
        // negate the angles
        *thetaFB *= -handedness;
        *thetaLR *= -handedness;
        *thetaTw *= -handedness;

        // Set Sw to swShift if there is a swing shift, otherwise Sw is 
        // zeroed out.
        *thetaSw = swShift ? *swShift : 0.0;
        break;

    case ZERO_TW:
        r = r * _RotateOntoProjected
            (r.TransformDir(FBAxisR), FBAxis, TwAxis, thetaSw);
        r = r * _RotateOntoProjected
            (r.TransformDir(FBAxisR), FBAxis, LRAxis, thetaLR);
        r = r * _RotateOntoProjected
            (r.TransformDir(TwAxisR), TwAxis, FBAxis, thetaFB);
        // negate the angles
        *thetaSw *= -handedness;
        *thetaFB *= -handedness;
        *thetaLR *= -handedness;
        break;

    case ZERO_FB:
        r = r * _RotateOntoProjected
            (r.TransformDir(TwAxisR), FBAxis, TwAxis, thetaSw);
        r = r * _RotateOntoProjected
            (r.TransformDir(TwAxisR), TwAxis, LRAxis, thetaLR);
        r = r * _RotateOntoProjected
            (r.TransformDir(FBAxisR), FBAxis, TwAxis, thetaTw);
        // negate the angles
        *thetaSw *= -handedness;
        *thetaLR *= -handedness;
        *thetaTw *= -handedness;
        break;

    case ZERO_LR:
        r = r * _RotateOntoProjected
            (r.TransformDir(TwAxisR), LRAxis, TwAxis, thetaSw);
        r = r * _RotateOntoProjected
            (r.TransformDir(TwAxisR), TwAxis, FBAxis, thetaFB);
        r = r * _RotateOntoProjected
            (r.TransformDir(FBAxisR), FBAxis, TwAxis, thetaTw);
        // negate the angles
        *thetaSw *= -handedness;
        *thetaFB *= -handedness;
        *thetaTw *= -handedness;
        break;
    };

    // The decomposition isn't unique.  Obviously, adding multiples of
    // 2pi is a no-op, but we've already coerced each angle onto the
    // interval [-pi, pi].  With 3 angles, you can also add an odd
    // multiple of pi to each angle, and negate the middle one.
    //
    // To understand this: Rotating by pi around 1 axis flips the
    // other 2.  To get back where you started, you've got to flip
    // each axis by pi with even parity.  angles are negated if
    // there've been odd flips at the time that their rotation is
    // applied.
    //  
    // Since we've got a 4th axis, we can apply the identity to the
    // 1st three angles, or the last 3, or the 1st 3 then the last 3
    // (or vice versa - they commute.)  That, plus leaving the angles
    // alone, gives us 4 distinct choices.
    //
    // We want to choose the one that minimizes sum of abs of the
    // angles.  We do the miniscule combinatorial optimization
    // exhaustively.

    //  Each angle flipped by pi in the min abs direction.
    double thetaLRp = *thetaLR + ( (*thetaLR > 0)? -M_PI : M_PI);
    double thetaFBp = *thetaFB + ( (*thetaFB > 0)? -M_PI : M_PI);
    double thetaTwp = *thetaTw + ( (*thetaTw > 0)? -M_PI : M_PI);
    double thetaSwp = *thetaSw + ( (*thetaSw > 0)? -M_PI : M_PI);

    // fill up a table with the possible transformations:
    //  0 - do nothing
    //  1 - transform 1st 3
    //  2 - 1 & 3 composed
    //  3 - transform last 3
    GfVec4d vals[4];
    vals[0] = GfVec4d( *thetaTw,  *thetaFB,     *thetaLR,     *thetaSw );
    vals[1] = GfVec4d( thetaTwp,  -thetaFBp,    thetaLRp,     *thetaSw );
    vals[2] = GfVec4d( thetaTwp,  -(*thetaFB),  -(*thetaLR),  thetaSwp );
    vals[3] = GfVec4d( *thetaTw,  thetaFBp,     -thetaLRp,    thetaSwp );

    // All four transforms are valid if we're not forcing any of the angles
    // to zero, but if we are zeroing an angle, then we only have to valid
    // options, the ones that don't flip the zeroed angle by pi.
    int numVals = zeroAngle == ZERO_NONE ? 4 : 2;
    switch (zeroAngle)
    {
    case ZERO_TW:
        vals[1] = vals[3];
        break;
    case ZERO_FB:
    case ZERO_LR:
        vals[1] = vals[2];
        break;
    default:
        break;
    };

    // Store the hint angles in a Tw,FB,LR,Sw ordered array to use for :
    //  1) 2*pi-Shifting
    //  2) calculating sum of absolute differences in order to select 
    //          final angle solution from candidates.
    GfVec4d hintAngles(hintTw, hintFB, hintLR, hintSw);

    // If using hint, then alter our 4 euler angle component values
    // to get them into a per angle mult 2*M_PI that is as close as 
    // possible to the hint angles.
    if (useHint ) {
        for (size_t i=0; i<4;i++) {
            vals[i] =  _PiShift(hintAngles, vals[i]) ;
        }
    }

    // find the min of weighted sum of abs.  The weight on the 2nd
    // angle is to ensure that it stays on [-pi/2, pi/2] If swing
    // isn't wired up, we leave out the 4-axis identity unless the client
    // has passed a swing shift value representing the target sw, with a
    // hint in the sw.
    //
    // if using hint angles, then we sum the differences between the
    // original angle hints and our candidates and select the min
    //
    double min = 0;
    int  i, j, mini = -1;

    for (i = 0; i < numVals; i++) {
        double sum = 0;
        GfVec4d hintDiff = vals[i]-hintAngles;
        for(j = 0;  j < 4; j++)
            sum += fabs(hintDiff[j]);
        if( (i == 0) || (sum < min) ) {
            min = sum;
            mini = i;
        }
    }

    // install the answer.
    *thetaTw = vals[mini][0];
    *thetaFB = vals[mini][1];
    *thetaLR = vals[mini][2];
    *thetaSw = vals[mini][3];

    // Oh, but there's more: Take the example of when we're decomposing
    // into tw, fb, and lr. When the middle angle, (fb) is PI/2, then
    // only (tw - lr) is significant, and at fb = -PI/2, only tw+lr is
    // significant, i.e. adding the same constant to both angles is an
    // identity.  Once again, we apply the min sum of abs rule.  This
    // happens because the PI/2 rotation collapses axis 1 onto axis 3.
    // That's what gimbal lock is.
    // 
    // This applies no matter which three angles we're decomposing into
    // except that in the case where we're solving tw, fb, sw or tw, lr, sw
    // we get this gimbal lock situation when the respective fb or lr are 
    // 0, PI, and -PI.  We can account for all these cases in the same 
    // function by shift fb and lr by PI/2 or -PI/2 when they are the middle 
    // angles.  Whether the shift is PI/2 or -PI/2 is dependent on the 
    // handedness of the basis matrix of the three axes as it flips the 
    // direction needed to the get the positive Tw or FB axis to align with 
    // the positive LR or Sw axis.
    GfMatrix3d basis;
    basis.SetRow(0, TwAxis);
    basis.SetRow(1, FBAxis);
    basis.SetRow(2, LRAxis);
    switch (zeroAngle)
    {
    case ZERO_NONE:
    case ZERO_SW:
        _ShiftGimbalLock(*thetaFB + M_PI/2 * basis.GetHandedness(), thetaTw, thetaLR);
        break;
    case ZERO_TW:
        _ShiftGimbalLock(*thetaLR + M_PI/2 * basis.GetHandedness(), thetaFB, thetaSw);
        break;
    case ZERO_FB:
        _ShiftGimbalLock(*thetaLR, thetaTw, thetaSw);
        break;
    case ZERO_LR:
        _ShiftGimbalLock(*thetaFB, thetaTw, thetaSw);
        break;
    };
}


#if 0
// XXX: I ported this code over to presto, but it is not
// yet being used. 

void 
GfRotation::ComposeRotation(double         tw,
                         double         fb,
                         double         lr,
                         double         sw,
                         GfMatrix4d     *rot,
                         GfVec3d        *TwAxis,
                         GfVec3d        *FBAxis,
                         GfVec3d        *LRAxis)
{
    GfVec3d             twAxis(0,0,1),
                        fbAxis(1,0,0),
                        lrAxis(0,1,0);

    vector<GfMatrix4d> matVec;
    matVec.resize(4);
    matVec[0].SetRotate(GfRotation(twAxis,tw));
    matVec[1].SetRotate(GfRotation(fbAxis,fb));
    matVec[2].SetRotate(GfRotation(lrAxis,lr));
    matVec[3].SetRotate(GfRotation(twAxis,sw));

    GfMatrix4d mat(1) ;
    for (size_t i=0; i< 4; i++) mat*=matVec[i];

    *rot = mat;
    *TwAxis = twAxis ;
    *FBAxis = fbAxis ;
    *LRAxis = lrAxis ;
}

#endif

GfVec3f
GfRotation::TransformDir( const GfVec3f &vec ) const
{
    return GfMatrix4d().SetRotate( *this ).TransformDir( vec );
}

GfVec3d
GfRotation::TransformDir( const GfVec3d &vec ) const
{
    return GfMatrix4d().SetRotate( *this ).TransformDir( vec );
}

GfRotation &
GfRotation::operator *=(const GfRotation &r)
{
    // Express both rotations as quaternions and multiply them
    GfQuaternion q = (r.GetQuaternion() * GetQuaternion()).GetNormalized();

    // We don't want to just call
    //          SetQuaternion(q);
    // here, because that could change the axis if the angle is a
    // multiple of 360 degrees. Duplicate the math here, preferring
    // the current axis for an identity rotation:
    double len = q.GetImaginary().GetLength();
    if (len > GF_MIN_VECTOR_LENGTH) {
        _axis  = q.GetImaginary() / len;
        _angle = 2.0 * GfRadiansToDegrees(acos(q.GetReal()));
    }
    else {
        // Leave the axis as is; just set the angle to 0.
        _angle = 0.0;
    }

    return *this;
}

std::ostream &
operator<<(std::ostream& out, const GfRotation& r)
{
    return out << '[' << Gf_OstreamHelperP(r.GetAxis()) << " " 
        << Gf_OstreamHelperP(r.GetAngle()) << ']';
}
