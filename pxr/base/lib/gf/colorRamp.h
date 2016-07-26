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
#ifndef GF_COLORRAMP_H
#define GF_COLORRAMP_H

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/rgb.h"
#include "pxr/base/tf/tf.h"
#include <stdio.h>

//!
// \file colorRamp.h
// \ingroup group_gf_Color
//

//!
// \class GfColorRamp colorRamp.h luxo/iwq/colorRamp.h
// \ingroup group_gf_Color
//
// \brief A color ramp, as commonly used in Pixar shaders.
//
// \see
// \c /usr/anim/bin/cramp \n
// \c /usr/anim/global/src/shaders/include/stdshad/Math00a.h: Math00aSmoothTriad
class GfColorRamp
{
public:
    //  \brief Constructor.
    // The default color ramp is a red->green->blue gradient.
    GfColorRamp(
	const GfRGB& cMin = GfRGB(1,0,0),
	const GfRGB& cMid = GfRGB(0,1,0),
	const GfRGB& cMax = GfRGB(0,0,1),
	float midPos = 0.5,
	float widthMin = 0.3,
	float widthMidIn = 0.3,
	float widthMidOut = 0.3,
	float widthMax = 0.5,
        bool useColorRamp = true,
        bool switchable = false
    ) :
        _useColorRamp(useColorRamp),
        _switchable(switchable),
	_cMin(cMin),
	_cMid(cMid),
	_cMax(cMax),
	_midPos(midPos),
	_widthMin(widthMin),
	_widthMidIn(widthMidIn),
	_widthMidOut(widthMidOut),
	_widthMax(widthMax)
    {
    } 

    //  \brief Constructor.
    // The default color ramp is a red->green->blue gradient.
    GfColorRamp(
        bool useColorRamp,
	const GfRGB& cMin = GfRGB(1,0,0),
	const GfRGB& cMid = GfRGB(0,1,0),
	const GfRGB& cMax = GfRGB(0,0,1),
	float midPos = 0.5,
	float widthMin = 0.3,
	float widthMidIn = 0.3,
	float widthMidOut = 0.3,
	float widthMax = 0.5,
        bool switchable = false
    ) :
        _useColorRamp(useColorRamp),
        _switchable(switchable),
	_cMin(cMin),
	_cMid(cMid),
	_cMax(cMax),
	_midPos(midPos),
	_widthMin(widthMin),
	_widthMidIn(widthMidIn),
	_widthMidOut(widthMidOut),
	_widthMax(widthMax)
    {
    } 

    //! \brief
    // Evaluate the ramp at the given value.  x is in [0..1].
    GfRGB Eval(const double x) const;

    bool operator ==(const GfColorRamp &ramp) const {
        return ((_useColorRamp == ramp._useColorRamp) &&
                (_cMin == ramp._cMin) &&
                (_cMid == ramp._cMid) &&
                (_cMax == ramp._cMax) &&
                (_midPos == ramp._midPos) &&
                (_widthMin == ramp._widthMin) &&
                (_widthMidIn == ramp._widthMidIn) &&
                (_widthMidOut == ramp._widthMidOut) &&
                (_widthMax == ramp._widthMax));
    }

    bool operator !=(const GfColorRamp &ramp) const {
        return !(*this == ramp);
    }

    bool GetUseColorRamp()      const   {return !_switchable || _useColorRamp;}

    const GfRGB &GetCMin()	const   { return _cMin; }
    const GfRGB &GetCMid()	const   { return _cMid; }
    const GfRGB &GetCMax()	const   { return _cMax; }

    double GetMidPos()		const   { return _midPos; }
    double GetWidthMin()	const   { return _widthMin; }
    double GetWidthMidIn()	const   { return _widthMidIn; }
    double GetWidthMidOut()	const   { return _widthMidOut; }
    double GetWidthMax()	const   { return _widthMax; }

    bool GetSwitchable()        const   { return _switchable; }

    void SetUseColorRamp(bool b)        { _useColorRamp = b; }

    void SetCMin(const GfRGB &c) 	{ _cMin = c; }
    void SetCMid(const GfRGB &c) 	{ _cMid = c; }
    void SetCMax(const GfRGB &c) 	{ _cMax = c; }

    void SetMidPos(double val)	 	{ _midPos = val; }
    void SetWidthMin(double val)	{ _widthMin = val; }
    void SetWidthMidIn(double val)  	{ _widthMidIn = val; }
    void SetWidthMidOut(double val) 	{ _widthMidOut = val; }
    void SetWidthMax(double val)    	{ _widthMax = val; }

    void SetSwitchable(bool b)          { _switchable = b; }

private:
    bool _useColorRamp, _switchable;
    GfRGB  _cMin, _cMid, _cMax;
    double _midPos, _widthMin, _widthMidIn, _widthMidOut, _widthMax;
};


#endif
