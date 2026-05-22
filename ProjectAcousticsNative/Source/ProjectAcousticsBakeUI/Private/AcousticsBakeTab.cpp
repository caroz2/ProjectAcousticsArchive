// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsBakeTab.h"
#include "SAcousticsEdit.h"
#include "AcousticsSharedState.h"
#include "AcousticsEdMode.h"
#include "Fonts/SlateFontInfo.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Misc/Timespan.h"
#include "Misc/Paths.h"
#include "AcousticsAzureCredentialsPanel.h"
#include "AcousticsPythonBridge.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SButton.h"
#include "ISourceControlProvider.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"
#include "Misc/MessageDialog.h"
#include "SourceControlOperations.h"
#include "HAL/PlatformFilemanager.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#define LOCTEXT_NAMESPACE "SAcousticsBakeTab"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAcousticsBakeTab::Construct(const FArguments& InArgs, SAcousticsEdit* ownerEdit)
{
    // If python isn't initialized, bail out
    if (!AcousticsSharedState::IsInitialized())
    {
        // clang-format off
        ChildSlot
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Python is required for Project Acoustics baking.\nPlease enable the Python plugin.")))
            ];
        // clang-format on
        return;
    }

    m_OwnerEdit = ownerEdit;

    const FString helpTextTitle = TEXT("Step Four");
    const FString helpText = TEXT("After completing the previous steps, submit the job for baking in the cloud. "
        "Make sure you have created your Azure Batch and Storage accounts.");

    const FString localBakeTextTitle = TEXT("Local Bake");
    const FString localBakeText = TEXT("As an alternative to acoustics baking on Azure, perform bakes on your local PC. ");

    // clang-format off
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SAcousticsEdit::MakeHelpTextWidget(helpTextTitle, helpText)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(SAcousticsAzureCredentialsPanel)
        ]

        // Compute pool configuration
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SAssignNew(m_ComputePoolPanel, SAcousticsComputePoolConfigurationPanel)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(SSeparator)
            .Orientation(Orient_Horizontal)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .Padding(FAcousticsEditSharedProperties::StandardPadding)
            .AutoWidth()
            [
                SNew(SButton)
                .IsEnabled(this, &SAcousticsBakeTab::ShouldEnableSubmitCancel)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .DesiredSizeScale(FVector2D(2.0f, 1.f))
                .Text(this, &SAcousticsBakeTab::GetSubmitCancelText)
                .OnClicked(this, &SAcousticsBakeTab::OnSubmitCancelButton)
                .ToolTipText(this, &SAcousticsBakeTab::GetSubmitCancelTooltipText)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(STextBlock)
            .Text(this, &SAcousticsBakeTab::GetProbeCountText)
            .Visibility_Lambda([this]() { return HaveValidSimulationConfig() ? EVisibility::Visible : EVisibility::Collapsed; })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(STextBlock)
            .Text(this, &SAcousticsBakeTab::GetCurrentStatus)
            .AutoWrapText(true)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::StandardPadding)
        [
            SNew(SSeparator)
            .Orientation(Orient_Horizontal)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(FAcousticsEditSharedProperties::ExtraPadding)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .Padding(0, 0, 0, 5)
            [
                SNew(SExpandableArea)
                .InitiallyCollapsed(true)
                .AreaTitle(FText::FromString(localBakeTextTitle))
                .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.2f))
                .AreaTitleFont(STYLER::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
                .BodyContent()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        [
                            SNew(SBox)
                            .MinDesiredWidth(91)
                            [
                                SNew(STextBlock)
                                .Font(STYLER::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
                                .AutoWrapText(true)
                                .Margin(FAcousticsEditSharedProperties::StandardTextMargin)
                                .Text(FText::FromString(localBakeText))
                            ]
                        ]
                    ]
                // Local Bake Buttons
                + SVerticalBox::Slot()
                    .Padding(FAcousticsEditSharedProperties::StandardPadding)
                    .AutoHeight()
                    [
                        SNew(SWrapBox)
							.UseAllottedSize(true)

                        + SWrapBox::Slot()
                        .Padding(FAcousticsEditSharedProperties::StandardPadding)
                        [
                            SNew(SBox)
                            .MinDesiredWidth(60.f)
                            .HeightOverride(25.f)
                            [
                                SNew(SButton)
                                .IsEnabled(this, &SAcousticsBakeTab::ShouldEnableLocalBakeButton)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                .Text(FText::FromString(TEXT("Prepare Local Bake")))
                                .OnClicked(this, &SAcousticsBakeTab::OnLocalBakeButton)
                                .ToolTipText(FText::FromString(TEXT("Generate package for local bake")))
                            ]
                        ]

                        + SWrapBox::Slot()
                        .Padding(FAcousticsEditSharedProperties::StandardPadding)
                        [
                            SNew(SBox)
                            .MinDesiredWidth(90.f)
                            .HeightOverride(25.f)
                            [
                                SNew(SButton)
                                .HAlign(HAlign_Center)
                                .VAlign(VAlign_Center)
                                .Text(FText::FromString(TEXT("Download Local Bake Tools")))
                                .OnClicked(this, &SAcousticsBakeTab::OnDownloadLocalBakeToolsButton)
                                .ToolTipText(FText::FromString(TEXT("Navigate to download page for the local bake tools")))
                            ]
                        ]
                    ]
                ]
            ]
        ]
    ];
    // clang-format on
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAcousticsBakeTab::Refresh()
{
    m_ComputePoolPanel->Refresh();
}

