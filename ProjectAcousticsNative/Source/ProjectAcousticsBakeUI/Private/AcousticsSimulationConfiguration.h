// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "Runtime/Core/Public/Async/Async.h"
#include "Runtime/Core/Public/Async/Future.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Math/IntVector.h"
#include "TritonPreprocessorApi.h"
#include "AcousticsMesh.h"
#include "AcousticsMaterialLibrary.h"
#include "AcousticsSimulationConfiguration.h"

enum class SimulationConfigurationState
{
    Unavailable = 0,
    InProcess,
    Ready,
    Failed
};

// C++ wrappers for Triton Preprocessor types
class AcousticsSimulationConfiguration final
{
public:
    ~AcousticsSimulationConfiguration();

    static TUniquePtr<AcousticsSimulationConfiguration> Create(const FString workingDir, const FString& configFile);
    static TUniquePtr<AcousticsSimulationConfiguration> Create(
        TSharedPtr<AcousticMesh> mesh, const TritonSimulationParameters& simulationParams,
        const TritonOperationalParameters& opParams, const AcousticsMaterialLibrary* library,
        TritonPreprocessorCallback callback);

    SimulationConfigurationState GetState() const;

    bool IsReady() const;

    int GetProbeCount() const;
    bool GetProbeList(TArray<FVector>& locations) const;

    bool GetVoxelMapInfo(FBox& box, FBox& boxTriton, FIntVector& voxelCounts, float& cellSize) const;
    bool IsVoxelOccupied(int x, int y, int z) const;

private:
    AcousticsSimulationConfiguration() : m_Handle(nullptr)
    {
    }

    bool Initialize(
        TSharedPtr<AcousticMesh> mesh, const TritonSimulationParameters& simulationParams,
        const TritonOperationalParameters& opParams, const AcousticsMaterialLibrary* library,
        TritonPreprocessorCallback& callback);

    bool Initialize(const FString& workingDir, const FString& configFilename);

private:
    TritonObject m_Handle;
    TFuture<bool> m_CreateProbesFuture;
};
