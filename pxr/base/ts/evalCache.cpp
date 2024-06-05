//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/evalCache.h"

#include "pxr/base/ts/data.h"
#include "pxr/base/ts/keyFrameUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

// static
Ts_UntypedEvalCache::SharedPtr
Ts_UntypedEvalCache::New(const TsKeyFrame &kf1, const TsKeyFrame &kf2)
{
    return Ts_GetKeyFrameData(kf1)->CreateEvalCache(Ts_GetKeyFrameData(kf2));
}

VtValue
Ts_UntypedEvalCache::EvalUncached(const TsKeyFrame &kf1,
                                    const TsKeyFrame &kf2,
                                    TsTime time)
{
    return Ts_GetKeyFrameData(kf1)->
        EvalUncached(Ts_GetKeyFrameData(kf2), time);
}

VtValue
Ts_UntypedEvalCache::EvalDerivativeUncached(const TsKeyFrame &kf1,
                                              const TsKeyFrame &kf2,
                                              TsTime time)
{
    return Ts_GetKeyFrameData(kf1)->
        EvalDerivativeUncached(Ts_GetKeyFrameData(kf2), time);
}


////////////////////////////////////////////////////////////////////////
// ::New implementations for Ts_EvalCache GfQuat specializations

std::shared_ptr<Ts_EvalCache<GfQuatd, true> >
Ts_EvalCache<GfQuatd, true>::New(const TsKeyFrame &kf1,
    const TsKeyFrame &kf2)
{
    return static_cast<const Ts_TypedData<GfQuatd>*>(
        Ts_GetKeyFrameData(kf1))->
            CreateTypedEvalCache(Ts_GetKeyFrameData(kf2));
}

std::shared_ptr<Ts_EvalCache<GfQuatf, true> >
Ts_EvalCache<GfQuatf, true>::New(const TsKeyFrame &kf1,
    const TsKeyFrame &kf2)
{
    return static_cast<const Ts_TypedData<GfQuatf>*>(
        Ts_GetKeyFrameData(kf1))->
            CreateTypedEvalCache(Ts_GetKeyFrameData(kf2));
}

PXR_NAMESPACE_CLOSE_SCOPE
