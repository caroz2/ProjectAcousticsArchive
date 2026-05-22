// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "Engine/AssetUserData.h"
#include "AcousticsMaterialUserData.generated.h"

// This class is used to persist acoustic material assignments on UE materials, and stored in the materials'
// AssetUserData list.
UCLASS(Config = AcousticsMaterials, PerObjectConfig, defaultconfig, NotBlueprintable, NonTransient)
class UAcousticsMaterialUserData : public UAssetUserData
{
    GENERATED_BODY()

public:
    // Absorption coefficient of the material
    UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "Acoustics")
    float Absorptivity;

    // Known acoustic material that was assigned to the given Unreal material
    UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "Acoustics")
    FString AssignedMaterialName;

    virtual bool IsEditorOnly() const override
    {
        return true;
    }
};
