// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include "Engine/GameEngine.h"
#include "AcousticsData.generated.h"

UCLASS(meta = (BlueprintSpawnableComponent))
class PROJECTACOUSTICS_API UAcousticsData : public UObject
{
    GENERATED_UCLASS_BODY()

public:
    /** Relative path to the ACE file. The actual ACE file must be manually placed at this location
     *   separate from this uasset, otherwise it may not be packaged as part of the game and the Project
     *   Acoustics runtime will not be able to find it. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acoustics")
    FString AceFilePath;

    virtual void PostRename(UObject* OldOuter, const FName OldName) override;

    void SetReimportFilepath(FString filepath)
    {
        m_ReimportFilepath = filepath;
    }
    FString GetReimportFilepath() const
    {
        return m_ReimportFilepath;
    }

private:
    void UpdateAceFilePath();

    // When reimporting an asset (e.g. doing a new bake of the same name and dragging
    // it into the content drawer on top of an existing one), UE sets the new file path
    // directly on the existing asset itself. Use this string to keep track of the new
    // location during the reimport process. It will be cleared after reimport is complete.
    FString m_ReimportFilepath;
};