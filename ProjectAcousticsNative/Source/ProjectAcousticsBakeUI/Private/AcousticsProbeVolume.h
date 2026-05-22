// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "GameFramework/Volume.h"
#include "AcousticsProbeVolume.generated.h"

UENUM()
enum class AcousticsVolumeType : uint8
{
    // All geometry falling inside the Acoustics Volume should be included in the acoustics bake
    Include,
    // All geometry falling inside the Acoustics Volume should be excluded from the acoustics bake
    Exclude,
    // All geometry falling inside the Acoustics Volume should be reassigned to a single material
    MaterialOverride,
    // All geometry falling inside the Acoustics Volume can have its material remapped based on a string comparison
    MaterialRemap,
    // Change the horizontal spacing between probes inside the Acoustics Volume
    ProbeSpacing
};

// This is a metadata volume that can be used to help configure the acoustics simulation
UCLASS(ClassGroup = ProjectAcoustics, hidecategories = (Advanced, Attachment), BlueprintType)
class AAcousticsProbeVolume : public AVolume
{
    GENERATED_UCLASS_BODY()

public:
    // Defines the type of metadata contained in this volume
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AcousticsVolume)
    AcousticsVolumeType VolumeType;

    // Used when volume type is MaterialOverride.
    // All meshes inside this volume will have this material name applied.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AcousticsVolume)
    FString MaterialName;

    // Used when volume type is MaterialReamp.
    // Changes one material to another one for all meshes inside this volume
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AcousticsVolume)
    TMap<FString, FString> MaterialRemapping;

    // Used when volume type is ProbeSpacing
    // Any probes placed inside this volume will conform to this max spacing value instead of the global default
    // Units are in centimeters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AcousticsVolume)
    float MaxProbeSpacing;

    // This class only helps with Acoustics pre-bake design, and is not meant for use in-game.
    virtual bool IsEditorOnly() const override
    {
        return true;
    }

    virtual bool CanEditChange(const FProperty* InProperty) const override;

    // Override for post edit change property to reset the override material name if remap mode is selected and vice
    // versa.
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

    // Prefix for the created material name for all the material remaps and overrides.
    static const FString RemapMaterialNamePrefix;
    static const FString OverrideMaterialNamePrefix;
};
