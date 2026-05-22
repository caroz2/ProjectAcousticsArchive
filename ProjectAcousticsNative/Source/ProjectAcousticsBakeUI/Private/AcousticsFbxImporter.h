// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#ifdef ENABLE_COLLISION_SUPPORT
#define FBXSDK_NEW_API
#include <cstdlib>
#include <cstring>
#include <fbxsdk.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Custom deleter for FBX types
template <typename T>
struct FbxDestroyer
{
    void operator()(T* object) const
    {
        if (object)
        {
            object->Destroy();
        }
    }
};

// FBX Importer Vertex
struct VertexFbx
{
    double X;
    double Y;
    double Z;

    VertexFbx()
    {
    }

    VertexFbx(double in_x, double in_y, double in_z) : X(in_x), Y(in_y), Z(in_z)
    {
    }
};

// FBX Importer Triangle
struct TriangleFbx
{
    int VertexIndices[3];
    int MaterialIndex;
};

// FBX Importer Mesh
class ObjectMeshFbx
{

public:
    ObjectMeshFbx(const FString& name) : m_Name(name)
    {
    }

    FString GetName() const
    {
        return m_Name;
    }

    TArray<VertexFbx> GetVertices() const
    {
        return m_Vertices;
    }

    TArray<VertexFbx>& GetVertices()
    {
        return m_Vertices;
    }

    TArray<TriangleFbx> GetTriangles() const
    {
        return m_Triangles;
    }

    TArray<TriangleFbx>& GetTriangles()
    {
        return m_Triangles;
    }

private:
    TArray<TriangleFbx> m_Triangles;
    TArray<VertexFbx> m_Vertices;
    FString m_Name;
};

// FBX Importer Interface
class AcousticsFbxImporter final
{
public:
    AcousticsFbxImporter();
    static TUniquePtr<AcousticsFbxImporter> Create();
    virtual ~AcousticsFbxImporter()
    {
    }

    bool ParseFbx(const FString& filePath, float unitAdjustment);

    float GetMeshUnitInMeters() const
    {
        return m_MeshUnitInMeters;
    }

    TArray<ObjectMeshFbx> GetMeshObjects() const
    {
        return m_OutMeshes;
    }

    TArray<ObjectMeshFbx>& GetMeshObjects()
    {
        return m_OutMeshes;
    }

    int GetMeshObjectCount()
    {
        return m_OutMeshes.Num();
    }

private:
    bool InitializeSdkManagerAndScene();
    bool FbxLoadScene(const char* fbxFilePath);
    void ParseScene(TArray<ObjectMeshFbx>& meshes, float scaleToMeters, FbxNode* node = nullptr);
    void ExtractMeshFromNode(TArray<ObjectMeshFbx>& meshes, float scaleToMeters, FbxNode* node);

private:
    TUniquePtr<FbxManager, FbxDestroyer<FbxManager>> m_SdkManager;
    TUniquePtr<FbxGeometryConverter> m_Converter;
    FbxScene* m_Scene; // Scene gets cleaned up when SDK manager is freed

    // Map from FBX metadata strings to indices
    TMap<FString, int> m_MaterialNameToCode;
    TArray<ObjectMeshFbx> m_OutMeshes;
    int m_NumMeshObjects;
    int m_NumObjectsUnhandledMaterials;
    float m_MeshUnitInMeters;
};

// This type exists solely to access FBXAxisSystem's protected function -- GetConversionMatrix()
class FbxAxisSystemProtectedAccessor : protected FbxAxisSystem
{
public:
    FbxAxisSystemProtectedAccessor(const FbxAxisSystem& Other) : FbxAxisSystem(Other)
    {
    }

    virtual ~FbxAxisSystemProtectedAccessor()
    {
    }

    void GetConversionMatrixFrom(FbxAxisSystem FromAxisSystem, FbxMatrix& ConversionMatrix)
    {
        GetConversionMatrix(FromAxisSystem, ConversionMatrix);
    }
};
#endif // ENABLE_COLLISION_SUPPORT