bool SAcousticsBakeTab::ShouldEnableSubmitCancel() const
{
    const auto& config = AcousticsSharedState::GetActiveJobInfo();
    // Submit is pending
    if (config.submit_pending)
    {
        return false;
    }

    // Either have a valid creds and config ready for submission or tracking an active bake job
    return (HaveValidAzureCredentials() && HaveValidSimulationConfig()) || config.job_id.IsEmpty() == false;
}

bool SAcousticsBakeTab::ShouldEnableLocalBakeButton() const
{
    return HaveValidSimulationConfig();
}

bool SAcousticsBakeTab::HaveValidAzureCredentials() const
{
    const auto& creds = AcousticsSharedState::GetAzureCredentials();
    if (creds.batch_url.IsEmpty() || creds.batch_name.IsEmpty() || creds.batch_key.IsEmpty() ||
        creds.storage_name.IsEmpty() || creds.storage_key.IsEmpty() || creds.toolset_version.IsEmpty())
    {
        return false;
    }
    return true;
}

bool SAcousticsBakeTab::HaveValidSimulationConfig() const
{
    return AcousticsSharedState::IsPrebakeActive() &&
        AcousticsSharedState::GetSimulationConfiguration()->IsReady();
}

FText SAcousticsBakeTab::GetSubmitCancelText() const
{
    const auto& info = AcousticsSharedState::GetActiveJobInfo();

    if (info.job_id.IsEmpty())
    {
        return FText::FromString(TEXT("Submit Azure Bake"));
    }
    else
    {
        return FText::FromString(TEXT("Cancel"));
    }
}

FText SAcousticsBakeTab::GetSubmitCancelTooltipText() const
{
    const auto& info = AcousticsSharedState::GetActiveJobInfo();

    if (info.job_id.IsEmpty())
    {
        return FText::FromString(TEXT("Submit to Azure Batch for processing"));
    }
    else
    {
        return FText::FromString(TEXT("Cancel currently active Azure Batch processing"));
    }
}

