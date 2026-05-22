// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "GameFramework/Volume.h"
#include "AcousticsPinnedProbe.generated.h"

// Pinned probes allow for manual placement of individual probes at any point in a scene that was baked with Project Acoustics.
// Probe points are possible player (listener) locations in the scene. Acoustic simulations are performed at each one of these points.
// At runtime, the listener location is interpolated among nearby probe points. Most probe points are automatically placed during the 
// pre-bake process (Probes tab), but this actor allows for placement of additional probes.
UCLASS(ClassGroup = ProjectAcoustics, hidecategories = (Advanced, Attachment), BlueprintType)
class AAcousticsPinnedProbe : public AActor
{
    GENERATED_UCLASS_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AcousticsPinnedProbe")
    class UStaticMeshComponent* ProbeMesh;

public:
    // This class only helps with Acoustics pre-bake design, and is not meant for use in-game.
    virtual bool IsEditorOnly() const override
    {
        return true;
    }
};