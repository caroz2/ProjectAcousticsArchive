// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "GameFramework/Volume.h"
#include "AcousticsDesignParams.h"
#include "AcousticsRuntimeVolume.generated.h"

/**
 *	Volume used to override runtime design parameters for acoustics audio components. Any acoustics audio components
 *	that can be overridden and are inside this volume will have their acoustic parameters be overridden by the values
 *	provided by this volume. Based on the type of parameter, the override will either be an addition or a
 *multiplication. For example, DecayTimeMultiplier override will be multiplied with the value in the component, while
 *WetnessAdjustment will be added to the value in the component. Thus, if an acoustics audio component is inside
 *multiple acoustics runtime volumes at any time, it will be affected by the overrides of all those volumes.
 */
UCLASS(ClassGroup = ProjectAcoustics, hidecategories = (Advanced, Attachment), BlueprintType)
class PROJECTACOUSTICS_API AAcousticsRuntimeVolume : public AVolume
{
    GENERATED_UCLASS_BODY()

public:
    /**
     *	The design params to override the acoustics audio components found inside this volume.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FAcousticsDesignParams OverrideDesignParams;
};