// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Runtime/Core/Public/Containers/Array.h"
#include "AcousticsPythonBridge.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "AcousticsComputePoolConfigurationPanel.generated.h"

UENUM()
enum class FVirtualMachineSize : uint8
{
    Standard_F8s_v2 = 0,
    Standard_F16s_v2,
    Standard_H8,
    Standard_H16
};

// The struct that contains the compute pool configuration data to be displayed.
USTRUCT(BlueprintType, meta = (DisplayName = "Compute Pool"))
struct FComputePoolConfig
{
    GENERATED_BODY()

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta =
            (DisplayName = "Virtual Machine",
             tooltip = "Type of VM used for each node in the Azure Batch compute pool"),
        Category = "Compute Pool")
    FVirtualMachineSize m_VirtualMachine = FVirtualMachineSize::Standard_F8s_v2;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta = (DisplayName = "Nodes", ClampMin = "1", tooltip = "Total number of VMs in the Azure Batch compute pool"),
        Category = "Compute Pool")
    int m_Nodes = 0;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta = (DisplayName = "Use Low Priority Nodes", tooltip = "Set this flag to use low-priority nodes"),
        Category = "Compute Pool")
    bool m_UseLowPriNodes = false;

    UPROPERTY(
        VisibleAnywhere, BlueprintReadOnly, NonTransactional, Transient,
        meta =
            (DisplayName = "Estimated Bake Time",
             tooltip = "Estimated time to get processing results back from Azure Batch"),
        Category = "Compute Pool")
    FString m_TimeEstimate;

    UPROPERTY(
        VisibleAnywhere, BlueprintReadOnly, NonTransactional, Transient,
        meta =
            (DisplayName = "Estimated Compute Cost",
             tooltip = "Total amount of computing time needed to run the simulation on Azure Batch"),
        Category = "Compute Pool")
    FString m_CostEstimate;
};

USTRUCT(BlueprintType, meta = (DisplayName = "Compute Pool"))
struct FComputePoolConfigurationDetails
{
    GENERATED_BODY()

    // The struct instance that is used to display the compute pool configuration.
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, meta = (DisplayName = "Compute Pool"),
        Category = "Compute Pool")
    FComputePoolConfig m_Pool;

    void Initialize();
    void UpdateConfiguration();
    void UpdateCostEstimate();
};

class SAcousticsComputePoolConfigurationPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAcousticsComputePoolConfigurationPanel)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void Refresh()
    {
        m_Pool.UpdateConfiguration();
    }

private:
    TSharedPtr<IStructureDetailsView> m_DetailsView;
    FComputePoolConfigurationDetails m_Pool;
};
