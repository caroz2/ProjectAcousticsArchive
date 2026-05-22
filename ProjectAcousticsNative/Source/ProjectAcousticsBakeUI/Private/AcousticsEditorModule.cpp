// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//   Created from SFoliageEdit class code

#include "AcousticsEditorModule.h"
#include "AcousticsEdMode.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Settings/ProjectPackagingSettings.h"
#include "AcousticsSharedState.h"
#include "Styling/SlateStyle.h"

#define LOCTEXT_NAMESPACE "FAcousticsEditorModule"

#define IMAGE_BRUSH(RelativePath, ...)                                                                                 \
    FSlateImageBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

void FAcousticsEditorModule::StartupModule()
{
    StyleSet = MakeShareable(new FSlateStyleSet("AcousticsEditor"));

    FString ContentDir = IPluginManager::Get().FindPlugin(c_PluginName)->GetBaseDir();

    FString dllPath = FPaths::Combine(
        ContentDir,
        TEXT("Source"),
        TEXT("ThirdParty"),
        TEXT("Win64"),
        TEXT("Release"),
        TEXT("Triton.Preprocessor.dll"));
    m_TritonPreprocessorDllHandle = FPlatformProcess::GetDllHandle(*dllPath);

    if (m_TritonPreprocessorDllHandle == nullptr)
    {
        UE_LOG(
            LogAcoustics,
            Error,
            TEXT("Could not find required DLL at %s. Acoustics module bake UI will not function."),
            *dllPath);
    }

    StyleSet->SetContentRoot(ContentDir);

    StyleSet->Set("PluginIcon.Acoustics", new IMAGE_BRUSH(TEXT("Resources/Icon128"), FVector2D(128.f, 128.f)));

    // Icon used as the Acoustics Mode icon in the Mode toolbar
    StyleSet->Set(
        "ModeIcon.Acoustics", new IMAGE_BRUSH(TEXT("Resources/3D_SpatialAudio_Icon_40X40"), FVector2D(40.f, 40.f)));

    StyleSet->Set("ClassThumbnail.AcousticsData", new IMAGE_BRUSH(TEXT("Resources/Icon128"), FVector2D(128.f, 128.f)));

    // Icons used for the different tabs in our own toolbar
    StyleSet->Set(
        "AcousticsEditMode.SetObjectTag", new IMAGE_BRUSH(TEXT("Resources/Selection-Modes-20"), FVector2D(20.f, 20.f)));
    StyleSet->Set(
        "AcousticsEditMode.SetMaterials", new IMAGE_BRUSH(TEXT("Resources/View-List-20"), FVector2D(20.f, 20.f)));
    StyleSet->Set(
        "AcousticsEditMode.SetProbes", new IMAGE_BRUSH(TEXT("Resources/Cube-Molecule-20"), FVector2D(20.f, 20.f)));
    StyleSet->Set(
        "AcousticsEditMode.SetBake", new IMAGE_BRUSH(TEXT("Resources/Cloud-Upload-20"), FVector2D(20.f, 20.f)));
    // Small icons are the same
    StyleSet->Set(
        "AcousticsEditMode.SetObjectTag.Small",
        new IMAGE_BRUSH(TEXT("Resources/Selection-Modes-20"), FVector2D(20.f, 20.f)));
    StyleSet->Set(
        "AcousticsEditMode.SetMaterials.Small", new IMAGE_BRUSH(TEXT("Resources/View-List-20"), FVector2D(20.f, 20.f)));
    StyleSet->Set(
        "AcousticsEditMode.SetProbes.Small",
        new IMAGE_BRUSH(TEXT("Resources/Cube-Molecule-20"), FVector2D(20.f, 20.f)));
    StyleSet->Set(
        "AcousticsEditMode.SetBake.Small", new IMAGE_BRUSH(TEXT("Resources/Cloud-Upload-20"), FVector2D(20.f, 20.f)));

    StyleSet->Set(
        "AcousticsEditMode.ActiveTabName.Text",
        FTextBlockStyle()
            .SetFont(DEFAULT_FONT("Bold", 14))
            .SetColorAndOpacity(FLinearColor::White)
            .SetShadowOffset(FVector2D(0, 1))
            .SetShadowColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 0.5)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);

    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin
    // file per-module
    FEditorModeRegistry::Get().RegisterMode<FAcousticsEdMode>(
        FAcousticsEdMode::EM_AcousticsEdModeId,
        LOCTEXT("AcousticsEdModeName", "Bake Acoustics"),
        FSlateIcon("AcousticsEditor", "ModeIcon.Acoustics"),
        true);

    // Add the Acoustics content directory to the always staged paths
    // This forces UE to correctly package any ACE files
    UProjectPackagingSettings* PackagingSettings =
        Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());
    FDirectoryPath acousticsPath;
    acousticsPath.Path = FString(TEXT("Acoustics"));
    int32 i;
    for (i = 0; i < PackagingSettings->DirectoriesToAlwaysStageAsUFS.Num(); i++)
    {
        if (PackagingSettings->DirectoriesToAlwaysStageAsUFS[i].Path == acousticsPath.Path)
        {
            break;
        }
    }

    if (i == PackagingSettings->DirectoriesToAlwaysStageAsUFS.Num())
    {
        PackagingSettings->DirectoriesToAlwaysStageAsUFS.Add(acousticsPath);
#if ENGINE_MAJOR_VERSION == 5
        PackagingSettings->TryUpdateDefaultConfigFile();
#else
        PackagingSettings->UpdateDefaultConfigFile();
#endif
    }
}

void FAcousticsEditorModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
    FEditorModeRegistry::Get().UnregisterMode(FAcousticsEdMode::EM_AcousticsEdModeId);
    FSlateStyleRegistry::UnRegisterSlateStyle(StyleSet->GetStyleSetName());

    // AcousticsSharedState contains objects that depend on the DLL we are about to unload.
    AcousticsSharedState::Destroy();

    FPlatformProcess::FreeDllHandle(m_TritonPreprocessorDllHandle);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAcousticsEditorModule, ProjectAcousticsBakeUI)