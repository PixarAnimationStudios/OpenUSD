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
#include "pxr/imaging/hgiGL/metrics.h"
#include "pxr/imaging/garch/glApi.h"

PXR_NAMESPACE_OPEN_SCOPE

uint64_t
HgiGLMetrics::StartGPUEvent(uint32_t packetId, uint64_t id)
{
    Packet &packet = _GetPacket(packetId);
    
    int32_t index =
        packet.eventsExpected.fetch_add(1, std::memory_order_relaxed);

    if (index < NUM_GPU_EVENTS) {
        GPUEvent& event = packet.events[index];
        glGenQueries(2, event.tokens);
        glQueryCounter(event.tokens[0], GL_TIMESTAMP);
        event.t0 = 0;
        return event.id;
    }

    return 0;
}

void
HgiGLMetrics::EndGPUEvent(uint32_t packetId, uint64_t id)
{
    Packet &packet = _GetPacket(packetId);

    for (uint32_t i = 0; i < NUM_GPU_EVENTS; ++i) {
        GPUEvent& event = packet.events[i];
        if (event.id == id) {
            glQueryCounter(event.tokens[1], GL_TIMESTAMP);
            event.t1 = 0;
            packet.eventsReceived.fetch_add(1, std::memory_order_relaxed);
            break;
        }
    }
}

void
HgiGLMetrics::_ReadGPUTimers(Packet* packet)
{
    // Read all of the timestamps
    for (uint32_t i = 0; i < packet->eventsReceived; ++i) {
        GPUEvent & eventI = packet->events[i];
        glGetQueryObjectui64v(eventI.tokens[0], GL_QUERY_RESULT, &eventI.t0);
        glGetQueryObjectui64v(eventI.tokens[1], GL_QUERY_RESULT, &eventI.t1);
    }
}
    
PXR_NAMESPACE_CLOSE_SCOPE
