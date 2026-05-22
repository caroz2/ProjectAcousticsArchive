// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsDynamicOpening.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "Runtime/Launch/Resources/Version.h"
#include "IAcoustics.h"
#include "AcousticsShared.h"
#include "Materials/Material.h"
#include "MathUtils.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Math/ConvexHull2d.h"
#endif
#include "Engine/StaticMesh.h"

UAcousticsDynamicOpening::UAcousticsDynamicOpening(const class FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Component state
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.SetTickFunctionEnable(true);
    bTickInEditor = false;
    bWantsOnUpdateTransform = true;

    // Static mesh
    Mobility = EComponentMobility::Static;
    ForcedLodModel = 0;
    bHiddenInGame = true;
    SetCollisionProfileName("NoCollision");
    SetGenerateOverlapEvents(false);
    CanCharacterStepUpOn = ECB_No;
    // If there is no geometry, assign default plane geometry
    // that is listed under "basic shapes" in editor
    if (GetStaticMesh() == NULL)
    {
        ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
        UStaticMesh* mesh = MeshAsset.Object;
        SetStaticMesh(mesh);
    }
    // Assign default engine brush material -
    // picked because it is transluscent in the editor
    ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
        TEXT("Material'/Engine/EngineMaterials/EditorBrushMaterial.EditorBrushMaterial'"));
    UMaterialInterface* material = MaterialAsset.Object;
    if (material)
    {
        SetMaterial(0, material);
    }

    // Acoustics
    DryAttenuationDb = 0.0f;
    WetAttenuationDb = 0.0f;
    Filtering = 0.0f;
    m_Acoustics = nullptr;
    AActor* owner = GetOwner();
    if (owner && !owner->ActorHasTag(c_AcousticsNavigationTag))
    {
        owner->Tags.Add(c_AcousticsNavigationTag);
    }

    FlattenZ();
}

FName UAcousticsDynamicOpening::Name() const
{
    auto* o = GetOwner();
    return o ? o->GetFName() : this->GetFName();
}

void UAcousticsDynamicOpening::BeginPlay()
{
    Super::BeginPlay();
    if (IAcoustics::IsAvailable())
    {
        // cache module instance
        m_Acoustics = &(IAcoustics::Get());
    }

    // Register this opening with acoustics system
    if (m_Acoustics && Vertices.Num() > 0)
    {
        // Transform component to world coordinate system. In principle, this work could
        // be done in the editor and we could just store the final world space
        // geometry. We defer this computation to here, at runtime because the former
        // did not play well with blueprint usage of this component class.
        // In particular, the Archetype blueprint (accessed by editing BP actor class in editor)
        // would always try and overwrite this component's cached mesh data in each BP
        // instance in the scene, either when editing it or during serialization.
        // The current design seems to sit better with Unreal, which wants each instance
        // of the blueprint to be a copy of the default blueprint object (the Archetype/CDO).
        // In this design, the components contents are identical, differing only the
        // world transform contained in the owning actor, which is applied here.
        // Although not as efficient as it could be, this is still reasonable: the complex
        // convex-hull code is still during edit time, and assuming the opening geometries
        // are quite simple, the transform work here is not that much.
        FVector tritonCenter, tritonNormal;
        TArray<FVector> tritonVerts;
        tritonVerts.Reserve(Vertices.Num());
        const auto& transform = GetComponentTransform();
        for (int32 i = 0; i < Vertices.Num(); i++)
        {
            tritonVerts.Add(m_Acoustics->WorldPositionToTriton(transform.TransformPosition(Vertices[i])));
        }
        tritonCenter = m_Acoustics->WorldPositionToTriton(transform.TransformPosition(Center));
        tritonNormal = m_Acoustics->WorldDirectionToTriton(transform.TransformVectorNoScale(Normal));

        if (!m_Acoustics->AddDynamicOpening(this, tritonCenter, tritonNormal, tritonVerts))
        {
            UE_LOG(
                LogAcousticsRuntime,
                Warning,
                TEXT("Dynamic opening [%s] failed to register with Acoustics. Disabled."),
                *Name().ToString());
        }

        m_Acoustics->UpdateDynamicOpening(this, DryAttenuationDb, WetAttenuationDb);
    }
}

void UAcousticsDynamicOpening::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unregister this opening
    if (m_Acoustics && Vertices.Num() > 0)
    {
        m_Acoustics->RemoveDynamicOpening(this);
    }

    Super::EndPlay(EndPlayReason);
}

// The opening is a 2D polygon - this flattens the shape in Z in its local space
// turning a cube into a plane, cylinder into a disc, etc., which can then be rotated
// in the editor.
void UAcousticsDynamicOpening::FlattenZ()
{
    const float zScale = 0.001f;
    if (GetRelativeScale3D().Z != zScale)
    {
        auto s = GetRelativeScale3D();
        s.Z = zScale;
        this->SetRelativeScale3D(s);
    }
}