FReply SAcousticsBakeTab::OnLocalBakeButton()
{
    auto desktopPlatform = FDesktopPlatformModule::Get();
    if (desktopPlatform)
    {
        // Have user specify where they want package to go
        FString localBakePackageDir;
        desktopPlatform->OpenDirectoryDialog(
            FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
            TEXT("Select a directory for local bake package"),
            AcousticsSharedState::GetProjectConfiguration().content_dir,
            localBakePackageDir);

        if (localBakePackageDir.IsEmpty())
        {
            UE_LOG(LogAcoustics, Warning, TEXT("No directory selected for local bake package"));
            return FReply::Handled();
        }

        // Copy bake input files
        auto voxPath = AcousticsSharedState::GetVoxFilepath();
        auto configPath = AcousticsSharedState::GetConfigFilepath();
        IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
        FileManager.CopyFile(*FPaths::Combine(localBakePackageDir, FPaths::GetCleanFilename(voxPath)), *voxPath);
        FileManager.CopyFile(*FPaths::Combine(localBakePackageDir, FPaths::GetCleanFilename(configPath)), *configPath);

        // Generate readme.txt file explaining how to perform local bakes
        FString readmeFilePath = FPaths::Combine(localBakePackageDir, TEXT("readme.txt"));
        FString readmeFileContents = FString::Printf(TEXT("Baking acoustics on a local PC is an option that many customers utilize when "
            "scene sizes are small and getting familiar with the technology is the goal. As scene sizes increase, using a service like "
            "Azure Batch will reduce the time it takes to complete a bake.\n\n"
            "After creating a local bake directory via the Prepare Local Bake button, select the Download Local Bake Tools button to "
            "download the bake tools package and place the tools in the chosen local bake directory.\n\n"
            "Once the tools and the configuration files are co-located in the local bake directory, execute the \"RunLocalBake.bat\" script "
            "to start a bake that will run serially on your local PC producing a new working directory matching the starting timestamp of "
            "the bake. To import the .ace file into your project, use the Content Browser to navigate to the Content\"Acoustics folder and "
            "either drag and drop your .ace file into the folder -or- select the Import button and find the .ace file you "
            "would like to import into the project. Once the import is complete, you can then set the Acoustics Data property of the "
            "AcousticsSpace actor to new .ace file. Consult the documentation on https://aka.ms/acoustics for more documentation about "
            "importing ACE files into a project.\n"),
            *FPaths::GetCleanFilename(configPath));
        FFileHelper::SaveStringToFile(readmeFileContents, *readmeFilePath);

        // Notify the user in console
        FString directoryMsg = FString::Printf(TEXT("Local bake package prepared in directory: %s"), *localBakePackageDir);
        UE_LOG(LogAcoustics, Display, TEXT("%s"), *directoryMsg);
        UE_LOG(LogAcoustics, Display, TEXT("LOCAL BAKE INSTRUCTIONS"));
        UE_LOG(LogAcoustics, Display, TEXT("======================="));
        UE_LOG(LogAcoustics, Display, TEXT("Executing the \"RunLocalBake.bat\" script will execute a bake process that runs serially on your local PC."));
        UE_LOG(LogAcoustics, Display, TEXT("An .ace file will be generated upon completion. To import the.ace file into your project,"));
        UE_LOG(LogAcoustics, Display, TEXT("use the Content Browser to navigate to the Content\\Acoustics folder and either drag and drop your"));
        UE_LOG(LogAcoustics, Display, TEXT(".ace file into the folder -or- select the Import button and select the .ace file that you would like to import"));
        UE_LOG(LogAcoustics, Display, TEXT("into the project. Once the import is complete, you can then set the Acoustics Data property of the "));
        UE_LOG(LogAcoustics, Display, TEXT("AcousticsSpace actor to new .ace file."));
        
        // Notify the user with notification
        FNotificationInfo Info(FText::Format(LOCTEXT("Local bake package prepared", "{0}"), FText::FromString(directoryMsg)));
        Info.ExpireDuration = 8.0f;
        FSlateNotificationManager::Get().AddNotification(Info);

        // Open file explorer with window to new local bake folder
        FPlatformProcess::ExploreFolder(*localBakePackageDir);

        // Make sure Content/Acoustics folder exists
        auto contentAcousticsDir = AcousticsSharedState::GetProjectConfiguration().game_content_dir;
        if (!FPaths::DirectoryExists(contentAcousticsDir))
        {
            FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*contentAcousticsDir);
        }
    }
    else
    {
        UE_LOG(LogAcoustics, Warning, TEXT("Local bakes only supported on desktop platform"));
    }

    return FReply::Handled();
}

FReply SAcousticsBakeTab::OnDownloadLocalBakeToolsButton() const
{
    FString url = TEXT("https://www.microsoft.com/en-us/download/details.aspx?id=104692");
    FPlatformProcess::LaunchURL(*url, nullptr, nullptr);
    return FReply::Handled();
}

FReply SAcousticsBakeTab::OnSubmitCancelButton()
{
    const auto& info = AcousticsSharedState::GetActiveJobInfo();

    if (info.job_id.IsEmpty())
    {
        // Check out the ace if it exists so that it can be overwritten by the bake process.
        FString AceFilePath = AcousticsSharedState::GetAceFilepath();
        if (FPaths::FileExists(AceFilePath))
        {
            if (FAcousticsEdMode::IsSourceControlAvailable())
            {
                USourceControlHelpers::CheckOutOrAddFile(AceFilePath);
            }

            // Set the read-only flag for the ace file to false so that it can be deleted.
            FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*AceFilePath, false);
        }

        if (AcousticsSharedState::IsAceFileReadOnly())
        {
            auto message = FString(TEXT("Please provide write access to  " + AcousticsSharedState::GetAceFilepath()));
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(message));
            return FReply::Handled();
        }
        else
        {
            auto consent = EAppReturnType::Ok;
            if (FPaths::FileExists(AcousticsSharedState::GetAceFilepath()))
            {
                auto message = FString(TEXT(
                    "Current results file " + AcousticsSharedState::GetAceFilepath() +
                    " will be replaced when simulation completes. Continue?"));
                consent = FMessageDialog::Open(EAppMsgType::OkCancel, FText::FromString(message));
            }

            if (consent == EAppReturnType::Ok)
            {
                SubmitForProcessing();
            }
            return FReply::Handled();
        }
    }
    else
    {
        return CancelJobProcessing();
    }
}

