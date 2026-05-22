// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsAudioComponent.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "Runtime/Launch/Resources/Version.h"
#include "IAcoustics.h"
#include "AcousticsShared.h"
#include "Materials/Material.h"
#include "MathUtils.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Math/ConvexHull2d.h"
#endif
#include "Engine/StaticMesh.h"
#include "DrawDebugHelpers.h"

UAcousticsAudioComponent::UAcousticsAudioComponent(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Settings.DesignParams = FAcousticsDesignParams::Default();
    Settings.ShowAcousticParameters = false;
    Settings.ApplyAcousticsVolumes = true;
    Settings.Resolver = AcousticsInterpolationDisambiguationMode::Default;
    Settings.PushDirection = FVector::ZeroVector;
}

FName UAcousticsAudioComponent::Name() const
{
    auto* o = GetOwner();
    return o ? o->GetFName() : this->GetFName();
}

#if WITH_EDITOR
bool UAcousticsAudioComponent::CanEditChange(const FProperty* InProperty) const
{
    const bool ParentVal = Super::CanEditChange(InProperty);

    // Only allow the PushDirection to be updated if the Resolver is set to Push
    if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FAcousticsSourceSettings, PushDirection))
    {
        return ParentVal && (Settings.Resolver == AcousticsInterpolationDisambiguationMode::Push);
    }

    return ParentVal;
}
#endif
