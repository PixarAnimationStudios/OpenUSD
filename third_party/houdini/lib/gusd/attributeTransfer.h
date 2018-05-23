//
// Copyright 2017 Pixar
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
#ifndef __GUSD_ATTRIBUTETRANSFER_H__
#define __GUSD_ATTRIBUTETRANSFER_H__


//std
#include <cmath>

// pxr
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include "USD_Utils.h"

// houdini
#include <OP/OP_Network.h>
#include <CH/CH_Channel.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace GusdUSD_AttributeTransfer
{

// =====================
// Parm setter templates
// =====================

template<typename T>
struct is_usd_vec_t : std::integral_constant<bool, GfIsGfVec<typename std::decay<T>::type>::value> {};

template<typename T>
struct is_arithmetic_t : std::integral_constant<bool, std::is_arithmetic<typename std::decay<T>::type>::value> {};

template<typename T>
struct is_string_convertible_t : std::integral_constant<bool, std::is_convertible<T, std::string>::value> {};

#define SFINAE_ARG( flag ) typename std::enable_if<flag>::type * = nullptr

template<typename T>
void setValue(const double time, PRM_Parm& parm, T && vector, bool setKey=false, SFINAE_ARG( is_usd_vec_t<T>::value ) )
{
    constexpr size_t dimensions = std::decay<T>::type::dimension;
    const size_t parmVectorSize = parm.getVectorSize();

    for(size_t index = 0; index < std::min(dimensions, parmVectorSize); ++index)
    {
        parm.setValue(time, vector[index], true, index);
        if (setKey)
        {
            parm.setKey(time, index);
        }
    }
}

template<typename T>
void setValue( const double time, PRM_Parm & parm, T && value, bool setKey=false, SFINAE_ARG( is_arithmetic_t<T>::value ) )
{
    const int index = 0;
    parm.setValue( time, std::forward<T>( value ), false, index);
    if (setKey)
    {
        parm.setKey(time, index);
    }
}

template<typename T>
void setValue( const double time, PRM_Parm & parm, T && value, bool setKey=false, SFINAE_ARG( is_string_convertible_t<T>::value ) )
{
    const int index = 0;
    std::string valueStr = value;
    parm.setValue( time, valueStr.c_str(), CH_STRING_LITERAL, false, index);
    if (setKey)
    {
        parm.setKey(time, index);
    }
}

template<typename T>
void setValue( const double time, PRM_Parm & parm, T && value, bool setKey=false, typename std::enable_if<std::is_same<T, SdfAssetPath>::value>::type * = nullptr )
{
    const int index = 0;

    const std::string& path = value.GetAssetPath();
    parm.setValue( time, path.c_str(), CH_STRING_LITERAL, false, index);
    if (setKey)
    {
        parm.setKey(time, index);
    }
}


// =======================
// Transfer Usd -> Houdini
// =======================

template<typename T, typename F>
void transferAttribute(const UsdAttribute& attribute, PRM_Parm& parm, double fps, F conversionFunc)
{
    if (!attribute)
    {
        return;
    }

    std::vector<double> timeCodes;
    attribute.GetTimeSamples(&timeCodes);

    bool animatedAttribute = timeCodes.size() > 0;
    if (!animatedAttribute)
    {
        timeCodes.push_back(UsdTimeCode::Default().GetValue());
    }

    for (auto& timeCode: timeCodes)
    {
        fpreal time = GusdUSD_Utils::GetNumericHoudiniTime(timeCode, fps);

        T value;
        attribute.Get(&value, timeCode);
        setValue(time, parm, conversionFunc(value), animatedAttribute);
    }
}


template<typename T>
void transferAttribute(const UsdAttribute& attribute, PRM_Parm& parm, double fps)
{
    // Using default conversion function which is simply returning the input reference.
    transferAttribute<T>(attribute, parm, fps, [&](T input)->T{ return input; });
}

// =======================
// Transfer Houdini -> Usd
// =======================


template<typename T, typename TO>
void transferVectorAttribute(const PRM_Parm& parm, UsdAttribute& attribute, const float time, const UsdTimeCode timeCode)
{
    if (!attribute)
    {
        std::cout << "Invalid Usd attribute for parm " << parm.getLabel() << "\n";
        return;
    }

    T valueIn;
    TO valueOut;

    for (size_t index = 0; index < TO::dimension; ++index)
    {
        parm.getValue(time, valueIn, index, 0);
        valueOut[index] = valueIn;
    }

    attribute.Set(valueOut, timeCode);
}

template<typename T, typename TO>
TO getVector(const PRM_Parm& parm, const float time)
{
    T value;
    TO result;

    const size_t parmVectorSize = parm.getVectorSize();

    for (size_t index = 0; index < TO::dimension; ++index)
    {
        // Use last value if the parm has less values than the output vector
        // E.g. parm = (1)     , vec3Result = (1,1,1)
        //      parm = (1,7)   , vec3Result = (1,7,7)
        //      parm = (1,7,4) , vec3Result = (1,7,4)
        if (index < parmVectorSize)
        {
            parm.getValue(time, value, index, 0);
        }
        result[index] = value;
    }

    return result;
}

template<typename T>
T getValue(const PRM_Parm& parm, const float time, int index, SFINAE_ARG( is_arithmetic_t<T>::value ))
{
    T value;
    parm.getValue(time, value, index, 0);
    return value;
}

template<typename T>
T getValue(const PRM_Parm& parm, const float time, int index, typename std::enable_if<std::is_same<T, UT_String>::value>::type * = nullptr )
{
    T value;
    parm.getValue(time, value, index, 0, 0);
    return value;
}

template<typename T, typename F>
void transferAttributeWithConversion(const PRM_Parm& parm, UsdAttribute& attribute, const float time, const UsdTimeCode timeCode, F conversionFunc)
{
    if (!attribute)
    {
        std::cout << "Invalid attribute for parm " << parm.getLabel() << "\n";
        return;
    }

    auto value = getValue<T>(parm, time, 0);
    attribute.Set(conversionFunc(value), timeCode);
}

template<typename T>
void transferAttribute(const PRM_Parm& parm, UsdAttribute& attribute, const float time, const UsdTimeCode timeCode)
{
    // Using default conversion function which is simply returning the input reference.
    transferAttributeWithConversion<T>(parm, attribute, time, timeCode, [&](T input)->T{ return input; });
}

template<typename T, typename TO>
void transferAttribute(const PRM_Parm& parm, UsdAttribute& attribute, const float time, const UsdTimeCode timeCode)
{
    // Using default conversion function which is simply returning the input reference.
    transferAttributeWithConversion<T>(parm, attribute, time, timeCode, [&](T input)->TO{ return (TO)input; });
}

} // namespace GusdUSD_AttributeTransfer

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_ATTRIBUTETRANSFER_H__
