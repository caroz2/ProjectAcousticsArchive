// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//   Created from SFoliageEdit class code

#pragma once

#include "CoreMinimal.h"
#include "EditorStyleSet.h"
#include "Framework/Commands/Commands.h"

/**
 * Acoustics Edit mode actions
 */
class FAcousticsEditCommands : public TCommands<FAcousticsEditCommands>
{
public:
    FAcousticsEditCommands()
        : TCommands<FAcousticsEditCommands>(
              "AcousticsEditMode", // Context name for fast lookup
              NSLOCTEXT(
                  "Contexts", "AcousticsEditMode", "Acoustics Edit Mode"), // Localized context name for displaying
              NAME_None,                                                   // Parent
              "AcousticsEditor"                                            // Icon Style Set
          )
    {
    }

    /**
     * Acoustics Edit Commands
     */

    /** Commands for the tabs switcher toolbar. */
    TSharedPtr<FUICommandInfo> SetObjectTag;
    TSharedPtr<FUICommandInfo> SetMaterials;
    TSharedPtr<FUICommandInfo> SetProbes;
    TSharedPtr<FUICommandInfo> SetBake;

    /**
     * Initialize commands
     */
    virtual void RegisterCommands() override;
};
