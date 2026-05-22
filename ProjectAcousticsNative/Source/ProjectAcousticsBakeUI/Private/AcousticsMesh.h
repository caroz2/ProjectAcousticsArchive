// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "TritonPreprocessorApi.h"
// Add include for non-unity build
#include "Templates/UniquePtr.h"

// C++ wrappers for Triton Preprocessor types
class AcousticMesh final
{
public:
    ~AcousticMesh();
    static TUniquePtr<AcousticMesh> Create();
    bool
    Add(ATKVectorD* vertices, int vertexCount, TritonAcousticMeshTriangleInformation* triangleInfos, int trianglesCount,
        MeshType type);
    bool AddProbeSpacingVolume(
        ATKVectorD* vertices, int vertexCount, TritonAcousticMeshTriangleInformation* triangleInfos, int trianglesCount,
        float spacing);
    bool AddPinnedProbe(ATKVectorD probeLocation);
    const TritonObject& GetHandle() const;
    bool HasNavigationMesh() const
    {
        return m_HasNavigationMesh;
    }
    bool HasGeometryMesh() const
    {
        return m_HasGeometryMesh;
    }

private:
    AcousticMesh() : m_Handle(nullptr), m_HasNavigationMesh(false), m_HasGeometryMesh(false)
    {
    }

    bool Initialize();

private:
    TritonObject m_Handle;
    bool m_HasNavigationMesh;
    bool m_HasGeometryMesh;
};
