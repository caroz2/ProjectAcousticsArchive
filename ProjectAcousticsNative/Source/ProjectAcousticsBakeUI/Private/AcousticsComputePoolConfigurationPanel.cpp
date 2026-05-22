// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsComputePoolConfigurationPanel.h"
#include "SAcousticsEdit.h"
#include "AcousticsSharedState.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Modules/ModuleManager.h"
#include "IStructureDetailsView.h"
#include "ComputePoolConfigurationDetailsCustomization.h"

#define LOCTEXT_NAMESPACE "SAcousticsComputePoolConfigurationPanel"

TMap<FVirtualMachineSize, FString> g_VirtualMachineSizeMap;

void SAcousticsComputePoolConfigurationPanel::Construct(const FArguments& InArgs)
{
    // Initialize VM size map
    g_VirtualMachineSizeMap.Empty();
    g_VirtualMachineSizeMap.Add(FVirtualMachineSize::Standard_F8s_v2, TEXT("Standard_F8s_v2"));
    g_VirtualMachineSizeMap.Add(FVirtualMachineSize::Standard_F16s_v2, TEXT("Standard_F16s_v2"));
    g_VirtualMachineSizeMap.Add(FVirtualMachineSize::Standard_H8, TEXT("Standard_H8"));
    g_VirtualMachineSizeMap.Add(FVirtualMachineSize::Standard_H16, TEXT("Standard_H16"));

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

    m_Pool.Initialize();

    // Register the detail customization class for the compute pool config panel.
    auto& propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    m_DetailsView = propertyModule.CreateStructureDetailView(detailsViewArgs, structureViewArgs, nullptr);
    m_DetailsView->GetDetailsView()->RegisterInstancedCustomPropertyTypeLayout(
        "ComputePoolConfig",
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(
            &FComputePoolConfigurationDetailsCustomization::MakeInstance));

    m_DetailsView->SetStructureData(MakeShareable(
        new FStructOnScope(FComputePoolConfigurationDetails::StaticStruct(), reinterpret_cast<uint8*>(&m_Pool))));
    m_DetailsView->GetOnFinishedChangingPropertiesDelegate().AddLambda(
        [this](const FPropertyChangedEvent&) { m_Pool.UpdateConfiguration(); });

    // clang-format off
    ChildSlot
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
    ];
    // clang-format on
}

void FComputePoolConfigurationDetails::Initialize()
{
    const auto& preconfiguredPool = AcousticsSharedState::GetComputePoolConfiguration();
    for (uint8 i = 0; i < 4; ++i)
    {
        auto key = static_cast<FVirtualMachineSize>(i);
        auto size = g_VirtualMachineSizeMap[key];
        if (size == preconfiguredPool.vm_size)
        {
            m_Pool.m_VirtualMachine = key;
            break;
        }
    }

    m_Pool.m_Nodes = preconfiguredPool.nodes;
    m_Pool.m_UseLowPriNodes = preconfiguredPool.use_low_priority_nodes;
    UpdateCostEstimate();
}

void FComputePoolConfigurationDetails::UpdateConfiguration()
{
    // Restrict the nodes to probe count
    auto config = AcousticsSharedState::GetSimulationConfiguration();
    if (config != nullptr && config->IsReady())
    {
        m_Pool.m_Nodes = FMath::Min(config->GetProbeCount(), m_Pool.m_Nodes);
    }

    // Setup the Python bridge to the CloudProcessor
    FComputePoolConfiguration pool;
    pool.vm_size = g_VirtualMachineSizeMap[m_Pool.m_VirtualMachine];
    pool.nodes = m_Pool.m_Nodes;
    pool.use_low_priority_nodes = m_Pool.m_UseLowPriNodes;
    AcousticsSharedState::SetComputePoolConfiguration(pool);

    UpdateCostEstimate();
}

void FComputePoolConfigurationDetails::UpdateCostEstimate()
{
    auto duration = FTimespan::FromMinutes(AcousticsSharedState::EstimateProcessingTime());
    if (duration == FTimespan::Zero())
    {
        m_Pool.m_TimeEstimate = TEXT("Not Available");
        m_Pool.m_CostEstimate = TEXT("Not Available");
    }
    else
    {
        auto formatUnit = [](auto token, auto value) {
            if (value > 0)
            {
                return FString::Printf(TEXT("%d %s%s "), value, token, value > 1 ? TEXT("s") : TEXT(""));
            }
            return FString();
        };
        auto formatDuration = [&formatUnit](const FTimespan& duration) {
            auto days = duration.GetDays();
            auto hours = duration.GetHours();
            auto minutes = duration.GetMinutes();
            FString value =
                formatUnit(TEXT("day"), days) + formatUnit(TEXT("hour"), hours) + formatUnit(TEXT("minute"), minutes);
            return value;
        };
        m_Pool.m_TimeEstimate = formatDuration(duration);
        m_Pool.m_CostEstimate = formatDuration(m_Pool.m_Nodes * duration);
    }
}

#undef LOCTEXT_NAMESPACE