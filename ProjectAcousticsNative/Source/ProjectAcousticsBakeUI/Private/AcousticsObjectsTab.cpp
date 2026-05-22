// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsObjectsTab.h"
#include "AcousticsSharedState.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckbox.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/SToolTip.h"
#include "SlateOptMacros.h"

#include "EditorModeManager.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

#include "CoreGlobals.h"
#include "Misc/ConfigCacheIni.h"
#include "SourceControlHelpers.h"
#include "Interfaces/IPluginManager.h"
#include <AcousticsShared.h>

#define LOCTEXT_NAMESPACE "SAcousticsObjectsTab"

// Static const string for the section name in the config file
// for the list of maps using physical materials.
const FString SAcousticsObjectsTab::UsePhysicalMaterialsSectionString = "UsePhysicalMaterials";

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAcousticsObjectsTab::Construct(const FArguments& InArgs, SAcousticsEdit* ownerEdit)
{
    m_Owner = ownerEdit;
    m_AcousticsEditMode =
        static_cast<FAcousticsEdMode*>(GLevelEditorModeTools().GetActiveMode(FAcousticsEdMode::EM_AcousticsEdModeId));
    FSlateFontInfo StandardFont = STYLER::GetFontStyle(TEXT("PropertyWindow.NormalFont"));

    const FString helpTextTitle = TEXT("Step One");
    const FString helpText =
        TEXT("Tag the geometry and navigation objects in the scene that should impact the acoustics simulation. Use the Bulk Selection Helpers to easily select all objects of a given type.");

    // Read the value of the UsePhysicalMaterials checkbox from the config file.
    FString CurrentMapName = GWorld->GetMapName();

    // Get whether physical materials are being used for acoustics bake in this level from the config file.
    // Always default to false before we search through the config file as each map can have its own setting,
    // and this variable is global for m_AcousticsEditMode.
    // Following ConfigFile code will change it if it is found in the configuration.
    m_AcousticsEditMode->UsePhysicalMaterials = false;
    FConfigFile* BaseProjectAcousticsConfigFile;
    FString ConfigFilePath;
    if (m_AcousticsEditMode->GetConfigFile(&BaseProjectAcousticsConfigFile, ConfigFilePath))
    {
        // GetBool() will not change m_AcousticsEditMode->UsePhysicalMaterials if the config is not found.
        BaseProjectAcousticsConfigFile->GetBool(
            *UsePhysicalMaterialsSectionString, *CurrentMapName, m_AcousticsEditMode->UsePhysicalMaterials);
    }

    // clang-format off
    ChildSlot
        [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(SErrorText)
            .Visibility_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive() ? EVisibility::Collapsed : EVisibility::Visible; })
            .ErrorText(LOCTEXT("ObjectsTabMessage", "Clear the preview on the Probes tab to make changes"))
            .BackgroundColor(STYLER::GetColor("InfoReporting.BackgroundColor"))
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SAcousticsEdit::MakeHelpTextWidget(helpTextTitle, helpText)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(SExpandableArea)
            .InitiallyCollapsed(true)
            .AreaTitle(FText::FromString("Bulk Selection Helpers"))
            .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.2f))
            .AreaTitleFont(STYLER::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
            .BodyContent()
            [
                // Object selection checkboxes
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .Padding(FAcousticsEditSharedProperties::StandardPadding)
                .AutoHeight()
                [
                    SNew(SHorizontalBox)

                    +SHorizontalBox::Slot()
                    .VAlign(VAlign_Center)
                    .Padding(FAcousticsEditSharedProperties::StandardPadding)
                    [
                        SNew(SWrapBox)
                            .UseAllottedSize(true)
                        .InnerSlotPadding({ 6, 5 })

                        +SWrapBox::Slot()
                        [
                            SNew(SBox)
                            .MinDesiredWidth(91)
                            [
                                SNew(SCheckBox)
                                .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                                .OnCheckStateChanged(this, &SAcousticsObjectsTab::OnCheckStateChanged_StaticMesh)
                                .IsChecked(this, &SAcousticsObjectsTab::GetCheckState_StaticMesh)
                                .ToolTipText(LOCTEXT("StaticMeshTooltip", "Select all static meshes marked as static or stationary (not moveable)"))
                                [
                                    SNew(STextBlock)
                                    .Text(LOCTEXT("StaticMesh", "Static Meshes"))
                                    .Font(StandardFont)
                                ]
                            ]
                        ]

                        + SWrapBox::Slot()
                        [
                            SNew(SBox)
                            .MinDesiredWidth(91)
                            [
                                SNew(SCheckBox)
                                .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                                .OnCheckStateChanged(this, &SAcousticsObjectsTab::OnCheckStateChanged_NavMesh)
                                .IsChecked(this, &SAcousticsObjectsTab::GetCheckState_NavMesh)
                                .ToolTipText(LOCTEXT("NavMeshTooltip", "Select all Navigation Meshes"))
                                [
                                    SNew(STextBlock)
                                    .Text(LOCTEXT("NavMesh", "Navigation Meshes"))
                                    .Font(StandardFont)
                                ]
                            ]
                        ]

                        + SWrapBox::Slot()
                        [
                            SNew(SBox)
                            .MinDesiredWidth(91)
                            [
                                SNew(SCheckBox)
                                .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                                .OnCheckStateChanged(this, &SAcousticsObjectsTab::OnCheckStateChanged_Landscape)
                                .IsChecked(this, &SAcousticsObjectsTab::GetCheckState_Landscape)
                                .ToolTipText(LOCTEXT("LandscapeTooltip", "Select all Landscapes"))
                                [
                                    SNew(STextBlock)
                                    .Text(LOCTEXT("Landscapes", "Landscapes"))
                                    .Font(StandardFont)
                                ]
                            ]
                        ]
                    ]
                ]

                // Object selection checkboxes - row 2
                + SVerticalBox::Slot()
                .Padding(FAcousticsEditSharedProperties::StandardPadding)
                .AutoHeight()
                [
                    SNew(SHorizontalBox)

                    +SHorizontalBox::Slot()
                    .VAlign(VAlign_Center)
                    .Padding(FAcousticsEditSharedProperties::StandardPadding)
                    [
                        SNew(SWrapBox)
                            .UseAllottedSize(true)
                        .InnerSlotPadding({ 6, 5 })

                        + SWrapBox::Slot()
                        [
                            SNew(SBox)
                            .MinDesiredWidth(91)
                            [
                                SNew(SCheckBox)
                                .IsEnabled(this, &SAcousticsObjectsTab::GetMovableStaticMeshEnabled)
                                .OnCheckStateChanged(this, &SAcousticsObjectsTab::OnCheckStateChanged_MovableStaticMesh)
                                .IsChecked(this, &SAcousticsObjectsTab::GetCheckState_MovableStaticMesh)
                                .ToolTipText(LOCTEXT("MovableMeshTooltip", "Include movable static meshes with static mesh selection. Note: The acoustics bake will only use the starting position of the movable mesh."))
                                [
                                    SNew(STextBlock)
                                    .Text(LOCTEXT("MovableStaticMesh", "Movable Static Meshes"))
                                    .Font(StandardFont)
                                ]
                            ]
                        ]
                    ]
                ]

                // Selection Buttons
                + SVerticalBox::Slot()
                .Padding(FAcousticsEditSharedProperties::StandardPadding)
                .AutoHeight()
                [
                    SNew(SWrapBox)
                        .UseAllottedSize(true)

                    // Select all instances
                    +SWrapBox::Slot()
                    .Padding(FAcousticsEditSharedProperties::StandardPadding)
                    [
                        SNew(SBox)
                        .MinDesiredWidth(60.f)
                        .HeightOverride(25.f)
                        [
                            SNew(SButton)
                            .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                            .HAlign(HAlign_Center)
                            .VAlign(VAlign_Center)
                            .OnClicked(this, &SAcousticsObjectsTab::OnSelectObjects)
                            .Text(LOCTEXT("Select", "Select"))
                            .ToolTipText(LOCTEXT("Select_Tooltip", "Selects all objects matching the filter"))
                        ]
                    ]

                    // Deselect everything
                    +SWrapBox::Slot()
                    .Padding(FAcousticsEditSharedProperties::StandardPadding)
                    [
                        SNew(SBox)
                        .MinDesiredWidth(90.f)
                        .HeightOverride(25.f)
                        [
                            SNew(SButton)
                            .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                            .HAlign(HAlign_Center)
                            .VAlign(VAlign_Center)
                            .OnClicked(this, &SAcousticsObjectsTab::OnUnselectObjects)
                            .Text(LOCTEXT("Deselect", "Deselect all"))
                            .ToolTipText(LOCTEXT("Unselect_Tooltip", "Deselect all objects"))
                        ]
                    ]
                ]
            ]
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::ExtraPadding)
        [
            SNew(SHeader)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("TagHeader", "Tagging"))
            ]
        ]

        // Selection count text
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Text_Lambda([this]() { return FText::FromString(m_NumSelected); })
        ]

        // Add tag selectors
        + SVerticalBox::Slot()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        .AutoHeight()
        [
            // Geometry Tag
            SNew(SWrapBox)
                .UseAllottedSize(true)
            +SWrapBox::Slot()
            .Padding(FAcousticsEditSharedProperties::StandardPadding * FMargin(0.f, 0.f, 1.f, 0.f))
            [
                SNew(SCheckBox)
                .Style(STYLER::Get(), "RadioButton")
                .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                .Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
                .IsChecked(this, &SAcousticsObjectsTab::IsAcousticsRadioButtonChecked)
                .OnCheckStateChanged(this, &SAcousticsObjectsTab::OnAcousticsRadioButtonChanged)
                .ToolTipText(LOCTEXT("GeometryTag_Tooltip", "Add the Geometry tag to any objects that will have an effect on the sound (walls, floors, etc)."))
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("GeometryTag", "Geometry"))
                    .Font(StandardFont)
                ]
            ]

            // Navigation Tag
            +SWrapBox::Slot()
            .Padding(FAcousticsEditSharedProperties::StandardPadding * FMargin(1.f, 0.f, 0.f, 0.f))
            [
                SNew(SCheckBox)
                .Style(STYLER::Get(), "RadioButton")
                .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                .Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
                .IsChecked(this, &SAcousticsObjectsTab::IsNavigationRadioButtonChecked)
                .OnCheckStateChanged(this, &SAcousticsObjectsTab::OnNavigationRadioButtonChanged)
                .ToolTipText(LOCTEXT("NavigationTag_Tooltip", "Add Navigation tag to meshes that define where the player can navigate. This informs where listener probes are placed for wave physics simulation. At least one object must have this tag."))
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("NavigationTag", "Navigation"))
                    .Font(StandardFont)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        .AutoHeight()
        [
            // Add Tag
            SNew(SWrapBox)
                .UseAllottedSize(true)
            +SWrapBox::Slot()
            .Padding(FAcousticsEditSharedProperties::StandardPadding * FMargin(0.f, 0.f, 1.f, 0.f))
            [
                SNew(SBox)
                .MinDesiredWidth(60.f)
                .HeightOverride(25.f)
                [
                    SNew(SButton)
                    .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                    .HAlign(HAlign_Center)
                    .VAlign(VAlign_Center)
                    .OnClicked(this, &SAcousticsObjectsTab::OnAddTag)
                    .Text(LOCTEXT("Tag", "Tag"))
                    .ToolTipText(LOCTEXT("AddTag_Tooltip", "Add Tag to all selected objects"))
                ]
            ]

            // Clear Tag
            +SWrapBox::Slot()
            .Padding(FAcousticsEditSharedProperties::StandardPadding * FMargin(1.f, 0.f, 1.f, 0.f))
            [
                SNew(SBox)
                .MinDesiredWidth(60.f)
                .HeightOverride(25.f)
                [
                    SNew(SButton)
                    .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                    .HAlign(HAlign_Center)
                    .VAlign(VAlign_Center)
                    .OnClicked(this, &SAcousticsObjectsTab::OnClearTag)
                    .Text(LOCTEXT("Untag", "Untag"))
                    .ToolTipText(LOCTEXT("ClearTag_Tooltip", "Remove Tag from all selected objects"))
                ]
            ]

            // Select All Tagged items
            + SWrapBox::Slot()
            .Padding(FAcousticsEditSharedProperties::StandardPadding * FMargin(1.f, 0.f, 0.f, 0.f))
            [
                SNew(SBox)
                .MinDesiredWidth(60.f)
                .HeightOverride(25.f)
                [
                    SNew(SButton)
                    .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                    .HAlign(HAlign_Center)
                    .VAlign(VAlign_Center)
                    .OnClicked(this, &SAcousticsObjectsTab::OnSelectAllTag)
                    .Text(LOCTEXT("SelectTagged", "Select Tagged"))
                    .ToolTipText(LOCTEXT("SelectAll_Tooltip", "Select all objects with current tag"))
                ]
            ]
        ]

        // Display Tagged Stats
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding * FMargin(1.f, 6.f, 1.f, 1.f))
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Text_Lambda([this]() { return FText::FromString(m_NumGeo); })
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding * FMargin(1.f, 0.f, 1.f, 1.f))
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .Text_Lambda([this]() { return FText::FromString(m_NumNav); })
        ]
		
		// Added a check box for physical material support.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FAcousticsEditSharedProperties::ExtraPadding * FMargin(1.f, 1.5f, 1.f, 1.f))
		[
			SNew(SBox)
			[
				SNew(SCheckBox)
                .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
				.OnCheckStateChanged(this, &SAcousticsObjectsTab::OnCheckStateChanged_UsePhysicalMaterials)
				.IsChecked(this, &SAcousticsObjectsTab::GetCheckState_UsePhysicalMaterials)
				.ToolTipText(LOCTEXT("UsePhysicalMaterialsTooltip", "Whether physical materials should be used"))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PhysMat", "Use Physical Materials"))
					.Font(StandardFont)
				]
			]
		]
    ];
    // clang-format on
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAcousticsObjectsTab::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    // For the objects tab, every frame we should count up the number of tagged items and update our counts
    // This is because users can change the tags outside of our UI. This method makes sure the displayed
    // tagged counts are always accurate

    int selected = 0;
    int geo = 0;
    int nav = 0;
    for (TActorIterator<AActor> ActorItr(GEditor->GetEditorWorldContext().World()); ActorItr; ++ActorItr)
    {
        AActor* actor = *ActorItr;
        if (actor->IsSelectedInEditor())
        {
            selected += 1;
        }
        if (actor->ActorHasTag(c_AcousticsGeometryTag))
        {
            geo += 1;
        }
        if (actor->ActorHasTag(c_AcousticsNavigationTag))
        {
            nav += 1;
        }
    }
    m_NumSelected = FString::Printf(TEXT("Currently selected objects: %d"), selected);
    m_NumGeo = FString::Printf(TEXT("Tagged objects for Geometry: %d"), geo);
    m_NumNav = FString::Printf(TEXT("Tagged objects for Navigation: %d"), nav);
}

