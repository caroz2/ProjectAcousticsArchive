// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Modules/ModuleManager.h"
#include "IAcoustics.h"
#include "UnrealTritonHooks.h"
#include "AcousticsDesignParams.h"
#include "TritonDebugInterface.h"
#include "Async/Async.h"
#include "MathUtils.h"

#if !UE_BUILD_SHIPPING
class FProjectAcousticsDebugRender;
#endif

// A generic class that accepts a function to do work in Unreal's ThreadPool system.
// Keeps track of when the task has finished its work or been abandoned. Up to the user
// to signal when the task has been queued with m_IsQueuedOrRunning
class FAcousticsQueuedWork : public IQueuedWork
{
public:
    FAcousticsQueuedWork(TFunction<void()>&& inFunction, volatile int32* inDoneCounter)
        : m_Function(inFunction), m_DoneCounter(inDoneCounter)
    {
    }

    virtual void DoThreadedWork() override
    {
        m_Function();
        SignalStop();
    }

    virtual void Abandon() override
    {
        SignalStop();
    }

    // Signal to the counters that this item has been queued or is running
    void SignalStart()
    {
        FPlatformAtomics::AtomicStore(&m_IsQueuedOrRunning, 1);
        FPlatformAtomics::InterlockedIncrement(m_DoneCounter);
    }

    // Signal to the counters that this item has finished, retracted, or abandoned
    void SignalStop()
    {
        FPlatformAtomics::AtomicStore(&m_IsQueuedOrRunning, 0);
        FPlatformAtomics::InterlockedDecrement(m_DoneCounter);
    }

    /** The function to execute on the Task Graph. */
    TFunction<void()> m_Function;

    // Keep track of when this task is running and not. User should set to 1
    // when they queue this task
    volatile int32 m_IsQueuedOrRunning = 0;

    // For updating a caller's running task counter
    volatile int32* m_DoneCounter;
};

// All the results from a Triton acoustics query
struct AcousticQueryResults
{
    TritonAcousticParameters AcousticParams;
    TritonDynamicOpeningInfo OpeningInfo;
    TritonRuntime::QueryDebugInfo QueryDebugInfo;
    // Whether the acoustic query was successful or not
    bool QueryResult;
};

// Holds the data for a queued acoustics query
struct AsyncAcousticQueryResults
{
    // Results from an acoustics query will be placed here when ready
    TFuture<AcousticQueryResults> QueryResults;
    // This is the queued work item. Saving this so we can retract it from the pool if needed
    TUniquePtr<FAcousticsQueuedWork> QueuedWork;
    // Whether or not this source has processed any frames so far
    bool HasProcessed = false;
    // Whether or not a retraction has been issued on this async query
    bool RetractionRequested = false;
};

class FProjectAcousticsModule : public IAcoustics
{
public:
    FProjectAcousticsModule();

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // IAcoustics
    virtual bool LoadAceFile(const FString& filePath, const float cacheScale) override;
    virtual void UnloadAceFile(bool clearOldQueries) override;

    virtual bool AddDynamicOpening(
        class UAcousticsDynamicOpening* opening, const FVector& center, const FVector& normal,
        const TArray<FVector>& vertices) override;
    virtual bool RemoveDynamicOpening(class UAcousticsDynamicOpening* opening) override;
    virtual bool UpdateDynamicOpening(
        class UAcousticsDynamicOpening* opening, float dryAttenuationDb, float wetAttenuationDb) override;

    virtual bool SetGlobalDesign(const FAcousticsDesignParams& params) override;
    virtual void SetSpaceTransform(const FTransform& newTransform) override;

    virtual bool UpdateObjectParameters(
        const uint64_t sourceObjectId, const FVector& sourceLocation, const FVector& listenerLocation,
        AcousticsObjectParams& parameters) override;
    virtual AcousticQueryResults GetAcousticQueryResults(
        const uint64_t sourceObjectId, const FVector& sourceLocation, const FVector& listenerLocation,
        AcousticsObjectParams objectParams);

    virtual void RegisterSourceObject(const uint64_t sourceObjectId) override;
    virtual void UnregisterSourceObject(const uint64_t sourceObjectId) override;

    virtual bool UpdateOutdoorness(const FVector& listenerLocation) override;
    virtual float GetOutdoorness() const override;
    virtual bool CalculateReverbSendWeights(
        const float targetReverbTime, const uint32_t numReverbs, const float* reverbTimes,
        float* reverbSendWeights) const override;

    virtual bool PostTick() override;

    virtual bool UpdateDistances(const FVector& listenerLocation) override;
    virtual bool QueryDistance(const FVector& lookDirection, float& outDistance) override;
    virtual void UpdateLoadedRegion(
        const FVector& playerPosition, const FVector& tileSize, const bool forceUpdate,
        const bool unloadProbesOutsideTile, const bool blockOnCompletion) override;

