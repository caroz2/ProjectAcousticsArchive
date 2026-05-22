// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "AcousticsSpatializerSettings.generated.h"

UENUM(BlueprintType)
enum class EFlexEngineType : uint8
{
    HIGH_QUALITY    UMETA(DisplayName = "High Quality"),
    LOW_QUALITY     UMETA(DisplayName = "Good Quality"),
    STEREO_PANNING  UMETA(DisplayName = "Stereo Panning")
};

UCLASS(config = Engine, defaultconfig)
class PROJECTACOUSTICSSPATIALIZER_API UAcousticsSpatializerSettings : public UObject
{
    GENERATED_BODY()

public:

    UAcousticsSpatializerSettings();

    // setting for modifying the spatializer quality level
    UPROPERTY(GlobalConfig, EditAnywhere, Category = "General", meta = (DisplayName = "Engine Type"))
    EFlexEngineType FlexEngineType;
};
