// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsParameterInterface.h"
#include "Sound/SoundBase.h"

// Required for ensuring the node is supported by all languages in engine.
#define LOCTEXT_NAMESPACE "ProjectAcousticsParameterInterface"

#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "ProjectAcoustics"

namespace AcousticsParameterInterface
{
    const FName Name = AUDIO_PARAMETER_INTERFACE_NAMESPACE;

    // The input parameters to the Project Acoustics MetaSound Interface
    namespace Inputs
    {
        const FName DryLoudness = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Dry.Loudness");
        const FName DryPathLength = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Dry.PathLength");
        const FName DryArrivalAzimuth = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Dry.ArrivalAzimuth");
        const FName DryArrivalElevation = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Dry.ArrivalElevation");
        const FName WetLoudness = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Wet.Loudness");
        const FName WetAngularSpread = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Wet.AngularSpread");
        const FName WetDecayTime = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Wet.DecayTime");
        const FName WetArrivalAzimuth = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Wet.ArrivalAzimuth");
        const FName WetArrivalElevation = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Wet.ArrivalElevation");
    } // namespace Inputs

    Audio::FParameterInterfacePtr GetInterface()
    {
        struct FInterface : public Audio::FParameterInterface
        {
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 2)
            FInterface() : FParameterInterface(AcousticsParameterInterface::Name, {1, 0}, *USoundBase::StaticClass())
#else
            FInterface() : FParameterInterface(AcousticsParameterInterface::Name, {1, 0})
#endif
            {
                Inputs = {
                    // Dry Loudness
                    {FText(),
                     LOCTEXT(
                         "DryLoudnessDescription",
                         "This is the dB attenuation due to diffraction around the scene for the shortest path "
                         "connecting source to listener"), // Hovertext
                     FName(),
                     {Inputs::DryLoudness, 0.0f}},
                    // Dry Path Length
                    {FText(),
                     LOCTEXT(
                         "DryPathLengthDescription",
                         "The shortest-path length in centimeters for sound to get from the audio source to listener, "
                         "including navigating around geometry"), // Hovertext
                     FName(),
                     {Inputs::DryPathLength, 0.0f}},
                    // Dry Arrival Azimuth
                    {FText(),
                     LOCTEXT(
                         "DryArrivalAzimuthDescription",
                         "Azimuth, in degrees, for which the dry sound arrives at the listener. 0 right, 90 front, 180 "
                         "left, 270 behind"), // Hovertext
                     FName(),
                     {Inputs::DryArrivalAzimuth, 90.0f}},
                    // Dry Arrival Elevation
                    {FText(),
                     LOCTEXT(
                         "DryArrivalElevationDescription",
                         "Elevation, in degrees, for which the dry sound arrives at the listener. 0 level, 90 above, "
                         "-90 below"), // Hovertext
                     FName(),
                     {Inputs::DryArrivalElevation, 0.0f}},
                    // Wet Loudness
                    {FText(),
                     LOCTEXT(
                         "WetLoudnessDescription",
                         "Models the power of reverberation in dB."), // Hovertext
                     FName(),
                     {Inputs::WetLoudness, -100.0f}},
                    // Wet Angular Spread
                    {FText(),
                     LOCTEXT(
                         "WetAngularSpreadDescription",
                         "Perceived width of reverberation, in degrees. Varies continuously with 0 indicating "
                         "localized reverb such as heard through a small window, and 360 meaning fully immersive "
                         "reverb in the center of a room"), // Hovertext
                     FName(),
                     {Inputs::WetAngularSpread, 360.0f}},
                    // Wet Decay Time
                    {FText(),
                     LOCTEXT(
                         "WetDecayTimeDescription",
                         "The reverberation time in seconds. The time it takes reverb to decay by 60dB"), // Hovertext
                     FName(),
                     {Inputs::WetDecayTime, 0.0f}},
                    // Wet Arrival Azimuth
                    {FText(),
                     LOCTEXT(
                         "WetArrivalAzimuthDescription",
                         "Azimuth, in degrees, for which the wet sound arrives at the listener. 0 right, 90 front, 180 "
                         "left, 270 behind"), // Hovertext
                     FName(),
                     {Inputs::WetArrivalAzimuth, 90.0f}},
                    // Wet Arrival Elevation
                    {FText(),
                     LOCTEXT(
                         "WetArrivalElevationDescription",
                         "Elevation, in degrees, for which the wet sound arrives at the listener. 0 level, 90 above, "
                         "-90 below"), // Hovertext
                     FName(),
                     {Inputs::WetArrivalElevation, 0.0f}},
                };
            }
        };

        static Audio::FParameterInterfacePtr InterfacePtr;
        if (!InterfacePtr.IsValid())
        {
            InterfacePtr = MakeShared<FInterface>();
        }

        return InterfacePtr;
    }
    void RegisterInterface()
    {
        Audio::IAudioParameterInterfaceRegistry& InterfaceRegistry = Audio::IAudioParameterInterfaceRegistry::Get();
        InterfaceRegistry.RegisterInterface(GetInterface());
    }
} // namespace AcousticsParameterInterface
#undef AUDIO_PARAMETER_INTERFACE_NAMESPACE
#undef LOCTEXT_NAMESPACE