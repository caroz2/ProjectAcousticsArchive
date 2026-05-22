// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Factories/Factory.h"
#include "AssetTypeActions_Base.h"
#include "AcousticsReverbSettingsFactory.generated.h"

class FAssetTypeActions_AcousticsReverbSettings : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;
    virtual const TArray<FText>& GetSubMenus() const override;
};

UCLASS(MinimalAPI, hidecategories = Object)
class UAcousticsReverbSettingsFactory : public UFactory
{
    GENERATED_UCLASS_BODY()

    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual uint32 GetMenuCategories() const override;
};
