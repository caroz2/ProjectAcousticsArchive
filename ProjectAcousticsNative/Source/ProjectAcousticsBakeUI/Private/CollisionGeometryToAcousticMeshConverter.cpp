// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#ifdef ENABLE_COLLISION_SUPPORT
#include "CollisionGeometryToAcousticMeshConverter.h"
#include "AcousticsFbxImporter.h"
#include "Runtime/Engine/Public/AssetExportTask.h"
#include "Editor/UnrealEd/Classes/Exporters/ExporterFbx.h"
#include "Editor/UnrealEd/Classes/Exporters/FbxExportOption.h"
#include "Editor/UnrealEd/Classes/Exporters/StaticMeshExporterFBX.h"
#include "Runtime/Engine/Classes/PhysicalMaterials/PhysicalMaterial.h"

bool CollisionGeometryToAcousticMeshConverter::AddCollisionGeometryToAcousticMesh(AcousticMesh* acousticMesh)
{
    TArray<AActor*> actors;
    for (TActorIterator<AActor> itr(GEditor->GetEditorWorldContext().World()); itr; ++itr)
    {
        auto actor = *itr;
        if (HasCollisionTag(actor))
        {
            actors.Add(actor);
        }
    }

    // We rely on UE4 to export the PhysX collision geometry to an intermediate FBX file.
    // This FBX is then parsed and the geometry from collision nodes (UCX_*) is added
    // to the acoustic mesh as acoustic geometry.
    FString fbxFilepath;
    auto result = ExportCollisionsToFbx(actors, &fbxFilepath);
    if (result)
    {
        result = ImportCollisionsFromFbx(acousticMesh, fbxFilepath);
    }
    return result;
}

//
// "Waterfalls" through the material hierarchy and tries to find the most detailed material
// name for use by the acoustics system.
//
// From low to high detail: Default, Material name, Physical Material name, Physical Audio Material name
// When multiple materials are present, priority is given to whichever gives the most detailed info.
// In case of the same detail (eg., two materials both providing audio material), we choose
// the first one.
FName CollisionGeometryToAcousticMeshConverter::ExtractPhysicalMaterialName(
    const TArray<UMaterialInterface*>& Materials)
{
    auto chosenQuality = -1;
    FName chosenName(TEXT("Default_Triton"));

    auto extractNameFromMaterial = [](const UMaterialInterface* material, int* quality) {
        auto name = material->GetName();
        *quality = 0;
        auto physicalMaterial = material->GetPhysicalMaterial();
        if (physicalMaterial != nullptr && physicalMaterial != GEngine->DefaultPhysMaterial)
        {
            name = physicalMaterial->GetName();
        }
        return FName(*name);
    };

    for (const auto material : Materials)
    {
        if (material)
        {
            auto matchQuality = -1;
            auto name = extractNameFromMaterial(material, &matchQuality);
            if (matchQuality > chosenQuality)
            {
                chosenQuality = matchQuality;
                chosenName = name;
            }
        }
    }
    return chosenName;
}

bool CollisionGeometryToAcousticMeshConverter::ExportCollisionsToFbx(
    const TArray<AActor*>& actors, FString* fbxFilepath)
{
    // Nothing to do?
    if (actors.Num() == 0)
    {
        return false;
    }

    // Make sure nothing is selected.
    // We'll mark the passed actors as selected and export to FBX.
    GEditor->SelectNone(true, true, false);
    for (auto actor : actors)
    {
        // Select the actor for collision goemetry export
        GEditor->SelectActor(actor, true, false, true, false);
    }

    // Export the world with selected actors to a transient FBX under the project's intermediate folder
    auto world = GEditor->GetEditorWorldContext().World();
    auto exportTask = NewObject<UAssetExportTask>();
    exportTask->Exporter = UExporter::FindExporter(world, TEXT("FBX"));
    if (exportTask->Exporter == nullptr)
    {
        UE_LOG(LogAcoustics, Error, TEXT("FBX exporter not found"));
        return false;
    }

    auto options = NewObject<UFbxExportOption>();
    options->Collision = true;
    exportTask->Options = options;
    exportTask->bAutomated = true;
    exportTask->bSelected = true;
    exportTask->bReplaceIdentical = true;
    exportTask->Object = world;
    auto filename = world->GetName() + TEXT(".fbx");
    auto filepath = FPaths::Combine(FPaths::ProjectIntermediateDir(), filename);
    exportTask->Filename = filepath;
    exportTask->bAutomated = true;
    if (UExporter::RunAssetExportTask(exportTask) == false)
    {
        UE_LOG(LogAcoustics, Error, TEXT("Failed to export collision meshes to FBX"));
        return false;
    }

    GEditor->SelectNone(true, true, false);

    // Return the FBX filepath to import from...
    *fbxFilepath = filepath;
    return true;
}

bool CollisionGeometryToAcousticMeshConverter::ImportCollisionsFromFbx(
    AcousticMesh* acouticsMesh, const FString& fbxFilepath)
{
    auto importer = AcousticsFbxImporter::Create();
    if (!importer)
    {
        UE_LOG(LogAcoustics, Error, TEXT("Failed to create acoustics FBX importer"));
    }

    if (!importer->ParseFbx(fbxFilepath, 1.0f))
    {
        UE_LOG(LogAcoustics, Error, TEXT("Could not parse exported collision FBX"));
        return false;
    }

    for (const auto& mesh : importer->GetMeshObjects())
    {
        // Only use the mesh if the prefix matches
        if (mesh.GetName().StartsWith(c_FbxCollisionPrefix))
        {
            TArray<ATKVectorF> tritonVertices;
            for (const auto& v : mesh.GetVertices())
            {
                tritonVertices.Add(
                    ATKVectorF{static_cast<float>(v.X), static_cast<float>(v.Y), static_cast<float>(v.Z)});
            }

            TArray<TritonAcousticMeshTriangleInformation> tritonTriangles;
            for (const auto& t : mesh.GetTriangles())
            {
                TritonAcousticMeshTriangleInformation info;
                info.Indices = ATKVectorI{t.VertexIndices[0], t.VertexIndices[1], t.VertexIndices[2]};
                // NOTE: Currently default wall code is used for all collision geometry
                info.MaterialCode = static_cast<TritonMaterialCode>(TRITON_DEFAULT_WALL_CODE);
                tritonTriangles.Add(info);
            }

            if (TritonPreprocessor_AcousticMesh_Add(
                    acouticsMesh->GetHandle(),
                    tritonVertices.GetData(),
                    tritonVertices.Num(),
                    tritonTriangles.GetData(),
                    tritonTriangles.Num(),
                    MeshTypeGeometry) == false)
            {
                return false;
            }
        }
    }
    return true;
}
#endif // ENABLE_COLLISION_SUPPORT