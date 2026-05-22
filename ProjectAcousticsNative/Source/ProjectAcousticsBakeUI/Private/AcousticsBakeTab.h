// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Runtime/Core/Public/Containers/Array.h"
#include "AcousticsPythonBridge.h"
#include "AcousticsComputePoolConfigurationPanel.h"

class SAcousticsEdit;

class SAcousticsBakeTab : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAcousticsBakeTab)
    {
    }
    SLATE_END_ARGS()

    /** SCompoundWidget functions */
    void Construct(const FArguments& InArgs, SAcousticsEdit* ownerEdit);

    /** Handles refreshing state on gaining focus */
    void Refresh();

private:
    FReply OnSubmitCancelButton();
    FReply OnLocalBakeButton();
    FReply SubmitForProcessing();
    FReply CancelJobProcessing();
    bool ShouldEnableSubmitCancel() const;
    bool ShouldEnableLocalBakeButton() const;
    bool HaveValidAzureCredentials() const;
    bool HaveValidSimulationConfig() const;
    FText GetSubmitCancelText() const;
    FText GetSubmitCancelTooltipText() const;
    FText GetProbeCountText() const;
    FText GetCurrentStatus() const;
    FReply OnDownloadLocalBakeToolsButton() const;

private:
    SAcousticsEdit* m_OwnerEdit;
    FString m_JobId;
    TSharedPtr<SAcousticsComputePoolConfigurationPanel> m_ComputePoolPanel;
    mutable FString m_Status;
    mutable FDateTime m_LastStatusCheckTime;
};
