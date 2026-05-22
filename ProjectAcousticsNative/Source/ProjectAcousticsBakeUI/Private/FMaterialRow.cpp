// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "FMaterialRow.h"
#include "AcousticsMaterialsTab.h"
#include "SAcousticsEdit.h"
#include "AcousticsEdMode.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Fonts/SlateFontInfo.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Slate/Public/Widgets/Text/STextBlock.h"
#include <assert.h>
#include "Containers/UnrealString.h"
#include "AcousticsSharedState.h"
#include "Misc/ConfigCacheIni.h"
#include "EditorModeManager.h"
#include "Interfaces/IPluginManager.h"
#include "SourceControlHelpers.h"

void FMaterialRow::Construct(
    const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<MaterialItem> InItem,
    TArray<TSharedPtr<TritonAcousticMaterial>> ComboBoxMaterials)
{
    m_MaterialItem = InItem;
    SMultiColumnTableRow<TSharedPtr<MaterialItem>>::Construct(FSuperRowType::FArguments(), InOwnerTable);
    m_ComboBoxMaterialsList = ComboBoxMaterials;
    m_AcousticsEditMode =
        static_cast<FAcousticsEdMode*>(GLevelEditorModeTools().GetActiveMode(FAcousticsEdMode::EM_AcousticsEdModeId));
}

TSharedRef<SWidget> FMaterialRow::GenerateWidgetForColumn(const FName& Column)
{
    // clang-format off
    if (Column == SAcousticsMaterialsTab::ColumnNameMaterial)
    {
        FText Label = FText::FromString(m_MaterialItem.Get()->UEMaterialName);

        return
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .Padding(2)
            [
                SNew(STextBlock)
                .Text(Label)
            ];
    }
    else if (Column == SAcousticsMaterialsTab::ColumnNameAcoustics)
    {
        return
            SAssignNew(m_AcousticsComboBox, SComboBox<TSharedPtr<TritonAcousticMaterial>>)
                .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                .OptionsSource(&m_ComboBoxMaterialsList)
                .OnSelectionChanged(this, &FMaterialRow::HandleComboBoxSelectionChanged)
                .OnGenerateWidget(this, &FMaterialRow::HandleComboBoxGenerateWidget)
                [
                    SNew(STextBlock)
                    .Text(this, &FMaterialRow::HandleComboBoxText)
                ];
    }
    else if (Column == SAcousticsMaterialsTab::ColumnNameAbsorption)
    {
        return
            SNew(SHorizontalBox)

            + SHorizontalBox::Slot()
            .FillWidth(90)
            .VAlign(VAlign_Center)
            .Padding(2, 0, 0, 0)
            [
                SNew(SNumericEntryBox<float>)
                    .ToolTipText(FText::FromString(TEXT("Adjust the absorption value for this material (custom material only). 0 = perfectly reflective, 1 = perfectly absorptive.")))
                    .IsEnabled_Lambda([]() { return !AcousticsSharedState::IsPrebakeActive(); })
                    .AllowSpin(true)
                    .Value(this, &FMaterialRow::GetAbsorptionValue)
                    .MinValue(0.0f)
                    .MaxValue(1.0f)
                    .MinSliderValue(0.0f)
                    .MaxSliderValue(1.0f)
                    .SliderExponent(3)
                    .SliderExponentNeutralValue(0.02)
                    .Delta(0.01f)
                    .ShiftMouseMovePixelPerDelta(5)
                    .OnValueChanged(this, &FMaterialRow::SetAbsorptionValue)
                    .OnValueCommitted(this, &FMaterialRow::UpdateFinalAbsorptionValue)
            ];
    }

    assert(false && "Coding error - unexpected column name");

    return
        SNew(STextBlock)
            .Text(FText::FromString(TEXT("n/a")));
}

TOptional<float> FMaterialRow::GetAbsorptionValue() const
{
    // We always return a single value.
    return m_MaterialItem->Absorption;
}

void FMaterialRow::SetAbsorptionValue(float newValue)
{
    if (newValue >= 0 && newValue <= 1)
    {
        m_MaterialItem->Absorption = newValue;
        m_MaterialItem->AcousticMaterialName = TEXT("Custom");
    }
}

void FMaterialRow::UpdateFinalAbsorptionValue(float NewValue, ETextCommit::Type CommitInfo)
{
    // User is done adjusting the value.
    SetAbsorptionValue(NewValue);
    UpdateUserDataOnMaterial();
    // When the user is done editing the absorption value and moves on, if we don't move the keyboard
    // focus away then it can jump back to the edit field they've finished working with and cause problems.
    FSlateApplication::Get().SetKeyboardFocus(m_AcousticsComboBox, EFocusCause::SetDirectly);
}

void FMaterialRow::HandleComboBoxSelectionChanged(
    TSharedPtr<TritonAcousticMaterial> NewSelection, ESelectInfo::Type SelectInfo)
{
    m_MaterialItem->Absorption = NewSelection->Absorptivity;
    m_MaterialItem->AcousticMaterialName = NewSelection->Name;

    UpdateUserDataOnMaterial();
}

void FMaterialRow::UpdateUserDataOnMaterial()
{
    FString tritonMaterialAsString = FString::Printf(TEXT("%s,%f"), *m_MaterialItem->AcousticMaterialName, m_MaterialItem->Absorption);
    FString ConfigFilePath;
    FConfigFile* BaseProjectAcousticsConfigFile;
    auto configLoaded = m_AcousticsEditMode->GetConfigFile(&BaseProjectAcousticsConfigFile, ConfigFilePath);
    if (configLoaded)
    {
        BaseProjectAcousticsConfigFile->SetString(*c_ConfigSectionMaterials, *m_MaterialItem->UEMaterialName, *tritonMaterialAsString);
        if (FAcousticsEdMode::IsSourceControlAvailable())
        {
            // Write the changes to materials to the base ini file instead of the
            USourceControlHelpers::CheckOutOrAddFile(ConfigFilePath);
        }

        BaseProjectAcousticsConfigFile->Write(ConfigFilePath);
    }
}

// Callback for generating the contents of each row in the Acoustics combo box dropdown.
TSharedRef<SWidget> FMaterialRow::HandleComboBoxGenerateWidget(TSharedPtr<TritonAcousticMaterial> InItem)
{
    // clang-format off
    return
        SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .FillWidth(90)
        .Padding(5.0f, 0.0f, 5.0f, 0.0f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
                .Text(FText::FromString(InItem->Name))
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(5.0f, 0.0f, 5.0f, 0.0f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
                .Text(FText::FromString(FString::Printf(TEXT("%0.2f"), InItem->Absorptivity)))
        ];

    // clang-format on
}

FText FMaterialRow::HandleComboBoxText() const
{
    if (m_MaterialItem->AcousticMaterialName == "reserveddefault")
    {
        return FText::FromString("Default");
    }

    return FText::FromString(m_MaterialItem->AcousticMaterialName);
}
