// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsMesh.h"
#include "Templates/UniquePtr.h"

AcousticMesh::~AcousticMesh()
{
    TritonPreprocessor_AcousticMesh_Destroy(m_Handle);
}

TUniquePtr<AcousticMesh> AcousticMesh::Create()
{
    auto instance = TUniquePtr<AcousticMesh>(new AcousticMesh());
    if (!instance->Initialize())
    {
        instance.Reset();
    }
    return instance;
}

bool AcousticMesh::Add(
    ATKVectorD* vertices, int vertexCount, TritonAcousticMeshTriangleInformation* triangleInfos, int trianglesCount,
    MeshType type)
{
    // Remember if navigation mesh is added
    if (type == MeshTypeNavigation)
    {
        m_HasNavigationMesh = true;
    }
    else if (type == MeshTypeGeometry)
    {
        m_HasGeometryMesh = true;
    }

    return TritonPreprocessor_AcousticMesh_Add(m_Handle, vertices, vertexCount, triangleInfos, trianglesCount, type);
}

bool AcousticMesh::AddProbeSpacingVolume(
    ATKVectorD* vertices, int vertexCount, TritonAcousticMeshTriangleInformation* triangleInfos, int trianglesCount,
    float spacing)
{
    // Spacing value in UE is in centimeters, but triton operates in meters
    return TritonPreprocessor_AcousticMesh_AddProbeSpacingVolume(
        m_Handle, vertices, vertexCount, triangleInfos, trianglesCount, spacing / 100);
}

bool AcousticMesh::AddPinnedProbe(ATKVectorD probeLocation)
{
    return TritonPreprocessor_AcousticMesh_AddPinnedProbe(m_Handle, probeLocation);
}

const TritonObject& AcousticMesh::GetHandle() const
{
    return m_Handle;
}

bool AcousticMesh::Initialize()
{
    return TritonPreprocessor_AcousticMesh_Create(&m_Handle);
}
