// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "SAcousticsEdit.h"
#include "Widgets/SCompoundWidget.h"

class SAcousticsObjectsTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAcousticsObjectsTab)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, SAcousticsEdit* ownerEdit);
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    // Checkbox handlers
    void OnCheckStateChanged_StaticMesh(ECheckBoxState InState);
    ECheckBoxState GetCheckState_StaticMesh() const;
    void OnCheckStateChanged_MovableStaticMesh(ECheckBoxState InState);
    ECheckBoxState GetCheckState_MovableStaticMesh() const;
    bool GetMovableStaticMeshEnabled() const;
    void OnCheckStateChanged_NavMesh(ECheckBoxState InState);
    ECheckBoxState GetCheckState_NavMesh() const;
    void OnCheckStateChanged_Landscape(ECheckBoxState InState);
    ECheckBoxState GetCheckState_Landscape() const;
    void OnCheckStateChanged_UsePhysicalMaterials(ECheckBoxState InState);
    ECheckBoxState GetCheckState_UsePhysicalMaterials() const;

    // Selects all the objects in the active scene that match the current filters
    FReply OnSelectObjects();
    FReply OnUnselectObjects();

    // Radio Button Handlers
    void OnAcousticsRadioButtonChanged(ECheckBoxState InState);
    ECheckBoxState IsAcousticsRadioButtonChecked() const;
    void OnNavigationRadioButtonChanged(ECheckBoxState InState);
    ECheckBoxState IsNavigationRadioButtonChecked() const;

    // Tag management functions
    FReply OnAddTag();
    FReply OnClearTag();
    FReply OnSelectAllTag();

    FAcousticsEdMode* m_AcousticsEditMode;
    SAcousticsEdit* m_Owner;
    FString m_NumSelected;
    FString m_NumNav;
    FString m_NumGeo;

private:
    // Static const string for the section name in config file
    // for list of maps that use physical materials.
    static const FString UsePhysicalMaterialsSectionString;
};
