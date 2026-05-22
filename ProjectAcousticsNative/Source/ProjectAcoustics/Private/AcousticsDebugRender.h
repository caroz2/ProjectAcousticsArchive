// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "AcousticsDesignParams.h"
#include "AcousticsSpace.h"
#include "QueryDebugInfo.h"

enum class AAFaceDirection
{
    X,
    Y,
    Z
};

class UWorld;
class UCanvas;

class FProjectAcousticsDebugRender
{
private:
    //! Additional debug information about emitter not contained in parameters cache
    struct EmitterDebugInfo
    {
        FName DisplayName;
        uint64_t SourceID;
        FVector SourceLocation;
        FVector ListenerLocation;
        bool DidQuerySucceed;
        AcousticsObjectParams objectParams;
        TritonRuntime::QueryDebugInfo queryDebugInfo;
        bool ShouldDraw;
    };

    class FProjectAcousticsModule* m_Acoustics;
    UWorld* m_World;
    UCanvas* m_Canvas;
    FVector m_CameraPos;
    FVector m_CameraLook;
    float m_CameraFOV;
    FString m_LoadedFilename;
    TMap<uint64_t, EmitterDebugInfo> m_DebugCache;
// Ifdef out for non-unity build
#if !UE_BUILD_SHIPPING
    void DrawDirection(const EmitterDebugInfo& info, const AcousticsObjectParams& params, const FColor& arrowColor);
    void DrawStats();
    void DrawVoxels();
    void DrawProbes();
    void DrawDistances();
    void DrawSources(AcousticsDrawParameters shouldDrawSourceParameters);
#endif
    // Exposed voxel distance
    float m_VoxelVisibleDistance = 1000.f;

public:
    FProjectAcousticsDebugRender(FProjectAcousticsModule* owner);

// Ifdef out for non-unity build
#if !UE_BUILD_SHIPPING
    void SetLoadedFilename(FString fileName);
    bool UpdateSourceAcoustics(
        uint64_t sourceID, FVector sourceLocation, FVector listenerLocation, bool didQuerySucceed,
        const AcousticsObjectParams& gameParams, const TritonRuntime::QueryDebugInfo& queryDebugInfo);
    bool UpdateSourceDebugInfo(uint64_t sourceID, bool shouldDraw, FName displayName, bool isBeingDestroyed);
    bool Render(
        UWorld* world, UCanvas* canvas, const FVector& cameraPos, const FVector& cameraLook, float cameraFOV,
        bool shouldDrawStats, bool shouldDrawVoxels, bool shouldDrawProbes, bool shouldDrawDistances,
        AcousticsDrawParameters shouldDrawSourceParameters);
    // Normal needs to point in an axis-aligned direction. Undefined behavior otherwise.
    static void DrawDebugAARectangle(
        const UWorld* inWorld, const FVector& faceCenter, const FVector& faceSize, AAFaceDirection dir,
        const FQuat& faceRotation, const FColor& color);
#endif

    void SetVoxelVisibleDistance(const float InVisibleDistance)
    {
        m_VoxelVisibleDistance = InVisibleDistance;
    }
};
