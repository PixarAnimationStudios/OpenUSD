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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_GLOBALS_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_GLOBALS_H

#include "pxr/pxr.h"

#ifdef OSMac_
#include <math.h>
#else
#include <cmath>
#endif
#include <wchar.h>
// TO DO: fix this to be driven by #defines.
#include "string.h"

PXR_NAMESPACE_OPEN_SCOPE

/**********************************************************************
 *
 *  BASIC TYPES
 *
 **********************************************************************/

#define LENCALL wcslen
typedef wchar_t CHARTYPE;

typedef float   NUMBER;  // a raw, scalar number.


/**********************************************************************
 *
 *  SUBSTRING RANGE REFERENCES
 *
 **********************************************************************/

/// \class CommonParserStRange
///
/// Chances are that parsing is happening inside of a buffer somewhere.
/// Rather than slicing and dicing new buffers, it's often practical to
/// just point to a substring within that buffer with pointer+length
/// semantics, which is what this class does.
/// 
/// NOTE: These objects are not assumed to own the referenced string.
/// That ownership must be managed separately (and the lifetime of that
/// string needs to exceed the lifetime of this reference to it.)
/// This is not usually a problem for string literals (lifetime==application
/// address space) or the buffer being worked on (lifetime >= parse.)
/// 
/// Furthermore, these objects make no assumptions about encoding, so
/// users of these types do need to concern themselves with "complex"
/// characters (such as multi-octet UTF-8 sequences, or UTF-16 Surrogate 
/// Pairs) making sure that character is fully contained within the
/// string range, and doesn't straddle the start or end boundary.
/// 
/// Finally, while the CommonParserStRange doesn't explicitly codify or enforce it,
/// the ATOM interface assumes all strings, regardless of encoding, represent
/// UNICODE codepoints.  It becomes the parser's responsibility to convert
/// from another character set into Unicode, and it's the sink's responsibility
/// to convert to another codepoint.
/// 
class CommonParserStRange
{
public:
    /// Default constructor: a null string.
    CommonParserStRange()
        : _pszStart(0)
        , _iLength(0)
    {
    }

    /// Copy constructor
    CommonParserStRange(const CommonParserStRange& oOther)
        : _pszStart(oOther._pszStart)
        , _iLength(oOther._iLength)
    {
    }

    /// Constructor from a string start pointer.
    CommonParserStRange(const CHARTYPE* pszLiteral)
        : _pszStart(pszLiteral)
        , _iLength(0)
    {
        if(pszLiteral)
            _iLength = (int)LENCALL(pszLiteral);
    }

    /// Constructor from a string start pointer and a length.
    CommonParserStRange(const CHARTYPE* pszOther,
            int iLen)
        : _pszStart(pszOther)
        , _iLength(iLen >= 0? iLen : 0)
    {
    }

    /// Constructor from a string start pointer and end pointer.
    CommonParserStRange(const CHARTYPE* pszStart,
            const CHARTYPE* pszEnd)
        : _pszStart(pszStart)
        , _iLength(pszEnd >= pszStart? (int)(pszEnd - pszStart + 1) : 0)
    {
    }

    ///
    ///  ACCESSORS
    ///

    /// Access the start of the string range
    const CHARTYPE* Start() const
    {
        return _pszStart;
    }

    /// Access the end of the string range (the last character in the range indicated.)
    /// Only valid if the string isn't empty.
    /// 
    /// Note: End is a completely synthetic characteristic of the CommonParserStRange; it's
    /// just a manifestation of start+len, hiding the invariable +1/-1isms.
    const CHARTYPE* End() const
    {
        if(_iLength > 0)
            return _pszStart + _iLength - 1;
        else
            return NULL;
    }

    /// Access the length of the string
    int Length() const
    {
        return _iLength;
    }

    /// Access characters within the range.
    /// From this perspective, anything out of bounds is non-existant: char 0 is returned.
    CHARTYPE operator[] (int iIndex) const
    {
        if(iIndex >=0 && iIndex < _iLength)
            return _pszStart[iIndex];
        else
            return 0;
    }

    /// The nth character beyond the end of the CommonParserStRange.
    const CHARTYPE* Beyond(int i = 0) const
    {
        return _pszStart + _iLength + i;
    }

    /// Last() - returns the Last character
    /// Last(2) - returns the second-to-last character, etc.
    /// Like reverse [] notation, except 1-based.
    CHARTYPE Last(int iIndex = 1) const
    {
        return operator[](Length()-iIndex);
    }

    /// Takes a "part" (ie, substring) of the indicated string range.
    CommonParserStRange Part(
        int iStart,
        int iLen) const
    {
        if(iStart < 0 || iStart >= _iLength)
            return CommonParserStRange();

        if(iStart + iLen > _iLength)
            iLen = _iLength - iStart;

        return CommonParserStRange(_pszStart+iStart,iLen);
    }

    /// Takes a "part" (ie substring) to end
    CommonParserStRange Part(int iStart) const
    {
        if(iStart < 0 || iStart >= _iLength)
            return CommonParserStRange();

        return CommonParserStRange(_pszStart+iStart,_iLength-iStart);
    }

    /// Finds the first occurance of chFind in the range.
    /// Returns a pointer to that character, or NULL if not found.
    const CHARTYPE* Find(const CHARTYPE chFind) const
    {
        for(int i=0;i<_iLength;i++) {
            if(_pszStart[i] == chFind)
                return _pszStart+i;
        }

        return NULL;
    }

    // Finds the first occurance of sFind in the range.
    // Returns a pointer to the start of that string, or NULL if not found.
    const CHARTYPE* Find(const CommonParserStRange sFind) const
    {
        int iMax = _iLength - sFind.Length();
        if(iMax >= 0) {
            CommonParserStRange sPart = Part(0,sFind.Length());
            for(int i=0; i<= iMax; i++, sPart.Move(1)) {
                if(sPart == sFind)
                    return sPart.Start();
            }
        }
        return NULL;
    }

    ///
    /// MUTATORS
    ///

    /// Resets the string to an "uninitialized" state (ie, not pointing to any buffer.)
    void Reset()
    {
        _pszStart = 0;
        _iLength = 0;
    }

    /// General-purpose mutator: sets the start and length.
    void Set(const CHARTYPE* pszStart,
             int iLen)
    {
        _pszStart = pszStart;
        _iLength  = iLen >= 0? iLen : 0;
    }

    /// General-purpose mutator: sets the start and end pointer.
    void Set(const CHARTYPE* pszStart,
             const CHARTYPE* pszEnd)
    {
        _pszStart = pszStart;
        _iLength  = pszEnd >= pszStart? (int)(pszEnd - pszStart + 1) : 0; 
    }

    /// Sets the start; length remains unchanged
    void SetStart(const CHARTYPE* pszStart)
    {
        _pszStart = pszStart;
    }

    /// Sets the length based on the intended "end" character.  "Backwards" strings
    /// are not allowed.
    void SetEnd(const CHARTYPE* pszEnd)
    {
        if(pszEnd >= _pszStart)
            _iLength = (int)(pszEnd - _pszStart + 1);
        else
            _iLength = 0;
    }

    /// Sets the start to a new string buffer, same as Set(pszStart,LENCALL(pszStart))
    void SetString(const CHARTYPE* pszStart)
    {
        _pszStart = pszStart;
        _iLength = (pszStart)?(int)(LENCALL(pszStart)) : 0;
    }

    /// Moves the entire string range iChars.
    void Move(int iChars)
    {
        _pszStart += iChars;
    }

    /// Moves only the start, end remains set.
    void MoveStart(int iChars)
    {
        _pszStart += iChars;
        _iLength  -= iChars;
        if(_iLength < 0)
            _iLength = 0;
    }

    /// Sets only the length of the range.
    void SetLength(int iLen)
    {
        _iLength = iLen >= 0? iLen : 0;
    }

    /// Adds or removes from the length of the range.
    void AddLength(int iLen)
    {
        _iLength = (_iLength + iLen >= 0)? _iLength + iLen : 0;
    }


    ///  Assignment/copy operator.
    CommonParserStRange& operator= (const CommonParserStRange& oOther)
    {
        this->_pszStart = oOther._pszStart;
        this->_iLength  = oOther._iLength;
        return *this;
    }

    /// Equality operator.  Note that, with the CommonParserStRange(const CHARTYPE* pszLiteral), 
    /// this also allows operations against string literals, so myStRange == L"Foo" works.
    bool operator== (const CommonParserStRange& oOther) const
    {
        if(this->_pszStart == oOther._pszStart
            && this->_iLength == oOther._iLength)
            return true;
        else if(this->_iLength != oOther._iLength)
            return false;
        else {
            for(int i=0;i<this->_iLength;i++) {
                if(this->_pszStart[i] != oOther._pszStart[i])
                    return false;
            }
            return true;
        }
    }

    /// Syntactic sugar: avoids (!(str == other))
    bool operator!= (const CommonParserStRange& oOther) const
    {
        return !(operator==)(oOther);
    }

    ///
    /// Parsing Utilities
    ///

    /// Split a string along a character separator.
    /// Returns a range pointing to the String Range occurring before the separator, and moves
    /// the start of this string past the separator.
    CommonParserStRange Split(const CHARTYPE chSep)
    {
        CommonParserStRange sRet;
        const CHARTYPE* p = Find(chSep);

        if(p != NULL) {
            sRet.Set(_pszStart,p-1);
            MoveStart(sRet.Length()+1);
        }
        else {
            sRet = *this;
            MoveStart(sRet.Length());
        }
        return sRet;
    }

    /// Split a string along a string range separator.
    /// Returns a range pointing to the String Range occurring before the separator, and moves
    /// the start of this string past the separator.
    CommonParserStRange Split(const CommonParserStRange sSep)
    {
        CommonParserStRange sRet;
        const CHARTYPE* p = Find(sSep);

        if(p != NULL) {
            sRet.Set(_pszStart,p-1);
            MoveStart(sRet.Length()+sSep.Length());
        }
        else {
            sRet = *this;
            MoveStart(sRet.Length());
        }
        return sRet;
    }


private:
    const wchar_t* _pszStart;
    int            _iLength;
};


/**********************************************************************
 *
 *  SIMPLE MATRIX CLASS
 *
 **********************************************************************/

/// This is a 3x3 matrix
#define e_00 0
#define e_01 1
#define e_02 2
#define e_10 3
#define e_11 4
#define e_12 5
#define e_20 6
#define e_21 7
#define e_22 8

