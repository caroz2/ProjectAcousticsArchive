// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "MemoryOverrides.h"
#include <array>

namespace TritonRuntime
{
    constexpr int MaxInterpProbes = 16;

    // Keep the following enum and array of strings in sync
    // While functionally nothing will break if they drift, this greatly helps our debugability.

    // Information about what happened for a given probe during interpolation step
    enum class ProbeInterpInfo : uint8_t
    {
        Used = 0,
        Unassigned,
        RejectedByAcousticTests,
        RejectedWeightTooSmall,
        RejectedTooMany,
        RejectedComputeParamsFailed,
        ProbeNotLoaded,
        ProbeLoadFailed,
        ProbeBakeFailed,
        Count
    };

    constexpr std::array<const char*, static_cast<uint8_t>(ProbeInterpInfo::Count)> c_ProbeInterpInfoNames = {
        "Used",
        "Unassigned",
        "Rejected By Acoustic Tests",
        "Rejected Weight Too Small",
        "Rejected Too Many",
        "Rejected Compute Params Failed",
        "Probe Not Loaded",
        "Probe Load Failed",
        "Probe Bake Failed"
    };

    struct ProbeInterpVals
    {
        TRITON_PREVENT_HEAP_ALLOCATION;

    public:
        int ProbeIndex;
        float weight;
        ProbeInterpInfo info;
        ProbeInterpVals();
    };
} // namespace TritonRuntime
