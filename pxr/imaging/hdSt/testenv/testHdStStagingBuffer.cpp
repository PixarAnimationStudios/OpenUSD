//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/stagingBuffer.h"

#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/base/tf/errorMark.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
_RunTest()
{
    TfErrorMark errors;
    HgiUniquePtr hgi = Hgi::CreateNamedHgi(HgiTokens->OpenGL);
    if (!hgi) {
        return false;
    }
    std::cout << "Hgi backend: " << hgi->GetAPIName() << '\n';
    HdStResourceRegistry registry(hgi.get());
    HdStStagingBuffer staging(&registry);

    constexpr size_t defaultLimit = 64 * 1024 * 1024;
    const char *setting = std::getenv("HDST_MAX_STAGING_BUFFER_SIZE");
    char *end = nullptr;
    errno = 0;
    const unsigned long long configured =
        setting ? std::strtoull(setting, &end, 10) : 0;
    const bool digits = setting && *setting &&
        std::all_of(setting, setting + std::char_traits<char>::length(setting),
                    [](char c) { return c >= '0' && c <= '9'; });
    const size_t limit = setting && end != setting && *end == '\0' &&
        digits && errno != ERANGE && configured > 0 &&
        configured <= std::numeric_limits<size_t>::max()
        ? static_cast<size_t>(configured) : defaultLimit;

    // Keep large-limit configuration checks within the default test budget.
    const size_t testLimit = std::min(limit, defaultLimit);
    const size_t total = 2 * testLimit + 137;
    const size_t dataSize = std::max(total, size_t(1024 * 1024 + 257));
    std::vector<uint8_t> source(dataSize + 64);
    std::vector<uint8_t> expected(dataSize + 64, 0xcc);
    uint32_t state = 0x159ab2f;
    for (uint8_t &byte : source) {
        state = state * 1664525u + 1013904223u;
        byte = static_cast<uint8_t>(state >> 24);
    }

    HgiBufferDesc desc;
    desc.debugName = "staging regression destination";
    desc.byteSize = expected.size();
    desc.usage = HgiBufferUsageStorage;
    desc.initialData = expected.data();
    HgiBufferHandle destination = hgi->CreateBuffer(desc);

    const auto check = [&]() {
        staging.Flush();
        registry.SubmitBlitWork(HgiSubmitWaitTypeWaitUntilCompleted);
        std::vector<uint8_t> actual(expected.size());
        HgiBufferGpuToCpuOp read;
        read.gpuSourceBuffer = destination;
        read.byteSize = actual.size();
        read.cpuDestinationBuffer = actual.data();
        HgiBlitCmdsUniquePtr cmds = hgi->CreateBlitCmds();
        cmds->CopyBufferGpuToCpu(read);
        hgi->SubmitCmds(cmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);
        return actual == expected;
    };

    // Fill and roll over a small staging buffer several times. Every source
    // takes the staged path; distinct bytes reveal premature staging reuse.
    staging.Resize(testLimit);
    for (size_t offset = 0; offset < total;) {
        HgiBufferCpuToGpuOp copy;
        copy.cpuSourceBuffer = source.data();
        copy.sourceByteOffset = 17 + offset;
        copy.gpuDestinationBuffer = destination;
        copy.destinationByteOffset = 11 + offset;
        copy.byteSize = std::min({std::max(size_t(1), testLimit / 3),
                                 size_t(128 * 1024), total - offset});
        staging.StageCopy(copy);
        std::copy_n(source.data() + copy.sourceByteOffset, copy.byteSize,
                    expected.data() + copy.destinationByteOffset);
        offset += copy.byteSize;
    }
    bool success = check();
    if (!success) {
        std::cerr << "Staging rollover or source/destination offset failed\n";
    }
    staging.Flush(); // Empty flush must not submit stale copies.

    // An oversized source can still be below the original direct-upload
    // threshold when the configured staging limit is small.
    HgiBufferCpuToGpuOp copy;
    copy.cpuSourceBuffer = source.data();
    copy.sourceByteOffset = 19;
    copy.gpuDestinationBuffer = destination;
    copy.destinationByteOffset = 11;
    copy.byteSize = testLimit + 123;
    staging.Resize(testLimit);
    staging.StageCopy(copy);
    std::copy_n(source.data() + copy.sourceByteOffset, copy.byteSize,
                expected.data() + copy.destinationByteOffset);
    if (!check()) {
        std::cerr << "Oversized source readback failed\n";
        success = false;
    }

    // Exercise the large-source direct path with a multi-chunk transfer.
    copy.byteSize = std::max(size_t(1024 * 1024), testLimit + 123);
    staging.Resize(testLimit);
    staging.StageCopy(copy);
    std::copy_n(source.data() + copy.sourceByteOffset, copy.byteSize,
                expected.data() + copy.destinationByteOffset);
    if (!check()) {
        std::cerr << "Direct upload readback failed\n";
        success = false;
    }

    // A zero estimate must not lose a subsequently resolved source. Reading
    // the entire destination also checks untouched prefix/suffix guard bytes.
    staging.Resize(0);
    copy.byteSize = 17;
    copy.sourceByteOffset = 3;
    staging.StageCopy(copy);
    std::copy_n(source.data() + copy.sourceByteOffset, copy.byteSize,
                expected.data() + copy.destinationByteOffset);
    if (!check()) {
        std::cerr << "Underestimated capacity or buffer reuse failed\n";
        success = false;
    }
    staging.StageCopy(HgiBufferCpuToGpuOp());
    staging.Flush();
    hgi->DestroyBuffer(&destination);
    return success && errors.IsClean();
}

int
main()
{
    GarchGLDebugWindow window("Storm staging buffer regression", 64, 64);
    window.Init();
    const bool success = _RunTest();
    std::cout << (success ? "OK" : "FAILED") << '\n';
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
