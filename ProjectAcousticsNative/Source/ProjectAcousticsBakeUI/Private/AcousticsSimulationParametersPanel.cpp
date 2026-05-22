// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsSimulationParametersPanel.h"
#include "SAcousticsEdit.h"
#include "Widgets/Input/SButton.h"
#include "AcousticsSharedState.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "SimulationParametersCustomization.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "SAcousticsSimulationParametersPanel"

void SAcousticsSimulationParametersPanel::Construct(const FArguments& InArgs)
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

    m_SimParamDetails.Initialize();
    auto& propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    m_DetailsView = propertyModule.CreateStructureDetailView(detailsViewArgs, structureViewArgs, nullptr);
    m_DetailsView->GetDetailsView()->RegisterInstancedCustomPropertyTypeLayout(
        "SimulationParameters",
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSimulationParametersCustomization::MakeInstance));
    m_DetailsView->GetOnFinishedChangingPropertiesDelegate().AddLambda(
        [this](const FPropertyChangedEvent&) { m_SimParamDetails.Update(); });
    m_DetailsView->SetStructureData(MakeShareable(
        new FStructOnScope(FSimulationParametersDetails::StaticStruct(), reinterpret_cast<uint8*>(&m_SimParamDetails))));

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
                .MinDesiredWidth(620)
                [
                    m_DetailsView->GetWidget()->AsShared()
                ]
            ]
        ]
    ];
    // clang-format on
}
#undef LOCTEXT_NAMESPACE