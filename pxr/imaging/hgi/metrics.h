//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HGI_METRICS_H
#define PXR_IMAGING_HGI_METRICS_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/api.h"

#include <chrono>
#include <atomic>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiMetrics
{
public:
    struct Summary {
        uint64_t packetId;
        uint64_t gpuElapsed;
        uint64_t gpuOccupied;
        uint64_t cpuElapsed;
    };
    
    HgiMetrics();
    
    virtual ~HgiMetrics();

    HGI_API
    void Reset();
    
    HGI_API
    std::vector<HgiMetrics::Summary>& GetLog();

    HGI_API
    void StartPacket();

    HGI_API
    void EndPacket();

    uint32_t GetActivePacketId() const { return _activePacketId; }

    HGI_API
    virtual uint64_t StartGPUEvent(uint32_t packetId, uint64_t id) = 0;

    HGI_API
    virtual void EndGPUEvent(uint32_t packetId, uint64_t id) = 0;

protected:
    #define NUM_PACKETS 8
    #define NUM_GPU_EVENTS 8
    
    struct GPUEvent {
        union {
            uint32_t tokens[2];
            uint64_t id;
        };
        uint64_t t0;
        uint64_t t1;
        bool merged;
    };

    struct Packet {
        uint32_t id;
        GPUEvent events[NUM_GPU_EVENTS];
        std::atomic<int32_t> eventsExpected;
        std::atomic<int32_t> eventsReceived;
        uint64_t cpuStart;
        Summary summary;
        bool timingCompleted;
    };

    Packet& _GetPacket(uint64_t packetId);
    uint32_t _ResolveGPUEvents();
    virtual void _ReadGPUTimers(Packet* packet);

    static uint64_t
    _GetNanoseconds();

    uint32_t _activePacketId;
    Packet _packets[NUM_PACKETS];
    std::vector<Summary> _log;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
