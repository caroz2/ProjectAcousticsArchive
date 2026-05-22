// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsDebugRenderer.h"
#include "MathUtils.h"
#include "EditorViewportClient.h"
#include "Editor.h"

AAcousticsDebugRenderer::AAcousticsDebugRenderer(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    m_VoxelInfoCached = false;
}

void AAcousticsDebugRenderer::SetConfiguration(TSharedPtr<AcousticsSimulationConfiguration> config)
{
    FScopeLock lock(&m_Lock);
    m_Config = config;

    // If the config is being reset, remove cached probes
    m_ProbeLocations.Empty();
    m_VoxelInfoCached = false;
}

bool AAcousticsDebugRenderer::ShouldTickIfViewportsOnly() const
{
    return true;
}

void AAcousticsDebugRenderer::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(false);
}

void AAcousticsDebugRenderer::UpdateCacheAndRender(FVector cameraPosition, FVector cameraDir, float cameraFOV)
{
    // Hold a local reference used for rendering debug info
    TSharedPtr<AcousticsSimulationConfiguration> config;
    {
        FScopeLock lock(&m_Lock);
        config = m_Config;
    }

    // Update rendering cache if needed
    if (config.IsValid() && config->IsReady())
    {
        if (m_ProbeLocations.Num() == 0)
        {
            config->GetProbeList(m_ProbeLocations);
        }

        if (!m_VoxelInfoCached)
        {
            m_VoxelInfoCached =
                config->GetVoxelMapInfo(m_VoxelMapBounds, m_VoxelMapBoundsTriton, m_VoxelCounts, m_VoxelCellSize);
        }

        if (ShouldRenderProbes)
        {
            RenderProbes(cameraPosition);
        }

        if (ShouldRenderVoxels)
        {
            RenderVoxels(config.Get(), cameraPosition, cameraDir, cameraFOV);
        }
    }
}

void AAcousticsDebugRenderer::Tick(float deltaSeconds)
{
    auto viewport = GEditor->GetActiveViewport();
    if (viewport == nullptr)
    {
        return;
    }
    auto* client = static_cast<FEditorViewportClient*>(viewport->GetClient());
    if (client)
    {
        auto cameraDir = client->GetViewRotation().Vector();
        auto cameraPosition = client->GetViewLocation();
        auto cameraFOV = client->ViewFOV;
        UpdateCacheAndRender(cameraPosition, cameraDir, cameraFOV);
    }
}

// Uncomment to also render the depth and height of the simulation region for each probe
//#define RENDER_PROBE_DEPTH_HEIGHT

void AAcousticsDebugRenderer::RenderProbes(FVector cameraPosition)
{
    const FVector probeBoxSize(10, 10, 10);
    const FColor probeBoxColor = FColor::Cyan;

    auto* world = GetWorld();
    if (!world)
    {
        return;
    }

    for (auto i = 0; i < m_ProbeLocations.Num(); i++)
    {
        const auto& location = m_ProbeLocations[i];
        // Use camera position for filtering.
        if ((location - cameraPosition).SizeSquared() > ProbesDrawDistance * ProbesDrawDistance)
        {
            continue;
        }

        DrawDebugSolidBox(world, location, probeBoxSize, probeBoxColor);
        DrawDebugBox(world, location, probeBoxSize, FColor::Black, false, -1.0f, 0, 2.0f);
    }
}

FIntVector AAcousticsDebugRenderer::MapPointToVoxel(const FVector& point) const
{
    const float cellSizeTriton = m_VoxelCellSize * AcousticsUtils::c_UnrealToTritonScale;
    FVector fractionalVoxel =
        (AcousticsUtils::UnrealPositionToTriton(point) - m_VoxelMapBoundsTriton.Min) / cellSizeTriton;

    return FIntVector(
        FMath::FloorToInt(fractionalVoxel.X),
        FMath::FloorToInt(fractionalVoxel.Y),
        FMath::FloorToInt(fractionalVoxel.Z));
}

