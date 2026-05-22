// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Views/STableRow.h"
#include "Runtime/Core/Public/Containers/Array.h"
#include "AcousticsMaterialLibrary.h"
#include "Materials/MaterialInterface.h" // UMaterialInterface

struct AcousticsMaterial;
class FAcousticsEdMode;

// Struct used for each entry in the list view
struct MaterialItem
{
    MaterialItem(FString name, FString acousticName, float coeff)
    {
        UEMaterialName = name;
        AcousticMaterialName = acousticName;
        Absorption = coeff;
    }

    FString UEMaterialName;
    FString AcousticMaterialName;
    float Absorption;
};

struct FMaterialRow : public SMultiColumnTableRow<TSharedPtr<MaterialItem>>
{
public:
    SLATE_BEGIN_ARGS(FMaterialRow)
    {
    }
    SLATE_END_ARGS()

    void Construct(
        const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<MaterialItem> InItem,
        TArray<TSharedPtr<TritonAcousticMaterial>> ComboBoxMaterials);

    TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Column);

    TOptional<float> GetAbsorptionValue() const;
    void SetAbsorptionValue(float newValue);
    void UpdateFinalAbsorptionValue(float NewValue, ETextCommit::Type CommitInfo);

private:
    TSharedPtr<SComboBox<TSharedPtr<TritonAcousticMaterial>>> m_AcousticsComboBox;
    TArray<TSharedPtr<TritonAcousticMaterial>> m_ComboBoxMaterialsList;
    TSharedPtr<MaterialItem> m_MaterialItem;

private:
    void HandleComboBoxSelectionChanged(TSharedPtr<TritonAcousticMaterial> NewSelection, ESelectInfo::Type SelectInfo);
    void UpdateUserDataOnMaterial();
    TSharedRef<SWidget> HandleComboBoxGenerateWidget(TSharedPtr<TritonAcousticMaterial> InItem);
    FText HandleComboBoxText() const;
    FAcousticsEdMode* m_AcousticsEditMode;
};