void SAcousticsObjectsTab::OnAcousticsRadioButtonChanged(ECheckBoxState inState)
{
    bool isChecked = (inState == ECheckBoxState::Checked ? true : false);
    if (isChecked)
    {
        m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsAcousticsRadioButtonChecked = true;
        m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsNavigationRadioButtonChecked = false;
    }
}

ECheckBoxState SAcousticsObjectsTab::IsAcousticsRadioButtonChecked() const
{
    return m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsAcousticsRadioButtonChecked
               ? ECheckBoxState::Checked
               : ECheckBoxState::Unchecked;
}

void SAcousticsObjectsTab::OnNavigationRadioButtonChanged(ECheckBoxState inState)
{
    bool isChecked = (inState == ECheckBoxState::Checked ? true : false);
    if (isChecked)
    {
        m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsNavigationRadioButtonChecked = true;
        m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsAcousticsRadioButtonChecked = false;
    }
}

ECheckBoxState SAcousticsObjectsTab::IsNavigationRadioButtonChecked() const
{
    return m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsNavigationRadioButtonChecked
               ? ECheckBoxState::Checked
               : ECheckBoxState::Unchecked;
}

void SAcousticsObjectsTab::OnCheckStateChanged_StaticMesh(ECheckBoxState InState)
{
    m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsStaticMeshChecked =
        (InState == ECheckBoxState::Checked ? true : false);

    // Unselect movable static mesh checkbox when static mesh becomes unselected
    if (InState == ECheckBoxState::Unchecked)
    {
        m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsMovableStaticMeshChecked = false;
    }
}

