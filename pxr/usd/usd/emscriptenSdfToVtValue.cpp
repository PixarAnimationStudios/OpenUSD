#include "pxr/usd/usd/emscriptenSdfToVtValue.h"

#include "pxr/base/tf/wrapTokenJs.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/usd/sdf/wrapAssetPathJs.h"
#include "pxr/usd/sdf/types.h"


#include <map>
#include <iostream>

////////////////////////////////////////////////////////////////////////
// Element sub-type.  e.g. GfVec3f -> float.
template <class T, class Enable = void>
struct Vt_GetSubElementType { typedef T Type; };

template <class T>
struct Vt_GetSubElementType<
    T, typename std::enable_if<pxr::GfIsGfVec<T>::value ||
                               pxr::GfIsGfMatrix<T>::value ||
                               pxr::GfIsGfQuat<T>::value ||
                               pxr::GfIsGfRange<T>::value>::type> {
    typedef typename T::ScalarType Type;
};

template <>
struct Vt_GetSubElementType<pxr::GfRect2i> { typedef int Type; };

///////////////////////////////////////////////////////////////////////
// Element sub-type dimension. e.g. GfVec3f -> 3
template <class T, class Enable = void>
struct Vt_GetSubElementDimension { static const int dimension = 1; };

template <class T>
struct Vt_GetSubElementDimension<
    T, typename std::enable_if<pxr::GfIsGfVec<T>::value ||
                               pxr::GfIsGfMatrix<T>::value ||
                               pxr::GfIsGfQuat<T>::value ||
                               pxr::GfIsGfRange<T>::value>::type> {
    static const int dimension = T::dimension; ;
};

template <>
struct Vt_GetSubElementDimension<pxr::GfRect2i> { static const int dimension = 2; };

template <typename T, int dimension>
class AssignArray {
    public:
        static void assignVtArray(pxr::VtArray<T> &v, const emscripten::val& jsVal, size_t len) {
            if (len > 0 && jsVal[0].isArray() && jsVal[0]["length"].as<size_t>() == Vt_GetSubElementDimension<T>::dimension) {
                v.reserve(len);

                for (size_t i = 0; i < len; ++i) {
                    v.push_back(jsVal[i].as<T>());
                }             
            }
            else {
                if (len % Vt_GetSubElementDimension<T>::dimension != 0) {
                    throw std::runtime_error("Incorrect dimension in assignment of array");
                }
                v.reserve(len / Vt_GetSubElementDimension<T>::dimension);

                size_t k = 0;
                for (size_t i = 0; i < len; i += Vt_GetSubElementDimension<T>::dimension) {
                    T newValue;
                    for (size_t j = 0; j < Vt_GetSubElementDimension<T>::dimension; j++) {
                        newValue[j] = jsVal[k++].as<typename Vt_GetSubElementType<T>::Type>();
                    }
                    v.push_back(newValue);
                }
            }
        }
};

template <typename T>
class AssignArray<T,1> {
    public:
        static void assignVtArray(pxr::VtArray<T> &v, const emscripten::val& jsVal, size_t len) {
            v.reserve(len);

            for (size_t i = 0; i < len; ++i) {
                v.push_back(jsVal[i].as<T>());
            }
        }
};

template <typename T>
pxr::VtValue createVtArray(const emscripten::val& jsVal) {
    if (!jsVal.isArray()) {
        std::cerr << "Failed to create a VtArray: Input is not an array!" << std::endl;
    }
    const size_t len = jsVal["length"].as<size_t>();
    pxr::VtArray<T> v;

    AssignArray<T, Vt_GetSubElementDimension<T>::dimension>::assignVtArray(v, jsVal, len);

    return pxr::VtValue(v);
}

#define EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(SDF_TYPENAME, TYPE) \
    {#SDF_TYPENAME, [](const emscripten::val& jsVal) { return pxr::VtValue(jsVal.as<TYPE>()); }}, \
    {"VtArray<" #SDF_TYPENAME ">", &createVtArray<TYPE>},

std::map<std::string, SdfToVtValueFunc> sdfToVtValueFuncs {
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(bool, bool)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(int, int)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(float, float)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(double, double)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(TfToken, pxr::TfToken)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(SdfAssetPath, pxr::SdfAssetPath)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(GfVec2f, pxr::GfVec2f)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(GfVec2d, pxr::GfVec2d)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(GfVec3f, pxr::GfVec3f)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(GfVec3d, pxr::GfVec3d)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(GfVec4f, pxr::GfVec4f)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(GfVec4d, pxr::GfVec4d)
    EMSCRIPTEN_REGISTER_SDFTYPE_CONVERSION(SdfSpecifier, pxr::SdfSpecifier)
};

SdfToVtValueFunc* UsdJsToSdfType(const std::string &targetType) {
    auto sdfToVtValue = sdfToVtValueFuncs.find(targetType);
    return sdfToVtValue != sdfToVtValueFuncs.end() ? &sdfToVtValue->second : NULL;
}

SdfToVtValueFunc* UsdJsToSdfType(pxr::SdfValueTypeName const &targetType) {
    return UsdJsToSdfType(targetType.GetType().GetTypeName());
}

pxr::VtValue GetVtValueFromEmscriptenVal(const emscripten::val& value, pxr::SdfValueTypeName const &targetType, bool* const success) {
    SdfToVtValueFunc* sdfToValue = UsdJsToSdfType(targetType);
    if (success != NULL) {
        *success = false;
    }
    if (sdfToValue != NULL) {
      if (success != NULL) {
          *success = true;
      }
      return (*sdfToValue)(value);
    }
    return pxr::VtValue();
}
