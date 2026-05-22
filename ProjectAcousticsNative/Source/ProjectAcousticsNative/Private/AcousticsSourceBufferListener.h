// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "AcousticsSourceDataOverride.h"
#include "AcousticsSpatialReverb.h"

// Saves input buffers for the Project Acoustics Source Data Override plugin
class FAcousticsSourceBufferListener : public ISourceBufferListener
{
public:
    FAcousticsSourceBufferListener(FAcousticsSourceDataOverride* ptr);

private:
    //~ Begin ISourceBufferListener
    void OnNewBuffer(const ISourceBufferListener::FOnNewBufferParams& InParams) override;
    void OnSourceReleased(const int32 SourceId) override;
    //~ End ISourceBufferListener

    class FAcousticsSourceDataOverride* m_SourceDataOverridePtr;
};