// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifdef ENABLE_COLLISION_SUPPORT

#include "AcousticsFbxImporter.h"
#include <algorithm>
#include "core/math/fbxmatrix.h"

AcousticsFbxImporter::AcousticsFbxImporter()
    : m_Scene(nullptr), m_NumMeshObjects(0), m_NumObjectsUnhandledMaterials(0), m_MeshUnitInMeters(1.0f)
{
}

TUniquePtr<AcousticsFbxImporter> AcousticsFbxImporter::Create()
{
    auto importer = MakeUnique<AcousticsFbxImporter>();
    if (importer && !importer->InitializeSdkManagerAndScene())
    {
        importer.Reset();
    }
    return importer;
}

bool AcousticsFbxImporter::InitializeSdkManagerAndScene()
{
    // Create the FBX SDK memory manager object, and a geometry converter
    m_SdkManager = TUniquePtr<FbxManager, FbxDestroyer<FbxManager>>(FbxManager::Create());
    if (m_SdkManager.Get() == nullptr)
    {
        return false;
    }

    auto ios = FbxIOSettings::Create(m_SdkManager.Get(), IOSROOT);
    if (ios == nullptr)
    {
        return false;
    }

    ios->SetBoolProp(IMP_FBX_MATERIAL, true);
    ios->SetBoolProp(IMP_FBX_TEXTURE, false);
    m_SdkManager->SetIOSettings(ios);

    m_Scene = FbxScene::Create(m_SdkManager.Get(), "");
    if (m_Scene == nullptr)
    {
        return false;
    }

    return true;
}

bool AcousticsFbxImporter::FbxLoadScene(const char* fbxFilePath)
{
    auto importer = TUniquePtr<FbxImporter, FbxDestroyer<FbxImporter>>(FbxImporter::Create(m_SdkManager.Get(), ""));
    if (importer.Get() == nullptr)
    {
        return false;
    }

    // Initialize the importer by providing a filename.
    if (!importer->Initialize(fbxFilePath, -1, m_SdkManager->GetIOSettings()))
    {
        return false;
    }

    m_Scene->Clear();

    if (!importer->Import(m_Scene))
    {
        return false;
    }

    return true;
}

void AcousticsFbxImporter::ParseScene(TArray<ObjectMeshFbx>& meshes, float scaleToMeters, FbxNode* node)
{
    // Call with nullptr means start from root
    if (node == nullptr)
    {
        node = m_Scene->GetRootNode();
    }

    // entry point of recursion
    if (node == m_Scene->GetRootNode())
    {
        m_Converter = MakeUnique<FbxGeometryConverter>(m_SdkManager.Get());
        m_NumMeshObjects = m_Scene->GetMemberCount(FbxCriteria::ObjectType(FbxMesh::ClassId));
        m_NumObjectsUnhandledMaterials = 0;
    }
    else if (
        node->GetNodeAttribute() != nullptr && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        ExtractMeshFromNode(meshes, scaleToMeters, node);
    }

    // recurse to children
    for (auto i = 0; i < node->GetChildCount(); i++)
    {
        auto child = node->GetChild(i);
        if (child != nullptr)
        {
            ParseScene(meshes, scaleToMeters, child);
        }
    }

    // Exiting whole recursion tree, cleanup
    if (node == m_Scene->GetRootNode())
    {
        m_Converter.Reset();

        if (m_NumObjectsUnhandledMaterials != 0)
        {
            UE_LOG(
                LogAcoustics,
                Warning,
                TEXT("Couldn't handle material mapping for %d objects, assumed no-material for them."),
                m_NumObjectsUnhandledMaterials);
        }
    }
}

