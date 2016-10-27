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
#include "pxr/base/gf/colorRamp.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/rgb.h"
#include "pxr/base/tf/type.h"

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfColorRamp>();
}

/*
 * A ramp from (X0,Y0) to (X1,Y1), with slope D0 at X0 and D1 at Y1.
 * The ramp is constructed piecewise from a quadratic "shoulder" segment,
 * then a linear segment, then a quadratic "shoulder" segment; the widths
 * of the shoulders are given by w0 and w1.
 */
static double
_Stramp(double X, double X0, double X1, double Y0, double Y1, double D0,
	double D1, double w0, double w1)
{
    double DX, DY, x, d0, d1, wnorm, y;

    /*
     * change variables to x in [0,1] and y in [0,1]
     */
    DY = Y1-Y0;
    DX = X1-X0;
    if (DY == 0 || DX==0) return Y0;
    x = (X-X0)/DX;
    d0 = D0*DX/DY;
    d1 = D1*DX/DY;

    /*
     * make sure shoulder widths don't sum to more than 1
     */
    wnorm = 1./GfMax<double>(1.,w0+w1);
    w0 *= wnorm;
    w1 *= wnorm;

    /* compute y */
    if (x <= 0.)
        y =  0.;
    else if (x >= 1.)
	y = 1.;
    else {
        double xr = 2.-w0-w1;
        double a = (2. - w1*d1 + (w1-2.)*d0)/(2.*xr);
        double b = (2. - w0*d0 + (w0-2.)*d1)/(2.*xr);
        if (x < w0)
            y = a*x*x/w0 + d0*x;
        else if (x > 1.-w1) {
            double omx = 1.-x;
            y = 1. - b*omx*omx/w1 - d1*omx;
        }
        else {
            double ya = a*w0 + d0*w0;
            double da = 2.*a + d0;
            y = ya + (x-w0)*da;
        }
    }

    /*
     * map y back to Y and return.  Note: analytically y is always in
     * [0,1], but numerically it might have noise so clamp it.
     */
    return GfClamp(y,0.,1.)*DY + Y0;
}

static GfRGB
_ShapeColorInterpExperiment(
    const GfRGB &C0, const GfRGB &C1, const GfRGB &Cmid,
    double slide, double width0, double width1, double /* widthMid */,
    double widthMid0, double widthMid1, double alpha)
{
    GfRGB Cret;

    for (int i=0; i<3; i++) {
        double c0 = C0[i];
        double c1 = C1[i];
        double cMid = Cmid[i];
        double cmin = GfMin(c0,c1);
        double cmax = GfMax(c0,c1);
        double slope;

        /*
         * determine slope at center point.
         */
        if (cMid <= cmin || cMid >= cmax) {
            /*
             * if the center is outside the min/max, we want it to be the
             * extremum, its slope should be 0.
             */
            slope = 0;
        }
        else {
            /* compute a desired slope by averaging the normalized
             * tangents, considering each segment to be linear.
             * (should this be a weighted average?  something else?)
             */
	    
            double tan0y = cMid-c0;
            double tan0x = slide;
            double tan1y = c1-cMid;
            double tan1x = 1.-slide;

            double len0 = hypot(tan0x,tan0y);
            double len1 = hypot(tan1x,tan1y);

	    double cseg, scale;

            slope = (len1*tan0y+len0*tan1y)/(len1*tan0x+len0*tan1x);
            
            /* that desired slope is OK if the center value is actually on the
             * segment from min to max.  but as the value approach either of
             * those bounds, we want the slope to be 0.  so we scale that slope
             * linearly with distance from the hypothetical intercept.
             */
            cseg = GfLerp(slide,c0,c1);
            scale = cMid < cseg ? (cMid-cmin)/(cseg-cmin)
                                      : (cMid-cmax)/(cseg-cmax);
            slope *= scale;
        }

        /*
         * now do a smoothramp on each side of the center, matching the
         * target slope.
         */
        if (alpha < slide) {
            Cret[i] = _Stramp(alpha,
                              0.,slide,
                              c0,cMid,
                              0.,slope,
                              width0,widthMid0);
        }
        else {
            Cret[i] = _Stramp(alpha,
                              slide,1.,
                              cMid,c1,
                              slope,0.,
                              widthMid1,width1);
        }
    }

    return Cret;
}

GfRGB
GfColorRamp::Eval(const double x) const
{
    return _ShapeColorInterpExperiment( _cMax, _cMin, _cMid,
                                        _midPos, _widthMax, _widthMin, 0 /* unused */,
                                        _widthMidOut, _widthMidIn, x );
}
