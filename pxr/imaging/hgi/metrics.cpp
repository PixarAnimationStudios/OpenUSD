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
#include "pxr/imaging/hgi/metrics.h"

#include <iostream>
#include <iomanip>

PXR_NAMESPACE_OPEN_SCOPE

uint64_t
HgiMetrics::_GetNanoseconds()
{
    using namespace std::chrono;
    auto now = steady_clock::now();
    // nanoseconds are used for compatibility with OpenGL queries.
    auto nSec = duration_cast<nanoseconds>(now.time_since_epoch());

    return nSec.count();
}

HgiMetrics::HgiMetrics()
    : _activePacketId(0)
{
    _log.reserve(128);
}

HgiMetrics::~HgiMetrics()
{
}

void
HgiMetrics::Reset()
{
    _log.clear();
}

HGI_API
std::vector<HgiMetrics::Summary>& HgiMetrics::GetLog()
{
    return _log;
}

void
HgiMetrics::StartPacket()
{
    Packet &packet = _GetPacket(_activePacketId);
    
    packet.id = _activePacketId;
    packet.eventsExpected = 0;
    packet.eventsReceived = 0;
    packet.timingCompleted = false;
    packet.cpuStart = _GetNanoseconds();
    
    for (uint32_t i = 0; i < NUM_GPU_EVENTS; ++i) {
        packet.events[i].id = 0;
        packet.events[i].merged = false;
    }
}

void
HgiMetrics::EndPacket()
{
    Packet &packet = _GetPacket(_activePacketId);
    packet.summary.cpuElapsed = _GetNanoseconds() - packet.cpuStart;
    packet.timingCompleted = true;

    uint32_t gpuPacketIndex = _ResolveGPUEvents();
    
    Packet &gpuPacket = _packets[gpuPacketIndex];
    
    static const size_t _maxLogEntries = 128;

    if (_log.size() < _maxLogEntries) {
        _log.push_back(gpuPacket.summary);
    } else {
        size_t index = _activePacketId % _maxLogEntries;
        _log[index] = gpuPacket.summary;
    }

    ++_activePacketId;
}

HgiMetrics::Packet&
HgiMetrics::_GetPacket(uint64_t packetId)
{
    return _packets[packetId % NUM_PACKETS];
}

uint32_t
HgiMetrics::_ResolveGPUEvents()
{
    Packet *validPacket = nullptr;
    uint64_t lastPacketId = 0;
    uint32_t gpuFrameIndex = 0;

    for (int i = 0; i < NUM_PACKETS; i++) {
        Packet *packet = &_packets[i];
        // To be a valid time it must have received all timing events back and
        // have its frame marked as finished.
        if (packet->id >= lastPacketId && packet->timingCompleted &&
            packet->eventsExpected == packet->eventsReceived &&
            packet->eventsExpected > 0) {
            validPacket = packet;
            gpuFrameIndex = i;
            lastPacketId = packet->id;
        }
    }

    if (!validPacket) {
        return 0;
    }

    _ReadGPUTimers(validPacket);

    GPUEvent* events = validPacket->events;

    // Account for overlaps between the events to work out the total elapsed
    // time and idle time.
    for (uint32_t i = 0; i < validPacket->eventsReceived; ++i) {
        GPUEvent & eventI = events[i];

        if (eventI.merged) {
            continue;
        }

        for (uint32_t j = i + 1; j < validPacket->eventsReceived; ++j) {
            GPUEvent & eventJ = events[j];
            // Check for overlap between the two events and merge.
            if ((eventI.t0 < eventJ.t0 && eventI.t1 > eventJ.t0) ||
                (eventI.t0 < eventJ.t1 && eventI.t1 > eventJ.t1) ||
                (eventI.t0 > eventJ.t0 && eventI.t1 < eventJ.t1) ||
                (eventI.t0 < eventJ.t0 && eventI.t1 > eventJ.t1)) {
                eventI.t0 = std::min(eventI.t0, eventJ.t0);
                eventI.t1 = std::max(eventI.t1, eventJ.t1);
                eventJ.merged = true;
            }
        }
    }
    
    // With the overlaps resolved, calculate the elapsed time from start of
    // first command buffer to the end of the last and the occupied GPU time
    // which excludes idle time.
    uint64_t minT0 = events[0].t0;
    uint64_t maxT1 = events[0].t1;
    validPacket->summary.packetId = validPacket->id;
    validPacket->summary.gpuOccupied = maxT1 - minT0;

    for (uint32_t i = 1; i < validPacket->eventsReceived; ++i) {
        GPUEvent const& eventI = events[i];

        if (eventI.merged) {
            continue;
        }

        minT0 = std::min(minT0, eventI.t0);
        maxT1 = std::max(maxT1, eventI.t1);
        validPacket->summary.gpuOccupied += (eventI.t1 - eventI.t0);
    }

    validPacket->summary.gpuElapsed = maxT1 - minT0;
    
    return gpuFrameIndex;
}

void
HgiMetrics::_ReadGPUTimers(Packet* packet)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
