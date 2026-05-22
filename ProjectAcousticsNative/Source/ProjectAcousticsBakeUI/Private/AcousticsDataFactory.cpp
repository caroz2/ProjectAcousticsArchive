// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsDataFactory.h"
#include "AcousticsData.h"
#include "Misc/Paths.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PackageTools.h"
#include "AcousticsEdMode.h"
#include "Logging/LogMacros.h"
#include "UObject/SavePackage.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFilemanager.h"

UAcousticsDataFactory::UAcousticsDataFactory(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Property initialization
    SupportedClass = UAcousticsData::StaticClass();
    Formats.Add(TEXT("ace;Project Acoustics Data"));

    // It's not valid to create a new ACE asset without a backing file
    // Turn off the "Create New" behavior, which will bypass any file import hooks
    bCreateNew = false;

    // We do want to support file import / drag-and-drop
    bEditorImport = true;

    // No meaningful editor
    bEditAfterNew = false;

    // Can't use text to intiailize object - must be ACE file
    bText = false;

    // Reimporting is allowed
    ImportPriority = DefaultImportPriority;

    // Scan the editor preferences to make it such that ace files in /Game/ do not trigger an
    // import dialog popup.
    FString aceWildcard = "*.ace";
    for (FAutoReimportDirectoryConfig& Setting : GetMutableDefault<UEditorLoadingSavingSettings>()->AutoReimportDirectorySettings)
    {
        // We only care about the /Game/ directory
        if (Setting.SourceDirectory.Compare("/Game/"))
        {
            continue;
        }

        // Check if we've already added the ACE wildcard rule
        bool wildcardCreated = false;
        for (FAutoReimportWildcard wildcardConfig : Setting.Wildcards)
        {
            if (!wildcardConfig.Wildcard.Compare(aceWildcard))
            {
                wildcardCreated = true;
                break;
            }
        }
        
        // Create a rule to ignore ACE files when scanning for files to auto-import
        // if we haven't already
        if (!wildcardCreated)
        {
            FAutoReimportWildcard wildcardConfig;
            wildcardConfig.bInclude = false;
            wildcardConfig.Wildcard = aceWildcard;
            Setting.Wildcards.Add(wildcardConfig);
        }
    }
}

bool UAcousticsDataFactory::FactoryCanImport(const FString& Filename)
{
    // check extension
    if (FPaths::GetExtension(Filename) == TEXT("ace"))
    {
        return true;
    }

    return false;
}

UObject* UAcousticsDataFactory::ImportFromFile(const FString& aceFilepath)
{
    auto name = FPaths::GetBaseFilename(aceFilepath);
    auto packageName = FString(TEXT("/Game/Acoustics/")) + name;
    auto packageFilename =
        FPackageName::LongPackageNameToFilename(packageName, FPackageName::GetAssetPackageExtension());

    // Does the UAsset already exist?
    auto uasset = LoadObject<UAcousticsData>(
        nullptr, *packageName, *packageFilename, LOAD_Verify | LOAD_NoWarn | LOAD_Quiet, nullptr);
    if (uasset)
    {
        return uasset;
    }

    // UAsset doesn't exist. Create one
    auto package = CreatePackage(*packageName);
    if (package == nullptr)
    {
        UE_LOG(
            LogAcoustics,
            Error,
            TEXT("Failed to create package %s while importing %s, please manually create an AcousticData asset named "
                 "%s."),
            *packageName,
            *aceFilepath,
            *name);
        return nullptr;
    }

    auto asset = NewObject<UAcousticsData>(package, UAcousticsData::StaticClass(), *name, RF_Standalone | RF_Public);
    if (asset)
    {
        FAssetRegistryModule::AssetCreated(asset);
        asset->MarkPackageDirty();
        asset->PostEditChange();
        asset->AddToRoot();
#if ENGINE_MAJOR_VERSION == 5
        FSavePackageArgs saveArgs;
        saveArgs.TopLevelFlags = RF_Standalone;
        UPackage::SavePackage(package, nullptr, *packageFilename, saveArgs);
#else
        UPackage::SavePackage(package, nullptr, RF_Standalone, *packageFilename);
#endif

    }
    else
    {
        UE_LOG(
            LogAcoustics,
            Error,
            TEXT("Failed to import %s, please manually create an AcousticData asset named %s."),
            *aceFilepath,
            *name);
    }
    return asset;
}

UObject* UAcousticsDataFactory::FactoryCreateFile(
    UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms,
    FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    // Copy over the ACE file to the target directory
    auto targetPath = FPackageName::LongPackageNameToFilename(InParent->GetName(), TEXT(".ace"));

    // Only need to copy if targetpath and filename are different. Otherwise, the ACE file is already in the target directory
    if (targetPath.Compare(Filename, ESearchCase::IgnoreCase) != 0)
    {
        IPlatformFile& pfm = FPlatformFileManager::Get().GetPlatformFile();
        if (!pfm.CopyFile(*targetPath, *Filename))
        {
            UE_LOG(LogAcoustics, Error, TEXT("Failed to copy %s, see output log for details."), *Filename);
            return nullptr;
        }
    }

    // With ACE copied, create the UAsset
    return ImportFromFile(Filename);
}


bool UAcousticsDataFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    UAcousticsData* data = Cast<UAcousticsData>(Obj);
    if (data != nullptr)
    {
        return true;
    }
    return false;
}

void UAcousticsDataFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    UAcousticsData* data = Cast<UAcousticsData>(Obj);
    if (data != nullptr && ensure(NewReimportPaths.Num() == 1))
    {
        data->SetReimportFilepath(NewReimportPaths[0]);
    }
}

EReimportResult::Type UAcousticsDataFactory::Reimport(UObject* Obj)
{
    if (Obj == nullptr || !Obj->IsA(UAcousticsData::StaticClass()))
    {
        return EReimportResult::Failed;
    }

    UAcousticsData* data = Cast<UAcousticsData>(Obj);

    // To re-import, only thing we need to do is re-copy the file
    auto originalPath = data->GetReimportFilepath();
    auto targetPath = FPaths::ProjectDir() + data->AceFilePath;
    IPlatformFile& pfm = FPlatformFileManager::Get().GetPlatformFile();
    if (!pfm.CopyFile(*targetPath, *originalPath))
    {
        UE_LOG(LogAcoustics, Error, TEXT("Failed to copy %s, see output log for details"), *originalPath);
        return EReimportResult::Failed;
    }

    // Done with reimport -- clear out the stored filepath
    data->SetReimportFilepath(FString());
    return EReimportResult::Succeeded;
}

int32 UAcousticsDataFactory::GetPriority() const
{
    return DefaultImportPriority;
}