// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AzureCredentialsCustomization.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "IDetailPropertyRow.h"

#define LOCTEXT_NAMESPACE "FAzureCredentialsCustomization"

TSharedRef<IPropertyTypeCustomization> FAzureCredentialsCustomization::MakeInstance()
{
    return MakeShareable(new FAzureCredentialsCustomization());
}

void FAzureCredentialsCustomization::CustomizeChildren(
    TSharedRef<class IPropertyHandle> propertyHandle, class IDetailChildrenBuilder& StructBuilder,
    IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    uint32 numChildren;
    propertyHandle->GetNumChildren(numChildren);
    for (auto i = 0u; i < numChildren; ++i)
    {
        auto handle = propertyHandle->GetChildHandle(i).ToSharedRef();
        auto& propertyRow = StructBuilder.AddProperty(handle);

        if (handle->GetProperty()->GetName() == GET_MEMBER_NAME_CHECKED(FAzureCredentials, batch_key).ToString() ||
            handle->GetProperty()->GetName() == GET_MEMBER_NAME_CHECKED(FAzureCredentials, storage_key).ToString() ||
            handle->GetProperty()->GetName() == GET_MEMBER_NAME_CHECKED(FAzureCredentials, acr_key).ToString())
        {
            FText text;
            handle->GetValueAsDisplayText(text);

            // clang-format off
            propertyRow
            .CustomWidget()
            .NameContent()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .FillWidth(1.0f)
                [
                    handle->CreatePropertyNameWidget()
                ]
            ]
            .ValueContent()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .FillWidth(1.0f)
                [
                    SNew(SEditableTextBox)
                    .MinDesiredWidth(400)
                    .IsPassword(true)
                    .Text(text)
                    .OnTextCommitted_Static(&FAzureCredentialsCustomization::OnKeyTextChanged, handle)
                ]
            ];
            // clang-format on
        }
    }
}

void FAzureCredentialsCustomization::OnKeyTextChanged(
    const FText& text, ETextCommit::Type commitInfo, TSharedRef<IPropertyHandle> handle)
{
    if (commitInfo == ETextCommit::Default || commitInfo == ETextCommit::OnEnter ||
        commitInfo == ETextCommit::OnUserMovedFocus)
    {
        FString key = text.ToString();
        handle->SetValue(key);
    }
}

#undef LOCTEXT_NAMESPACE