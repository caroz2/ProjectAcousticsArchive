// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsReverbSettingsFactory.h"
#include "AcousticsSpatializerSettings.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

FText FAssetTypeActions_AcousticsReverbSettings::GetName() const
{
    return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AcousticsReverbPluginPreset", "Project Acoustics Reverb Settings");
}

FColor FAssetTypeActions_AcousticsReverbSettings::GetTypeColor() const
{
    return FColor(100, 100, 100);
}

UClass* FAssetTypeActions_AcousticsReverbSettings::GetSupportedClass() const
{
    return UAcousticsSpatializerSettings::StaticClass();
}

uint32 FAssetTypeActions_AcousticsReverbSettings::GetCategories()
{
    return EAssetTypeCategories::Sounds;
}

const TArray<FText>& FAssetTypeActions_AcousticsReverbSettings::GetSubMenus() const
{
    static const TArray<FText> SubMenus
    {
        NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AssetSoundAcousticsSubMenu", "Project Acoustics")
    };

    return SubMenus;
}

UAcousticsReverbSettingsFactory::UAcousticsReverbSettingsFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SupportedClass = UAcousticsSpatializerSettings::StaticClass();

    bCreateNew = true;
    bEditorImport = false;
    bEditAfterNew = true;
}

UObject* UAcousticsReverbSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UAcousticsSpatializerSettings>(InParent, InName, Flags);
}

uint32 UAcousticsReverbSettingsFactory::GetMenuCategories() const
{
    return EAssetTypeCategories::Sounds;
}
