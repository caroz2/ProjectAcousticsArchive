// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSpace.h"
#include "IAcoustics.h"
#include <Classes/GameFramework/HUD.h>
#include <Classes/GameFramework/PlayerController.h>

// Console commands for toggling debug info
static TAutoConsoleVariable<int32>
    CVarAcousticsDrawVoxels(TEXT("PA.DrawVoxels"), 0, TEXT("Show Project Acoustics voxels?"));
static TAutoConsoleVariable<int32>
    CVarAcousticsDrawProbes(TEXT("PA.DrawProbes"), 0, TEXT("Show Project Acoustics probes?"));
static TAutoConsoleVariable<int32>
    CVarAcousticsDrawDistances(TEXT("PA.DrawDistances"), 0, TEXT("Show Project Acoustics distance data?"));
static TAutoConsoleVariable<int32>
    CVarAcousticsShowStats(TEXT("PA.ShowStats"), 0, TEXT("Show Project Acoustics statistics?"));
static TAutoConsoleVariable<int32> CVarAcousticsShowAllSourceParameters(
    TEXT("PA.ShowAllSourceParameters"), 0,
    TEXT("0: Ignored, 1: Show acoustic parameters debug display for all sources, 2: Hide acoustic parameters "
         "debug display for all sources, 3: Let the individual source decide whether to show acoustic "
         "parameters debug display\n"));

AAcousticsSpace::AAcousticsSpace(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    // The source component is set to tick TG_DuringPhysics.
    // We set the acoustic space's tick here to be
    // sequenced after all source ticks using TG_PostPhysics.
    // This avoids a potential extra frame delay in refreshing the
    // acoustic parameters. This can have audible issues,
    // especially for new sounds spawned in an occluded position
    // on this frame
    PrimaryActorTick.TickGroup = TG_PostPhysics;

    // Main parameters
    TileSize = FVector(5000, 5000, 5000);
    AutoStream = true;
    UpdateDistances = false;
    CacheScale = 1.0f;

    m_Acoustics = nullptr;

    // Debug controls
    AcousticsEnabled = true;
    DrawStats = false;
    DrawSourceParameters = AcousticsDrawParameters::PerSourceControl;
    DrawVoxels = false;
    DrawProbes = false;
    DrawDistances = false;

    m_LastSpaceTransform = FTransform::Identity;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("AcousticsSpaceRoot"));
}

void AAcousticsSpace::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(true);
    if (IAcoustics::IsAvailable())
    {
        // cache module instance
        m_Acoustics = &(IAcoustics::Get());

        m_LastSpaceTransform = GetActorTransform();
        m_Acoustics->SetSpaceTransform(m_LastSpaceTransform);

#if !UE_BUILD_SHIPPING
        // Update with current enabled state
        m_Acoustics->SetEnabled(AcousticsEnabled);
#endif

        auto success = LoadAcousticsData(AcousticsData);

        if (success && AutoStream)
        {
            // Stream in the first tile if AutoLoad is enabled
            auto listenerPosition = GetListenerPosition();
            m_Acoustics->UpdateLoadedRegion(listenerPosition, TileSize, true, true, false);
        }
    }

#if !UE_BUILD_SHIPPING
    // On startup, tell our HUD to allow draw debug overlays
    // and start calling our PostRenderFor()
    auto fpc = GetWorld()->GetFirstPlayerController();
    if (fpc)
    {
        auto HUD = fpc->GetHUD();
        if (HUD != nullptr)
        {
            HUD->bShowOverlays = true;
            HUD->AddPostRenderedActor(this);
        }
        else
        {
            UE_LOG(
                LogAcousticsRuntime,
                Warning,
                TEXT("FirstPlayerController needs to have a HUD in order to display Project Acoustics debug visualizations"));
        }
    }
#endif //! UE_BUILD_SHIPPING
}

