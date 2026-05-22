// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//   Created from SFoliageEdit class code

#include "SAcousticsEdit.h"
#include "AcousticsEdMode.h"
#include "Fonts/SlateFontInfo.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/SOverlay.h"
#include "Styling/SlateTypes.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "EditorModeManager.h"
#include "EditorModes.h"

#include "AcousticsEditActions.h"
#include "AcousticsProbesTab.h"
#include "AcousticsBakeTab.h"
#include "AcousticsMaterialsTab.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SErrorText.h"
#include "Styling/SlateStyleRegistry.h"
#include "AcousticsSharedState.h"
#include "AcousticsObjectsTab.h"
#if ENGINE_MAJOR_VERSION == 5
#include "Widgets/Input/SSegmentedControl.h"
#endif

#define LOCTEXT_NAMESPACE "AcousticsEd_Mode"

const FMargin FAcousticsEditSharedProperties::StandardPadding = FMargin(6.f, 3.f);
const FMargin FAcousticsEditSharedProperties::ExtraPadding = FMargin(6.f, 12.f);
const FMargin FAcousticsEditSharedProperties::StandardLeftPadding = FMargin(6.f, 3.f, 3.f, 3.f);
const FMargin FAcousticsEditSharedProperties::StandardExtraTopPadding = FMargin(3.f, 20.f, 3.f, 3.f);
const FMargin FAcousticsEditSharedProperties::StandardRightPadding = FMargin(3.f, 3.f, 6.f, 3.f);
const FMargin FAcousticsEditSharedProperties::StandardTextMargin = FMargin(2.f, 2.f);

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAcousticsEdit::Construct(const FArguments& InArgs)
{
    m_AcousticsEditMode =
        static_cast<FAcousticsEdMode*>(GLevelEditorModeTools().GetActiveMode(FAcousticsEdMode::EM_AcousticsEdModeId));

    auto acousticsSlateStyle = FSlateStyleRegistry::FindSlateStyle("AcousticsEditor");

    // Set the initial UI shared state from loaded configuration
    AcousticsSharedState::Initialize();

    const FText BlankText = LOCTEXT("Blank", "");

    // clang-format off
    ChildSlot
    [
        SNew(SVerticalBox)

#pragma region ErrorTextArea
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 0, 0, 5)
        [
            SAssignNew(m_ErrorText, SErrorText)
        ]
#pragma endregion ErrorTextArea

#pragma region MainWindowArea
        + SVerticalBox::Slot()
        .FillHeight(1.0)
        .Padding(0)
        [
#pragma region ToolbarPlusTabContent
            SNew(SVerticalBox)

#pragma region Toolbar
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(1.f, 5.f, 0.f, 5.f)
            [
            #if ENGINE_MAJOR_VERSION == 5
                SNew(SSegmentedControl<AcousticsActiveTab>)
                .Value(this, &SAcousticsEdit::GetActiveTab)
                .OnValueChanged(this, &SAcousticsEdit::OnActiveTabChanged)
                +SSegmentedControl<AcousticsActiveTab>::Slot(AcousticsActiveTab::ObjectTag)
                .Text(LOCTEXT("ObjectsTabText", "Objects"))
                .Icon(acousticsSlateStyle->GetBrush("AcousticsEditMode.SetObjectTag"))
                .ToolTip(LOCTEXT("TagObjectsTooltip", "1. Tag scene objects for acoustics geometry and navigation."))
                + SSegmentedControl<AcousticsActiveTab>::Slot(AcousticsActiveTab::Materials)
                .Text(LOCTEXT("MaterialsTabText", "Materials"))
                .Icon(acousticsSlateStyle->GetBrush("AcousticsEditMode.SetMaterials"))
                .ToolTip(LOCTEXT("MaterialsTabTooltip", "2. Assign acoustic properties to materials"))
                + SSegmentedControl<AcousticsActiveTab>::Slot(AcousticsActiveTab::Probes)
                .Text(LOCTEXT("ProbesTabText", "Probes"))
                .Icon(acousticsSlateStyle->GetBrush("AcousticsEditMode.SetProbes"))
                .ToolTip(LOCTEXT("ProbesTabTooltip", "3. Calculate probes and voxels (pre-bake)"))
                + SSegmentedControl<AcousticsActiveTab>::Slot(AcousticsActiveTab::Bake)
                .ToolTip(LOCTEXT("BakeTabText", "4. Submit cloud bake"))
                .Icon(acousticsSlateStyle->GetBrush("AcousticsEditMode.SetBake"))
                .Text(LOCTEXT("BakeTabTooltip", "Bake"))
            #else
                BuildToolBar()
            #endif
            ]
            
#pragma endregion Toolbar

#pragma region TabContent
            + SVerticalBox::Slot()
            .Padding(0.f, 2.f, 2.f, 0.f)
            [
                SNew(SBorder)
                .BorderImage(STYLER::GetBrush("ToolPanel.DarkGroupBorder"))
                .Padding(FAcousticsEditSharedProperties::StandardPadding)
                [
                    SNew(SVerticalBox)

#pragma region TabTitle
                    // Active Tab Title
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SHorizontalBox)

                        + SHorizontalBox::Slot()
                        .Padding(FAcousticsEditSharedProperties::StandardLeftPadding)
                        .HAlign(HAlign_Left)
                        [
                            SNew(STextBlock)
                            .Text(this, &SAcousticsEdit::GetActiveTabName)
                            .TextStyle(acousticsSlateStyle, "AcousticsEditMode.ActiveTabName.Text")
                        ]
                    ]
#pragma endregion TabTitle

                    // The tabs go here. Each should be a SVerticalBox::Slot() that sets its visibility
                    //   to Visible when selected and Collapsed otherwise.
#pragma region Tabs
                    + SVerticalBox::Slot()
                    .FillHeight(1.0)
                    [
                        SNew(SBorder)
                        .BorderImage(STYLER::GetBrush("ToolPanel.DarkGroupBorder"))
                        .Padding(FAcousticsEditSharedProperties::StandardPadding)
                        .Visibility(this, &SAcousticsEdit::GetObjectTagTabVisibility)
                        [
                            SNew(SScrollBox)
                            .ScrollBarAlwaysVisible(false)

                            + SScrollBox::Slot()
                            [
                                BuildObjectTagTab()
                            ]
                        ]
                    ]

                    + SVerticalBox::Slot()
                    .FillHeight(1.0)
                    [
                        SNew(SBorder)
                        .BorderImage(STYLER::GetBrush("ToolPanel.DarkGroupBorder"))
                        .Padding(FAcousticsEditSharedProperties::StandardPadding)
                        .Visibility(this, &SAcousticsEdit::GetMaterialsTabVisibility)
                        [
                            // Materials tab handles scrolling itself (in the ListView)
                            BuildMaterialsTab()
                        ]
                    ]

                    + SVerticalBox::Slot()
                    .FillHeight(1.0)
                    [
                        SNew(SBorder)
                        .BorderImage(STYLER::GetBrush("ToolPanel.DarkGroupBorder"))
                        .Padding(FAcousticsEditSharedProperties::StandardPadding)
                        .Visibility(this, &SAcousticsEdit::GetProbesTabVisibility)
                        [
                            SNew(SScrollBox)
                            .ScrollBarAlwaysVisible(false)

                            + SScrollBox::Slot()
                            [
                                BuildProbesTab()
                            ]
                        ]
                    ]

                    + SVerticalBox::Slot()
                    .FillHeight(1.0)
                    [
                        SNew(SBorder)
                        .BorderImage(STYLER::GetBrush("ToolPanel.DarkGroupBorder"))
                        .Padding(FAcousticsEditSharedProperties::StandardPadding)
                        .Visibility(this, &SAcousticsEdit::GetBakeTabVisibility)
                        [
                            SNew(SScrollBox)
                            .ScrollBarAlwaysVisible(false)

                            + SScrollBox::Slot()
                            [
                                BuildBakeTab()
                            ]
                        ]
                    ]
#pragma endregion Tabs
                ]
            ]
