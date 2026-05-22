// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Modules/ModuleManager.h"
#include "AcousticsReverbSettingsFactory.h"

class FAcousticsSpatializerEditorModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    TSharedPtr<FAssetTypeActions_AcousticsReverbSettings> AcousticsReverbSettingsActions;
};
