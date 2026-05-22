// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSourceDataOverrideSourceSettingsFactory.h"
#include "AcousticsSourceDataOverrideSourceSettings.h"
#include "AssetTypeCategories.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_AcousticsSourceDataOverrideSourceSettings::GetName() const
{
   return NSLOCTEXT("ProjectAcoustics", "AssetTypeActions_AcousticsSourceDataOverrideSourceSettings", "Project Acoustics Source Data Override Source Settings");
}

const TArray<FText>& FAssetTypeActions_AcousticsSourceDataOverrideSourceSettings::GetSubMenus() const
{
    static const TArray<FText> SubMenus
    {
        NSLOCTEXT("ProjectAcoustics", "AssetSoundProjectAcousticsSubMenu", "Project Acoustics")
    };

    return SubMenus;
}

FColor FAssetTypeActions_AcousticsSourceDataOverrideSourceSettings::GetTypeColor() const
{
   return FColor(100, 100, 100);
}

UClass* FAssetTypeActions_AcousticsSourceDataOverrideSourceSettings::GetSupportedClass() const
{
   return UAcousticsSourceDataOverrideSourceSettings::StaticClass();
}

uint32 FAssetTypeActions_AcousticsSourceDataOverrideSourceSettings::GetCategories()
{
   return EAssetTypeCategories::Sounds;
}

UAcousticsSourceDataOverrideSourceSettingsFactory::UAcousticsSourceDataOverrideSourceSettingsFactory(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
   SupportedClass = UAcousticsSourceDataOverrideSourceSettings::StaticClass();

   bCreateNew = true;
   bEditorImport = true;
   bEditAfterNew = true;
}

UObject* UAcousticsSourceDataOverrideSourceSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
   FFeedbackContext* Warn)
{
   return NewObject<UAcousticsSourceDataOverrideSourceSettings>(InParent, Name, Flags);
}

uint32 UAcousticsSourceDataOverrideSourceSettingsFactory::GetMenuCategories() const
{
   return EAssetTypeCategories::Sounds;
}

#undef LOCTEXT_NAMESPACE
