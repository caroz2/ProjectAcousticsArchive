// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsPinnedProbe.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "Runtime/Launch/Resources/Version.h"
#include "AcousticsEdMode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include <AcousticsShared.h>

AAcousticsPinnedProbe::AAcousticsPinnedProbe(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsEditorOnlyActor = true;

    Tags.Add(c_AcousticsNavigationTag);

    // Mesh sphere
    ProbeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProbeMesh"));
    SetRootComponent(ProbeMesh);

    ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
    ProbeMesh->SetStaticMesh(MeshAsset.Object);
    ProbeMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));

    ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
        TEXT("Material'/Engine/EngineMaterials/CubeMaterial.CubeMaterial'"));
    UMaterialInterface* material = MaterialAsset.Object;
    if (material)
    {
        ProbeMesh->SetMaterial(0, material);
    }
}