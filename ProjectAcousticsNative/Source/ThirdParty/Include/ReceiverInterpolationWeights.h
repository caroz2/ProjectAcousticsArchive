// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "MemoryOverrides.h"
#include "TritonVector.h"

namespace TritonRuntime
{
    struct ReceiverInterpolationWeights
    {
        TRITON_PREVENT_HEAP_ALLOCATION;

    public:
        static constexpr int MaxInterpSamples = 8;

        static const Triton::Vec3i InterpBoxCornerOffsets[MaxInterpSamples];

        bool SampleValid[MaxInterpSamples];
        float weight[MaxInterpSamples];
        Triton::Vec3i MinCornerSampleCell3DIndex;
        Triton::Vec3f ReceiverCoordsInSimRegion[MaxInterpSamples];
        float SafetyDist[MaxInterpSamples];
        float BlockDecompressionTimes[MaxInterpSamples];
        bool WasCacheHit[MaxInterpSamples];

        ReceiverInterpolationWeights();

        void Clear();

        int Count();

        void operator=(int value);
    };
} // namespace TritonRuntime
