// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Components/AudioComponent.h"
#include "AcousticsDesignParams.h"
#include "AcousticsSourceDataOverrideSourceSettings.h"
#include "AcousticsAudioComponent.generated.h"

// AcousticsAudioComponent is a normal Unreal AudioComponent that plays sound
// with additional per-source settings for Project Acoustics. These per-source 
// settings will overwrite any settings from the Project Acoustics Source
// Data Override Source Settings
UCLASS(
    ClassGroup = Acoustics, AutoExpandCategories = (Transform, StaticMesh, Acoustics),
    AutoCollapseCategories = (Physics, Collision, Lighting, Rendering, Cooking, Tags), BlueprintType, Blueprintable,
    meta = (BlueprintSpawnableComponent))
class PROJECTACOUSTICSNATIVE_API UAcousticsAudioComponent : public UAudioComponent
{
    GENERATED_UCLASS_BODY()

public:

    // The per-source settings for this AcousticsAudioComponent
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics",
        meta = (ShowOnlyInnerProperties))
    FAcousticsSourceSettings Settings;

    virtual bool IsEditorOnly() const override
    {
        return false;
    }

    // Overridden methods
#if WITH_EDITOR
    virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

private:
    FName Name() const;
};