/// \class CommonParserMatrix
///
/// Essentially just wraps a 9-element array of NUMBERS; provides basic operator access.  
/// Note: CommonParserMatrix never "owns" the array it refers to.
/// 
class CommonParserMatrix
{
public:
    /// Standard constructor: gimme a 9-element array, and I'll give you a matrix.
    CommonParserMatrix(NUMBER* pElements)
        : _pElements(pElements)
    {
    }

    /// Allows derivations to take ownership of their own arrays, if desired.
    virtual ~CommonParserMatrix() {};

    /// Treat the CommonParserMatrix as an array of numbers.
    operator NUMBER* () const
    {
        return _pElements;
    }

    /// Copy/assignment operator.
    CommonParserMatrix& operator= (const CommonParserMatrix& oOther)
    {
        for(int i=0;i<9;i++)
            this->_pElements[i] = oOther._pElements[i];

        return *this;
    }

    /// Assignment operator, assign from an array.
    CommonParserMatrix& operator= (const NUMBER* m)
    {
        for(int i=0;i<9;i++)
            this->_pElements[i] = m[i];

        return *this;
    }

    /// Standard matrix multiply: M = M * N. The right-hand-side is an array of numbers.
    void operator*= (const NUMBER* o);

    /// Standard matrix multiply: M = M * N. The right-hand-side is another matrix.
    void operator*= (const CommonParserMatrix& oOther)
    {
        // Borrow the array overload
        (this->operator*=)((NUMBER*)(oOther));
    }

    /// Access a row, as an array. Allows myMatrix[iRow][iCol] access.
    NUMBER* operator[] (int iRow) const
    {
        return _pElements + iRow*3;
    }

    /// Populates the matrix with all zeros.
    void SetZero();

    /// Populates the matrix with the identity matrix. (1's along row==col axis, 0's elsewhere.)
    void SetIdentity();

private:
    NUMBER* _pElements;
};



/**********************************************************************
 *
 *  MEASURES
 *
 **********************************************************************/

/// \enum CommonParserMeasureUnit
///
/// The unit of measure.
/// 
enum CommonParserMeasureUnit {
    /// Unitless; just a number
    CommonParserMeasureUnitUnitless, 
    /// Units of the surrounding model space
    CommonParserMeasureUnitModel,    
    /// Physical device units
    CommonParserMeasureUnitPixels,   
    /// 1/72 of an inch; Twips = Points * 20, Pica = 12 Points
    /// qv http://en.wikipedia.org/wiki/Point_%28typography%29
    CommonParserMeasureUnitPoints,   
    /// an "em" (1.0 times height of current font.) qv http ://en.wikipedia.org/wiki/Em_%28typography%29
    CommonParserMeasureUnitEm,       
    /// The x-height (height of lower-case letter)  qv http://en.wikipedia.org/wiki/X-height
    CommonParserMeasureUnitEx,       
    /// Scale, 1 = normal (100%), 2 = twice normal, etc.  
    CommonParserMeasureUnitProportion,
};

/// \class CommonParserMeasure
///
/// // A CommonParserMeasure is basically a numeric quantity and a unit measure.
/// There's no one canonical coordinate space amongst the markup grammars,
/// so this type endeavors to describe more than just the numerical quantity
/// associated with a measure.  The Sink is in a better position to reconcile
/// model-space coordinates with device-space coordinates, or do proportional
/// computations.
///
/// To put it another way: is a font size of "12" really twelve points, twelve 
/// pixels, twelve twips, or ...?
///
/// There's also an option to point to the original string representation in
/// the markup.
class CommonParserMeasure
{
public:
    /// Default constructor: a unitless zero.
    CommonParserMeasure() = default;

    /// Standard constructor: number, unit, and the source reference (which can be NULL)
    CommonParserMeasure(
        NUMBER nNum, 
        CommonParserMeasureUnit eUnit, 
        const CommonParserStRange* pRef)
        : _nNumber(nNum)
        , _eUnits(eUnit)
    {
       if(pRef)
            _oRef = *pRef;
    }

    /// Access the numeric part of the measure.
    NUMBER Number() const
    { return _nNumber; }

    /// Access the unit type of the measure.
    CommonParserMeasureUnit Units() const
    { return _eUnits; }

    /// The parser's string reference
    const CommonParserStRange& Reference()  const
    { return _oRef; }

    /// The assignment operator.
    CommonParserMeasure& operator= (const CommonParserMeasure& oOther)
    {
        _nNumber = oOther._nNumber;
        _eUnits  = oOther._eUnits;

        return *this;
    }

    /// The equal operator.
    bool operator== (const CommonParserMeasure& oOther) const
    {
        return _nNumber == oOther._nNumber 
            && _eUnits  == oOther._eUnits;  // _oRef isn't comparable.
    }

private:
    CommonParserStRange  _oRef;
    NUMBER   _nNumber = 0;
    CommonParserMeasureUnit _eUnits = CommonParserMeasureUnitUnitless;
};


/**********************************************************************
 *
 *  RADIAL MEASURES
 *
 **********************************************************************/
// This encapsulates the computation from "human" units (degrees, gon)
// to the more typical units employed in computation (radians.)  The design
// is to keep the "type" of units in an angular measure in the developer's
// mind, without having to remember the math.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif//M_PI

/// \class CommonParserRadialMeasure
///
/// These three light-weight classes (no virtual methods, no complex operations) are defined 
/// inline for best performance.
/// The ATOM interface is defined in terms of this CommonParserRadialMeasure, but two derivations 
/// can be used that mask the conversion to/from Degrees or Gon (aka Grad, Gradient, ...)
/// 
class CommonParserRadialMeasure
{
public:
    /// The constructor from a radian value.
    CommonParserRadialMeasure(NUMBER nRadians)
        : _nRadians(nRadians)
    {}

    /// The copy constructor.
    CommonParserRadialMeasure(const CommonParserRadialMeasure& oOther)
        : _nRadians(oOther._nRadians)
    {
    }

    /// Access the radians value.
    NUMBER Radians() const
    {
        return _nRadians;
    }

    /// The assignment operator.
    CommonParserRadialMeasure& operator= (const CommonParserRadialMeasure& oOther)
    {
        _nRadians = oOther._nRadians;
        return *this;
    }

    /// The equal operator.
    bool operator==(const CommonParserRadialMeasure& o) const
    { 
        return _nRadians == o._nRadians;
    }
    // For other relops, use Radians() or deriv::Angle().

protected:    
    NUMBER _nRadians;
};

/// \class CommonParserRadialMeasure
///
/// If you prefer to deal with degrees, you can provide one of these in lieu of a plain ol' radial
/// measure.
/// usage:
///  SomethingTakingRadialMeasure(CommonParserDegreeRadialMeasure(45));
/// and:
///  CommonParserDegreeRadialMeasure deg = SomethingReturningRadialMeasure();
///  NUMBER degrees = deg.Angle()
///
class CommonParserDegreeRadialMeasure: public CommonParserRadialMeasure
{
public:
    /// The constructor from a degree value.
    CommonParserDegreeRadialMeasure(NUMBER nDegrees)
    : CommonParserRadialMeasure(ToRadians(nDegrees))
    {}

    /// The copy constructor.
    CommonParserDegreeRadialMeasure(const CommonParserRadialMeasure& oOther)
    : CommonParserRadialMeasure(oOther)
    {
    }

    /// The assignment operator.
    CommonParserDegreeRadialMeasure& operator= (NUMBER nDegrees)
    {
        _nRadians = ToRadians(nDegrees);
		return *this;
    }

    /// The equal operator.
    bool operator == (NUMBER nDegrees)
    {
        return _nRadians == ToRadians(nDegrees);
    }

    /// Convert to an angle.
    operator NUMBER ()
    {
        return Angle();
    }

    /// Access the angle value.
    NUMBER Angle()
    {
        return (NUMBER)(_nRadians * 180 / M_PI);
    }

private:
    NUMBER ToRadians(NUMBER nDegrees)
    {
        return (NUMBER)(nDegrees * M_PI / 180.0);
    }
};

/// \class CommonParserRadialMeasure
///
/// Fundamentally the same as CommonParserDegreeRadialMeasure, but based on the Gon (1/400th of 
/// a circle) as opposed to Degree (1/360th of a circle.)
/// 
class CommonParserGonRadialMeasure: public CommonParserRadialMeasure
{
    CommonParserGonRadialMeasure(NUMBER nDegrees)
    : CommonParserRadialMeasure(ToRadians(nDegrees))
    {}

    /// The copy constructor.
    CommonParserGonRadialMeasure(const CommonParserRadialMeasure& oOther)
    : CommonParserRadialMeasure(oOther)
    {
    }

    /// Convert to an angle.
    operator NUMBER ()
    {
        return Angle();
    }

    /// Access the angle value.
    NUMBER Angle()
    {
        return (NUMBER)(_nRadians * 200 / M_PI);
    }

    /// Access the radians value.
    NUMBER ToRadians(NUMBER nDegrees)
    {
        return (NUMBER)(nDegrees * M_PI / 200.0);
    }
};

/**********************************************************************
 *
 *  COLOR
 *
 **********************************************************************/

#define ATOM_COLOR_B_BITS 0x000000FF
#define ATOM_COLOR_G_BITS 0x0000FF00
#define ATOM_COLOR_R_BITS 0x00FF0000
#define ATOM_COLOR_A_BITS 0xFF000000

#define ATOM_COLOR_B_SHIFT  0
#define ATOM_COLOR_G_SHIFT  8
#define ATOM_COLOR_R_SHIFT 16
#define ATOM_COLOR_A_SHIFT 24

/// \class CommonParserRadialMeasure
///
/// The representation of color.
/// 
class CommonParserColor
{
public:
    /// The default constructor.
    CommonParserColor()
        : _argb(0)
    {}

    /// The constructor from four channels.
    CommonParserColor(int r,
                      int g, 
                      int b, 
                      int a=0xff)
        : _argb(_ToLongARGB(r,g,b,a))
    {
    }

    /// The constructor from a full color value.
    CommonParserColor(long argb)
        : _argb(argb)
    {
    }

    /// If the color is valid.
    bool IsNullColor() { return _argb == 0; }

    /// Access the red channel.
    int R() const { return (_argb & ATOM_COLOR_R_BITS) >> ATOM_COLOR_R_SHIFT; }

    /// Access the green channel.
    int G() const { return (_argb & ATOM_COLOR_G_BITS) >> ATOM_COLOR_G_SHIFT; }

    /// Access the blue channel.
    int B() const { return (_argb & ATOM_COLOR_B_BITS) >> ATOM_COLOR_B_SHIFT; }