#pragma endregion TabContent
#pragma endregion ToolbarPlusTabContent
        ]
#pragma endregion MainWindowArea
    ];
    // clang-format on

    SetError(TEXT(""));

    RefreshFullList();
}

void SAcousticsEdit::SetError(FString errorText)
{
    m_ErrorText->SetError(FText::FromString(errorText));
}

TSharedRef<SWidget> SAcousticsEdit::BuildObjectTagTab()
{
    return SNew(SAcousticsObjectsTab, this);
}

TSharedRef<SWidget> SAcousticsEdit::BuildMaterialsTab()
{
    TSharedPtr<SAcousticsMaterialsTab> materialsTab;
    auto widget = SAssignNew(materialsTab, SAcousticsMaterialsTab);
    m_AcousticsEditMode->SetMaterialsTab(MoveTemp(materialsTab));
    return widget;
}

TSharedRef<SWidget> SAcousticsEdit::BuildProbesTab()
{
    return SNew(SAcousticsProbesTab, this);
}

TSharedRef<SWidget> SAcousticsEdit::BuildBakeTab()
{
    TSharedPtr<SAcousticsBakeTab> bakeTab;
    auto widget = SAssignNew(bakeTab, SAcousticsBakeTab, this);
    m_AcousticsEditMode->SetBakeTab(MoveTemp(bakeTab));
    return widget;
}