// Get location of first listener
FVector AAcousticsSpace::GetListenerPosition()
{
    auto world = GetWorld();
    auto fpc = world->GetFirstPlayerController();
    if (fpc != nullptr)
    {
        FVector Location, Front, Right;
        fpc->GetAudioListenerPosition(Location, Front, Right);
        return Location;
    }
    else
    {
        return FVector::ZeroVector;
    }
}

// Note: This function will be called after all source component ticks.
void AAcousticsSpace::Tick(float deltaSeconds)
{
    Super::Tick(deltaSeconds);

    if (!m_Acoustics)
    {
        return;
    }

    // Update global design tweaks
    {
        m_Acoustics->SetGlobalDesign(GlobalDesignParams);
    }

    // Update things dependent only on listener
    if (GetWorld()->IsGameWorld())
    {
        auto currentTx = GetActorTransform();
        if (!currentTx.Equals(m_LastSpaceTransform))
        {
            m_Acoustics->SetSpaceTransform(currentTx);
            m_LastSpaceTransform = currentTx;
        }

        auto listenerPosition = GetListenerPosition();

        // Update streaming
        if (AutoStream)
        {
            m_Acoustics->UpdateLoadedRegion(listenerPosition, TileSize, false, true, false);
        }

        // If there are active emitters in the scene, they will
        // update outdoorness each frame automatically. But if there are
        // no active emitters this frame, we hand-crank outdoorness.
        m_Acoustics->UpdateOutdoorness(listenerPosition);

        // Update distances
        if (UpdateDistances)
        {
            m_Acoustics->UpdateDistances(listenerPosition);
        }
    }

    // Inform processing for this frame is complete, updates internal per-frame state
    m_Acoustics->PostTick();
}

void AAcousticsSpace::BeginDestroy()
{
    Super::BeginDestroy();
    if (m_Acoustics)
    {
        m_Acoustics->UnloadAceFile(true);
    }
}

void AAcousticsSpace::PostActorCreated()
{
    UWorld* world = this->GetWorld();
    // Snap the transform to the origin to avoid confusion when users
    // first drag & drop an Acoustics Space actor into their scene.
    // Only do this if we're in the editor. If not in the editor, the transform
    // passed on initialization will be used
    if (world && !world->HasBegunPlay())
    {
        SetActorTransform(FTransform::Identity);
        UE_LOG(
            LogAcousticsRuntime,
            Log,
            TEXT("Snapping newly created AcousticsSpace actor to the origin. Modify its transform to move probes / "
                 "voxels relative to the world origin."));
    }
}

void AAcousticsSpace::ForceLoadTile(FVector centerPosition, bool unloadProbesOutsideTile, bool blockOnCompletion)
{
    if (!m_Acoustics)
    {
        return;
    }

    m_Acoustics->UpdateLoadedRegion(centerPosition, TileSize, true, unloadProbesOutsideTile, blockOnCompletion);
}

bool AAcousticsSpace::LoadAcousticsData(UAcousticsData* newData)
{
    AcousticsData = newData;
    if (newData == nullptr)
    {
        if (m_Acoustics)
        {
            m_Acoustics->UnloadAceFile(false);
        }
        return true;
    }
    auto filePath = newData->AceFilePath;
    return LoadAceFile(filePath);
}

bool AAcousticsSpace::LoadAceFile(FString filePath)
{
    if (!m_Acoustics)
    {
        return false;
    }

    auto success = m_Acoustics->LoadAceFile(filePath, CacheScale);
    if (success)
    {
        if (AutoStream)
        {
            auto listenerPosition = GetListenerPosition();
            m_Acoustics->UpdateLoadedRegion(listenerPosition, TileSize, true, true, false);
        }
    }
    else
    {
        UE_LOG(LogAcousticsRuntime, Error, TEXT("Failed to load ACE file [%s]"), *filePath);
        return false;
    }

    return true;
}

