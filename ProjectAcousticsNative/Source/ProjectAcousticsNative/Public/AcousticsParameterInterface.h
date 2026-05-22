// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "Engine/Attenuation.h"
#include "IAudioExtensionPlugin.h"
#include "IAudioParameterInterfaceRegistry.h"

// Defines a MetaSound interface that inputs Project Acoustics parameters
namespace AcousticsParameterInterface
{
    const extern FName Name;

    namespace Inputs
    {
        const extern FName DryLoudness;
        const extern FName DryPathLength;
        const extern FName DryArrivalAzimuth;
        const extern FName DryArrivalElevation;
        const extern FName WetLoudness;
        const extern FName WetAngularSpread;
        const extern FName WetDecayTime;
        const extern FName WetArrivalAzimuth;
        const extern FName WetArrivalElevation;
    } // namespace Inputs

    Audio::FParameterInterfacePtr GetInterface();
    void RegisterInterface();
} // namespace AcousticsParameterInterface
