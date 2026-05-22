// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#ifdef ENABLE_COLLISION_SUPPORT
#include "AcousticsMesh.h"

// Tag used to mark actors that need to use collision mesh, instead of the render mesh, as the acoustic mesh
const FName c_AcousticsCollisionTag = "AcousticsCollisionGeometry";

// FBX format uses UCX_ prefix for collision mesh nodes
const FString c_FbxCollisionPrefix = TEXT("UCX_");

class CollisionGeometryToAcousticMeshConverter final
{
public:
    // Use the collision instead of render meshes as acoustic meshes for the passed actor
    static bool AddCollisionGeometryToAcousticMesh(AcousticMesh* acousticMesh);

private:
    static bool HasCollisionTag(AActor* actor)
    {
        return actor->ActorHasTag(c_AcousticsCollisionTag);
    }

    static FName ExtractPhysicalMaterialName(const TArray<UMaterialInterface*>& Materials);
    static bool ExportCollisionsToFbx(const TArray<AActor*>& actors, FString* fbxFilepath);
    static bool ImportCollisionsFromFbx(AcousticMesh* acouticsMesh, const FString& fbxFilepath);
};
#endif // ENABLE_COLLISION_SUPPORT