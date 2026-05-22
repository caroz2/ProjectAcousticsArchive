// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsSourceDataOverrideSourceSettings.h"

UAcousticsSourceDataOverrideSourceSettings::UAcousticsSourceDataOverrideSourceSettings()
{
    Settings.DesignParams = FAcousticsDesignParams::Default();
    Settings.ShowAcousticParameters = false;
    Settings.ApplyAcousticsVolumes = true;
    Settings.Resolver = AcousticsInterpolationDisambiguationMode::Default;
    Settings.PushDirection = FVector::ZeroVector;
}

#if WITH_EDITOR
bool UAcousticsSourceDataOverrideSourceSettings::CanEditChange(const FProperty* InProperty) const
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