// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsRuntimeVolume.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Components/BrushComponent.h"

AAcousticsRuntimeVolume::AAcousticsRuntimeVolume(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OverrideDesignParams.OcclusionMultiplier = 1.0f;
    OverrideDesignParams.WetnessAdjustment = 0.0f;
    OverrideDesignParams.DecayTimeMultiplier = 1.0f;
    OverrideDesignParams.OutdoornessAdjustment = 0.0f;

    // Disable blocking collision
    if (UPrimitiveComponent* PrimitiveComponent = FindComponentByClass<UPrimitiveComponent>())
    {
        PrimitiveComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
    }
}