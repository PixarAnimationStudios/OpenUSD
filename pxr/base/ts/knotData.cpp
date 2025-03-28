//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/knotData.h"
#include "pxr/base/ts/valueTypeDispatch.h"

PXR_NAMESPACE_OPEN_SCOPE


Ts_KnotData::Ts_KnotData()
    : time(0.0),
      preTanWidth(0.0),
      postTanWidth(0.0),
      nextInterp(TsInterpHeld),
      curveType(TsCurveTypeBezier),
      dualValued(false)
{
}

namespace
{
    template <typename T>
    struct _DataCreator
    {
        void operator()(Ts_KnotData **dataOut)
        {
            *dataOut = new Ts_TypedKnotData<T>();
        }
    };

    template <typename T>
    struct _ProxyCreator
    {
        void operator()(Ts_KnotData *data, Ts_KnotDataProxy **proxyOut)
        {
            *proxyOut = new Ts_TypedKnotDataProxy<T>(
                static_cast<Ts_TypedKnotData<T>*>(data));
        }
    };
}

// static
Ts_KnotData* Ts_KnotData::Create(const TfType valueType)
{
    Ts_KnotData *result = nullptr;
    TsDispatchToValueTypeTemplate<_DataCreator>(
        valueType, &result);
    return result;
}

bool Ts_KnotData::operator==(const Ts_KnotData &other) const
{
    return time == other.time
        && preTanWidth == other.preTanWidth
        && postTanWidth == other.postTanWidth
        && dualValued == other.dualValued
        && nextInterp == other.nextInterp
        && curveType == other.curveType;
}

// static
std::unique_ptr<Ts_KnotDataProxy>
Ts_KnotDataProxy::Create(Ts_KnotData *data, const TfType valueType)
{
    Ts_KnotDataProxy *result = nullptr;
    TsDispatchToValueTypeTemplate<_ProxyCreator>(
        valueType, data, &result);
    return std::unique_ptr<Ts_KnotDataProxy>(result);
}

Ts_KnotDataProxy::~Ts_KnotDataProxy() = default;


PXR_NAMESPACE_CLOSE_SCOPE