FReply SAcousticsBakeTab::SubmitForProcessing()
{
    AcousticsSharedState::SubmitForProcessing();
    return FReply::Handled();
}

FReply SAcousticsBakeTab::CancelJobProcessing()
{
    AcousticsSharedState::CancelProcessing();
    return FReply::Handled();
}

FText SAcousticsBakeTab::GetProbeCountText() const
{
    FText message;
    if (AcousticsSharedState::IsPrebakeActive())
    {
        auto config = AcousticsSharedState::GetSimulationConfiguration();
        message = FText::FromString(FString::Printf(TEXT("Probe Count: %d"), config->GetProbeCount()));
    }
    return message;
}

FText SAcousticsBakeTab::GetCurrentStatus() const
{
    const auto& info = AcousticsSharedState::GetActiveJobInfo();

    FString jobInfo;
    FString header = TEXT("Status: ");
    if (info.job_id.IsEmpty() && info.submit_pending == false)
    {
        m_Status = TEXT("");
        // Inform user about the Bake button state
        if (ShouldEnableSubmitCancel())
        {
            m_Status = TEXT("Ready for processing\n");
        }
        else
        {
            if (HaveValidSimulationConfig() == false)
            {
                m_Status = TEXT(
                    "Please generate a simulation configuration using the Probes tab to enable acoustics baking\n");
            }
            else if (HaveValidAzureCredentials() == false)
            {
                m_Status = TEXT("Please provide Azure account credentials to enable acoustics baking\n");
            }
        }

        // Check for an existing ACE file and inform about location
        auto aceFile = AcousticsSharedState::GetAceFilepath();
        if (FPaths::FileExists(aceFile))
        {
            m_Status = m_Status + TEXT("\nFound existing simulation results in ") + aceFile + "\n";

            // Track the total time taken for a bake.
            if (AcousticsSharedState::BakeStartTime.GetTicks() != 0)
            {
                if (AcousticsSharedState::BakeEndTime.GetTicks() == 0)
                {
                    AcousticsSharedState::BakeEndTime = FDateTime::Now();

                    // Check out the generated ace and uasset file.
                    // reaches here if baking process just ended.
                    FString UassetFilePath = aceFile;
                    UassetFilePath.RemoveFromEnd("ace");
                    UassetFilePath.Append("uasset");

                    // Use CheckOutOrAddFile so that new ace and uasset for new map can be added to source control.
                    if (FAcousticsEdMode::IsSourceControlAvailable())
                    {
                        USourceControlHelpers::CheckOutOrAddFile(aceFile);
                        USourceControlHelpers::CheckOutOrAddFile(UassetFilePath);
                    }

                    FString AceFileBackup = AcousticsSharedState::GetAceFileBackupPath();
                    if (FPaths::FileExists(AceFileBackup))
                    {
                        FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*AceFileBackup);
                    }
                }
                if (AcousticsSharedState::BakeEndTime.GetTicks() != 0)
                {
                    m_Status.Append(FString::Printf(
                        TEXT("Started: %s\nEnded: %s\nTotal Duration: %s\n"),
                        *AcousticsSharedState::BakeStartTime.ToString(),
                        *AcousticsSharedState::BakeEndTime.ToString(),
                        *(AcousticsSharedState::BakeEndTime - AcousticsSharedState::BakeStartTime).ToString()));
                }
            }
        }
    }
    else
    {
        // Check if it's time to query for status
        auto elapsed = FDateTime::Now() - m_LastStatusCheckTime;
        if (elapsed > FTimespan::FromSeconds(30))
        {
            auto status = AcousticsSharedState::GetCurrentStatus();
            m_Status = status.message;
            if (status.succeeded)
            {
                m_OwnerEdit->SetError(TEXT(""));
            }
            else
            {
                UE_LOG(LogAcoustics, Error, TEXT("%s"), *m_Status);
                m_OwnerEdit->SetError(m_Status);
            }
            m_LastStatusCheckTime = FDateTime::Now();
        }

        if (info.submit_pending == false)
        {
            jobInfo = TEXT("Job ID: ") + info.job_id + TEXT("\n");
            jobInfo = jobInfo + TEXT("Submit Time: ") + info.submit_time;
        }
    }
    return FText::FromString(header + m_Status + TEXT("\n\n") + jobInfo);
}

#undef LOCTEXT_NAMESPACE