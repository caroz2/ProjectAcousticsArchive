// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "SimulationParametersCustomization.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "IDetailPropertyRow.h"

#define LOCTEXT_NAMESPACE "FSimulationParametersCustomization"

TSharedRef<IPropertyTypeCustomization> FSimulationParametersCustomization::MakeInstance()
{
    return MakeShareable(new FSimulationParametersCustomization());
}

void FSimulationParametersCustomization::CustomizeChildren(
    TSharedRef<class IPropertyHandle> propertyHandle, class IDetailChildrenBuilder& StructBuilder,
    IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    uint32 numChildren;
    propertyHandle->GetNumChildren(numChildren);
    for (auto i = 0u; i < numChildren; ++i)
    {
        auto handle = propertyHandle->GetChildHandle(i).ToSharedRef();
        auto& propertyRow = StructBuilder.AddProperty(handle);
    }
}

#undef LOCTEXT_NAMESPACE