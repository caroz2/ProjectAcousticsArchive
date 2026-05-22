// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSourceDataOverrideSettings.h"
#include "AcousticsSourceDataOverride.h"

//DEFINE_LOG_CATEGORY(LogAcousticsNative)

UAcousticsSourceDataOverrideSettings::UAcousticsSourceDataOverrideSettings() :
    ReverbBusesPreset(EReverbBusesPreset::Default)
{
    // Configure the saved presets, default and custom.
    FReverbBusesInfo defaultBuses;
    defaultBuses.ShortIndoorReverbSubmixName = FString(TEXT("/ProjectAcoustics/NativeReverb/Submix_IndoorReverbShort.Submix_IndoorReverbShort"));
    defaultBuses.MediumIndoorReverbSubmixName = FString(TEXT("/ProjectAcoustics/NativeReverb/Submix_IndoorReverbLong.Submix_IndoorReverbLong"));
    defaultBuses.LongIndoorReverbSubmixName = FString(TEXT("/ProjectAcoustics/NativeReverb/Submix_IndoorReverbExtraLong.Submix_IndoorReverbExtraLong"));

    defaultBuses.ShortOutdoorReverbSubmixName = FString(TEXT("/ProjectAcoustics/NativeReverb/Submix_OutdoorReverbShort.Submix_OutdoorReverbShort"));
    defaultBuses.MediumOutdoorReverbSubmixName = FString(TEXT("/ProjectAcoustics/NativeReverb/Submix_OutdoorReverbLong.Submix_OutdoorReverbLong"));
    defaultBuses.LongOutdoorReverbSubmixName = FString(TEXT("/ProjectAcoustics/NativeReverb/Submix_OutdoorReverbExtraLong.Submix_OutdoorReverbExtraLong"));

    defaultBuses.ShortReverbLength = 0.5f;
    defaultBuses.MediumReverbLength = 1.5f;
    defaultBuses.LongReverbLength = 3.0f;
    ReverbBusesPresetMap.Add(EReverbBusesPreset::Default, defaultBuses);

    // Use out default by default
    SetReverbBuses(defaultBuses);

    // Custom preset leaves empty slate for user
    FReverbBusesInfo customBuses;
    customBuses.ShortIndoorReverbSubmixName = FString(TEXT(""));
    customBuses.MediumIndoorReverbSubmixName = FString(TEXT(""));
    customBuses.LongIndoorReverbSubmixName = FString(TEXT(""));

    customBuses.ShortOutdoorReverbSubmixName = FString(TEXT(""));
    customBuses.MediumOutdoorReverbSubmixName = FString(TEXT(""));
    customBuses.LongOutdoorReverbSubmixName = FString(TEXT(""));
    customBuses.ShortReverbLength = 0.0f;
    customBuses.MediumReverbLength = 0.0f;
    customBuses.LongReverbLength = 0.0f;
    ReverbBusesPresetMap.Add(EReverbBusesPreset::Custom, customBuses);

}

void UAcousticsSourceDataOverrideSettings::SetReverbBuses(FReverbBusesInfo buses)
{
    ShortIndoorReverbSubmix = buses.ShortIndoorReverbSubmixName;
    MediumIndoorReverbSubmix = buses.MediumIndoorReverbSubmixName;
    LongIndoorReverbSubmix = buses.LongIndoorReverbSubmixName;

    ShortOutdoorReverbSubmix = buses.ShortOutdoorReverbSubmixName;
    MediumOutdoorReverbSubmix = buses.MediumOutdoorReverbSubmixName;
    LongOutdoorReverbSubmix = buses.LongOutdoorReverbSubmixName;

    ShortReverbLength = buses.ShortReverbLength;
    MediumReverbLength = buses.MediumReverbLength;
    LongReverbLength = buses.LongReverbLength;
}