    /// Access the alpha channel.
    int A() const { return (_argb & ATOM_COLOR_A_BITS) >> ATOM_COLOR_A_SHIFT; }

    /// Access the full color value.
    long LongARGB() const
    {
        return _argb;
    }

    /// Access the full color value as ABGR format.
    long LongABGR() const
    {
        return _ToLongARGB(B(),G(),R(),A());
    }

    /// Set the alpha channel.
    void SetR(int r)
    {
        _argb &= ~ATOM_COLOR_R_BITS;
        _argb |= (r << ATOM_COLOR_R_SHIFT) & ATOM_COLOR_R_BITS;
    }

    /// Set the alpha channel.
    void SetG(int g)
    {
        _argb &= ~ATOM_COLOR_G_BITS;
        _argb |= (g << ATOM_COLOR_G_SHIFT) & ATOM_COLOR_G_BITS;
    }

    /// Set the alpha channel.
    void SetB(int b)
    {
        _argb &= ~ATOM_COLOR_B_BITS;
        _argb |= (b << ATOM_COLOR_B_SHIFT) & ATOM_COLOR_B_BITS;
    }

    /// Set the alpha channel.
    void SetA(int a)
    {
        _argb &= ~ATOM_COLOR_A_BITS;
        _argb |= (a << ATOM_COLOR_A_SHIFT) & ATOM_COLOR_A_BITS;
    }

    /// Set the value to null.
    void SetNull()
    {
        _argb = 0;
    }

    /// Set the full value of the color.
    void SetLongARGB(long argb)
    {
        _argb = argb;
    }

    /// Set the full value of the color in ABGR format.
    void SetLongABGR(long abgr)
    {
        // This basically swaps blue and red, which are occupying each others' bits.

        // Preserve the components, moving them to their intended bit positions.
        long lBlue = ((abgr & ATOM_COLOR_R_BITS) >> ATOM_COLOR_R_SHIFT) << ATOM_COLOR_B_SHIFT;
        long lRed  = ((abgr & ATOM_COLOR_B_BITS) >> ATOM_COLOR_B_SHIFT) << ATOM_COLOR_R_SHIFT;
        // Keep what's already in place: Alpha and Green.
        abgr &= ATOM_COLOR_A_BITS | ATOM_COLOR_G_BITS;
        // Reincorporate what's been swapped around.
        abgr |= lBlue | lRed;

        SetLongARGB(abgr);
    }

    /// Set the channels in RGBA order.
    void Set(int r, 
             int g, 
             int b, 
             int a = 0xff)
    {
        SetLongARGB(_ToLongARGB(r,g,b,a));
    }

    /// Set the channels in BGRA order.
    void SetBGRA(int b, 
                 int g, 
                 int r, 
                 int a = 0xff)
    {
        SetLongARGB(_ToLongARGB(b,g,r,a)); // just swap the Red and Blue.
    }

    /// The assignment operator.
    CommonParserColor& operator=(const CommonParserColor& o)
    {
        _argb = o._argb;
        return *this;
    }

    /// The equal operator.
    bool operator==(const CommonParserColor& o) const
    {
        return _argb == o._argb;
    }

private:
    /// Convert to ARGB.
    long _ToLongARGB(int r,
                     int g,
                     int b, 
                     int a) const
    {
        return ((r << ATOM_COLOR_R_SHIFT) & ATOM_COLOR_R_BITS)
              |((g << ATOM_COLOR_G_SHIFT) & ATOM_COLOR_G_BITS)
              |((b << ATOM_COLOR_B_SHIFT) & ATOM_COLOR_B_BITS)
              |((a << ATOM_COLOR_A_SHIFT) & ATOM_COLOR_A_BITS);
    }

    long _argb;
};


/**********************************************************************
 *
 *  STATUS OBJECT
 *
 **********************************************************************/
/// \enum CommonParserStatusType
///
/// The type of status.
/// 
enum CommonParserStatusType {
    /// ----- Successful results; CommonParserStatus::Succeeded() returns true -----
    /// Normal successful status
    CommonParserStatusTypeOk = 0,          
    /// Okay, but continuing
    CommonParserStatusTypeContinue = 1,          
    /// Okay, but operation completed
    CommonParserStatusTypeDone = 2,         
    /// Okay, but ignoring unsupported feature.
    CommonParserStatusTypeIgnoredUnsupported = 3,          
    /// Operation was innocuous, no action taken.
    CommonParserStatusTypeUnchanged = 4,          
    /// Operation replaced an existing item.
    CommonParserStatusTypeReplaced = 5,          

    /// ----- Unsuccessful results -----
    /// Callee not in the proper state to receive call
    CommonParserStatusTypeNotReady = 0x80000001, 
    /// Invalid argument.
    CommonParserStatusTypeInvalidArg = 0x80000002, 
    /// Out of memory
    CommonParserStatusTypeNoMemory = 0x80000003, 
    /// Out of some other resource
    CommonParserStatusTypeNoResource = 0x80000004, 
    /// Item is already present, and cannot be replaced.
    CommonParserStatusTypeAlreadyPresent = 0x80000005, 
    /// Item is not present, cannot be found.
    CommonParserStatusTypeNotPresent = 0x80000006, 
    /// Incomplete string: premature EOS
    CommonParserStatusTypeIncompleteString = 0x80000007, 
    /// Incomplete string: unmatched character.
    CommonParserStatusTypeUnmatchedConstruct = 0x80000008, 
    /// Illegal character sequence.
    CommonParserStatusTypeUnexpectedCharacter = 0x80000009, 
    /// Grammatically correct, but unknown option.
    CommonParserStatusTypeUnknownMarkup = 0x8000000a, 
    /// Version markings indicate unknown version.
    CommonParserStatusTypeUnknownVersion = 0x8000000b,
    /// Missing a required part.
    CommonParserStatusTypeMissingRequired = 0x8000000c, 
    /// Something rotten in the state of callee.
    CommonParserStatusTypeInternalError = 0x8000fffd, 
    /// Something outside of Capabilities.
    CommonParserStatusTypeNotSupported = 0x8000fffe, 
    /// Item not implemented.
    CommonParserStatusTypeNotImplemented = 0x8000ffff, 

    // ---------Abandoned results----------
    CommonParserStatusTypeAbandoned = 0x80010000, 
    /// User requested abandonment of operation.
    CommonParserStatusTypeAbandonByUserRequest = 0x80020000, 

    CommonParserStatusTypeUninitialized = 0xffffffff
};

/// \class CommonParserStatus
///
/// The status or parse process.
/// 
struct CommonParserStatus
{
    /// The default constructor.
    CommonParserStatus() 
        : _eResult(CommonParserStatusTypeUninitialized)
    {}

    /// The constructor from a status enumeration.
    CommonParserStatus(CommonParserStatusType e)
        : _eResult(e) 
    {}

    /// The assignment operator.
    CommonParserStatus& operator= (const CommonParserStatus& oOther)
    {
        _eResult = oOther._eResult;

        return *this;
    }

    /// The equal operator.
    bool operator== (CommonParserStatusType e)
    {
        return _eResult == e;
    }

    /// Get the enumerator of status.
    CommonParserStatusType Result() const { return _eResult; }

    /// Check if it succeeds.
    bool Succeeded() const
    {
        return (_eResult & 0x80000000) == 0;
    }

private:
    CommonParserStatusType _eResult;
};


/**********************************************************************
 *
 *  BASIC PARTICLES
 *
 **********************************************************************/
/// \class CommonParserParticle
///
/// Just a common ancestor.  
/// Offers basic particle characteristics:
/// 1. Cloning
/// 2. Simple "linked list" aggregation.
class CommonParserParticle
{
public:
    /// The default constructor.
    CommonParserParticle() 
        : _pNext(NULL) {}

    /// The destructor.
    virtual ~CommonParserParticle() {};

    /// Clone the particle.
    virtual CommonParserParticle* Clone() const = 0;

protected:
    // This is the implementation, and derivations can
    // "brand" (that is, expose, with stricter type requirements)
    // these methods.

    /// Get the next particle.
    const CommonParserParticle* _Next()  const
    { return _pNext; }

    /// Set the next particle.
    void _SetNext(CommonParserParticle* pNext)
    { _pNext = pNext; }

    /// Append the particle at the end of the list.
    void _Append(const CommonParserParticle* pEnd);

    /// Add a particle to a list.
    static CommonParserStatus _AddToList(
        CommonParserParticle*& pList, 
        const CommonParserParticle& oParticle);

private:
    CommonParserParticle* _pNext;
};