FVector AAcousticsDebugRenderer::MapVoxelToPoint(const FIntVector& voxel) const
{
    const float cellSizeTriton = m_VoxelCellSize * AcousticsUtils::c_UnrealToTritonScale;
    FVector pointTriton = m_VoxelMapBoundsTriton.Min + cellSizeTriton * FVector(voxel) + cellSizeTriton * 0.5f;

    return AcousticsUtils::TritonPositionToUnreal(pointTriton);
}

void AAcousticsDebugRenderer::RenderVoxels(
    const AcousticsSimulationConfiguration* config, FVector cameraPosition, FVector cameraDir, float cameraFOV)
{
    auto voxelColor = FColor::Green;
    // Range in cm we should see the voxels.
    const auto visibleDistance = VoxelsDrawDistance;

    DrawDebugBox(GetWorld(), m_VoxelMapBounds.GetCenter(), m_VoxelMapBounds.GetExtent(), voxelColor);

    const auto regionMinOffset = FVector(visibleDistance, visibleDistance, visibleDistance / 2);
    const auto regionMaxOffset = FVector(visibleDistance, visibleDistance, visibleDistance);

    // Voxel box center is slightly lower so we're closer to the ground
    auto regionCenter = cameraPosition - FVector(0, 0, 50.0f);
    auto voxelSize = FVector(m_VoxelCellSize);

    FIntVector vox0 = MapPointToVoxel(regionCenter - regionMinOffset);
    FIntVector vox1 = MapPointToVoxel(regionCenter + regionMaxOffset);
    FIntVector minVoxTriton(FMath::Min(vox0.X, vox1.X), FMath::Min(vox0.Y, vox1.Y), FMath::Min(vox0.Z, vox1.Z));
    FIntVector maxVoxTriton(FMath::Max(vox0.X, vox1.X), FMath::Max(vox0.Y, vox1.Y), FMath::Max(vox0.Z, vox1.Z));

    // Clamping to within distance of 1 to the edge because we consult neighboring
    // voxels to determine if faces should be rendered - avoids out of bounds
    // access into voxel map
    minVoxTriton.X = FMath::Clamp<int>(minVoxTriton.X, 1, m_VoxelCounts.X - 1);
    maxVoxTriton.X = FMath::Clamp<int>(maxVoxTriton.X, 1, m_VoxelCounts.X - 1);
    minVoxTriton.Y = FMath::Clamp<int>(minVoxTriton.Y, 1, m_VoxelCounts.Y - 1);
    maxVoxTriton.Y = FMath::Clamp<int>(maxVoxTriton.Y, 1, m_VoxelCounts.Y - 1);
    minVoxTriton.Z = FMath::Clamp<int>(minVoxTriton.Z, 1, m_VoxelCounts.Z - 1);
    maxVoxTriton.Z = FMath::Clamp<int>(maxVoxTriton.Z, 1, m_VoxelCounts.Z - 1);

    auto centerOffset = m_VoxelCellSize * 0.5f;
    auto startVoxelCenter = MapVoxelToPoint(minVoxTriton);
    auto currentVoxelCenter = startVoxelCenter;

    // Slightly larger than half-FOV so edge of conical culling region
    // doesn't become visible on screen corners
    const float cullingAngle = 0.55f * cameraFOV;
    const float cosHalfFrustumAngle = FMath::Cos(cullingAngle * PI / 180.0f);

    // The Unreal increment vectors corresponding to moving by one voxel each in x,y,z
    // in Triton coordinates
    FVector cellIncrement = MapVoxelToPoint(FIntVector(1, 1, 1)) - MapVoxelToPoint(FIntVector(0, 0, 0));
    FVector halfCellIncrement = cellIncrement * 0.5f;

    //(x,y,z) enumerate over the voxel box oriented in Triton's coordinate system
    for (int x = minVoxTriton.X; x < maxVoxTriton.X; x++, currentVoxelCenter.X += cellIncrement.X)
    {
        for (int y = minVoxTriton.Y; y < maxVoxTriton.Y; y++, currentVoxelCenter.Y += cellIncrement.Y)
        {
            for (int z = minVoxTriton.Z; z < maxVoxTriton.Z; z++, currentVoxelCenter.Z += cellIncrement.Z)
            {
                auto cameraToVoxel = currentVoxelCenter - cameraPosition;
                cameraToVoxel.Normalize();
                auto viewFrustumDotProd = FVector::DotProduct(cameraToVoxel, cameraDir);

                if (viewFrustumDotProd > 0.0f && viewFrustumDotProd > cosHalfFrustumAngle)
                {
                    // Draw faces only for occupied voxels
                    if (config->IsVoxelOccupied(x, y, z))
                    {
                        // Only consider the 3 faces visible to camera. For each axis, we
                        // have two faces pointing in opposite directions, each shared with a neighboring voxel.
                        // So for each axis (X,Y,Z) figure out which of the two faces we want to render,
                        // indicated by the signed direction (dx/dy/dz) to that neighbor for each axis
                        int dx = (cameraToVoxel.X * cellIncrement.X > 0) ? -1 : 1;
                        int dy = (cameraToVoxel.Y * cellIncrement.Y > 0) ? -1 : 1;
                        int dz = (cameraToVoxel.Z * cellIncrement.Z > 0) ? -1 : 1;

                        // For the three front faces, only render if the face is on the
                        // surface -- that is, the voxel across it is air.

                        // front-face in X
                        if (!config->IsVoxelOccupied(x + dx, y, z))
                        {
                            auto faceCenter = currentVoxelCenter;
                            faceCenter.X += halfCellIncrement.X * dx;
                            DrawDebugAARectangle(GetWorld(), faceCenter, voxelSize, AAFaceDirection::X, voxelColor);
                        }

                        // front-face in Y
                        if (!config->IsVoxelOccupied(x, y + dy, z))
                        {
                            auto faceCenter = currentVoxelCenter;
                            faceCenter.Y += halfCellIncrement.Y * dy;
                            DrawDebugAARectangle(GetWorld(), faceCenter, voxelSize, AAFaceDirection::Y, voxelColor);
                        }

                        // front-face in Z
                        if (!config->IsVoxelOccupied(x, y, z + dz))
                        {
                            auto faceCenter = currentVoxelCenter;
                            faceCenter.Z += halfCellIncrement.Z * dz;
                            DrawDebugAARectangle(GetWorld(), faceCenter, voxelSize, AAFaceDirection::Z, voxelColor);
                        }
                    }
                }
            }
            currentVoxelCenter.Z = startVoxelCenter.Z;
        }
        currentVoxelCenter.Y = startVoxelCenter.Y;
        currentVoxelCenter.Z = startVoxelCenter.Z;
    }
}

