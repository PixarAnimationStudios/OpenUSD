//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/binary.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/valueTypeDispatch.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stl.h"

#include <map>
#include <utility>
#include <limits>
#include <memory>
#include <cstring>

PXR_NAMESPACE_OPEN_SCOPE


// Verify that type sizes are the same on all platforms.
static_assert(sizeof(double) == 8);
static_assert(sizeof(float) == 4);
static_assert(sizeof(GfHalf) == 2);

// Byte-oriented I/O.  This works because we assume every platform is
// little-endian, and the bit patterns of arithmetic types are always the same.
// See arch/assumptions.cpp.  For GfHalf, we control the bit pattern.
//
// The explicit types in calls are technically unnecessary.  They are to help
// document the format.

template <typename T>
static void _WriteBytes(
    std::vector<uint8_t>* const buf,
    const T &value)
{
    const size_t origSize = buf->size();
    buf->resize(origSize + sizeof(T));
    memcpy(buf->data() + origSize, &value, sizeof(T));
}

template <typename T>
static bool _ReadBytes(
    const uint8_t** const readPtr,
    size_t* const remain,
    T* const valueOut)
{
    if (*remain < sizeof(T))
    {
        TF_RUNTIME_ERROR("Unexpected end of data while parsing");
        return false;
    }

    memcpy(valueOut, *readPtr, sizeof(T));

    *readPtr += sizeof(T);
    *remain -= sizeof(T);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// WRITE TO BINARY DATA

namespace
{
    template <typename T>
    struct _BinaryDataWriter
    {
        void operator()(
            const Ts_SplineData &dataIn,
            const bool isHermite,
            std::vector<uint8_t>* const buf)
        {
            const Ts_TypedSplineData<T> &data =
                static_cast<const Ts_TypedSplineData<T>&>(dataIn);

            // Knot count.
            if (data.knots.size() > std::numeric_limits<uint32_t>::max())
            {
                TF_CODING_ERROR("Huge number of spline knots, cannot write");
                return;
            }
            _WriteBytes<uint32_t>(
                buf, static_cast<uint32_t>(data.knots.size()));

            for (const Ts_TypedKnotData<T> knot : data.knots)
            {
                // Flag byte:
                // Bit 0: whether dual-valued.
                // Bits 1-2: next segment interpolation mode.
                // Bit 3: curve type.
                // Bit 4: whether pre-tangent is in Maya form.
                // Bit 5: whether post-tangent is in Maya form.
                uint8_t flagByte = knot.dualValued;
                flagByte |= static_cast<uint8_t>(knot.nextInterp) << 1;
                flagByte |= static_cast<uint8_t>(knot.curveType) << 3;
                flagByte |= knot.preTanMayaForm << 4;
                flagByte |= knot.postTanMayaForm << 5;
                _WriteBytes<uint8_t>(buf, flagByte);

                // Knot time and value.
                _WriteBytes<double>(buf, knot.time);
                _WriteBytes<T>(buf, knot.value);

                // Pre-value, if dual-valued.
                if (knot.dualValued)
                {
                    _WriteBytes<T>(buf, knot.preValue);
                }

                // Tangent widths, if not Hermite.
                if (!isHermite)
                {
                    _WriteBytes<double>(buf, knot.preTanWidth);
                    _WriteBytes<double>(buf, knot.postTanWidth);
                }

                // Tangent slopes or heights.
                // Interpretation will be governed by the Maya-form flags.
                _WriteBytes<T>(buf, knot.preTanSlope);
                _WriteBytes<T>(buf, knot.postTanSlope);
            }
        }
    };
}

// static
void Ts_BinaryDataAccess::GetBinaryData(
    const TsSpline &spline,
    std::vector<uint8_t>* const buf,
    const std::unordered_map<TsTime, VtDictionary>** const customDataOut)
{
    // If spline is empty, output trivial data: empty blob, empty customData.
    // In practice this won't be hit because our caller will inline empty
    // splines.
    if (!spline._data)
    {
        static const std::unordered_map<TsTime, VtDictionary> emptyCustomData;
        *customDataOut = &emptyCustomData;
        return;
    }

    const Ts_SplineData &data = *(spline._data.get());
    const TfType valueType = spline.GetValueType();
    const bool hasLoops = spline.HasInnerLoops();
    const bool isHermite = (spline.GetCurveType() == TsCurveTypeHermite);

    // Buffer size:
    // Header: 2
    // Extraps: 2 * sizeof(double)
    // Loop params: sizeof(LoopParams)
    // Knots: N * sizeof(Ts_TypedKnotData<T>)
    const size_t bufSize =
        2
        + 2 * sizeof(double)
        + sizeof(TsLoopParams)
        + data.times.size() * data.GetKnotStructSize();
    buf->reserve(bufSize);

    // Map of value types to descriptors.
    static const std::map<TfType, uint8_t> typeMap = {
        { TfType(),             0 },    // Can be valid with no knots.
        { Ts_GetType<double>(), 1 },
        { Ts_GetType<float>(),  2 },
        { Ts_GetType<GfHalf>(), 3 } };

    const uint8_t typeDescriptor = TfMapLookupByValue(typeMap, valueType, 0);

    // Header byte 1:
    // Bits 0-3: version.  Must exist in all versions.
    // Bits 4-5: value type.
    // Bit 6: whether time-valued.
    // Bit 7: curve type.
    uint8_t headerByte = GetBinaryFormatVersion();
    headerByte |= typeDescriptor << 4;
    headerByte |= data.timeValued << 6;
    headerByte |= static_cast<uint8_t>(data.curveType) << 7;
    _WriteBytes<uint8_t>(buf, headerByte);

    // Header byte 2:
    // Bits 0-2: pre-extrapolation mode.
    // Bits 3-5: post-extrapolation mode.
    // Bit 6: whether inner loops enabled.
    headerByte = static_cast<uint8_t>(data.preExtrapolation.mode);
    headerByte |= static_cast<uint8_t>(data.postExtrapolation.mode) << 3;
    headerByte |= hasLoops << 6;
    _WriteBytes<uint8_t>(buf, headerByte);

    // For each sloped extrapolation, write slope.
    if (data.preExtrapolation.mode == TsExtrapSloped)
    {
        _WriteBytes<double>(buf, data.preExtrapolation.slope);
    }
    if (data.postExtrapolation.mode == TsExtrapSloped)
    {
        _WriteBytes<double>(buf, data.postExtrapolation.slope);
    }

    // Write inner loop params, if applicable.
    if (hasLoops)
    {
        const TsLoopParams &lp = data.loopParams;
        _WriteBytes<double>(buf, lp.protoStart);
        _WriteBytes<double>(buf, lp.protoEnd);
        _WriteBytes<int32_t>(buf, lp.numPreLoops);
        _WriteBytes<int32_t>(buf, lp.numPostLoops);
        _WriteBytes<double>(buf, lp.valueOffset);
    }

    // Write knot data, if any.  This is value-type-specific.
    if (valueType)
    {
        TsDispatchToValueTypeTemplate<_BinaryDataWriter>(
            valueType, data, isHermite, buf);
    }

    // Provide a diagnostic if we under-reserved.
    TF_VERIFY(buf->size() <= bufSize);

    // Custom data is returned separately.  Our caller knows how to serialize
    // dictionaries, so we don't need to.
    *customDataOut = &(data.customData);
}

////////////////////////////////////////////////////////////////////////////////
// READ FROM BINARY DATA

#define READ(dest)                                   \
    if (!_ReadBytes(readPtr, remain, dest))          \
    {                                                \
        *ok = false;                                 \
        return;                                      \
    }

namespace
{
    template <typename T>
    struct _BinaryDataReaderV1
    {
        void operator()(
            Ts_SplineData* const dataIn,
            const bool isHermite,
            const uint8_t** const readPtr,
            size_t* const remain,
            bool* const ok)
        {
            Ts_TypedSplineData<T>* const data =
                static_cast<Ts_TypedSplineData<T>*>(dataIn);

            *ok = true;

            // Knot count.
            uint32_t knotsRemain = 0;
            READ(&knotsRemain);

            while (knotsRemain)
            {
                knotsRemain--;

                Ts_TypedKnotData<T> knot;

                // Flag byte.
                uint8_t flagByte = 0;
                READ(&flagByte);
                knot.dualValued = flagByte & 0x01;
                knot.nextInterp =
                    static_cast<TsInterpMode>((flagByte & 0x06) >> 1);
                knot.curveType =
                    static_cast<TsCurveType>((flagByte & 0x08) >> 3);
                knot.preTanMayaForm = flagByte & 0x10;
                knot.postTanMayaForm = flagByte & 0x20;

                // Knot time and value.
                READ(&knot.time);
                READ(&knot.value);

                // Pre-value, if dual-valued.
                if (knot.dualValued)
                {
                    READ(&knot.preValue);
                }

                // Tangent widths, if not Hermite.
                if (!isHermite)
                {
                    READ(&knot.preTanWidth);
                    READ(&knot.postTanWidth);
                }

                // Tangent slopes or heights.
                // Interpretation will be governed by the Maya-form flags.
                READ(&knot.preTanSlope);
                READ(&knot.postTanSlope);

                data->times.push_back(knot.time);
                data->knots.push_back(knot);
            }
        }
    };
}

#undef READ
#define READ(dest)                                   \
    if (!_ReadBytes(&readPtr, &remain, dest))        \
    {                                                \
        return {};                                   \
    }

// static
TsSpline Ts_BinaryDataAccess::_ParseV1(
    const std::vector<uint8_t> &buf,
    std::unordered_map<TsTime, VtDictionary> &&customData)
{
    const uint8_t *readPtr = buf.data();
    size_t remain = buf.size();

    // Map of value-type descriptors to value types.
    static const std::map<uint8_t, TfType> typeMap = {
        { 0, Ts_GetType<double>() },    // Value type unspecified.
        { 1, Ts_GetType<double>() },
        { 2, Ts_GetType<float>()  },
        { 3, Ts_GetType<GfHalf>() } };

    // Header byte 1.
    uint8_t headerByte = 0;
    READ(&headerByte);
    const uint8_t typeDescriptor = (headerByte & 0x30) >> 4;
    const TfType valueType =
        TfMapLookupByValue(typeMap, typeDescriptor, TfType());
    if (!valueType)
    {
        TF_RUNTIME_ERROR("Bad spline type descriptor");
        return {};
    }

    // Now that we know value type, create typed SplineData.
    std::unique_ptr<Ts_SplineData> data(Ts_SplineData::Create(valueType));

    // Read flags.
    data->isTyped = (typeDescriptor != 0);
    data->timeValued = headerByte & 0x40;
    data->curveType = static_cast<TsCurveType>((headerByte & 0x80) >> 7);
    const bool isHermite = (data->curveType == TsCurveTypeHermite);

    // Header byte 2.
    READ(&headerByte);
    data->preExtrapolation.mode =
        static_cast<TsExtrapMode>(headerByte & 0x07);
    data->postExtrapolation.mode =
        static_cast<TsExtrapMode>((headerByte & 0x18) >> 3);
    const bool hasLoops = headerByte & 0x40;

    // For each sloped extrapolation, read slope.
    if (data->preExtrapolation.mode == TsExtrapSloped)
    {
        READ(&(data->preExtrapolation.slope));
    }
    if (data->postExtrapolation.mode == TsExtrapSloped)
    {
        READ(&(data->postExtrapolation.slope));
    }

    // Read inner loop params, if present.
    if (hasLoops)
    {
        TsLoopParams* const lp = &(data->loopParams);
        READ(&(lp->protoStart));
        READ(&(lp->protoEnd));
        READ(&(lp->numPreLoops));
        READ(&(lp->numPostLoops));
        READ(&(lp->valueOffset));
    }

    // Read knot data, if any.  This is value-type-specific.
    if (valueType)
    {
        bool ok = false;
        TsDispatchToValueTypeTemplate<_BinaryDataReaderV1>(
            valueType, data.get(), isHermite, &readPtr, &remain, &ok);
        if (!ok)
        {
            return {};
        }
    }

    // Provide a diagnostic if we left any data unread.
    TF_VERIFY(remain == 0);

    // Move externally-parsed customData into SplineData.
    data->customData = std::move(customData);

    // Wrap SplineData in Spline.
    TsSpline spline;
    spline._data.reset(data.release());
    return spline;
}

#undef READ

// static
TsSpline Ts_BinaryDataAccess::CreateSplineFromBinaryData(
    const std::vector<uint8_t> &buf,
    std::unordered_map<TsTime, VtDictionary> &&customData)
{
    // Check for trivial data.
    if (buf.empty())
    {
        return TsSpline();
    }

    // Check version and parse.
    const uint8_t version = buf[0] & 0x0F;
    if (version == 1)
    {
        return _ParseV1(buf, std::move(customData));
    }
    else
    {
        // Bad version, or future version.  For a future version, caller should
        // have detected at a higher level that this data isn't something that
        // this software version is forward-compatible with.
        TF_CODING_ERROR("Unknown spline data version %u", version);
        return TsSpline();
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