TSharedRef<SWidget> SAcousticsEdit::MakeHelpTextWidget(const FString& title, const FString& text)
{
    // clang-format off
    return
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .Padding(0, 0, 0, 5)
        [
            SNew(SExpandableArea)
            .AreaTitle(FText::FromString(title))
            .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.2f))
            .AreaTitleFont(STYLER::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
            .BodyContent()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    [
                        SNew(SBox)
                        .MinDesiredWidth(91)
                        [
                            SNew(STextBlock)
                            .Font(STYLER::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
                            .AutoWrapText(true)
                            .Margin(FAcousticsEditSharedProperties::StandardTextMargin)
                            .Text(FText::FromString(text))
                        ]
                    ]
                ]
            ]
        ];
    // clang-format on
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAcousticsEdit::RefreshFullList()
{
}

TSharedRef<SWidget> SAcousticsEdit::BuildToolBar()
{
    FToolBarBuilder Toolbar(m_AcousticsEditMode->UICommandList, FMultiBoxCustomization::None);

    {
        Toolbar.AddToolBarButton(FAcousticsEditCommands::Get().SetObjectTag);
        Toolbar.AddToolBarButton(FAcousticsEditCommands::Get().SetMaterials);
        Toolbar.AddToolBarButton(FAcousticsEditCommands::Get().SetProbes);
        Toolbar.AddToolBarButton(FAcousticsEditCommands::Get().SetBake);
    }

    // clang-format off
    return
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        [
            SNew(SOverlay)
            + SOverlay::Slot()
            [
                SNew(SBorder)
                .HAlign(HAlign_Center)
                .Padding(0)
                .BorderImage(STYLER::GetBrush("NoBorder"))
                .IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
                [
                    Toolbar.MakeWidget()
                ]
            ]
        ];
    // clang-format on
}

bool SAcousticsEdit::IsObjectMarkTab() const
{
    return (m_AcousticsEditMode->AcousticsUISettings.CurrentTab == AcousticsActiveTab::ObjectTag);
}

bool SAcousticsEdit::IsMaterialsTab() const
{
    return (m_AcousticsEditMode->AcousticsUISettings.CurrentTab == AcousticsActiveTab::Materials);
}

bool SAcousticsEdit::IsProbesTab() const
{
    return (m_AcousticsEditMode->AcousticsUISettings.CurrentTab == AcousticsActiveTab::Probes);
}

bool SAcousticsEdit::IsBakeTab() const
{
    return (m_AcousticsEditMode->AcousticsUISettings.CurrentTab == AcousticsActiveTab::Bake);
}

FText SAcousticsEdit::GetActiveTabName() const
{
    FText OutText = LOCTEXT("AcousticsTabName_Invalid", "InvalidState");

    if (IsObjectMarkTab())
    {
        OutText = LOCTEXT("AcousticsTabName_ObjectMark", "Objects");
    }
    else if (IsMaterialsTab())
    {
        OutText = LOCTEXT("AcousticsTabName_Materials", "Materials");
    }
    else if (IsProbesTab())
    {
        OutText = LOCTEXT("AcousticsTabName_Probes", "Probes");
    }
    else if (IsBakeTab())
    {
        OutText = LOCTEXT("AcousticsTabName_Bake", "Bake");
    }

    return OutText;
}

#undef LOCTEXT_NAMESPACE
