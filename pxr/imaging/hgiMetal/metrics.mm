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
#include "pxr/imaging/hgiMetal/metrics.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

uint64_t
HgiMetalMetrics::StartGPUEvent(uint32_t packetId, uint64_t id)
{
    // We should be locking a mutex here, but because of performance concerns
    // we ignore the possibility of a race condition between this and the
    // EndGPUEvent.
    
    Packet &packet = _GetPacket(packetId);
    
    int32_t index =
        packet.eventsExpected.fetch_add(1, std::memory_order_relaxed);

    if (index < NUM_GPU_EVENTS) {
        GPUEvent& event = packet.events[index];
        event.t0 = _GetNanoseconds();
        event.id = id;
    }

    return id;
}

void
HgiMetalMetrics::EndGPUEvent(uint32_t packetId, uint64_t id)
{
    // We should be locking a mutex here, but because of performance concerns
    // we ignore the possibility of a race condition between this and the
    // EndGPUEvent.
    
    Packet &packet = _GetPacket(packetId);

    for (uint32_t i = 0; i < NUM_GPU_EVENTS; ++i) {
        if (packet.events[i].id == id) {
            packet.events[i].t1 = _GetNanoseconds();
            packet.eventsReceived.fetch_add(1, std::memory_order_relaxed);
            break;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
