// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ComputePoolConfigurationDetailsCustomization.h"
#include "Widgets/Input/SCheckBox.h"
#include "IDetailPropertyRow.h"
#include "AcousticsComputePoolConfigurationPanel.h"

#define LOCTEXT_NAMESPACE "FComputePoolConfigurationCustomization"

TSharedRef<IPropertyTypeCustomization> FComputePoolConfigurationDetailsCustomization::MakeInstance()
{
    return MakeShareable(new FComputePoolConfigurationDetailsCustomization());
}

void FComputePoolConfigurationDetailsCustomization::CustomizeChildren(
    TSharedRef<class IPropertyHandle> propertyHandle, class IDetailChildrenBuilder& ChildBuilder,
    IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    uint32 numChildren;
    propertyHandle->GetNumChildren(numChildren);
    for (auto i = 0u; i < numChildren; ++i)
    {
        auto handle = propertyHandle->GetChildHandle(i).ToSharedRef();
        auto& propertyRow = ChildBuilder.AddProperty(handle);
    }
}

#undef LOCTEXT_NAMESPACE