    virtual FVector TritonPositionToWorld(const FVector& vec) const override;
    virtual FVector WorldPositionToTriton(const FVector& vec) const override;
    virtual FVector TritonScaleToWorld(const FVector& vec) const override;
    virtual FVector WorldScaleToTriton(const FVector& vec) const override;
    virtual FVector TritonDirectionToWorld(const FVector& vec) const override;
    virtual FVector WorldDirectionToTriton(const FVector& vec) const override;
    virtual VectorF TritonDirectionToHrtfEngine(const VectorF& vec) const override;
    virtual FQuat GetSpaceRotation() const override;

#if !UE_BUILD_SHIPPING
    virtual void SetEnabled(bool isEnabled) override;

    virtual void
    UpdateSourceDebugInfo(uint64_t sourceID, bool shouldDraw, FName displayName, bool isBeingDestroyed) override;

    // Allow to set the exposed voxel distance
    void SetVoxelVisibleDistance(const float InVisibleDistance) override;

    virtual void DebugRender(
        UWorld* world, UCanvas* canvas, const FVector& cameraPos, const FVector& cameraLook, float cameraFOV,
        bool shouldDrawStats, bool shouldDrawVoxels, bool shouldDrawProbes, bool shouldDrawDistances,
        AcousticsDrawParameters shouldDrawSourceParameters) override;

    TritonRuntime::TritonAcousticsDebug* GetTritonDebugInstance() const;
    bool IsAceFileLoaded() const
    {
        return m_AceFileLoaded;
    }

    int64 GetMemoryUsed() const
    {
        return m_TritonMemHook != nullptr ? m_TritonMemHook->GetTotalMemoryUsed() : 0;
    }

    int64 GetDiskBytesRead() const
    {
        return m_TritonIOHook != nullptr ? m_TritonIOHook->GetBytesRead() : 0;
    }

#endif

private:
    // Triton members
    TritonRuntime::TritonAcoustics* m_Triton;
    bool m_TritonInstanceCreated;
    bool m_AceFileLoaded;
    FVector m_LastLoadCenterPosition;
    FVector m_LastLoadTileSize;
    TUniquePtr<TritonRuntime::FTritonMemHook> m_TritonMemHook;
    TUniquePtr<TritonRuntime::FTritonLogHook> m_TritonLogHook;
    TUniquePtr<TritonRuntime::FTritonUnrealIOHook> m_TritonIOHook;
    TUniquePtr<TritonRuntime::FTritonAsyncTaskHook> m_TritonTaskHook;
    bool m_IsOutdoornessStale;
    float m_CachedOutdoorness;
    FAcousticsDesignParams m_GlobalDesign;
    FTransform m_SpaceTransform;
    FTransform m_InverseSpaceTransform;

    // Holds all async acoustic queries for each source before they've been returned to the caller
    // Key is the sourceID, value is the acoustic query results
    TMap<uint64_t, AsyncAcousticQueryResults> m_AcousticQueryResultMap;

    // Map for all access to m_AcousticQueryResultMap. This map can be accessed by the game/audio thread and the
    // background thread responsible for doing acoustic queries
    FCriticalSection m_AcousticQueryResultMapLock;

    // Thread pool responsible for maintaining our own pool of thread(s) for running background acoustic queries
    FQueuedThreadPool* m_ThreadPool;

    // Keep track of how many background queries are queued or running
    volatile int32 m_NumRunningTasks;

#if !UE_BUILD_SHIPPING
    bool m_IsEnabled;
    TUniquePtr<FProjectAcousticsDebugRender> m_DebugRenderer;
#endif

    // Helpers
    bool GetAcousticParameters(
        const FVector& sourceLocation, const FVector& listenerLocation, TritonAcousticParameters& params,
        TritonDynamicOpeningInfo& outOpeningInfo, const TritonRuntime::InterpolationConfig& radiationDir, TritonRuntime::QueryDebugInfo* outDebugInfo = nullptr);
    void WaitForRunningTasks();
};

// Statistics hooks
DECLARE_CYCLE_STAT_EXTERN(
    TEXT("Update Acoustics Object Params"), STAT_Acoustics_UpdateObjectParams, STATGROUP_Acoustics, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Query Acoustics"), STAT_Acoustics_Query, STATGROUP_Acoustics, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Query Outdoorness"), STAT_Acoustics_QueryOutdoorness, STATGROUP_Acoustics, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Load Region"), STAT_Acoustics_LoadRegion, STATGROUP_Acoustics, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Load Ace File"), STAT_Acoustics_LoadAce, STATGROUP_Acoustics, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Clear Ace File"), STAT_Acoustics_ClearAce, STATGROUP_Acoustics, );