// Ensure Z extent of shape always stays flattened so its a 2D polygon
void UAcousticsDynamicOpening::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
    Super::OnUpdateTransform(UpdateTransformFlags, Teleport);
    FlattenZ();
}

void UAcousticsDynamicOpening::TickComponent(
    float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    auto* world = GetWorld();
    if (world && world->IsGameWorld() && m_Acoustics && Vertices.Num() > 0)
    {
        m_Acoustics->UpdateDynamicOpening(this, DryAttenuationDb, WetAttenuationDb);
    }
}

#if WITH_EDITOR
void UAcousticsDynamicOpening::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
    Super::PostEditChangeProperty(e);

    auto world = GetWorld();
    if (!world)
    {
        return;
    }

    if (world->IsEditorWorld())
    {
        if (e.MemberProperty != nullptr)
        {
            if (e.MemberProperty->GetFName() == FName("RelativeScale3D"))
            {
                FlattenZ();
            }
        }
    }
}

// Update opening geometry when user is saving scene
#if ENGINE_MAJOR_VERSION == 5
void UAcousticsDynamicOpening::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
    Super::PreSave(ObjectSaveContext);
#else
void UAcousticsDynamicOpening::PreSave(const class ITargetPlatform* TargetPlatform)
{
    Super::PreSave(TargetPlatform);
#endif
    UpdateOpeningGeometry();
}

bool UAcousticsDynamicOpening::ExtractOpeningGeometry(
    TArray<FVector>& outVertices, FVector& outCenter, FVector& outNormal) const
{
    outVertices.Empty();
    outCenter = FVector::ZeroVector;

    UStaticMesh* mesh = GetStaticMesh();
    if (!mesh)
    {
        UE_LOG(
            LogAcousticsRuntime,
            Error,
            TEXT("Dynamic Opening [%s] doesn't have an assigned static mesh. Ignoring."),
            *Name().ToString());
        return false;
    }

    const auto checkHasVerts = true;
    const auto LOD = 0;
    if (!mesh->HasValidRenderData(checkHasVerts, LOD))
    {
        UE_LOG(
            LogAcousticsRuntime,
            Error,
            TEXT("Dynamic Opening: [%s]. There is no valid render data for LOD %d. Ignoring."),
            *Name().ToString(),
            LOD);
        return false;
    }

    const auto& renderData = mesh->GetLODForExport(LOD);
    const auto& vertexBuffer = renderData.VertexBuffers.PositionVertexBuffer;

    TArray<FVector> vertsOrig;
    {
        const auto vertexCount = vertexBuffer.GetNumVertices();
        for (auto i = 0u; i < vertexCount; ++i)
        {
            // If actor is provided, its actor-to-world transform is used,
            // otherwise vertices are interpreted to be directly in world coordinates
            const auto& vertexPos = vertexBuffer.VertexPosition(i);
            // Flatten out Z extent in local coordinate system.
            // Convex hull is then computed below in 2D XY plane.
            vertsOrig.Add(FVector(vertexPos.X, vertexPos.Y, 0));
        }
    }

    {
        // Our code requires vertices in the returned hull are CW or CCW ordered.
        // Unreal seems to use the Gift Wrapping algorithm, so this condition is met.
        TArray<int32> hullIndices;
        ConvexHull2D::ComputeConvexHull(vertsOrig, hullIndices);

        if (hullIndices.Num() < 3)
        {
            UE_LOG(
                LogAcousticsRuntime,
                Error,
                TEXT("Dynamic Opening: [%s]. Failed to compute convex hull of portal geometry. Returned number of "
                     "verts: %d. Ignoring."),
                *Name().ToString(),
                hullIndices.Num());
            return false;
        }

        for (auto index : hullIndices)
        {
            const auto& v = vertsOrig[index];
            outVertices.Add(v);
            outCenter += v;
        }

        outCenter /= static_cast<float>(outVertices.Num());
    }

    outNormal = FVector(0, 0, 1);
    return true;
}

bool UAcousticsDynamicOpening::UpdateOpeningGeometry()
{
    return ExtractOpeningGeometry(Vertices, Center, Normal);
}

// Update opening geometry when user requests center during Acoustics baking
bool UAcousticsDynamicOpening::ComputeCenter(FVector& outCenter)
{
    if (UpdateOpeningGeometry())
    {
        // Use the static transform method during baking, instead of transform from 
        // the runtime Triton instance (which accounts for dynamic world origin).
        // The latter does not make sense in editor and m_Acoustics will be null when
        // this function is called.
        outCenter = AcousticsUtils::UnrealPositionToTriton(GetComponentTransform().TransformPosition(Center));
        return true;
    }
    else
    {
        return false;
    }
}
#endif
