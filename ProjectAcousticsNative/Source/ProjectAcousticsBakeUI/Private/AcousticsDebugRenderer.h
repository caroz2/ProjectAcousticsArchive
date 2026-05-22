// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "Classes/GameFramework/Actor.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include "AcousticsSimulationConfiguration.h"
#include "AcousticsDebugRenderer.generated.h"

enum class AAFaceDirection
{
    X,
    Y,
    Z
};

// Contains debug controls for Project Acoustics pre-bakes. This can show the voxels and probes for
// a level. These are automatically added to a level when a pre-bake is performed (in Probes tab).
UCLASS(config = Engine, hidecategories = Auto, BlueprintType, Blueprintable, ClassGroup = ProjectAcoustics)
class AAcousticsDebugRenderer : public AActor
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics", meta = (DisplayName = "Render Probes"))
    bool ShouldRenderProbes = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics", meta = (DisplayName = "Render Voxels"))
    bool ShouldRenderVoxels;

    // Expose draw distance for voxel
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics", meta = (DisplayName = "Voxels Draw Distance"))
    float VoxelsDrawDistance = 1000.0f;

    // Expose Probes' draw distance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics", meta = (DisplayName = "Probes Draw Distance"))
    float ProbesDrawDistance = 2000.0f;

public:
    void SetConfiguration(TSharedPtr<AcousticsSimulationConfiguration> config);
    virtual bool ShouldTickIfViewportsOnly() const override;
    virtual void Tick(float deltaSeconds) override;
    virtual void BeginPlay() override;

private:
    void UpdateCacheAndRender(FVector cameraPosition, FVector cameraDir, float cameraFOV);
    void RenderProbes(FVector cameraPosition);
    void RenderVoxels(
        const AcousticsSimulationConfiguration* config, FVector cameraPosition, FVector cameraDir, float cameraFOV);

    void DrawDebugAARectangle(
        const UWorld* inWorld, const FVector& faceCenter, const FVector& faceSize, AAFaceDirection dir,
        const FColor& color);

    FIntVector MapPointToVoxel(const FVector& point) const;
    FVector MapVoxelToPoint(const FIntVector& voxel) const;

private:
    TSharedPtr<AcousticsSimulationConfiguration> m_Config;
    FCriticalSection m_Lock;
    TArray<FVector> m_ProbeLocations;
    TArray<float> m_ProbeDepths;
    TArray<float> m_ProbeHeights;
    bool m_VoxelInfoCached;
    FBox m_VoxelMapBounds;
    FBox m_VoxelMapBoundsTriton;
    FIntVector m_VoxelCounts;
    float m_VoxelCellSize;
};
