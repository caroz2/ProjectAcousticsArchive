// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"
#include "AcousticsSourceDataOverrideSourceSettingsFactory.generated.h"

// For displaying our custom source settings in Asset menus
class FAssetTypeActions_AcousticsSourceDataOverrideSourceSettings : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;
    virtual const TArray<FText>& GetSubMenus() const override;
};

// For creating our custom source data override source settings
UCLASS(MinimalAPI, hidecategories = Object)
class UAcousticsSourceDataOverrideSourceSettingsFactory : public UFactory
{
    GENERATED_UCLASS_BODY()

    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual uint32 GetMenuCategories() const override;
};