// Normal needs to point in an axis-aligned direction. Undefined behavior otherwise.
inline void AAcousticsDebugRenderer::DrawDebugAARectangle(
    const UWorld* inWorld, const FVector& faceCenter, const FVector& faceSize, AAFaceDirection dir, const FColor& color)
{
    FVector offset = faceSize * 0.5f;
    FVector minCorner, dv1, dv2;

    switch (dir)
    {
        case AAFaceDirection::X:
            offset.X = 0;
            minCorner = faceCenter - offset;
            dv1 = FVector(0, faceSize.Y, 0);
            dv2 = FVector(0, 0, faceSize.Z);
            break;
        case AAFaceDirection::Y:
            offset.Y = 0;
            minCorner = faceCenter - offset;
            dv1 = FVector(faceSize.X, 0, 0);
            dv2 = FVector(0, 0, faceSize.Z);
            break;
        case AAFaceDirection::Z:
            offset.Z = 0;
            minCorner = faceCenter - offset;
            dv1 = FVector(faceSize.X, 0, 0);
            dv2 = FVector(0, faceSize.Y, 0);
            break;
        default:
            return;
    }

    FVector corner1 = minCorner + dv1;
    FVector corner2 = minCorner + dv1 + dv2;
    FVector corner3 = minCorner + dv2;

    DrawDebugLine(inWorld, minCorner, corner1, color);
    DrawDebugLine(inWorld, corner1, corner2, color);
    DrawDebugLine(inWorld, corner2, corner3, color);
    DrawDebugLine(inWorld, corner3, minCorner, color);
}