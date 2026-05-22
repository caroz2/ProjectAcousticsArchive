// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include "Classes/GameFramework/Actor.h"
#include "Modules/ModuleManager.h"
#include "AcousticsDesignParams.h"
#include "AcousticsData.h"
#include "AcousticsSpace.generated.h"

UENUM()
enum class AcousticsDrawParameters : uint8
{
    // Show acoustic parameters debug display for all sources in-editor
    ShowAllParameters,
    // Hide acoustic parameters debug display for all sources in-editor
    HideAllParameters,
    // Let the individual source decide whether to display acoustic parameters debug display
    PerSourceControl
};

// Loads the Project Acoustics data file (.ACE) and contains the global settings for acoustics. One of these is needed
// per level.
UCLASS(
    config = Engine, hidecategories = Auto, AutoExpandCategories = (Transform, Acoustics), BlueprintType, Blueprintable,
    ClassGroup = Acoustics, meta = (BlueprintSpawnableComponent))
class PROJECTACOUSTICS_API AAcousticsSpace : public AActor
{
    GENERATED_UCLASS_BODY()

public:
    /* ACE file to load. ACE files must be located in <project dir>/Content/Acoustics/, however
       the AcousticsData.uasset can be placed anywhere. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
    UAcousticsData* AcousticsData;

    /** Tile size for streaming acoustic data. Probes within this tile centered at player are kept loaded in RAM.
     * Small tile size will reduce RAM but at cost of frequent loading. Huge sizes containing all probes will load
     * full data into RAM. Unless tile is too small to keep up with player motion, acoustics is unaffected by tile size.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FVector TileSize;

    /** If enabled, the ACE file will be automatically streamed into memory as the player navigates
     * through the environment. If disabled, the ACE file must be streamed manually via blueprint functions
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    bool AutoStream;

    /** Controls the size of the cache used for Acoustic queries. 0 = no cache, 1 = full cache
     * Smaller caches use less RAM, but have longer lookup times
     * Must be set before the ACE file is loaded
     */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics",
        meta = (UIMin = 0, ClampMin = 0, UIMax = 1, ClampMax = 1))
    float CacheScale;

    /** Will update distance data around listener location at each tick.
     * The distance data is retrievable in blueprint/code
     */
    UPROPERTY(EditAnywhere, Category = "Acoustics")
    bool UpdateDistances;

    /////////////////// DESIGN CONTROLS //////////////////
    /**
     *	The design params used to override acoustics for all sound sources in the scene
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FAcousticsDesignParams GlobalDesignParams;

    /////////////////// DEBUG CONTROLS //////////////////

    /** Toggle acoustic effects on or off. In the off state, the effects
     * will be as if there were no geometry in the world. There
     * will be no occlusion or reverberation, but distance attenuation will
     * remain active. This can be helpful to quickly narrow down whether
     * an audio issue is related to the geometry-based physics calculations.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics|Debug Controls")
    bool AcousticsEnabled;

    /** Will draw overall stats for the acoustics system
     */
    UPROPERTY(EditAnywhere, Category = "Acoustics|Debug Controls")
    bool DrawStats;

    /**
    * Toggle rendering of the Acoustic Parameter Debug Display for ALL sources in the scene.
    * Allows you to force all debug displays to be shown or hidden for all sources. Or you
    * can leave it up to debug controls on the individual sources.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics|Debug Controls")
    AcousticsDrawParameters DrawSourceParameters;

    /**
     * Enable rendering of voxelized acoustics geometry. Verify acoustic simulation voxel data.
     * If emitter or player is inside these voxels, acoustic queries will fail.
     * Note that only voxels within approximately 5m of the camera position will be drawn.
     * The voxel data is obtained from the currently loaded ACE file.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics|Debug Controls")
    bool DrawVoxels;

    /** How far away voxels should be rendered from the camera (cm)
     */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics|Debug Controls",
        meta = (DisplayName = "Voxel Render Distance"))
    float VoxelsVisibleDistance = 1000.0f;

    /** Enable rendering of player probes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics|Debug Controls")
    bool DrawProbes;

    /** Enable rendering of distance data around listener.
     */
    UPROPERTY(EditAnywhere, Category = "Acoustics|Debug Controls")
    bool DrawDistances;

    /////////////////// BLUEPRINT UTILITY FUNCTIONS //////////////////

    /** Force streaming of tile around given location, along with option to block on the data to be streamed in.
     */
    UFUNCTION(BlueprintCallable, Category = "Acoustics")
    void ForceLoadTile(FVector centerPosition, bool unloadProbesOutsideTile, bool blockOnCompletion);

    /** Load the ACE file specified by AcousticsData. If newBake is null, will unload any previously loaded data.
        Returns true on success, false if there was a problem loading/unloading the data. */
    UFUNCTION(BlueprintCallable, Category = "Acoustics")
    bool LoadAcousticsData(UAcousticsData* newBake);

    /** Get distance from listener looking in given direction using an internal
     * baked distance map that is updated if UpdateDistances is true.
     * The value is smoothed over a cone and precomputed, so it is not sensitive
     * to small geometry and doesn't cost real-time ray tracing. Useful for
     * hooking up discrete reflections or general geometric query.
     */
    UFUNCTION(BlueprintCallable, Category = "Acoustics")
    bool QueryDistance(const FVector lookDirection, float& distance);

    /** Get the current "outdoorness" value at listener location.
     * 0 is fully indoors, 1 is fully outdoors. Value will vary smoothly
     * as player walks from inside a room to outside. Can be useful for
     * controlling loudness of outdoor ambiences like wind or rain.
     */
    UFUNCTION(BlueprintCallable, Category = "Acoustics")
    bool GetOutdoorness(float& outdoorness);

    /** Toggle acoustic effects on or off. In the off state, the effects
     * will be as if there were no geometry in the world. There
     * will be no occlusion or reverberation, but distance attenuation will
     * remain active. This can be helpful to quickly narrow down whether
     * an audio issue is related to the geometry-based physics calculations.
     */
    UFUNCTION(BlueprintCallable, Category = "Acoustics")
    void SetAcousticsEnabled(bool isEnabled);

    // AActor methods
    void BeginPlay() override;
    void Tick(float deltaSeconds) override;
    void BeginDestroy() override;
    void PostActorCreated() override;

#if WITH_EDITOR
    void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
    void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

private:
    // Helper to convert from UAcousticsData to a real filepath that Triton can load
    bool LoadAceFile(FString filePath);
    FVector GetListenerPosition();
    class IAcoustics* m_Acoustics;

    FTransform m_LastSpaceTransform;

#if !UE_BUILD_SHIPPING
private:
    virtual void
    PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir) override;
#endif // UE_BUILD_SHIPPING
};
