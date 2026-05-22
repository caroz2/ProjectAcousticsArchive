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
#include "AcousticsAzureCredentialsPanel.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Azure Account"))
struct FAzureCredentialsDetails
{
    GENERATED_BODY()

    void Initialize()
    {
        m_Creds = AcousticsSharedState::GetAzureCredentials();
    }

    void Update()
    {
        return AcousticsSharedState::SetAzureCredentials(m_Creds);
    }

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, meta = (DisplayName = "Azure Account"),
        Category = "Azure Account")
    FAzureCredentials m_Creds;
};

class SAcousticsAzureCredentialsPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAcousticsAzureCredentialsPanel)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply OnAzurePortalButton();

private:
    TSharedPtr<IStructureDetailsView> m_DetailsView;
    FAzureCredentialsDetails m_Creds;
};