bool AAcousticsSpace::QueryDistance(const FVector lookDirection, float& distance)
{
    if (!m_Acoustics)
    {
        distance = 0;
        return false;
    }

    return m_Acoustics->QueryDistance(lookDirection, distance);
}

bool AAcousticsSpace::GetOutdoorness(float& outdoorness)
{
    if (!m_Acoustics)
    {
        outdoorness = 0;
        return false;
    }

    outdoorness = m_Acoustics->GetOutdoorness();
    return true;
}

void AAcousticsSpace::SetAcousticsEnabled(bool isEnabled)
{
    if (m_Acoustics)
    {
        AcousticsEnabled = isEnabled;
#if !UE_BUILD_SHIPPING
        m_Acoustics->SetEnabled(AcousticsEnabled);
#endif
    }
}

#if WITH_EDITOR
void AAcousticsSpace::EditorApplyScale(
    const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
{
    UE_LOG(LogAcousticsRuntime, Warning, TEXT("Modifying the scale of an AcousticsSpace actor is not supported."));
}

// React to changes in properties that are not handled in Tick()
void AAcousticsSpace::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
    Super::PostEditChangeProperty(e);
    // Prevent the scale from being modified in editor and reset it to (1, 1, 1)
    FName PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
    if (PropertyName == USceneComponent::GetRelativeScale3DPropertyName())
    {
        FTransform transform = GetActorTransform();
        if (transform.GetScale3D() != FVector::OneVector)
        {
            UE_LOG(
                LogAcousticsRuntime, Warning, TEXT("Modifying the scale of an AcousticsSpace actor is not supported."));
            transform.SetScale3D(FVector::OneVector);
            SetActorTransform(transform);
        }
    }
    auto world = GetWorld();
    if (world && world->IsGameWorld())
    {
        // If ace file name updated, load new file
        PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AAcousticsSpace, AcousticsData))
        {
            LoadAcousticsData(AcousticsData);
        }

#if !UE_BUILD_SHIPPING
        // React to acoustic effects being toggled
        PropertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AAcousticsSpace, AcousticsEnabled))
        {
            if (m_Acoustics)
            {
                m_Acoustics->SetEnabled(AcousticsEnabled);
            }
        }
#endif
    }
}
#endif

#if !UE_BUILD_SHIPPING
void AAcousticsSpace::PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir)
{
    if (!m_Acoustics)
    {
        return;
    }

    float CameraFOV = 90.0f;
    if (PC && PC->PlayerCameraManager)
    {
        CameraFOV = PC->PlayerCameraManager->GetFOVAngle();
    }

    m_Acoustics->SetVoxelVisibleDistance(VoxelsVisibleDistance);

    auto drawSourceParameters = DrawSourceParameters;

    // Let the CVar overwrite the AcousticsSpace property
    auto showAllSourcesCvarOverride = CVarAcousticsShowAllSourceParameters.GetValueOnAnyThread();
    switch (showAllSourcesCvarOverride)
    {
        case 1:
            drawSourceParameters = AcousticsDrawParameters::ShowAllParameters;
            break;
        case 2:
            drawSourceParameters = AcousticsDrawParameters::HideAllParameters;
            break;
        case 3:
            drawSourceParameters = AcousticsDrawParameters::PerSourceControl;
            break;
        default:
            break;
    }

    m_Acoustics->DebugRender(
        GetWorld(),
        Canvas,
        CameraPosition,
        CameraDir,
        CameraFOV,
        DrawStats || CVarAcousticsShowStats.GetValueOnGameThread() > 0,
        DrawVoxels || CVarAcousticsDrawVoxels.GetValueOnGameThread() > 0,
        DrawProbes || CVarAcousticsDrawProbes.GetValueOnGameThread() > 0,
        (UpdateDistances && (DrawDistances || CVarAcousticsDrawDistances.GetValueOnGameThread() > 0)),
        drawSourceParameters);
}
#endif // UE_BUILD_SHIPPING