ECheckBoxState SAcousticsObjectsTab::GetCheckState_StaticMesh() const
{
    return m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsStaticMeshChecked ? ECheckBoxState::Checked
                                                                                           : ECheckBoxState::Unchecked;
}

void SAcousticsObjectsTab::OnCheckStateChanged_MovableStaticMesh(ECheckBoxState InState)
{
    m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsMovableStaticMeshChecked =
        (InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SAcousticsObjectsTab::GetCheckState_MovableStaticMesh() const
{
    return m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsMovableStaticMeshChecked ? ECheckBoxState::Checked
        : ECheckBoxState::Unchecked;
}

bool SAcousticsObjectsTab::GetMovableStaticMeshEnabled() const
{
    return m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsStaticMeshChecked && !AcousticsSharedState::IsPrebakeActive();
}

void SAcousticsObjectsTab::OnCheckStateChanged_NavMesh(ECheckBoxState InState)
{
    m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsNavMeshChecked =
        (InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SAcousticsObjectsTab::GetCheckState_NavMesh() const
{
    return m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsNavMeshChecked ? ECheckBoxState::Checked
                                                                                        : ECheckBoxState::Unchecked;
}

void SAcousticsObjectsTab::OnCheckStateChanged_Landscape(ECheckBoxState InState)
{
    m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsLandscapeChecked =
        (InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SAcousticsObjectsTab::GetCheckState_Landscape() const
{
    return m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsLandscapeChecked ? ECheckBoxState::Checked
                                                                                          : ECheckBoxState::Unchecked;
}

// Update when the physical material checkbox state changes.
void SAcousticsObjectsTab::OnCheckStateChanged_UsePhysicalMaterials(ECheckBoxState InState)
{
    m_AcousticsEditMode->UsePhysicalMaterials = (InState == ECheckBoxState::Checked ? true : false);

    // Write to config file.
    FConfigFile* BaseProjectAcousticsConfigFile;
    FString ConfigFilePath;
    if (m_AcousticsEditMode->GetConfigFile(&BaseProjectAcousticsConfigFile, ConfigFilePath))
    {
        BaseProjectAcousticsConfigFile->SetString(
            *UsePhysicalMaterialsSectionString,
            *(GWorld->GetMapName()),
            (m_AcousticsEditMode->UsePhysicalMaterials ? TEXT("true") : TEXT("false")));
        if (FAcousticsEdMode::IsSourceControlAvailable())
        {
            USourceControlHelpers::CheckOutOrAddFile(ConfigFilePath);
        }
        BaseProjectAcousticsConfigFile->Write(ConfigFilePath);
    }
}

ECheckBoxState SAcousticsObjectsTab::GetCheckState_UsePhysicalMaterials() const
{
    return m_AcousticsEditMode->UsePhysicalMaterials ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FReply SAcousticsObjectsTab::OnSelectObjects()
{
    m_AcousticsEditMode->SelectObjects();
    return FReply::Handled();
}
FReply SAcousticsObjectsTab::OnUnselectObjects()
{
    GEditor->SelectNone(true, true, false);
    return FReply::Handled();
}

FReply SAcousticsObjectsTab::OnAddTag()
{
    auto success = false;
    if (m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsNavigationRadioButtonChecked)
    {
        success = m_AcousticsEditMode->TagNavigation(true);
        if (!success)
        {
            m_Owner->SetError(TEXT("Failed to tag one or more objects for Navigation. See Output Log for more details."));
        }
    }
    else
    {
        success = m_AcousticsEditMode->TagGeometry(true);
        if (!success)
        {
            m_Owner->SetError(TEXT("Failed to tag one or more objects for Geometry. See Output Log for more details."));
        }
    }
    if (success)
    {
        m_Owner->SetError(TEXT(""));
    }
    return FReply::Handled();
}

FReply SAcousticsObjectsTab::OnClearTag()
{
    if (m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsNavigationRadioButtonChecked)
    {
        m_AcousticsEditMode->TagNavigation(false);
    }
    else
    {
        m_AcousticsEditMode->TagGeometry(false);
    }
    return FReply::Handled();
}

FReply SAcousticsObjectsTab::OnSelectAllTag()
{
    auto tag = c_AcousticsGeometryTag;
    if (m_AcousticsEditMode->AcousticsUISettings.ObjectsTabSettings.IsNavigationRadioButtonChecked)
    {
        tag = c_AcousticsNavigationTag;
    }

    GEditor->SelectNone(true, true, false);
    for (TActorIterator<AActor> ActorItr(GEditor->GetEditorWorldContext().World()); ActorItr; ++ActorItr)
    {
        AActor* actor = *ActorItr;
        if (actor->ActorHasTag(tag))
        {
            GEditor->SelectActor(actor, true, false, true, false);
        }
    }

    GEditor->NoteSelectionChange();
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
