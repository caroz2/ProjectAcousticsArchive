// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsAzureCredentialsPanel.h"
#include "SAcousticsEdit.h"
#include "Widgets/Input/SButton.h"
#include "AcousticsSharedState.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "AzureCredentialsCustomization.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "SAcousticsAzureCredentialsPanel"

void SAcousticsAzureCredentialsPanel::Construct(const FArguments& InArgs)
{
    // Initialize settings view
    FDetailsViewArgs detailsViewArgs;
    detailsViewArgs.bUpdatesFromSelection = false;
    detailsViewArgs.bLockable = false;
    detailsViewArgs.bAllowSearch = false;
    detailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    // Disable unused vertical scrollbar
    detailsViewArgs.bShowScrollBar = false;

    FStructureDetailsViewArgs structureViewArgs;
    structureViewArgs.bShowObjects = true;
    structureViewArgs.bShowAssets = true;
    structureViewArgs.bShowClasses = true;
    structureViewArgs.bShowInterfaces = true;

    m_Creds.Initialize();
    auto& propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    m_DetailsView = propertyModule.CreateStructureDetailView(detailsViewArgs, structureViewArgs, nullptr);
    m_DetailsView->GetDetailsView()->RegisterInstancedCustomPropertyTypeLayout(
        "AzureCredentials",
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FAzureCredentialsCustomization::MakeInstance));
    m_DetailsView->GetOnFinishedChangingPropertiesDelegate().AddLambda(
        [this](const FPropertyChangedEvent&) { m_Creds.Update(); });
    m_DetailsView->SetStructureData(MakeShareable(
        new FStructOnScope(FAzureCredentialsDetails::StaticStruct(), reinterpret_cast<uint8*>(&m_Creds))));

    // clang-format off
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        [
            SNew(SScrollBox)
            .Orientation(EOrientation::Orient_Horizontal)
            + SScrollBox::Slot()
            [
                SNew(SBox)
                .MinDesiredWidth(500)
                [
                    m_DetailsView->GetWidget()->AsShared()
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .Padding(FAcousticsEditSharedProperties::StandardPadding)
            .AutoWidth()
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .Text(FText::FromString(TEXT("Launch Azure Portal")))
                .OnClicked(this, &SAcousticsAzureCredentialsPanel::OnAzurePortalButton)
            ]
        ]
    ];
    // clang-format on
}

FReply SAcousticsAzureCredentialsPanel::OnAzurePortalButton()
{
    FString url = TEXT("https://portal.azure.com");
    FPlatformProcess::LaunchURL(*url, nullptr, nullptr);
    return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE