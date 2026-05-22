// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Components/StaticMeshComponent.h"
#if ENGINE_MAJOR_VERSION == 5
#include "UObject/ObjectSaveContext.h"
#endif
#include "AcousticsDynamicOpening.generated.h"

UCLASS(
    ClassGroup = Acoustics, AutoExpandCategories = (Transform, StaticMesh, Acoustics),
    AutoCollapseCategories = (Physics, Collision, Lighting, Rendering, Cooking, Tags), BlueprintType, Blueprintable,
    meta = (BlueprintSpawnableComponent))
class PROJECTACOUSTICS_API UAcousticsDynamicOpening : public UStaticMeshComponent
{
    GENERATED_UCLASS_BODY()

public:
    /** Specify dB attenuation on dry audio going through this opening
     */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics",
        meta = (UIMin = -120, ClampMin = -120, UIMax = 0, ClampMax = 0))
    float DryAttenuationDb;

    /** Specify dB attenuation on wet audio going through this opening
     */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics",
        meta = (UIMin = -120, ClampMin = -120, UIMax = 0, ClampMax = 0))
    float WetAttenuationDb;

    /** For additional effects processing from doors. If a game object's sound goes
     * through this opening, this value will be multiplied by 100 to turn into percentage,
     * and set as value for "AcousticsOpeningFiltering" RTPC on that game object.
     */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics",
        meta = (UIMin = 0, ClampMin = 0, UIMax = 1, ClampMax = 1))
    float Filtering;

    virtual bool IsEditorOnly() const override
    {
        return false;
    }

    // Overridden methods
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
    virtual void
    TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    virtual void OnUpdateTransform(
        EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

private:
    class IAcoustics* m_Acoustics;

    void FlattenZ();
    FName Name() const;

    UPROPERTY()
    TArray<FVector> Vertices;

    UPROPERTY()
    FVector Center;

    UPROPERTY()
    FVector Normal;

#if WITH_EDITOR
    // Employed during baking. These also populate the UPROPERTYs above
    // that get serialized.
public:
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#if ENGINE_MAJOR_VERSION == 5
    virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
#else
    virtual void PreSave(const class ITargetPlatform* TargetPlatform) override;
#endif
    bool ComputeCenter(FVector& Center);

private:
    bool UpdateOpeningGeometry();
    bool ExtractOpeningGeometry(TArray<FVector>& outVertices, FVector& outCenter, FVector& outNormal) const;
#endif
};