#define ATOM_PARTICLE_DECL(_part,_name)                                 \
    CommonParser ## _part ## ParticleType Type() const override;                 \
                                                                        \
    CommonParser ## _part ## Particle* Clone() const override;          \
                                                                        \
    CommonParser ## _part ## Particle& operator= (const CommonParser ## _part ## Particle& o) override;          \
                                                                        \
    bool operator==(const CommonParser ## _part ## Particle& o) const override;    

#define ATOM_PARTICLE_IMPL(_part,_name)                                 \
                                                                        \
    CommonParser ## _part ## ParticleType                               \
    CommonParser ## _name ## _part ## Particle::Type() const            \
    { return CommonParser ## _part ## ParticleType ## _name;}


// Exposes a declaration for a default constructor and a destructor
#define ATOM_PARTICLE_CDTOR(_class)                                     \
    _class(): _oVal(NULL){}                                            \
    ~_class();

// Exposes the AddToList operation in a specific type; inline, for perf.
#define ATOM_PARTICLE_ADDTOLIST(_typ,_head)                             \
    CommonParserStatus AddToList(const _typ& oEnd); // { CommonParserParticle::AddToList((_typ*)_head,oEnd); }


// A "general" particle is one that represents a single value of a specific
// type.  StyleParticles, for example, fall into this category.
#define ATOM_GENERAL_PARTICLE_DECL(_name, _cat, _type, _extra)          \
class CommonParser ## _name ## _cat ## Particle: public CommonParser ## _cat ## Particle                \
{                                                                       \
public:                                                                 \
    CommonParser ## _name ## _cat ## Particle(_type oVal);              \
    _extra                                                              \
    CommonParser ## _cat ## ParticleType Type()  const  override;       \
    _type                Value() const;                                 \
    CommonParser ## _cat ## Particle* Clone() const override;           \
    CommonParser ## _cat ## Particle& operator= (const CommonParser ## _cat ## Particle& o) override;    \
    bool operator==(const CommonParser ## _cat ## Particle& o) const override;   \
private:                                                                \
    _type _oVal;                                                        \
};

// A "simple" particle is one that contains its value completely 
// within itself (ie, not a list.)
#define ATOM_SIMPLE_PARTICLE_DECL(_name,_cat,_type) ATOM_GENERAL_PARTICLE_DECL(_name,_cat,_type,)
// A "list" particle is one that contains the head of a list of (sub)particles, and so 
// needs special list management semantics: def-ctor, dtor, and access to append.
#define ATOM_LIST_PARTICLE_DECL(  _name,_cat,_type)                     \
    ATOM_GENERAL_PARTICLE_DECL(_name,_cat,_type*,                       \
        ATOM_PARTICLE_CDTOR(CommonParser ## _name ## _cat ## Particle)\
        ATOM_PARTICLE_ADDTOLIST(_type, _oVal))


// Default implementation for Clone.  Used by simple particles
#define ATOM_SIMPLE_CLONE_IMPL(_name,_cat)                              \
    CommonParser ## _cat ## Particle* CommonParser ## _name ## _cat ## Particle::Clone() const          \
    { return new CommonParser ## _name ## _cat ## Particle(_oVal); }   \

// General implementation for particles.
#define ATOM_GENERAL_PARTICLE_IMPL(_name, _cat, _type,_extra)           \
    CommonParser ## _name ## _cat ## Particle :: CommonParser ## _name ## _cat ## Particle(_type oVal)  \
    : _oVal(oVal)                                                       \
    {}                                                                  \
                                                                        \
    _extra                                                              \
                                                                        \
    CommonParser ## _cat ## ParticleType                                \
    CommonParser ## _name ## _cat ## Particle::Type() const             \
    { return CommonParser ## _cat ## ParticleType ## _name;}            \
                                                                        \
    _type CommonParser ## _name ## _cat ## Particle::Value() const { return _oVal; }   \
                                                                        \
    CommonParser ## _cat ## Particle&                                   \
    CommonParser ## _name ## _cat ## Particle::operator= (const CommonParser ## _cat ## Particle& o) \
    {                                                                   \
      if(Type() == o.Type())                                            \
        this->_oVal = ((const CommonParser ## _name ## _cat ## Particle&)o)._oVal;    \
      return *this;                                                     \
    }                                                                   \
                                                                        \
    bool                                                                \
    CommonParser ## _name ## _cat ## Particle::operator== (const CommonParser ## _cat ## Particle& o) const\
    {                                                                   \
      if(Type() != o.Type())                                            \
        return false;                                                   \
      else                                                              \
        return this->_oVal == ((CommonParser ## _name ## _cat ## Particle&)o)._oVal;  \
    }

#define ATOM_SIMPLE_PARTICLE_IMPL(_name,_cat,_type)                     \
    ATOM_GENERAL_PARTICLE_IMPL(_name,_cat,_type,ATOM_SIMPLE_CLONE_IMPL(_name,_cat))

#define ATOM_LIST_PARTICLE_IMPL(_name,_cat,_type)                       \
    ATOM_GENERAL_PARTICLE_IMPL(_name,_cat,_type,)






#define ATOM_TRANSFORM_PARTICLE_BASE  0x0000 // zero
#define ATOM_STYLE_PARTICLE_BASE      0x1000
#define ATOM_CAPABILITY_PARTICLE_BASE 0x2000
#define ATOM_LOCATION_PARTICLE_BASE   0x3000


/**********************************************************************
 *
 *  STYLE PARTICLES
 *
 **********************************************************************/
/// \enum CommonParserStyleParticleType
///
/// The type of style particle.
///
enum CommonParserStyleParticleType {
    CommonParserStyleParticleTypeOther = ATOM_STYLE_PARTICLE_BASE,

    /// Typeface used, eg "Times New Roman"
    CommonParserStyleParticleTypeTypeface,           
    /// The font is SHX font or not
    CommonParserStyleParticleTypeIsSHX,              
    /// The pitch family.
    CommonParserStyleParticleTypePitchFamily,        
    /// The character set.
    CommonParserStyleParticleTypeCharacterSet, 
    /// The alternative typeface if the current typeface can not support the character.
    CommonParserStyleParticleTypeAltTypefaces,  
    /// Typographical (cell-height) size of font
    CommonParserStyleParticleTypeSize,               
    /// Size of font expressed as Cap-Height.
    CommonParserStyleParticleTypeCapSize,            
    /// Italic variant
    CommonParserStyleParticleTypeItalic,             
    /// Underlined
    CommonParserStyleParticleTypeUnderline,          
    /// Overlined
    CommonParserStyleParticleTypeOverline,           
    /// Struck-through.
    CommonParserStyleParticleTypeStrikethrough,      
    /// Uppercase, lowercase, small-caps
    CommonParserStyleParticleTypeCaseShift,          
    /// Normal, Bold, ...     See enum.
    CommonParserStyleParticleTypeFontWeight,         
    /// RGB + Alpha
    CommonParserStyleParticleTypeFillColor,          
    /// (Outline) stroke line weight.  Omitted if font is solid
    CommonParserStyleParticleTypeStrokeWeight,       
    /// (Outline) stroke line color.  (ditto)
    CommonParserStyleParticleTypeStrokeColor,        
    /// Boolean property: stroke rendered behind (before) fill?
    CommonParserStyleParticleTypeStrokeBehind,       
    /// Inter-character spacing: Amount of space added to character advance.
    CommonParserStyleParticleTypeTrackingAugment,    
    /// Describes vertical relationship of text to insertion point.
    CommonParserStyleParticleTypeVerticalAlignment,  
    /// Describes horizontal relationship of text to insertion point.
    CommonParserStyleParticleTypeHorizontalAlignment,
    /// Vertical relationship of text to other runs on same line.
    CommonParserStyleParticleTypeAdvanceAlignment,   
    /// *Multi-line* treatment of text: flush left ... fully justified.
    CommonParserStyleParticleTypeJustification,      
    /// *Multi-line* treatment: distance from one baseline to the next.
    CommonParserStyleParticleTypeLineHeight,         
    /// *Multi-line* treatment: extra distance to appear before paragraph.
    CommonParserStyleParticleTypeBeforePara,         
    /// *Multi-line* treatment: extra distance to appear after paragraph.
    CommonParserStyleParticleTypeAfterPara,          
    /// Identifies a resolved or unresolved Reference, or something to be treated as such.
    CommonParserStyleParticleTypeReferenceExpansion, 
    /// Color of background.
    CommonParserStyleParticleTypeBackgroundColor
};
/// \class CommonParserStyleParticle
///
/// This is the base (abstract) class for style particle.
/// 
class CommonParserStyleParticle: public CommonParserParticle 
{
public:
    /// The type of the particle.
    virtual CommonParserStyleParticleType Type() const = 0;

    /// Clone the particle.
    virtual CommonParserStyleParticle* Clone() const = 0;

    /// The assignment operator.
    virtual CommonParserStyleParticle& operator= (const CommonParserStyleParticle& o) = 0;

    /// The equal operator.
    virtual bool operator==(const CommonParserStyleParticle& o) const = 0;

    /// Get the next particle.
    const CommonParserStyleParticle* Next() const
    { 
        return (CommonParserStyleParticle*)CommonParserParticle::_Next(); 
    }

    /// Set the next particle.
    void SetNext(CommonParserStyleParticle* pNext)
    {
        CommonParserParticle::_SetNext(pNext);
    }

    /// Append the particle to the end of the list.
    void Append(const CommonParserStyleParticle* pEnd)
    {
        CommonParserParticle::_Append(pEnd);
    }

protected:
    /// The default constructor.
    CommonParserStyleParticle() {};
};

/*
  For consistency, each Style CommonParserParticle is modeled after the following
  macro.  For example

  STYLE_PARTICLE_DECL(Font,ASTRING)

  will produce a class declaration:
  1. a class FontStyleParticle (derived from CommonParserStyleParticle) that
  2. has a Type() method that returns keFont, and
  3. a Value() method that returns a value of type ASTRING.
  4. Clone method that creates a clone of itself
  5. Assignment operator that receives an assignment of a 
     like-typed particle.

  STYLE_PARTICLE_IMPL will produce the implemented methods.

  Generics are an alternative, but depending on the compiler,
  that can be bloaty.

  Also, RTTI is an alternative to the Type() method, but that
  would require a cascading collection of if(type-determination)
  statements; using an enum allows a switch statement.
*/
#define ATOM_STYLE_PARTICLE_DECL(     _name, _type)     ATOM_SIMPLE_PARTICLE_DECL(       _name, Style, _type)

#define ATOM_STYLE_PARTICLE_IMPL(     _name, _type)     ATOM_SIMPLE_PARTICLE_IMPL(       _name, Style, _type)

#define ATOM_STYLE_PARTICLE_TBLR_DECL(_name, _type)       \
    ATOM_STYLE_PARTICLE_DECL( Top ##  _name, _type)       \
    ATOM_STYLE_PARTICLE_DECL( Bottom##_name, _type)       \
    ATOM_STYLE_PARTICLE_DECL( Left ## _name, _type)       \
    ATOM_STYLE_PARTICLE_DECL( Right ##_name, _type)

#define ATOM_STYLE_PARTICLE_TBLR_IMPL( _name, _type)      \
    ATOM_STYLE_PARTICLE_IMPL( Top ##  _name, _type)       \
    ATOM_STYLE_PARTICLE_IMPL( Bottom##_name, _type)       \
    ATOM_STYLE_PARTICLE_IMPL( Left ## _name, _type)       \
    ATOM_STYLE_PARTICLE_IMPL( Right ##_name, _type)

/// \enum CommonParserFontWeightType
///
/// The font weight type.
///
enum CommonParserFontWeightType
{
    CommonParserFontWeightTypeDontCare = 0,
    CommonParserFontWeightTypeThin = 100,

    CommonParserFontWeightTypeUltraLight = 200,
    CommonParserFontWeightTypeExtraLight = 200,

    CommonParserFontWeightTypeLight = 300,

    CommonParserFontWeightTypeNormal = 400,
    CommonParserFontWeightTypeRegular = 400,

    CommonParserFontWeightTypeMedium = 500,

    CommonParserFontWeightTypeSemiBold = 600,
    CommonParserFontWeightTypeDemiBold = 600,

    CommonParserFontWeightTypeBold = 700,

    CommonParserFontWeightTypeUltraBold = 800,
    CommonParserFontWeightTypeExtraBold = 800,

    CommonParserFontWeightTypeBlack = 900,
    CommonParserFontWeightTypeHeavy = 900
};

/// \enum CommonParserPitchFamilyType
///
/// The pitch family type.
///
enum CommonParserPitchFamilyType
{
    /// Fixed width font, like Courier
    CommonParserPitchFamilyTypeFixedPitch    = 1,     
    /// Variable width font, like Times Roman, etc.
    CommonParserPitchFamilyTypeVariablePitch = 2,      

    // --- above can be |'d with below ---
    /// Roman-like font, with serifs
    CommonParserPitchFamilyTypeRoman      = (1<<4),    

    CommonParserPitchFamilyTypeSerif      = (1<<4),

    CommonParserPitchFamilyTypeSwiss      = (2<<4),    
    /// Sans-serif font, like Helvetica
    CommonParserPitchFamilyTypeSansSerif  = (2<<4),
    /// Uniform stroke thickness, typically sans-serif
    CommonParserPitchFamilyTypeModern     = (3<<4),    
    /// Handwriting-like (cursive styles, etc.)
    CommonParserPitchFamilyTypeScript     = (4<<4),    
    /// More fanciful fonts (Wingdings, etc.)
    CommonParserPitchFamilyTypeDecorative = (5<<4),    
};

/// \enum CommonParserJustificationType
///
/// The justification type for a paragraph style.
///
enum CommonParserJustificationType
{
    CommonParserJustificationTypeLeft,
    CommonParserJustificationTypeCentered,          
    CommonParserJustificationTypeRight,
    CommonParserJustificationTypeJustified
};

/// \enum CommonParserVerticalAlignmentType
///
/// The alignment type in vertical direction.
///
enum CommonParserVerticalAlignmentType
{
    /// Top of ascent
    CommonParserVerticalAlignmentTypeAscender,    
    /// Top of small lowercase letters
    CommonParserVerticalAlignmentTypeXHeight,      
    /// Arithmetic midpoint between base and Ascent.
    CommonParserVerticalAlignmentTypeMid,          
    /// Font baseline
    CommonParserVerticalAlignmentTypeBaseline,     
    /// Bottom of descent
    CommonParserVerticalAlignmentTypeDescender     
};

/// \enum CommonParserHorizontalAlignmentType
///
/// The alignment type in horizontal direction.
///
enum CommonParserHorizontalAlignmentType
{
    CommonParserHorizontalAlignmentTypeLeft,
    CommonParserHorizontalAlignmentTypeMiddle,
    CommonParserHorizontalAlignmentTypeRight
};

/// \enum CommonParserTextLineType
///
/// The type of underline, overline and strikethrough.
///
enum CommonParserTextLineType
{
    CommonParserTextLineTypeNone,
    CommonParserTextLineTypeSingle,
    CommonParserTextLineTypeDouble,
    CommonParserTextLineTypeDotted,
};

/// \enum CommonParserCaseShiftType
///
/// The case of the characters.
///
enum CommonParserCaseShiftType
{
    CommonParserCaseShiftTypeNoShift, 
    /// Lowercase letters are replaced with full-size capitals  
    CommonParserCaseShiftTypeUppercase,   
    /// Uppercase letters are replaced with lowercase.
    CommonParserCaseShiftTypeLowercase,   
    /// Lowercase letters are replaced with smaller capitals.
    CommonParserCaseShiftTypeSmallCaps,   
};

/// \enum CommonParserReferenceExpansionType
///
/// Many markup languages have a means of referencing external text;
/// MTEXT has %< .. >%, XML has &entity; notation, etc.  The parser may not have access to this 
/// information, so it relies on an external agent to substitute (expand) the original reference 
/// with the intended text.
/// 
enum CommonParserReferenceExpansionType
{
    /// Normal text, not a reference.
    CommonParserReferenceExpansionTypeNotReference  = 0x00000,  
    /// Represents the original reference text
    CommonParserReferenceExpansionTypeSource        = 0x00001,  
    /// Represents the expanded text, the result of the
    CommonParserReferenceExpansionTypeExpanded      = 0x00002,  
};

/// \enum CommonParserReferenceExpansionType
///
/// The type of line height measure.
/// 
enum CommonParserLineHeightMeasureType {
    CommonParserLineHeightMeasureTypeDefault,
    CommonParserLineHeightMeasureTypeAtLeast,
    CommonParserLineHeightMeasureTypeExactly,
    CommonParserLineHeightMeasureTypeMultiple
};

/// \class CommonParserLineHeightMeasure
///
/// Many markup languages have a means of referencing external text; MTEXT has %< .. >%, XML 
/// has &entity; notation, etc.  The parser may not have access to this information, so it relies 
/// on an external agent to substitute (expand) the original reference with the intended text.
/// 
class CommonParserLineHeightMeasure
{
public:
    CommonParserMeasure _lineHeight;
    CommonParserLineHeightMeasureType _lineHeightType;

    /// The constructor from the height value and height type.
    CommonParserLineHeightMeasure(CommonParserMeasure measure, 
                                  CommonParserLineHeightMeasureType type)
        : _lineHeight(measure)
        , _lineHeightType(type)
    {}

    /// The equal operator.
    bool operator == (const CommonParserLineHeightMeasure& compare) const
    {
        return _lineHeight == compare._lineHeight && _lineHeightType == compare._lineHeightType;
    };
};

// These are the defined style particles.  All sinks should accept at least a subset of these, 
// and gracefully ignore any particle it doesn't support, though in some cases it may choose to 
// abandon the parse (rare!).
ATOM_STYLE_PARTICLE_DECL(Typeface,            CommonParserStRange)           // Something like "Times new Roman" or "Sans Serif"
ATOM_STYLE_PARTICLE_DECL(IsSHX,               bool)                          // If the font is a SHX font or not
ATOM_STYLE_PARTICLE_DECL(PitchFamily,         CommonParserPitchFamilyType) // Font-matching heuristics if font isn't known.
ATOM_STYLE_PARTICLE_DECL(CharacterSet,        int)                           // Font-matching heuristics for which character set.
ATOM_STYLE_PARTICLE_DECL(AltTypefaces,        CommonParserStRange)           // Alternate typefaces, to be tried if Typeface isn't found.
ATOM_STYLE_PARTICLE_DECL(Size,                CommonParserMeasure)           // Typographical definition: cell height |   Mutually
ATOM_STYLE_PARTICLE_DECL(CapSize,             CommonParserMeasure)           // Alternate definition: Cap height Size |   Exclusive
ATOM_STYLE_PARTICLE_DECL(FontWeight,          CommonParserFontWeightType)  // Weight of the font (400 = normal, 700 = bold)
ATOM_STYLE_PARTICLE_DECL(Italic,              bool)                          // Italic variation of font?
ATOM_STYLE_PARTICLE_DECL(Underline,           CommonParserTextLineType)    // None/Single/Double/Dotted.  Sink may support only
ATOM_STYLE_PARTICLE_DECL(Overline,            CommonParserTextLineType)    // ..                          a limited subset
ATOM_STYLE_PARTICLE_DECL(Strikethrough,       CommonParserTextLineType)    // ..                          and modify rendering.
ATOM_STYLE_PARTICLE_DECL(CaseShift,           CommonParserCaseShiftType)   // Small Caps, etc.
ATOM_STYLE_PARTICLE_DECL(FillColor,           CommonParserColor)             // CommonParserColor (with Alpha) of the fill
ATOM_STYLE_PARTICLE_DECL(StrokeWeight,        CommonParserMeasure)           // Line weight of the stroke.
ATOM_STYLE_PARTICLE_DECL(StrokeColor,         CommonParserColor)             // CommonParserColor (with Alpha) of the stroke.
ATOM_STYLE_PARTICLE_DECL(StrokeBehind,        bool)                          // Does the stroke get rendered behind fill? (eg SFA vs SVG)
ATOM_STYLE_PARTICLE_DECL(TrackingAugment,     CommonParserMeasure)           // Inter-character space, to apply between each character.
ATOM_STYLE_PARTICLE_DECL(VerticalAlignment,   CommonParserVerticalAlignmentType)   // Vertical relationship of text to insertion point/path
ATOM_STYLE_PARTICLE_DECL(HorizontalAlignment, CommonParserHorizontalAlignmentType) // Horizontal relationship of text to insertion point/path
ATOM_STYLE_PARTICLE_DECL(AdvanceAlignment,    CommonParserMeasure)           // Vertical relationship of text runs with respect to other text on line
ATOM_STYLE_PARTICLE_DECL(Justification,       CommonParserJustificationType)       // Multi-line support: 
ATOM_STYLE_PARTICLE_DECL(LineHeight,          CommonParserLineHeightMeasure)         // Multi-line support: distance from base-line to base-line
ATOM_STYLE_PARTICLE_DECL(BeforePara,          CommonParserMeasure)           // Multi-line support: extra space before a "paragraph" unit
ATOM_STYLE_PARTICLE_DECL(AfterPara,           CommonParserMeasure)           // Multi-line support: extra space after a "paragraph" unit.
ATOM_STYLE_PARTICLE_DECL(ReferenceExpansion,  CommonParserReferenceExpansionType)  // Reference Expansion
ATOM_STYLE_PARTICLE_DECL(BackgroundColor,     CommonParserColor)             // CommonParserColor (with Alpha) of the background.

/**********************************************************************
 *
 *  STYLE DESCRIPTION
 *
 **********************************************************************/

/// \class CommonParserStyleDescription
///
/// This interface merely describes a "style", a set of particles.
/// 
class CommonParserStyleDescription
{
public:
    /// Get the particle for the description.
    virtual const CommonParserStyleParticle* Description() const = 0;

    /// Get a particle of the type from the description.
    virtual const CommonParserStyleParticle* DescriptionParticle(
        CommonParserStyleParticleType eType) const = 0;
};

/// \class CommonParserStyleDescription
///
/// This interface describes a *change* in style, as well as the final state (inherited).
/// 
class CommonParserStyleChange: public CommonParserStyleDescription
{
public:
     /// Contains the changes in style.
     virtual const CommonParserStyleParticle* Deltas() const = 0;
};

/// \class CommonParserStyleDescription
///
/// This interface describes a table of styles, a la a "style sheet".
/// Each style is named.  Note that the implementation can late-bind the resolution of names to 
/// styles rather than requiring all styles be defined prior to parsing.
///
class CommonParserStyleTable
{
public:
    /// Does a lookup of the style requested.  Returns NULL if no style has
    /// the name given by sName.
    virtual const CommonParserStyleDescription* operator[] (
        const CommonParserStRange& sName) const = 0;

    /// Permits (late?) addition of styles to the style table.
    /// (Used by parser to augment styles that may be defined in markup.)
    /// Should supercede any style with the same that already exists.
    virtual CommonParserStatus AddStyle(const CommonParserStRange& sName, 
                                        const CommonParserStyleDescription* pStyle) = 0;
};



/**********************************************************************
 *
 *  TRANSFORMS
 *
 **********************************************************************/
/// \enum CommonParserTransformParticleType
///
/// The type of transform particle.
///
enum CommonParserTransformParticleType {
    CommonParserTransformParticleTypeNone = 0, 
    CommonParserTransformParticleTypeScale = 0x01,
    CommonParserTransformParticleTypeSkew = 0x02,
    CommonParserTransformParticleTypeTranslation = 0x04,
    CommonParserTransformParticleTypeRotation = 0x08,
    CommonParserTransformParticleTypeArbitrary = 0x10,
};

/// \enum CommonParserTransformParticleSemantics
///
/// The semantics of transform particle.
///
enum CommonParserTransformParticleSemantics {
    CommonParserTransformParticleSemanticsUndefined = 0,
    CommonParserTransformParticleSemanticsOblique = 0x01,
    CommonParserTransformParticleSemanticsWidth = 0x02,
};
/// \class CommonParserStyleDescription
///
/// A transform particle describes how the space the text is being rendered into should be 
/// transformed.
/// 
class  CommonParserTransformParticle: public CommonParserParticle 
{
public:
    /// The type of the particle.
    virtual CommonParserTransformParticleType Type() const = 0;

    /// Clone the particle.
    virtual CommonParserTransformParticle* Clone() const = 0;

    /// The semantics of the particle.
    virtual CommonParserTransformParticleSemantics Semantics() const = 0;

    /// The assignment operator.
    virtual CommonParserTransformParticle& operator= (
        const CommonParserTransformParticle& /*o*/)
    {
		return *this;
    }

    /// The equal operator.
    virtual bool operator==(const CommonParserTransformParticle& o)const = 0;

    // When requested, the particle will populate the 3x3 matrix with its
    // transformation.  It can assume that the matrix is pre-initialized to
    // the Identity state.
    virtual void SetMatrix(CommonParserMatrix& m) const = 0;

    /// Check if the transform matrix is identity.
    virtual bool IsIdentity()  const = 0;

    /// Get the next transform particle.
    const CommonParserTransformParticle* Next() const
    {
        return (CommonParserTransformParticle*) CommonParserParticle::_Next();
    }

    /// Append the particle at the end of list.
    void Append(const CommonParserTransformParticle* pEnd)
    {
        CommonParserParticle::_Append(pEnd);
    }

    /// Set the next transform particle.
    void SetNext(CommonParserTransformParticle* pNext)
    {
        CommonParserParticle::_SetNext(pNext);
    }

protected:
    /// The default constructor.
    CommonParserTransformParticle() {};

private:
    CommonParserTransformParticle* _pNext;
};

#define TRANSFORM_PARTICLE_XY_DECL(_name,_type)                   \
class CommonParser ## _name ## TransformParticle:                 \
    public CommonParserTransformParticle                          \
{                                                                 \
public:                                                           \
    CommonParser ## _name ## TransformParticle(                   \
        _type x,                                                  \
        _type y,                                                  \
        CommonParserTransformParticleSemantics value);\
    CommonParserTransformParticleType Type() const override;      \
    CommonParserTransformParticleSemantics Semantics() const override;     \
    void  Semantics(CommonParserTransformParticleSemantics);      \
                                                                  \
    _type _name ## X() const;                     \
    _type _name ## Y() const;                     \
                                                                  \
    CommonParserTransformParticle* Clone() const override;        \
    CommonParserTransformParticle& operator= (const CommonParserTransformParticle& o) override;    \
    bool operator==(const CommonParserTransformParticle& o)const override; \
    void SetMatrix(CommonParserMatrix& m) const override;         \
    bool IsIdentity() const override;                             \
private:                                                          \
    _type _x;                                                     \
    _type _y;                                                     \
    CommonParserTransformParticleSemantics _semantics;                        \
};

// Note that SetMatrix is not implemented here; it must be implemented manually for each derivation.
#define TRANSFORM_PARTICLE_XY_IMPL(_name,_type)                             \
    CommonParser ## _name ## TransformParticle::CommonParser ## _name ## TransformParticle(\
        _type x,                                                            \
        _type y,                                                            \
        CommonParserTransformParticleSemantics value)   \
    : _x(x)                                                                 \
    , _y(y)                                                                 \
    , _semantics(value)                                                     \
    {}                                                                      \
                                                                            \
    CommonParserTransformParticleType        \
    CommonParser ## _name ## TransformParticle::Type() const                \
    { return CommonParserTransformParticleType ## _name; }                  \
                                                                            \
    CommonParserTransformParticleSemantics   \
    CommonParser ## _name ## TransformParticle::Semantics() const           \
    { return _semantics; }                                                  \
                                                                            \
    void  CommonParser ## _name ## TransformParticle::Semantics(            \
        CommonParserTransformParticleSemantics value)  \
    { _semantics = value; }                                                 \
                                                                            \
    _type CommonParser ## _name ## TransformParticle::_name ## X() const \
    { return _x; }                                                          \
    _type CommonParser ## _name ## TransformParticle::_name ## Y() const \
    { return _y; }                                                          \
                                                                            \
    CommonParserTransformParticle*                                          \
    CommonParser ## _name ## TransformParticle::Clone() const               \
    { return new CommonParser ## _name ## TransformParticle(_x,_y, _semantics); }        \
                                                                            \
    CommonParserTransformParticle&                                          \
    CommonParser ## _name ## TransformParticle::operator= (const CommonParserTransformParticle& o) \
    {                                                                       \
        if(Type() == o.Type()) {                                            \
            this->_x = ((const CommonParser ## _name ## TransformParticle&)o)._x;        \
            this->_y = ((const CommonParser ## _name ## TransformParticle&)o)._y;        \
            this->_semantics = ((const CommonParser ## _name ## TransformParticle&)o)._semantics;  \
        }                                                                   \
        return *this;                                                       \
    }                                                                       \
    bool CommonParser ## _name ## TransformParticle::operator==(            \
        const CommonParserTransformParticle& o) const                       \
    {   if (Type() != o.Type())                                             \
            return false;                                                   \
        const CommonParser ## _name ## TransformParticle& oOther =          \
            (const CommonParser ## _name ## TransformParticle&)o;           \
        return this->_x == oOther._x && this->_y == oOther._y &&            \
        this->_semantics == oOther._semantics;                              \
    }

// Note that SetMatrix is not implemented here; it must be
// implemented manually for each derivation.
#define TRANSFORM_PARTICLE_DECL(_name,_type)                      \
class  CommonParser ## _name ## TransformParticle:                \
    public CommonParserTransformParticle                          \
{                                                                 \
public:                                                           \
    CommonParser ## _name ## TransformParticle(                   \
        _type v,                                                  \
        CommonParserTransformParticleSemantics value);   \
    CommonParserTransformParticleType Type() const override;      \
    CommonParserTransformParticleSemantics  Semantics() const override;     \
    void  Semantics(CommonParserTransformParticleSemantics);      \
                                                                  \
    _type _name() const;                          \
                                                                  \
    CommonParserTransformParticle* Clone() const override;        \
    CommonParserTransformParticle& operator= (const CommonParserTransformParticle& o) override;    \
    bool operator==(const CommonParserTransformParticle& o)const override;  \
    void SetMatrix(CommonParserMatrix& m) const override;         \
    bool IsIdentity() const override;                             \
private:                                                          \
    _type  _v;                                                    \
    CommonParserTransformParticleSemantics _semantics;            \
};

#define TRANSFORM_PARTICLE_IMPL(_name,_type)                                  \
    CommonParser ## _name ## TransformParticle::CommonParser ## _name ## TransformParticle(\
        _type v,                                                              \
        CommonParserTransformParticleSemantics value)\
        : _v(v)                                                               \
        , _semantics(value)                                                   \
    {}                                                                        \
                                                                              \
    CommonParserTransformParticleType          \
    CommonParser ## _name ## TransformParticle::Type() const                  \
    { return CommonParserTransformParticleType ## _name; }                    \
                                                                              \
    CommonParserTransformParticleSemantics     \
    CommonParser ## _name ## TransformParticle::Semantics() const             \
    { return _semantics; }                                                    \
                                                                              \
    void  CommonParser ## _name ## TransformParticle::Semantics(              \
        CommonParserTransformParticleSemantics value)\
    { _semantics = value; }                                                   \
                                                                              \
    _type CommonParser ## _name ## TransformParticle::_name () const\
    { return _v; }                                                            \
                                                                              \
    CommonParserTransformParticle*                                            \
    CommonParser ## _name ## TransformParticle::Clone() const                 \
    { return new CommonParser ## _name ## TransformParticle(_v, _semantics); }\
                                                                              \
    CommonParserTransformParticle&                                            \
    CommonParser ## _name ## TransformParticle::operator= (                   \
        const CommonParserTransformParticle& o)                               \
    {                                                                         \
        if(Type() == o.Type()) {                                              \
            this->_v = ((const CommonParser ## _name ## TransformParticle&)o)._v;           \
            this->_semantics = ((const CommonParser ## _name ## TransformParticle&)o)._semantics;   \
        }                                                                     \
        return *this;                                                         \
    }                                                                         \
    bool CommonParser ## _name ## TransformParticle::operator== (             \
        const CommonParserTransformParticle& o)const                          \
    {   if (Type() != o.Type())                                               \
            return false;                                                     \
        const CommonParser ## _name ## TransformParticle& oOther =            \
            (const CommonParser ## _name ## TransformParticle&)o;             \
        return this->_v == oOther._v &&                                       \
        this->_semantics == oOther._semantics;                                \
    }



TRANSFORM_PARTICLE_XY_DECL(Scale,         NUMBER)
TRANSFORM_PARTICLE_XY_DECL(Skew,          CommonParserRadialMeasure)
TRANSFORM_PARTICLE_XY_DECL(Translation,   NUMBER)
TRANSFORM_PARTICLE_DECL(   Rotation,      CommonParserRadialMeasure)
TRANSFORM_PARTICLE_DECL(   Arbitrary,     CommonParserMatrix);


/// \class CommonParserTransform
///
/// Describes a complete transformation.
/// 
class  CommonParserTransform
{
public:
    /// Populates the indicated matrix with the cumulative transform as represented
    /// by the separate TransformParticles described below.
    /// Return is the union of all CommonParserTransformParticle types: for example, 
    ///   keNone           indicates no transform in effect, 
    ///   keScale | keSkew indicates that both of these transforms are in effect 
    /// You would need to traverse the Description() list to know their order, 
    /// if that was important to you.
    virtual CommonParserTransformParticleType 
        AsMatrix(CommonParserMatrix* ) const = 0;

    /// A list of the individual TransformParticles that go into making the matrix.
    virtual const CommonParserTransformParticle* Description() const = 0;
};

/// \class CommonParserTransformChange
///
/// Describes how the transform has changed since the last TextRun, in addition to how the 
/// overall transformation state (inherited).
/// 
class  CommonParserTransformChange: public CommonParserTransform
{
public:
    /// Contains the changes in style.
    virtual const CommonParserTransformParticle* Deltas() const = 0;
};


/**********************************************************************
 *
 *  REFERENCE RESOLVER
 *
 **********************************************************************/
// Fields or other inserted codes not intrinsically understood by the
// markup language are resolved by this mechansim.
// For example: MTEXT %<...>%, XML &Entities;, etc.
//

class CommonParserEnvironment; // forward reference
/// \class CommonParserReferenceResolver
///
/// Resolve the reference in the markup.
/// 
class CommonParserReferenceResolver
{
public:
    /// Allows the resolver to set up or allocate
    virtual CommonParserStatus Initialize() = 0;

    /// Requests the resolver to resolve a reference.
    virtual CommonParserStatus Resolve(const CommonParserStRange sParserName,
                                       const CommonParserStRange sReference,
                                       CommonParserStRange& sResult,
                                       CommonParserEnvironment* pEnv) = 0;

    /// Allows the resolver to clean up.
    virtual CommonParserStatus Terminate() = 0;
};

/**********************************************************************
 *
 *  ENVIRONMENT
 *
 **********************************************************************/

class CommonParserSink; // forward declaration.

/// \class CommonParserEnvironment
///
/// This interface contains settings pertinent to the parsing/rendering operation.
/// 
class CommonParserEnvironment
{
public:
    /// This is the "default" style that is in effect in the absence 
    /// of any other markup.
    virtual const CommonParserStyleDescription* AmbientStyle() const = 0;

    /// Any transform that is in effect.  Cumulative w/ TextRun.
    virtual const CommonParserTransform* AmbientTransform() const = 0;

    /// This contains the complete repertoire of styles known.
    /// Note that these may be defined out-of-band (not within the
    /// markup) though the parser can augment the dictionary if
    /// necessary.
    virtual const CommonParserStyleTable* StyleDictionary() const = 0;

    /// This is the recipient of a parser's effort.
    virtual CommonParserSink* Sink() const = 0;

    /// This is the mechanism whereby the parser can expand fields
    /// (which may not be governed by the markup language.)
    virtual CommonParserReferenceResolver* References() const = 0;

    /// This is the color that the text is being rendered over.
    virtual CommonParserColor CanvasColor() const = 0;
};




/**********************************************************************
 *
 *  LOCATION
 *
 **********************************************************************/
/// \class CommonParserStyleParticleType
///
/// The type of location particle.
///
enum CommonParserLocationParticleType {
    /// Defines a bookmark location.
    CommonParserLocationParticleTypeBookmark = ATOM_LOCATION_PARTICLE_BASE,
    /// Request to return to a bookmark.
    CommonParserLocationParticleTypeReturnToBookmark,                      
    /// ... under some condition.
    CommonParserLocationParticleTypeConditionalReturnToBookmark,           
    /// Move a relative amount.
    CommonParserLocationParticleTypeRelative,                              
    /// Location is absolute position.
    CommonParserLocationParticleTypePoint,                                 
    /// Location is a path.
    CommonParserLocationParticleTypePath,           
    /// Move to the beginning of the next line.
    CommonParserLocationParticleTypeLineBreak,                             
};

/// \class CommonParserLocationParticle
///
/// Describes the location information.
/// 
class CommonParserLocationParticle: public CommonParserParticle
{
public:
    /// Get the type of the particle.
    virtual CommonParserLocationParticleType Type() const = 0;

    /// Clone the particle.
    virtual CommonParserLocationParticle* Clone() const = 0;

    /// The assignment operator.
    virtual CommonParserLocationParticle& operator= (const CommonParserLocationParticle& o) = 0;

    /// The equal operator.
    virtual bool operator==(const CommonParserLocationParticle& o) const = 0;

    /// Get the next particle.
    const CommonParserLocationParticle* Next() const
    { 
        return (CommonParserLocationParticle*) CommonParserParticle::_Next(); 
    }

    /// Append the particle to the end of the list.
    void Append(const CommonParserLocationParticle* pEnd)
    { 
        CommonParserParticle::_Append(pEnd);
    }

    /// Set the next particle.
    void SetNext(CommonParserLocationParticle* pNext)
    {
        CommonParserParticle::_SetNext(pNext);
    }

protected:
    CommonParserLocationParticle() {};

private:
    CommonParserLocationParticle* _pNext;
};

/// \class CommonParserBookmarkLocationParticle
///
/// This particle requests the Sink to remember the current location in the indicated location 
/// of a point array, as it will be referenced by a future particle.  (Since text metrics aren't 
/// necessarily available to the markup, this is information only the sink or other renderer is 
/// likely to maintain.)
/// 
class CommonParserBookmarkLocationParticle: public CommonParserLocationParticle
{
public:
    /// The constructor from a bookmark index.
    CommonParserBookmarkLocationParticle(int iIndex);

    ATOM_PARTICLE_DECL(Location, Bookmark)

    /// Get the bookmark.
    int Index() const;

private:
    int _iIndex;
};

/// \class CommonParserReturnToBookmarkLocationParticle
///
/// This particle requests the Sink to update its current location to be the previously bookmarked
/// location.
/// 
class CommonParserReturnToBookmarkLocationParticle: public CommonParserLocationParticle
{
public:
    /// The constructor from a bookmark index.
    CommonParserReturnToBookmarkLocationParticle(int iIndex);

    ATOM_PARTICLE_DECL(Location,ReturnToBookmark)

    /// The bookmark to return to.
    int Index() const;

private:
    int _iIndex;
};

/// \class CommonParserConditionType
///
/// The condition when return to bookmark.
/// 
enum CommonParserConditionType {
    /// if Bookmark[i] is farther along advance vector,
    CommonParserConditionTypeFarthestAdvance,
    /// if Bookmark[i] is less father along (behind)
    CommonParserConditionTypeLeastAdvance,
};

/// \class CommonParserConditionalReturnToBookmarkLocationParticle
///
/// Return to the bookmark conditionally.
/// 
class  CommonParserConditionalReturnToBookmarkLocationParticle: public CommonParserLocationParticle
{
public:
    /// The constructor from a bookmark index and a condition type.
    CommonParserConditionalReturnToBookmarkLocationParticle(
        int iIndex, 
        CommonParserConditionType eType);

    ATOM_PARTICLE_DECL(Location,ConditionalReturnToBookmark)

    /// The bookmark to return to.
    int Index() const;

    /// Condition under which to return.
    CommonParserConditionType Condition() const;
private:
    int _iIndex;
    CommonParserConditionType _eCondition;
};

/// The sink should keep a table of points at least this size.
const int kiBookmarkTableSize = 8;

/// \class CommonParserSubSemanticType
///
/// The semantic for a paragraph.
/// 
enum CommonParserSubSemanticType {
    CommonParserSubSemanticTypeUndefined,
    CommonParserSubSemanticTypeLeftIndent,
    CommonParserSubSemanticTypeFirstLineIndent,
    CommonParserSubSemanticTypeRightIndent
};

/// \class CommonParserConditionalReturnToBookmarkLocationParticle
///
/// This particle describes a relative location (ie, a vector).
/// The Sink should update the "current" location using this vector, transformed by the
/// CommonParserEnvironment->AmbientTransform() but not this TextRun's Transform, prior to 
/// rendering the current TextRun.
/// 
class  CommonParserRelativeLocationParticle: public CommonParserLocationParticle
{
public:
    /// The constructor from the relative position.
    CommonParserRelativeLocationParticle(CommonParserMeasure mAdvance,CommonParserMeasure mRise);

    ATOM_PARTICLE_DECL(Location,Relative)

    /// Get the semantics.
    virtual CommonParserSubSemanticType Semantic()
    {
        return _semantic;
    }

    /// Set the semantics.
    void Semantic(CommonParserSubSemanticType value)
    {
        _semantic = value;
    }

    /// The amount to advance along the baseline (in horizontal text, this
    /// is parallel with X axis.)
    CommonParserMeasure Advance()   const;

    /// The amount to move perpendicular to the baseline (y axis in horizontal text) 
    /// Positive values represent "up" relative to letter.
    CommonParserMeasure Rise()      const;

private:
    CommonParserMeasure _mAdvance;
    CommonParserMeasure _mRise;
    CommonParserSubSemanticType _semantic;
};

/// \class CommonParserPointLocationParticle
///
/// The particle describes an absolute location (ie, a point).
/// 
class  CommonParserPointLocationParticle: public CommonParserLocationParticle
{
public:
    /// The constructor from the point location.
    CommonParserPointLocationParticle(NUMBER x,NUMBER y);

    ATOM_PARTICLE_DECL(Location,Point)

    /// The x value of a pre-translated point.
    NUMBER X()   const;

    /// The y value of a pre-translated point.
    NUMBER Y()   const;

private:
    NUMBER _x;
    NUMBER _y;
};

/// \class CommonParserPointLocationParticle
///
/// Identifies the beginning of a new line. In left-justified (aka flush-left, ragged-right) text, 
/// this is synonymous with a Return to the start of the previous line and a Relative movement of 
/// Rise = -1 x line height, however other justifications can only predict the Rise; the actual 
/// Advance computation needs to be left to the Sink.  Semantics will tell the Sink if the break 
/// is at a paragraph boundary.
/// 
class  CommonParserLineBreakLocationParticle: public CommonParserLocationParticle
{
public:
    /// The default constructor.
    CommonParserLineBreakLocationParticle();

    ATOM_PARTICLE_DECL(Location,LineBreak)
};

/// \enum CommonParserSemanticType
///
/// The semantic of the location.
/// 
enum CommonParserSemanticType {
    /// Normal advance.
    CommonParserSemanticTypeNormal = 0x0001, 
    /// Begin a paragraph (implicitly also begin a line)
    CommonParserSemanticTypeParagraph = 0x0002,
    /// Begin a line
    CommonParserSemanticTypeLine = 0x0004,
    /// Begin a tab column
    CommonParserSemanticTypeTabColumn = 0x0008,
    /// Begin a table
    CommonParserSemanticTypeTable = 0x0010,       
    /// End a table        
    CommonParserSemanticTypeEndTable = 0x0020, 
    /// Begin a table row 
    CommonParserSemanticTypeRow = 0x0040,   
    /// Begin a table cell  
    CommonParserSemanticTypeCell = 0x0080, 
    /// Begins a Superscript
    CommonParserSemanticTypeSuperscript = 0x0100, 
    /// Ends a Superscript
    CommonParserSemanticTypeEndSuperscript = 0x0200,
    /// Begins a Subscript
    CommonParserSemanticTypeSubscript = 0x0400, 
    /// Ends a Subscript
    CommonParserSemanticTypeEndSubscript = 0x0800, 
    /// Begins an inline "block" of more complex text
    CommonParserSemanticTypeInlineBlock = 0x1000, 
    /// Ends an inline "block" of more complex text
    CommonParserSemanticTypeEndInlineBlock = 0x2000,
    /// Begins a new column of text
    CommonParserSemanticTypeFlowColumn = 0x4000, 
};

/// \class CommonParserLocation
///
/// This describes any change in location of the rendered text.  It is assumed that the Sink 
/// is maintaining a "current" location as determined by each TextRun's extent: that is, as the 
/// Sink renders each TextRun, it updates a position that indicates where the next TextRun will be
/// rendered.  The descriptions here modify the "current" location immediately prior to the 
/// rendering of the current Contents.
/// 
class  CommonParserLocation
{
public:
    /// Describes the nature of the location change.
    virtual CommonParserSemanticType Semantics() const = 0;

    /// Zero or more operations to effect the location change.
    virtual CommonParserLocationParticle* Operations() const = 0;
};

/**********************************************************************
 *
 *  TEXT RUN "STRUCTURE"
 *
 **********************************************************************/
/// \enum CommonParserShapeType
///
/// The shape of the whole text.
/// 
enum CommonParserShapeType
{
    /// markup is describing a flowing sequence, a la HTML <span>
    CommonParserShapeTypeFlow,      
    /// markup is describing a rectangular block, a la HTML <div>
    CommonParserShapeTypeBlock     
};

/// \class CommonParserStructure
///
/// Provides structural information of the markup.  Of more interest to converter sinks than 
/// rendering sinks.
/// 
class CommonParserStructure
{
public:
    /// Current depth within the markup.
    virtual int Depth() const = 0;

    /// Pointer to an outer CommonParserStructure (with Depth()-1)
    virtual CommonParserStructure* Outer() const = 0;

    /// What is the "shape" of the run?  Does it flow and wrap, or... ?
    virtual CommonParserShapeType Shape() const = 0;

    /// Is selection considered continuous with previous run?
    virtual bool Continuous() const = 0;
};

/**********************************************************************
 *
 *  TEXT RUN INTERFACE
 *
 **********************************************************************/

/// \class CommonParserTextRun
///
/// A text run describes a consecutive sequence of characters all sharing a common style, graphical
/// transformation, and/or location, as reported by the markup.
/// 
class CommonParserTextRun 
{
public:
    /// Structural information about the markup being parsed.
    virtual const CommonParserStructure* Structure() const = 0;

    /// The actual style characteristics in effect for the 
    /// text run, and what's changed from the previous run.
    virtual const CommonParserStyleChange* Style() const = 0;

    /// The transformation in effect for the text run, and
    /// the component transforms.
    virtual const CommonParserTransformChange* Transform() const = 0;

    /// The location of the indicated contents.
    virtual const CommonParserLocation* Location() const = 0;

    /// The contents of the text run described.
    virtual const CommonParserStRange Contents() const = 0;
};

/**********************************************************************
 *
 *  MARKUP PARSER INTERFACE
 *
 **********************************************************************/
class CommonParserGenerator; // forward declaration
/// \class CommonParserParser
///
/// A Markup Parser
/// 
class  CommonParserParser
{
public:
    /// Parse a markup string by creating an "environment" with all the ambient settings, then 
    /// combine that with a string to parse, and give it to this method.
    virtual CommonParserStatus Parse(const CommonParserStRange sMarkup,
                                     CommonParserEnvironment* pEnv) = 0;

    /// Gives you a pointer to its generator, if any.
    /// CAN BE NULL.
    virtual CommonParserGenerator* GetGenerator() = 0;
};

/**********************************************************************
 *
 *  ABANDONMENT INTERFACE
 *
 **********************************************************************/
/// \class CommonParserAbandonment
///
/// The status when we abort the parse process.
/// 
class  CommonParserAbandonment
{
public:
    /// Indicates the reason for abandonment.
    virtual const CommonParserStatus   Reason()    = 0;

    /// The string being parsed.
    virtual const CommonParserStRange& Markup()    = 0;

    /// Local context of the string being parsed.
    /// (the line on which it occurs, maybe including adjacent lines)
    virtual const CommonParserStRange& Context()   = 0;

    /// The specific location where the error Occurred.
    virtual const CommonParserStRange& Position()  = 0;
};


/**********************************************************************
 *
 *  SINK INTERFACE
 *
 **********************************************************************/
/// \class CommonParserSinkStateType
///
/// The type of the sink state.
/// 
enum CommonParserSinkStateType {
    /// Before Initialize, and/or after Terminate
    CommonParserSinkStateTypeWaiting,      
    /// After Initialize, before Terminate or Abandon
    CommonParserSinkStateTypeInitialized,  
    /// After Abandon, before Terminate
    CommonParserSinkStateTypeAbandoned,   
};

/// \class CommonParserAbandonment
///
/// The output of atom.
/// 
class  CommonParserSink {
public:

    /// Allows the sink to report on its current state.
    virtual CommonParserSinkStateType SinkState() = 0;

    /// This always starts the Parsing event stream
    /// Entry condition: SinkState() == keWaiting.
    /// Exit condition if successful: SinkState() == keInitialized
    /// Returns:
    ///   keOk
    ///   keNotReady: SinkState() is not keWaiting
    virtual CommonParserStatus Initialize(CommonParserEnvironment*) = 0;

    /// You get zero or more of these, based on the string.
    /// Entry condition: SinkState() == keInitialized
    /// Exit condition: SinkState() == keInitialized
    /// Returns:
    ///   keOk:
    ///   keContinue:
    ///   keAbandoned: request abandonment.
    ///   keNotReady: SinkState() is not keInitialized
    virtual CommonParserStatus TextRun(CommonParserTextRun*,CommonParserEnvironment*) = 0;

    /// An error is detected by Parser.  About to terminate; info on what's wrong.
    /// Entry condition: SinkState() == keInitialized
    /// Exit condition: SinkState() == keAbandoned
    /// Returns:
    ///   keContinue: attempt to continue parsing?
    ///   keAbandoned: abandoned.
    ///   other: informational return.
    virtual CommonParserStatus Abandon(CommonParserAbandonment*,CommonParserEnvironment*) = 0;

    /// This always ends the event stream.
    /// Entry condition: SinkState() == keInitialized or == keAbandoned
    /// Exit condition: SinkState() == keWaiting
    /// Returns:
    ///   keOk:
    ///   keNotReady: SinkState() is not keInitialized or keAbandoned
    virtual CommonParserStatus Terminate(CommonParserEnvironment*) = 0;

    /// Gives you a pointer to its generator, if any.
    /// THIS CAN RETURN NULL (say, for an app-hosted sink)
    virtual CommonParserGenerator* GetGenerator() = 0;
};



/**********************************************************************
 *
 *  CONSTRUCTION INTERFACE
 *
 **********************************************************************/
/// \class CommonParserGenerator
///
/// The parser implements this tiny object, a class factory and lifetime manager for that parser 
/// type. Theoretically, this is a singleton in the module that the parser resides. When that
/// module is loaded, the generator registers itself into the CommonParserUniverse (via the implementation's 
/// constructor) and unregisters itself when unloaded.
/// 
class  CommonParserGenerator
{
public:
    /// The name of the markup this parser represents
    /// such as "SVG" or "RTF" or ...
    virtual const CommonParserStRange Name() const = 0;

    /// Documentation of the parser/generator (for 
    /// version reporting, etc.)  A human-readable string.
    virtual const CommonParserStRange Description() const  = 0;

    /// Creates an instance to a new Parser
    virtual CommonParserStatus Create(CommonParserParser**) = 0;

    /// Takes a pointer to an existing parser and destroys it.
    virtual CommonParserStatus Destroy(CommonParserParser*) = 0;

    /// Inexpensive way of determining if there's an associated sink.
    virtual bool HasSink() const = 0;

    /// And when you actually want one, this is how to get it.
    /// Creates an instance to a new Sink
    virtual CommonParserStatus Create(CommonParserSink**) = 0;

    // Takes a pointer to an existing sink and destroys it.
    virtual CommonParserStatus Destroy(CommonParserSink*) = 0;

    /// The universe is destroyed so that this generator 
    /// needn't do unregistration.
    virtual CommonParserStatus RegisterNull()  = 0;
};


/// \class CommonParserUniverse
///
/// The hosting application implements this interface to manage the various parsers. Parsers (or 
/// more precisely, their Generators) use this to register themselves. The application then queries
/// to utilize whatever parsers are registered.
/// 
class  CommonParserUniverse
{
public:
    /// Registers a Parser's Generator, used by the parsing module
    /// when introduced to the universe.
    virtual CommonParserStatus Register(CommonParserGenerator*) = 0;

    /// Unregisters a Parser's Generator.
    virtual CommonParserStatus Unregister(CommonParserGenerator*) = 0;

    /// How many parser/generators are registered?
    virtual int RegisteredCount() = 0;

    /// Gets a parser generator (by position in registration list)
    /// to allow the application to begin a parsing operation.
    /// 0 <= iIndex < RegisteredCount();
    ///
    /// Note: the iIndex is NOT a key, as registration may change
    /// the order of Parser Generators... USE ONLY Name() TO
    /// GET A PERSISTENT KEY for any specific Parser Generator.
    virtual CommonParserGenerator* GetGenerator(int iIndex) = 0;

    /// Same as above, but indexed off the CommonParserGenerator::Name()
    /// method.  USING Name() IS THE ONLY ASSURED WAY OF GETTING 
    /// THE RIGHT PARSER.
    virtual CommonParserGenerator* GetGenerator(const CommonParserStRange& sName) = 0;
};

// Implementing platforms need to implement this 
// one standard method.
CommonParserUniverse* BigBang();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_GLOBALS_H