#if WITH_EDITOR
void UAcousticsSourceDataOverrideSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

    auto reverbBusPreset = ReverbBusesPresetMap[ReverbBusesPreset];

    if ((PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ReverbType)))
    {
        if (ReverbType == EAcousticsReverbType::SpatialReverb && !c_SpatialReverbSupported)
        {
            UE_LOG(
                LogAcousticsNative,
                Error,
                TEXT("Project Acoustics SDO Spatial Reverb is not supported below UE 5.1. "
                    "Please change the ReverbType in the Project Acoustics SDO Project Settings"));
            return;
        }
    }
    else if ((PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ReverbBusesPreset)))
    {
        // Update all reverb bus fields based on the preset chosen
        SetReverbBuses(reverbBusPreset);

        for (TFieldIterator<FProperty> PropIt(GetClass()); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortIndoorReverbSubmix) ||
                Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumIndoorReverbSubmix) ||
                Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongIndoorReverbSubmix) ||
                Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortOutdoorReverbSubmix) ||
                Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumOutdoorReverbSubmix) ||
                Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongOutdoorReverbSubmix) ||
                Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortReverbLength) ||
                Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumReverbLength) ||
                Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongReverbLength))
            {
                UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
            }
        }
    }
    else if (this->ReverbBusesPreset == EReverbBusesPreset::Custom && PropertyChangedEvent.ChangeType & EPropertyChangeType::ValueSet)
    {
        // If we are changing a setting in the custom preset, store it so that
        // it can be reused when swapping between default and custom
        // Only do this update when the value is set (e.g. when users are done dragging sliders)

        FProperty* Property = nullptr;

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortIndoorReverbSubmix))
        {
            ShortIndoorReverbSubmixCustom = ShortIndoorReverbSubmix;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].ShortIndoorReverbSubmixName = ShortIndoorReverbSubmixCustom.ToString();
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortIndoorReverbSubmixCustom));
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumIndoorReverbSubmix))
        {
            MediumIndoorReverbSubmixCustom = MediumIndoorReverbSubmix;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].MediumIndoorReverbSubmixName = MediumIndoorReverbSubmixCustom.ToString();
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumIndoorReverbSubmixCustom));
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongIndoorReverbSubmix))
        {
            LongIndoorReverbSubmixCustom = LongIndoorReverbSubmix;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].LongIndoorReverbSubmixName = LongIndoorReverbSubmixCustom.ToString();
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongIndoorReverbSubmixCustom));
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortOutdoorReverbSubmix))
        {
            ShortOutdoorReverbSubmixCustom = ShortOutdoorReverbSubmix;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].ShortOutdoorReverbSubmixName = ShortOutdoorReverbSubmixCustom.ToString();
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortOutdoorReverbSubmixCustom));
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumOutdoorReverbSubmix))
        {
            MediumOutdoorReverbSubmixCustom = MediumOutdoorReverbSubmix;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].MediumOutdoorReverbSubmixName = MediumOutdoorReverbSubmixCustom.ToString();
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumOutdoorReverbSubmixCustom));
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongOutdoorReverbSubmix))
        {
            LongOutdoorReverbSubmixCustom = LongOutdoorReverbSubmix;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].LongOutdoorReverbSubmixName = LongOutdoorReverbSubmixCustom.ToString();
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongOutdoorReverbSubmixCustom));
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortReverbLength))
        {
            ShortReverbLengthCustom = ShortReverbLength;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].ShortReverbLength = ShortReverbLength;
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortReverbLengthCustom));
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumReverbLength))
        {
            MediumReverbLengthCustom = MediumReverbLength;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].MediumReverbLength = MediumReverbLength;
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumReverbLengthCustom));
        }
        else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongReverbLength))
        {
            LongReverbLengthCustom = LongReverbLength;
            ReverbBusesPresetMap[EReverbBusesPreset::Custom].LongReverbLength = LongReverbLength;
            Property = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongReverbLengthCustom));
        }

        // If we changed a custom property, update the custom copy in the config file
        if (Property != nullptr)
        {
            UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
        }
    }
}

bool UAcousticsSourceDataOverrideSettings::CanEditChange(const FProperty* InProperty) const
{
    const bool ParentVal = Super::CanEditChange(InProperty);

    if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortIndoorReverbSubmix) ||
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumIndoorReverbSubmix) ||
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongIndoorReverbSubmix) ||
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortOutdoorReverbSubmix) ||
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumOutdoorReverbSubmix) ||
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongOutdoorReverbSubmix) ||
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ShortReverbLength) ||
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, MediumReverbLength) ||
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, LongReverbLength))
    {
        // Only allow the reverb bus fields to be editable if using Custom preset and using stereo convolution reverb
        return ParentVal && 
            (ReverbBusesPreset == EReverbBusesPreset::Custom) && 
            (ReverbType == EAcousticsReverbType::StereoConvolution);
    }
    else if (
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, ReverbBusesPreset))
    {
        // Only allow the reverb bus preset type to be editable if using stereo convolution reverb
        return ParentVal && (ReverbType == EAcousticsReverbType::StereoConvolution);
    }
    else if (
        InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UAcousticsSourceDataOverrideSettings, SpatialReverbQuality))
    {
        // Only allow the spatial reverb quality to be editable if using spatial reverb
        return ParentVal && (ReverbType == EAcousticsReverbType::SpatialReverb);
    }
    else
    {
        return ParentVal;
    }
}

void UAcousticsSourceDataOverrideSettings::PostInitProperties()
{
    Super::PostInitProperties();

    // Once the project is loaded, ensure that the current configuration is dumped to the configuration
    // file, which is necessary for pathing to the default reverb impulse responses
    for (FProperty* Property = this->GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext)
    {
        UpdateSinglePropertyInConfigFile(Property, GetDefaultConfigFilename());
    }

    // Set our custom reverb bus preset based on the last used custom settings
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].ShortIndoorReverbSubmixName = ShortIndoorReverbSubmixCustom.ToString();
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].MediumIndoorReverbSubmixName = MediumIndoorReverbSubmixCustom.ToString();
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].LongIndoorReverbSubmixName = LongIndoorReverbSubmixCustom.ToString();
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].ShortOutdoorReverbSubmixName = ShortOutdoorReverbSubmixCustom.ToString();
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].MediumOutdoorReverbSubmixName = MediumOutdoorReverbSubmixCustom.ToString();
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].LongOutdoorReverbSubmixName = LongOutdoorReverbSubmixCustom.ToString();
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].ShortReverbLength = ShortReverbLengthCustom;
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].MediumReverbLength = MediumReverbLengthCustom;
    ReverbBusesPresetMap[EReverbBusesPreset::Custom].LongReverbLength = LongReverbLengthCustom;
}
#endif
