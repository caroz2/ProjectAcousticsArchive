// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Runtime/Core/Public/Containers/Array.h"
#include "AcousticsSharedState.h"
#include "AcousticsPythonBridge.h"
#include "IStructureDetailsView.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UObject/ObjectMacros.h"
#include "AcousticsSimulationParametersPanel.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Simulation Parameters"))
struct FSimulationParametersDetails
{
    GENERATED_BODY()

    void Initialize()
    {
        m_SimParams = AcousticsSharedState::GetSimulationParameters();
    }

    void Update()
    {
        return AcousticsSharedState::SetSimulationParameters(m_SimParams);
    }

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, meta = (DisplayName = "Simulation Parameters",
            Category = "Simulation Parameters"))
    FSimulationParameters m_SimParams;
};

// Displays a details panel with our simulation parameters, synced to the python bridge
class SAcousticsSimulationParametersPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAcousticsSimulationParametersPanel)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void Refresh()
    {
        m_SimParamDetails.Initialize();
    }

private:
    TSharedPtr<IStructureDetailsView> m_DetailsView;
    FSimulationParametersDetails m_SimParamDetails;
};
