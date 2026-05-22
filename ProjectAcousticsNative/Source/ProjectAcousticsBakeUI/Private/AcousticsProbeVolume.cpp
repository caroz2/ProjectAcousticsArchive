// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsProbeVolume.h"
#include "Runtime/Launch/Resources/Version.h"
#include "AcousticsEdMode.h"
#include <AcousticsShared.h>

const FString AAcousticsProbeVolume::RemapMaterialNamePrefix = "Remap_";
const FString AAcousticsProbeVolume::OverrideMaterialNamePrefix = "Override_";

AAcousticsProbeVolume::AAcousticsProbeVolume(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    VolumeType = AcousticsVolumeType::Include;
    bIsEditorOnlyActor = true;

    Tags.Add(c_AcousticsNavigationTag);

    // Disable collision
    if (UPrimitiveComponent* PrimitiveComponent = FindComponentByClass<UPrimitiveComponent>())
    {
        PrimitiveComponent->SetCollisionProfileName(TEXT("NoCollision"));
    }

}

// Only show the MaterialName property if VolumeType == MaterialOverride
bool AAcousticsProbeVolume::CanEditChange(const FProperty* InProperty) const
{
    const bool ParentVal = Super::CanEditChange(InProperty);

    if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AAcousticsProbeVolume, MaterialName))
    {
        return ParentVal && VolumeType == AcousticsVolumeType::MaterialOverride;
    }

    // Only show the MaterialRemapping property if VolumeType == MaterialRemap
    if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AAcousticsProbeVolume, MaterialRemapping))
    {
        return ParentVal && VolumeType == AcousticsVolumeType::MaterialRemap;
    }

    // Only allow max probe spacing if volume type is probe spacing
    if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AAcousticsProbeVolume, MaxProbeSpacing))
    {
        return ParentVal && VolumeType == AcousticsVolumeType::ProbeSpacing;
    }

    return ParentVal;
}

// If volume mode is changed between override and remap, reset the other's value.
void AAcousticsProbeVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.Property &&
        PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AAcousticsProbeVolume, VolumeType))
    {
        if (VolumeType == AcousticsVolumeType::MaterialOverride)
        {
            MaterialRemapping.Empty();
        }
        else if (VolumeType == AcousticsVolumeType::MaterialRemap)
        {
            MaterialName.Empty();
        }
        else
        {
            MaterialRemapping.Empty();
            MaterialName.Empty();
        }
    }
}