void AcousticsFbxImporter::ExtractMeshFromNode(TArray<ObjectMeshFbx>& meshes, float ScaleToMeters, FbxNode* node)
{
    m_Converter->Triangulate(node->GetNodeAttribute(), true);

    auto transformMatrix = static_cast<FbxMatrix>(node->EvaluateGlobalTransform());
    auto attributeCount = node->GetNodeAttributeCount();

    // Loop on all meshes present in node
    for (auto attr = 0; attr < attributeCount; attr++)
    {
        auto attribute = node->GetNodeAttributeByIndex(attr);

        // Of everything attached to this node, only process meshes
        if (attribute == nullptr || attribute->GetAttributeType() != FbxNodeAttribute::eMesh)
        {
            continue;
        }

        auto mesh = static_cast<FbxMesh*>(attribute);

        ObjectMeshFbx outputMesh(node->GetName());

        // Incorporate node's materials into global dictionary, assign new codes to any new names
        for (auto poly = 0; poly < node->GetMaterialCount(); poly++)
        {
            auto name = FString(node->GetMaterial(poly)->GetName());
            if (m_MaterialNameToCode.Find(name) == nullptr)
            {
                m_MaterialNameToCode.Add(name, m_MaterialNameToCode.Num());
            }
        }

        // Maps global control point index to indexing in mesh
        // Triangles in mesh use this local indexing, making
        // each object mesh completely self sufficient, containing
        // transformed vertex coordinates in a common global coordinate system.
        TMap<int, int> vertIndexGlobalToLocal;
        vertIndexGlobalToLocal.Empty();

        // Find first layer with any material and use that.
        FbxLayerElementMaterial* materialInfo = nullptr;
        for (auto layer = 0; layer < mesh->GetLayerCount() && materialInfo == nullptr; layer++)
        {
            if (mesh->GetLayer(layer) == nullptr)
            {
                continue;
            }

            materialInfo = mesh->GetLayer(layer)->GetMaterials();
        }

        // If the mapping mode is not per-polygon, punt on material handling
        if (materialInfo != nullptr && materialInfo->GetMappingMode() != FbxLayerElement::eByPolygon &&
            materialInfo->GetMappingMode() != FbxLayerElement::eAllSame)
        {
            materialInfo = nullptr;
            m_NumObjectsUnhandledMaterials++;
        }

        check(
            materialInfo == nullptr || materialInfo->GetReferenceMode() == FbxLayerElement::eIndexToDirect &&
                                           "This always holds true for FBX2011 onwards as per doc page on "
                                           "FbxLayerElementMaterial class. Old-format FBX?");

        for (auto poly = 0; poly < mesh->GetPolygonCount(); poly++)
        {
            TriangleFbx triangle;

            //
            // triangle geom
            //
            int numVertices = mesh->GetPolygonSize(poly);
            check(numVertices <= 3 && "Mesh should be triangulated by now");

            // Failsafe for release build
            numVertices = FGenericPlatformMath::Min(numVertices, 3);

            for (auto vert = 0; vert < numVertices; vert++)
            {
                auto globalVertIndex = mesh->GetPolygonVertex(poly, vert);

                int vertIndexInObject;
                auto value = vertIndexGlobalToLocal.Find(globalVertIndex);
                if (value == nullptr)
                {
                    vertIndexInObject = vertIndexGlobalToLocal.Num();

                    vertIndexGlobalToLocal.Add(globalVertIndex, vertIndexInObject);

                    // Multiply global transform matrix by the control point vector
                    auto ControlPoint = transformMatrix.MultNormalize(mesh->GetControlPointAt(globalVertIndex));

                    // After transform, apply scaling to make vertex coordinates in metric units
                    VertexFbx v(
                        ScaleToMeters * ControlPoint.mData[0],
                        ScaleToMeters * ControlPoint.mData[1],
                        ScaleToMeters * ControlPoint.mData[2]);

                    outputMesh.GetVertices().Add(v);
                }
                else
                {
                    vertIndexInObject = *value;
                }

                triangle.VertexIndices[vert] = vertIndexInObject;
            }

            //
            // triangle material
            //
            FString materialName = TEXT("MaterialAbsent");
            if (materialInfo != nullptr)
            {
                switch (materialInfo->GetMappingMode())
                {
                    case FbxLayerElement::eByPolygon:
                    {
                        auto& arr = materialInfo->GetIndexArray();
                        if (poly < arr.GetCount())
                        {
                            int materialIndex = arr.GetAt(poly);
                            if (materialIndex >= 0 && materialIndex < node->GetMaterialCount())
                            {
                                materialName = node->GetMaterial(materialIndex)->GetName();
                            }
                        }
                        else if (0 < arr.GetCount())
                        {
                            int materialIndex = arr.GetAt(0);
                            if (materialIndex >= 0 && materialIndex < node->GetMaterialCount())
                            {
                                materialName = node->GetMaterial(materialIndex)->GetName();
                            }
                        }
                    }
                    break;
                    case FbxLayerElement::eAllSame:
                    {
                        auto& arr = materialInfo->GetIndexArray();

                        if (0 < arr.GetCount())
                        {
                            int materialIndex = arr.GetAt(0);
                            materialName = node->GetMaterial(materialIndex)->GetName();
                        }
                    }
                    break;
                    default:
                        check(false);
                }
            }

            check(
                m_MaterialNameToCode.Find(materialName) &&
                "All material names attached to node should have been accounted for earlier.");

            triangle.MaterialIndex = m_MaterialNameToCode[materialName];

            outputMesh.GetTriangles().Add(triangle);
        } // Loop on polygons

        meshes.Add(outputMesh);
    } // Loop on meshes in node
}

bool AcousticsFbxImporter::ParseFbx(const FString& filePath, float unitAdjustment)
{
    m_OutMeshes.Empty();
    m_MaterialNameToCode.Empty();
    m_MaterialNameToCode.Add(TEXT("MaterialAbsent"), -1);

    if (!FbxLoadScene(TCHAR_TO_ANSI(*filePath)))
    {
        return false;
    }

    m_MeshUnitInMeters =
        static_cast<float>(m_Scene->GetGlobalSettings().GetSystemUnit().GetConversionFactorTo(FbxSystemUnit::m));
    const float scaleToMeters = m_MeshUnitInMeters * unitAdjustment;

    // Convert scene to maya Z up, right-handed coordinate system.
    FbxAxisSystem::MayaZUp.ConvertScene(m_Scene);

    // Fill output meshes and materials
    ParseScene(m_OutMeshes, scaleToMeters);

    return true;
}
#endif // ENABLE_COLLISION_SUPPORT