// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
// Add include for non-unity build
#include "CoreMinimal.h"
#include "TritonPreprocessorApi.h"

class AcousticsMaterialLibrary final
{
public:
    ~AcousticsMaterialLibrary();
    static TUniquePtr<AcousticsMaterialLibrary> Create(FString filename);
    static TUniquePtr<AcousticsMaterialLibrary> Create(TMap<FString, float> dictionary);
    const TritonObject& GetHandle() const;
    bool FindMaterialCode(FString name, TritonMaterialCode* code) const;
    TritonAcousticMaterial GetMaterialInfo(TritonMaterialCode code) const;
    void GetKnownMaterials(TArray<TritonAcousticMaterial>& materialList, TArray<TritonMaterialCode>& codeList) const;
    bool GuessMaterialInfoFromGeneralName(
        FString nameForMatch, TritonAcousticMaterial& material, TritonMaterialCode& code) const;

private:
    AcousticsMaterialLibrary() : m_Handle(nullptr)
    {
    }

    bool CacheKnownMaterials();

    bool Initialize(FString filename);
    bool Initialize(TMap<FString, float> dictionary);

private:
    TritonObject m_Handle;
    TArray<TritonAcousticMaterial> m_cachedMaterialList;
    TArray<TritonMaterialCode> m_cachedCodeList;
};
