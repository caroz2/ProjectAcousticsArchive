// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsData.h"

UAcousticsData::UAcousticsData(const class FObjectInitializer& ObjectInitializer)
{
    UpdateAceFilePath();
}

void UAcousticsData::PostRename(UObject* OldOuter, const FName OldName)
{
    UpdateAceFilePath();
}

void UAcousticsData::UpdateAceFilePath()
{
    AceFilePath = TEXT("Content/Acoustics/") + GetName() + TEXT(".ace");
}