// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Runtime/Core/Public/Containers/Array.h"
#include "FMaterialRow.h"
#include "AcousticsMaterialLibrary.h"

// Forward declaration used
// instead of an include to avoid cyclical dependencies.
class FAcousticsEdMode;

class SAcousticsMaterialsTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAcousticsMaterialsTab)
    {
    }
    SLATE_END_ARGS()

    /** SCompoundWidget functions */
    void Construct(const FArguments& InArgs);

    void PublishMaterialLibrary();
    void UpdateUEMaterials();

    static FName ColumnNameMaterial;
    static FName ColumnNameAcoustics;
    static FName ColumnNameAbsorption;

    // Read-only accessor for the material item list.
    const TArray<TSharedPtr<MaterialItem>>& GetMaterialItemsList() const
    {
        return m_Items;
    }

private:
    TSharedRef<ITableRow>
    OnGenerateRowForMaterialList(TSharedPtr<MaterialItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
    void OnRowSelectionChanged(TSharedPtr<MaterialItem> InItem, ESelectInfo::Type SelectInfo);
    void AddNewUEMaterial(FString materialName);
    void AddNewUEMaterialWithMigrationSupport(UMaterialInterface* curMaterial);
    void InitKnownMaterialsList();

    EColumnSortMode::Type GetColumnSortMode(const FName ColumnId) const;
    void OnColumnNameSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode);

    TArray<TSharedPtr<TritonAcousticMaterial>> m_ComboboxMaterialsList;
    TArray<TritonAcousticMaterial> m_KnownMaterials;
    TArray<TritonMaterialCode> m_KnownMaterialCodes;
    TArray<TSharedPtr<MaterialItem>> m_Items;
    TSharedPtr<SListView<TSharedPtr<MaterialItem>>> m_ListView;
    EColumnSortMode::Type SortMode;
    FAcousticsEdMode* m_AcousticsEditMode;
};
