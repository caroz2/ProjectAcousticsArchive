// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsEditActions.h"

#define LOCTEXT_NAMESPACE "AcousticsEditCommands"

void FAcousticsEditCommands::RegisterCommands()
{
    UI_COMMAND(
        SetObjectTag,
        "Objects",
        "Tag objects for acoustics",
        EUserInterfaceActionType::ToggleButton,
        FInputChord());
    UI_COMMAND(
        SetMaterials,
        "Materials",
        "Assign acoustic properties to materials",
        EUserInterfaceActionType::ToggleButton,
        FInputChord());
    UI_COMMAND(
        SetProbes, "Probes", "Calculate probes and voxels", EUserInterfaceActionType::ToggleButton, FInputChord());
    UI_COMMAND(SetBake, "Bake", "Submit cloud bake", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE
