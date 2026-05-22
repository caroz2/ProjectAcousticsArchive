// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsMaterialLibrary.h"

AcousticsMaterialLibrary::~AcousticsMaterialLibrary()
{
    TritonPreprocessor_MaterialLibrary_Destroy(m_Handle);
}

TUniquePtr<AcousticsMaterialLibrary> AcousticsMaterialLibrary::Create(FString filename)
{
    auto instance = TUniquePtr<AcousticsMaterialLibrary>(new AcousticsMaterialLibrary());
    if (!instance->Initialize(filename))
    {
        instance.Reset();
    }

    return instance;
}

TUniquePtr<AcousticsMaterialLibrary> AcousticsMaterialLibrary::Create(TMap<FString, float> dictionary)
{
    auto instance = TUniquePtr<AcousticsMaterialLibrary>(new AcousticsMaterialLibrary());
    if (!instance->Initialize(dictionary))
    {
        instance.Reset();
    }

    return instance;
}

bool AcousticsMaterialLibrary::CacheKnownMaterials()
{
    int materialCount = 0;

    if (!TritonPreprocessor_MaterialLibrary_GetCount(m_Handle, &materialCount))
    {
        return false;
    }

    m_cachedMaterialList.SetNumZeroed(materialCount);
    m_cachedCodeList.SetNumZeroed(materialCount);

    return TritonPreprocessor_MaterialLibrary_GetKnownMaterials(
        m_Handle, m_cachedMaterialList.GetData(), m_cachedCodeList.GetData(), materialCount);
}

const TritonObject& AcousticsMaterialLibrary::GetHandle() const
{
    return m_Handle;
}

bool AcousticsMaterialLibrary::FindMaterialCode(FString name, TritonMaterialCode* code) const
{
    return TritonPreprocessor_MaterialLibrary_GetMaterialCode(m_Handle, TCHAR_TO_ANSI(*name), code);
}

TritonAcousticMaterial AcousticsMaterialLibrary::GetMaterialInfo(TritonMaterialCode code) const
{
    TritonAcousticMaterial materialInfo{"FAIL", 0.0f};
    TritonPreprocessor_MaterialLibrary_GetMaterialInfo(m_Handle, code, &materialInfo);
    return materialInfo;
}

void AcousticsMaterialLibrary::GetKnownMaterials(
    TArray<TritonAcousticMaterial>& materialList, TArray<TritonMaterialCode>& codeList) const
{
    // Copy the arrays
    materialList = m_cachedMaterialList;
    codeList = m_cachedCodeList;
}

bool AcousticsMaterialLibrary::GuessMaterialInfoFromGeneralName(
    FString nameForMatch, TritonAcousticMaterial& material, TritonMaterialCode& code) const
{
    // Check if the name to match is empty so before using the triton processor functions
    // so that it doesn't crash.
    if (nameForMatch.IsEmpty() || !TritonPreprocessor_MaterialLibrary_GuessMaterialCodeFromGeneralName(
                                      m_Handle, TCHAR_TO_ANSI(nameForMatch.GetCharArray().GetData()), &code))
    {
        return false;
    }

    return TritonPreprocessor_MaterialLibrary_GetMaterialInfo(m_Handle, code, &material);
}

bool AcousticsMaterialLibrary::Initialize(FString filename)
{
    if (filename.IsEmpty())
    {
        return false;
    }

    if (!TritonPreprocessor_MaterialLibrary_CreateFromFile(TCHAR_TO_ANSI(*filename), &m_Handle))
    {
        return false;
    }

    // In order to allow for calls to GetKnownMaterials() without calling into native and trying
    // to acquire the data lock, we cache the material data here. It is assumed that it will never change.
    return CacheKnownMaterials();
}

bool AcousticsMaterialLibrary::Initialize(TMap<FString, float> dictionary)
{
    TArray<TritonAcousticMaterial> materials;
    for (const auto& pair : dictionary)
    {
        TritonAcousticMaterial material;
        strncpy_s(material.Name, TRITON_MAX_NAME_LENGTH, TCHAR_TO_ANSI(*(pair.Key)), pair.Key.Len());
        material.Absorptivity = pair.Value;
        materials.Add(material);
    }

    if (!TritonPreprocessor_MaterialLibrary_CreateFromMaterials(materials.GetData(), materials.Num(), &m_Handle))
    {
        return false;
    }

    // In order to allow for calls to GetKnownMaterials() without calling into native and trying
    // to acquire the data lock, we cache the material data here. It is assumed that it will never change.
    return CacheKnownMaterials();
}
