// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "MemoryOverrides.h"
#include "ProbeInterp.h"
#include "ReceiverInterpolationWeights.h"
#include <array>

namespace TritonRuntime
{
    struct DynamicOpeningDebugInfo
    {
        TRITON_PREVENT_HEAP_ALLOCATION;

    public:
        bool DidGoThroughOpening;
        bool DidProcessingSucceed;
        uint64_t OpeningID;
        Triton::Vec3f Center;
        int BoundProbeID;
        Triton::Vec3f StringTightenedPoint;
        float DistanceDifference;
    };

    class QueryDebugInfo
    {
        friend class DecodedProbeData;
        friend class SpatialProbeDistribution;
        friend class EncodedMapData;
        friend class DebugInfoWeightSetter;

        TRITON_PREVENT_HEAP_ALLOCATION;

    public:
        static constexpr int MaxMessages = 16;

        enum MessageType : uint8_t
        {
            NoError = 0,
            Warning,
            Error,
            Fatal,
            Count
        };

        static constexpr std::array<const char*, static_cast<uint8_t>(MessageType::Count)> c_MessageTypeStrings = {
            "No Error", "Warning", "Error", "Fatal"};

        struct DebugMessage
        {
            TRITON_PREVENT_HEAP_ALLOCATION;

        public:
            static constexpr int MaxMessageLength = 128;

            MessageType Type;
            wchar_t MessageString[MaxMessageLength];

            inline void ResetType()
            {
                Type = NoError;
            }
        };

    private:
        DebugMessage _Messages[MaxMessages];
        int _MessageCount;
        bool _DidQuerySucceed;

        ProbeInterpVals _ProbeWeights[MaxInterpProbes];
        ReceiverInterpolationWeights _ReceiverWeights[MaxInterpProbes];

        bool _DidConsiderDynamicOpenings;
        DynamicOpeningDebugInfo _DynamicOpeningInfo;

        bool _PushMessage(MessageType type, const wchar_t* Message);
        void _Reset();

        void _SetProbeWeights(const ProbeInterpVals* vals);
        void _SetReceiverWeights(int ProbeIndex, const ReceiverInterpolationWeights& ReceiverWeights);

    public:
        QueryDebugInfo();
        ~QueryDebugInfo();

        int CountMessagesOfType(MessageType type) const;

        void GetProbeInterpWeights(ProbeInterpVals OutWeights[MaxInterpProbes]) const;
        ReceiverInterpolationWeights GetReceiverInterpWeightsForProbe(int ProbeIndex) const;
        int GetMessageCount() const;
        bool DidQuerySucceed() const;
        const DebugMessage* GetMessageList(int& OutNumMessages) const;

        bool DidConsiderDynamicOpenings() const;
        const DynamicOpeningDebugInfo* GetDynamicOpeningDebugInfo() const;

    private:
        // The strings that are actually sent to _PushMessage()

        // Errors
        static constexpr const wchar_t* c_ErrorProbeInterp = L"Probe interpolation failed";
        static constexpr const wchar_t* c_ErrorParamCompute = L"Source interpolation failed";

        // Warnings
        static constexpr const wchar_t* c_WarnContrastUnresolved = L"Receiver loudness contrast unresolved";
        static constexpr const wchar_t* c_WarnAllSamplesDiscarded = L"Receiver weights too small";
        static constexpr const wchar_t* c_WarnNoProbes = L"No probes found near listener location";
        static constexpr const wchar_t* c_WarnAllProbesDiscarded = L"All probes discarded during interpolation";

        // Infos
        static constexpr const wchar_t* c_InfoContrastDetected = L"Receiver loudness contrast detected";
        static constexpr const wchar_t* c_InfoNoInterpResolver = L"No interpolation resolver provided";
        static constexpr const wchar_t* c_InfoExtrapolation = L"Extrapolation performed for source location outside simulation region";
        static constexpr const wchar_t* c_InfoProbeQuery = L"Listener outside probe safety region, falling back to acoustic query";
    };
} // namespace TritonRuntime