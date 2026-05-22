// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "SAcousticsEdit.h"
#include "Widgets/SCompoundWidget.h"
#include "Runtime/Core/Public/Containers/Array.h"
#include "AcousticsMesh.h"
#include "AcousticsSimulationParametersPanel.h"
#include "AcousticsProbesTab.generated.h"

UENUM()
enum class FResolution : uint8
{
    Coarse = 0,
    Fine
};

static TArray<TSharedPtr<FString>> g_ResolutionNames = {
    MakeShareable(new FString(TEXT("Coarse"))), MakeShareable(new FString(TEXT("Fine")))};

static TArray<float> g_ResolutionFrequencies = {250, 500};

static inline FResolution LabelToResolution(TSharedPtr<FString> label)
{
    return FString(TEXT("Coarse")).Compare(*label) == 0 ? FResolution::Coarse : FResolution::Fine;
}

static inline FText FrequencyToResolution(float frequency)
{
    if (frequency == 250)
    {
        return FText::FromString(*g_ResolutionNames[static_cast<uint8>(FResolution::Coarse)]);
    }
    else if (frequency == 500)
    {
        return FText::FromString(*g_ResolutionNames[static_cast<uint8>(FResolution::Fine)]);
    }
    else
    {
        // If it's different, that means they modified it manually in the simulation parameters. 
        // But we don't want Custom to be part of the enum because we  don't want the user to select 
        // it in the dropdown.
        return FText::FromString("Custom");
    }
}

class SAcousticsProbesTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAcousticsProbesTab)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, SAcousticsEdit* ownerEdit);

private:
    FText GetCalculateClearText() const;
    FText GetCalculateClearTooltipText() const;
    FReply OnCalculateClearButton();
    // Added functions associated with the check out button for the config and vox files.
    FReply OnCheckOutFilesButton();
    void CheckOutVoxAndConfigFile();
    bool CanCheckOutFiles() const;

    FReply OnResetSimulationParametersButton();
    EVisibility GetSimulationParameterVisibility() const;
    void OnCheckStateChanged_ShowSimulationParameters(ECheckBoxState InState);
    ECheckBoxState GetCheckState_ShowSimulationParameters() const;

    FText GetCurrentResolutionLabel() const;
    TSharedRef<SWidget> MakeResolutionOptionsWidget(TSharedPtr<FString> inString);
    void OnResolutionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
    FText GetPrefixText() const;
    void OnPrefixTextChange(const FText& NewText, ETextCommit::Type CommitInfo);
    FText GetDataFolderPath() const;
    FReply OnAcousticsDataFolderButtonClick();
    void ComputePrebake();

    void AddStaticMeshToAcousticMesh(
        AcousticMesh* acousticMesh, AActor* actor, const FTransform& worldTransform, const UStaticMesh* mesh,
        const TArray<UMaterialInterface*>& materials, MeshType type, TArray<uint32>& materialIDsNotFound,
        UPhysicalMaterial* physMatOverride = nullptr);

    // Function to export landscape to raw mesh
    bool ExportLandscapeToRawMesh(
        class ALandscapeProxy* LandscapeActor, int32 InExportLOD, struct FMeshDescription& OutRawMesh,
        TArray<class ULandscapeLayerInfoObject*>& TriangleLayerInfo, const FBoxSphereBounds& InBounds,
        bool ShouldIgnoreBounds = false) const;

    void AddLandscapeToAcousticMesh(
        AcousticMesh* acousticMesh, class ALandscapeProxy* actor, MeshType type, TArray<uint32>& materialIDsNotFound,
        const FBoxSphereBounds& BoundsOfInterest);

    void AddVolumeToAcousticMesh(
        AcousticMesh* acousticMesh, class AAcousticsProbeVolume* Actor, TArray<uint32>& materialIDsNotFound);
    void AddPinnedProbeToAcousticMesh(AcousticMesh* acousticMesh, const FVector& probeLocation);

    void AddNavmeshToAcousticMesh(
        AcousticMesh* acousticMesh, class ARecastNavMesh* navActor, TArray<UMaterialInterface*> materials,
        TArray<uint32>& materialIDsNotFound);
    bool ShouldEnableForProcessing() const;
    TOptional<float> GetProgressBarPercent() const;
    EVisibility GetProgressBarVisibility() const;

    static bool ComputePrebakeCallback(const char* message, int progress);
    static void ResetPrebakeCalculationState();
    static bool IsOverlapped(
        const class AAcousticsProbeVolume* ProbeVolume, const ATKVectorD& Vertex1, const ATKVectorD& Vertex2,
        const ATKVectorD& Vertex3);

    TritonMaterialCode GetMaterialCodeForStaticMeshFace(
        const UStaticMesh* mesh, const TArray<UMaterialInterface*>& materials, uint32 face,
        TArray<uint32>& materialIDsNotFound, UPhysicalMaterial* physMatOverride = nullptr);

    TritonMaterialCode GetMaterialCodeForLandscapeFace(
        const TArray<class ULandscapeLayerInfoObject*>& layers, uint32 face, TArray<uint32>& layerMaterialIDsNotFound,
        UPhysicalMaterial* physMatOverride = nullptr);

    void ApplyOverridesAndRemapsFromProbeVolumesOnTriangle(
        const TArray<ATKVectorD>& vertices, uint32 index1, uint32 index2, uint32 index3,
        TritonMaterialCode MaterialCode, TritonAcousticMeshTriangleInformation& triangleInfo);

private:
    TSharedPtr<FString> m_CurrentResolution;
    FString m_AcousticsDataFolderPath;
    TSharedPtr<class SEditableTextBox> m_PrefixTextBox;
    FString m_Prefix;
    SAcousticsEdit* m_OwnerEdit;
    static FString m_CurrentStatus;
    static float m_CurrentProgress;
    static bool m_CancelRequest;
    static bool m_ShowSimulationParameters;

    TArray<class AAcousticsProbeVolume*> m_MaterialOverrideVolumes;
    TArray<class AAcousticsProbeVolume*> m_MaterialRemapVolumes;

    FAcousticsEdMode* m_AcousticsEditMode;

    TSharedPtr<SAcousticsSimulationParametersPanel> m_SimParamsPanel;
};
