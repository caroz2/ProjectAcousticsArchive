// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsDebugRender.h"

#if !UE_BUILD_SHIPPING
#include "ProjectAcoustics.h"
#include "MathUtils.h"
#include <DrawDebugHelpers.h>
#include <Classes/Engine/Font.h>
#include <Classes/Engine/Canvas.h>
#include "Engine/Engine.h"
#include "AcousticsShared.h"

using namespace TritonRuntime;

static constexpr float c_MaxDebugDrawDistance = 5000.0f;
static constexpr auto c_ParamBarLen = 180;
static constexpr float c_ArrowLen = 300.0f;
static constexpr float c_ArrowLabelDist = 0.25f;
static constexpr float c_SourceBoxSize = 4.0f;
static constexpr float c_ProbeBoxSize = 10.0f;
static constexpr float c_DynamicOpeningBoxSize = 5.0f;
static constexpr float c_TextScale = 1.0f;
static constexpr float c_DebugVerbosity = 2;

// Draws formatted text next to a 3D location, in screen space.
class FDebugMultiLinePrinter
{
private:
    bool m_BehindCamera;
    FVector2D m_ScreenPos;
    UCanvas* m_Canvas;
    float m_TextScale;
    float m_LineHeight;

public:
    FDebugMultiLinePrinter(UCanvas* Canvas, FVector WorldPos, FVector CameraLoc, FVector CameraLookAt)
        : m_BehindCamera(true), m_Canvas(Canvas), m_TextScale(c_TextScale)
    {
        auto Font = GEngine->GetLargeFont();
        m_LineHeight = m_TextScale * Font->GetMaxCharHeight();

        // don't draw text behind the camera
        if (((WorldPos - CameraLoc) | CameraLookAt) > 0.0f)
        {
            m_BehindCamera = false;
            FVector ScreenLoc = m_Canvas->Project(WorldPos);
            m_ScreenPos = FVector2D(ScreenLoc.X, ScreenLoc.Y);
        }
    }

    FDebugMultiLinePrinter(UCanvas* Canvas, FVector2D ScreenPos)
        : m_BehindCamera(false), m_ScreenPos(ScreenPos), m_Canvas(Canvas), m_TextScale(c_TextScale)
    {
        auto Font = GEngine->GetLargeFont();
        m_LineHeight = m_TextScale * Font->GetMaxCharHeight();
    }

    void DrawText(const FString& Text, FColor Color = FColor::White, bool DropShadow = true)
    {
        if (m_BehindCamera)
        {
            return;
        }

        auto Font = GEngine->GetLargeFont();

        FCanvasTextItem TI(m_ScreenPos, FText::FromString(Text), Font, Color);
        TI.Scale.Set(m_TextScale, m_TextScale);
        if (DropShadow)
        {
            TI.EnableShadow(FLinearColor::Black);
        }

        m_Canvas->DrawItem(TI);
        m_ScreenPos.Y += m_LineHeight;
    }

    float GetLineHeight() const
    {
        return m_LineHeight;
    }

    // The line printer implicitly maintains a "cursor" location whose Y coordinate
    // increases at each DrawText. This call takes the lower left corner of the rectangle
    // to be drawn relative to this cursor location.
    void DrawRect(FVector2D LowerLeftOffset, FVector2D Size, FColor Color = FColor::White)
    {
        if (m_BehindCamera)
        {
            return;
        }

        FVector2D UpperLeft = m_ScreenPos + LowerLeftOffset;
        UpperLeft.Y = UpperLeft.Y - Size.Y;

        FCanvasTileItem TileItem(UpperLeft, Size, Color);
        TileItem.BlendMode = SE_BLEND_Translucent;
        m_Canvas->DrawItem(TileItem);
    }
};

enum class ParamType
{
    Distance,
    Loudness,
    Decay,
    Angle
};

// Visual rendering properties of parameters
struct ParamProps
{
public:
    static float MapDBToAlpha(float dBValue)
    {
        float mappedValue = 255 * MapDBToLinear(1.5f * dBValue);
        return mappedValue;
    }

private:
    FString PrintDelay(float delay)
    {
        float delayms = delay * 1000.0f;
        FString base = FString::Printf(TEXT("%.1f"), delayms);
        return base;
    }

    FString PrintDistance(float distance)
    {
        FString base = FString::Printf(TEXT("%.1f"), distance);
        return base;
    }

    FString PrintAngle(float angle)
    {
        FString base = FString::Printf(TEXT("%d"), static_cast<int>(angle));
        return base;
    }

    // Prints with fixed number of digits before the decimal point, prepending 0's if necessary
    FString PrintDB(float v, int width = 2, bool ShouldPrintPositiveSign = true)
    {
        FString prefixString = TEXT("");
        if (v < 0)
        {
            v = -v;
            prefixString = TEXT("-");
        }
        else if (v > 0 && ShouldPrintPositiveSign)
        {
            prefixString = TEXT("+");
        }

        FString base = FString::Printf(TEXT("%.2f"), v);
        // decimal point and decimal portion take two positions
        int numIntDigits = base.Len() - 2;
        for (int i = 0; i < width - numIntDigits; i++)
        {
            prefixString += TEXT("0");
        }

        return prefixString + base;
    }

    static inline float MapDBToLinear(float dBValue)
    {
        static const float minDb = -60.0f;
        static const float maxDb = 0.0f;

        float val = (dBValue - minDb) / (maxDb - minDb);
        return FMath::Clamp(val, 0.0f, 66.0f / 60.0f);
    }

    static inline float MapDecayTimeToLinear(float decayTime)
    {
        static const float minDecayTime = 0.0f;
        static const float maxDecayTime = 3.0f;

        float val = (decayTime - minDecayTime) / (maxDecayTime - minDecayTime);
        return FMath::Clamp(val, 0.0f, 1.0f);
    }

    static inline float MapDistanceToLinear(float distance)
    {
        static const float minDist = 0.0f;
        static const float maxDist = 100.0f;

        float val = (distance - minDist) / (maxDist - minDist);
        return FMath::Clamp(val, 0.0f, 1.0f);
    }

    static inline float MapAngleToLinear(float angle)
    {
        static const float minAngle = 0.0f;
        static const float maxAngle = 360.0f;

        float val = (angle - minAngle) / (maxAngle - minAngle);
        return FMath::Clamp(val, 0.0f, 1.0f);
    }

    static FColor MapDBToColor(float dBValue)
    {
        float mappedValue = 255 * MapDBToLinear(dBValue);
        return FColor(255, FMath::Min(mappedValue, 255.0f), mappedValue > 255.0f ? 255 : 0);
    }

    static FColor MapDecayTimeToColor(float decayTime)
    {
        float mappedValue = 255 * MapDecayTimeToLinear(decayTime);
        return FColor(255, mappedValue, 0);
    }

    static FColor MapDistanceToColor(float distance)
    {
        float mappedValue = 255 * MapDistanceToLinear(distance);
        return FColor(255, mappedValue, 0);
    }

    static FColor MapAngleToColor(float angle)
    {
        float mappedValue = 255 * MapAngleToLinear(angle);
        return FColor(255, mappedValue, 0);
    }

public:
    ParamType type;
    float value;
    FString label;
    FString dispval;
    FColor textcolor;
    FColor barcolor;
    float barlen;

    ParamProps(ParamType t, float v, FString l) : type(t), value(v), label(l)
    {
        const float F = TritonAcousticParameters::FailureCode;
        textcolor = value == F ? FColor::Red : FColor::White;
        switch (type)
        {
            case ParamType::Distance:
                dispval = value == F ? "Failed" : PrintDistance(value);
                barcolor = MapDistanceToColor(value);
                barlen = MapDistanceToLinear(value);
                break;
            case ParamType::Loudness:
                dispval = value == F ? "Failed" : PrintDB(value);
                barcolor = MapDBToColor(value);
                barlen = MapDBToLinear(value);
                break;
            case ParamType::Decay:
                dispval = value == F ? "Failed" : PrintDB(value, 1, false);
                barcolor = MapDecayTimeToColor(value);
                barlen = MapDecayTimeToLinear(value);
                break;
            case ParamType::Angle:
                dispval = value == F ? "Failed" : PrintAngle(value);
                barcolor = MapAngleToColor(value);
                barlen = MapAngleToLinear(value);
                break;
            default:
                check(false);
        }
    }
};
#endif

FProjectAcousticsDebugRender::FProjectAcousticsDebugRender(FProjectAcousticsModule* owner)
    : m_Acoustics(owner), m_World(nullptr), m_Canvas(nullptr), m_LoadedFilename("")
{
}

#if !UE_BUILD_SHIPPING
bool FProjectAcousticsDebugRender::UpdateSourceAcoustics(
    uint64_t sourceID, FVector sourceLocation, FVector listenerLocation, bool didQuerySucceed,
    const AcousticsObjectParams& objectParams, const TritonRuntime::QueryDebugInfo& queryDebugInfo)
{
    // Event name and drawing flags are set each frame by source
    // using separate call after updating acoustics, based on user choices.
    const FName eventName = FName("");
    const bool shouldDraw = false;

    if (!m_DebugCache.Contains(sourceID))
    {
        m_DebugCache.Add(
            sourceID,
            {eventName,
             sourceID,
             sourceLocation,
             listenerLocation,
             didQuerySucceed,
             objectParams,
             queryDebugInfo,
             shouldDraw});
    }
    else
    {
        // Only update properties that we have updates for
        m_DebugCache[sourceID].SourceID = sourceID;
        m_DebugCache[sourceID].SourceLocation = sourceLocation;
        m_DebugCache[sourceID].ListenerLocation = listenerLocation;
        m_DebugCache[sourceID].DidQuerySucceed = didQuerySucceed;
        m_DebugCache[sourceID].objectParams = objectParams;
        m_DebugCache[sourceID].queryDebugInfo = queryDebugInfo;
    }

    return true;
}

bool FProjectAcousticsDebugRender::UpdateSourceDebugInfo(
    uint64_t sourceID, bool shouldDraw, FName displayName, bool isBeingDestroyed)
{
    // Remove this source's information so we stop rendering it
    if (isBeingDestroyed)
    {
        m_DebugCache.Remove(sourceID);
        return true;
    }

    auto* debugData = m_DebugCache.Find(sourceID);
    if (!debugData)
    {
        return false;
    }

    debugData->ShouldDraw = shouldDraw;
    debugData->DisplayName = displayName;
    return true;
}

bool FProjectAcousticsDebugRender::Render(
    UWorld* world, UCanvas* canvas, const FVector& cameraPos, const FVector& cameraLook, float cameraFOV,
    bool shouldDrawStats, bool shouldDrawVoxels, bool shouldDrawProbes, bool shouldDrawDistances,
    AcousticsDrawParameters shouldDrawSourceParameters)
{
    m_World = world;
    m_Canvas = canvas;
    m_CameraPos = cameraPos;
    m_CameraLook = cameraLook;
    m_CameraFOV = cameraFOV;

    if (!m_World || !m_Canvas)
    {
        return false;
    }

    if (shouldDrawStats)
    {
        DrawStats();
    }

    if (shouldDrawVoxels)
    {
        DrawVoxels();
    }

    if (shouldDrawProbes)
    {
        DrawProbes();
    }

    if (shouldDrawDistances)
    {
        DrawDistances();
    }

    // Per-source flag determines if each source is rendered or not
    DrawSources(shouldDrawSourceParameters);


    return true;
}

void FProjectAcousticsDebugRender::SetLoadedFilename(FString fileName)
{
    m_LoadedFilename = fileName;
}

void FProjectAcousticsDebugRender::DrawStats()
{
    FDebugMultiLinePrinter Panel(m_Canvas, FVector2D(20, 20));
    Panel.DrawRect(FVector2D(-10, -5), FVector2D(500, -105), FColor(0, 0, 0, 128));
    Panel.DrawText(FString::Printf(TEXT("[Acoustics Status]")), FColor::Green);
    // Panel.DrawText(FString::Printf(TEXT("Enabled : [%s]"), m_IsAcousticsEnabled ? TEXT("YES") : TEXT("NO")),
    // FColor::White);

    auto tritonDebug = m_Acoustics->GetTritonDebugInstance();
    if (tritonDebug)
    {
        if (m_Acoustics->IsAceFileLoaded())
        {
            auto probeCount = tritonDebug->GetNumProbes();
            Panel.DrawText(
                FString::Printf(TEXT("Loaded: %s [%d probes]"), *m_LoadedFilename, probeCount), FColor::White);
        }
        else
        {
            Panel.DrawText(FString::Printf(TEXT("Loaded: None")), FColor::Red);
        }
    }

    const auto memoryUsed = m_Acoustics->GetMemoryUsed();
    Panel.DrawText(FString::Printf(TEXT("RAM usage: [%d]MB"), memoryUsed >> 20), FColor::White);
    const auto diskBytesRead = m_Acoustics->GetDiskBytesRead();
    Panel.DrawText(FString::Printf(TEXT("Disk reads: [%d]MB"), diskBytesRead >> 20), FColor::White);
    Panel.DrawText(
        FString::Printf(TEXT("Outdoorness: [%d%%]"), static_cast<int>(m_Acoustics->GetOutdoorness() * 100.0f)),
        FColor::White);
}

// Normal needs to point in an axis-aligned direction. Undefined behavior otherwise.
void FProjectAcousticsDebugRender::DrawDebugAARectangle(
    const UWorld* inWorld, const FVector& faceCenter, const FVector& faceSize, AAFaceDirection dir, const FQuat& faceRotation, const FColor& color)
{
    FVector offset = faceSize * 0.5f;
    FVector dv1, dv2;

    switch (dir)
    {
        case AAFaceDirection::X:
            offset.X = 0;
            dv1 = FVector(0, faceSize.Y, 0);
            dv2 = FVector(0, 0, faceSize.Z);
            break;
        case AAFaceDirection::Y:
            offset.Y = 0;
            dv1 = FVector(faceSize.X, 0, 0);
            dv2 = FVector(0, 0, faceSize.Z);
            break;
        case AAFaceDirection::Z:
            offset.Z = 0;
            dv1 = FVector(faceSize.X, 0, 0);
            dv2 = FVector(0, faceSize.Y, 0);
            break;
        default:
            return;
    }

    // Rotate the corner offsets according to the space rotation
    FVector rotatedOffset = faceRotation.RotateVector(offset);
    FVector rotatedDv1 = faceRotation.RotateVector(dv1);
    FVector rotatedDv2 = faceRotation.RotateVector(dv2);

    FVector minCorner = faceCenter - rotatedOffset;
    FVector corner1 = minCorner + rotatedDv1;
    FVector corner2 = minCorner + rotatedDv1 + rotatedDv2;
    FVector corner3 = minCorner + rotatedDv2;

    DrawDebugLine(inWorld, minCorner, corner1, color);
    DrawDebugLine(inWorld, corner1, corner2, color);
    DrawDebugLine(inWorld, corner2, corner3, color);
    DrawDebugLine(inWorld, corner3, minCorner, color);
}

void FProjectAcousticsDebugRender::DrawVoxels()
{
    if (!m_Acoustics->IsAceFileLoaded())
    {
        return;
    }

    auto tritonDebug = m_Acoustics->GetTritonDebugInstance();
    if (tritonDebug == nullptr)
    {
        return;
    }

    // Convert to Triton coordinates
    auto tritonPlayerPos = m_Acoustics->WorldPositionToTriton(m_CameraPos);
    auto tritonLookDir = m_Acoustics->WorldDirectionToTriton(m_CameraLook);

    // Select region of voxels near listener
    const auto voxelColor = FColor(0, 255, 0, 0);
    // Slightly larger than half-FOV so edge of conical culling region
    // doesn't become visible on screen corners
    const auto frustumHalfAngleDegrees = 0.55f * m_CameraFOV;
    // Range in cm we should see the voxels
    const auto visibleDistance = m_VoxelVisibleDistance;
    const auto regionMinOffset = m_Acoustics->WorldScaleToTriton(FVector(visibleDistance, visibleDistance, visibleDistance / 2));
    const auto regionMaxOffset = m_Acoustics->WorldScaleToTriton(FVector(visibleDistance, visibleDistance, visibleDistance));

    // Voxel box center is slightly lower so we're closer to the ground
    auto regionCenter = tritonPlayerPos - AcousticsUtils::UnrealPositionToTriton(FVector(0, 0, 50.0f));
    auto minCornerIn = AcousticsUtils::ToTritonVectorDouble(regionCenter - regionMinOffset);
    auto maxCornerIn = AcousticsUtils::ToTritonVectorDouble(regionCenter + regionMaxOffset);
    auto voxelSection = tritonDebug->GetVoxelmapSection(minCornerIn, maxCornerIn);
    if (voxelSection == nullptr)
    {
        return;
    }

    const auto minC = voxelSection->GetMinCorner();
    auto minCorner = AcousticsUtils::ToFVector(minC);
    const auto incr = voxelSection->GetCellIncrementVector();
    auto cellIncrement = AcousticsUtils::ToFVector(incr);
    auto halfCellIncrement = cellIncrement * 0.5f;

    // We start from x=y=z=1, not 0, so the extra cellIncrement
    auto startVoxelCenter = minCorner + cellIncrement + halfCellIncrement;
    auto currentVoxelCenter = startVoxelCenter;
    const auto cosHalfFrustumAngle = FMath::Cos(frustumHalfAngleDegrees * PI / 180.0f);
    const auto voxelSizeGame = m_Acoustics->TritonScaleToWorld(cellIncrement).GetAbs();
    const auto numVoxels = voxelSection->GetNumCells();

    for (auto x = 1u; x < numVoxels.x - 1; x++, currentVoxelCenter.X += cellIncrement.X)
    {
        for (auto y = 1u; y < numVoxels.y - 1; y++, currentVoxelCenter.Y += cellIncrement.Y)
        {
            for (auto z = 1u; z < numVoxels.z - 1; z++, currentVoxelCenter.Z += cellIncrement.Z)
            {
                // Poor Man's simple frustum culling with a conical frustum
                auto cameraToVoxel = currentVoxelCenter - tritonPlayerPos;
                cameraToVoxel.Normalize();
                auto viewFrustumDotProd = FVector::DotProduct(cameraToVoxel, tritonLookDir);
                if (viewFrustumDotProd > cosHalfFrustumAngle)
                {
                    if (voxelSection->IsVoxelWall(x, y, z))
                    {
                        // Only consider the 3 faces visible to camera
                        int dx = (cameraToVoxel.X * cellIncrement.X > 0) ? -1 : 1;
                        int dy = (cameraToVoxel.Y * cellIncrement.Y > 0) ? -1 : 1;
                        int dz = (cameraToVoxel.Z * cellIncrement.Z > 0) ? -1 : 1;

                        // For the three front faces, only render if the face is on the
                        // surface -- that is, the voxel across it is air.

                        // front-face in X
                        if (!voxelSection->IsVoxelWall(x + dx, y, z))
                        {
                            auto faceCenter = currentVoxelCenter;
                            faceCenter.X += halfCellIncrement.X * dx;

                            DrawDebugAARectangle(
                                m_World,
                                m_Acoustics->TritonPositionToWorld(faceCenter),
                                voxelSizeGame,
                                AAFaceDirection::X,
                                m_Acoustics->GetSpaceRotation(),
                                voxelColor);
                        }

                        // front-face in Y
                        if (!voxelSection->IsVoxelWall(x, y + dy, z))
                        {
                            auto faceCenter = currentVoxelCenter;
                            faceCenter.Y += halfCellIncrement.Y * dy;
                            DrawDebugAARectangle(
                                m_World,
                                m_Acoustics->TritonPositionToWorld(faceCenter),
                                voxelSizeGame,
                                AAFaceDirection::Y,
                                m_Acoustics->GetSpaceRotation(),
                                voxelColor);
                        }

                        // front-face in Z
                        if (!voxelSection->IsVoxelWall(x, y, z + dz))
                        {
                            auto faceCenter = currentVoxelCenter;
                            faceCenter.Z += halfCellIncrement.Z * dz;
                            DrawDebugAARectangle(
                                m_World,
                                m_Acoustics->TritonPositionToWorld(faceCenter),
                                voxelSizeGame,
                                AAFaceDirection::Z,
                                m_Acoustics->GetSpaceRotation(),
                                voxelColor);
                        }
                    }
                }
            }
            currentVoxelCenter.Z = startVoxelCenter.Z;
        }
        currentVoxelCenter.Y = startVoxelCenter.Y;
        currentVoxelCenter.Z = startVoxelCenter.Z;
    }

    VoxelmapSection::Destroy(voxelSection);
}

void FProjectAcousticsDebugRender::DrawProbes()
{
    if (!m_Acoustics->IsAceFileLoaded())
    {
        return;
    }
    auto tritonDebug = m_Acoustics->GetTritonDebugInstance();
    if (tritonDebug == nullptr)
    {
        return;
    }

    int numProbes = tritonDebug->GetNumProbes();

    for (int i = 0; i < numProbes; i++)
    {
        ProbeMetadata probeMetadata;
        if (tritonDebug->GetProbeMetadata(i, probeMetadata))
        {
            FColor probeColor;
            switch (probeMetadata.State)
            {
                case LoadState::Loaded:
                {
                    probeColor = FColor::Cyan;
                    break;
                }
                case LoadState::NotLoaded:
                {
                    probeColor = FColor(100);
                    break;
                }
                case LoadState::LoadInProgress:
                {
                    probeColor = FColor::Blue;
                    break;
                }
                case LoadState::DoesNotExist:
                {
                    probeColor = FColor::Black;
                    break;
                }
                case LoadState::Invalid:
                case LoadState::LoadFailed:
                default:
                {
                    probeColor = FColor::Red;
                    break;
                }
            }

            DrawDebugSolidBox(
                m_World,
                m_Acoustics->TritonPositionToWorld(AcousticsUtils::ToFVector(probeMetadata.Location)),
                FVector(c_ProbeBoxSize),
                m_Acoustics->GetSpaceRotation(),
                probeColor);

            DrawDebugBox(
                m_World,
                m_Acoustics->TritonPositionToWorld(AcousticsUtils::ToFVector(probeMetadata.Location)),
                FVector(c_ProbeBoxSize),
                m_Acoustics->GetSpaceRotation(),
                probeColor,
                false,
                -1.0f,
                0,
                2.0f);
        }
    }
}

// Coarsely samples a sphere of directions around the listener. For each direction, it uses the distance query
// to compute a distance, and renders a box in that direction at that distance, with a little bit of scaling to
// pull in the box closer than surfaces
void FProjectAcousticsDebugRender::DrawDistances()
{
    const float toRad = PI / 180;
    // azimuth angle increment
    const float dAz = 15;
    // elevation angle increment
    const float dEl = 25;
    // Pull in distances so distance indicator boxes are closer than geometry, and become visible
    const float distScale = 0.75f;

    // Don't go right up to poles of sphere of directions
    const float maxEl = 75;

    const int halfNumElevation = FMath::RoundToInt(maxEl / dEl);

    const int belowHorizon = -halfNumElevation;
    const int aboveHorizon = halfNumElevation;

    const float distMapBoxLength = 10.0f;
    const FColor distMapBoxColor(255, 255, 128, 0);
    FVector distMapBoxSize(distMapBoxLength, distMapBoxLength, distMapBoxLength);

    // Make sure we sample at exactly zero elevation
    for (int elNum = belowHorizon; elNum <= aboveHorizon; elNum++)
    {
        const float elevation = elNum * dEl * toRad;
        const float horiz = FMath::Cos(elevation);
        const float z = FMath::Sin(elevation);

        for (float az = 0; az < 360; az += dAz)
        {
            const float azimuth = az * toRad;

            const float x = horiz * FMath::Cos(azimuth);
            const float y = horiz * FMath::Sin(azimuth);

            FVector lookDirection = FVector(x, y, z);
            float distance = 0;
            m_Acoustics->QueryDistance(lookDirection, distance);

            FVector drawLocation = m_CameraPos + lookDirection * FMath::Max(0.0f, distance * distScale);
            DrawDebugBox(m_World, drawLocation, distMapBoxSize, distMapBoxColor);
        }
    }
}

static FColor MakeRandomColor(uint64_t index)
{
    // Compute a unique pretty color per sound
    auto hue = static_cast<uint8>(index % 255);
    return FLinearColor::MakeFromHSV8(hue, 0, 170).ToFColor(true);
}

void FProjectAcousticsDebugRender::DrawDirection(
    const EmitterDebugInfo& info, const AcousticsObjectParams& params, const FColor& arrowColor)
{
    /// Dry
    // Draw arrow pointing towards dry arrival direction
    FVector dirToDryEmitter =
        m_Acoustics->TritonDirectionToWorld(AcousticsUtils::ToFVector(params.TritonParams.Dry.ArrivalDirection));
    FVector vecStart = info.ListenerLocation - FVector(0, 0, 25);
    FVector dryVecEnd = info.ListenerLocation + dirToDryEmitter * c_ArrowLen;
    DrawDebugDirectionalArrow(m_World, vecStart, dryVecEnd, 50.0f, arrowColor, false, -1.0f, 0, 0.5f);

    // Dry text
    FVector dryLabelPos = (1 - c_ArrowLabelDist) * vecStart + c_ArrowLabelDist * dryVecEnd;
    FDebugMultiLinePrinter p(m_Canvas, dryLabelPos, m_CameraPos, m_CameraLook);
    auto dryText = FString::Printf(TEXT("Dry:%s"), *info.DisplayName.ToString());
    p.DrawText(dryText, arrowColor, true);

    /// Wet
    // Draw cone pointing towards wet arrival direction. Cone fills up circle according to how much spread
    FVector dirToWetEmitter =
        m_Acoustics->TritonDirectionToWorld(AcousticsUtils::ToFVector(params.TritonParams.Wet.ArrivalDirection));
    FVector wetVecEnd = info.ListenerLocation + dirToWetEmitter * c_ArrowLen;
    FVector wetLabelPos = (1 - c_ArrowLabelDist) * vecStart + c_ArrowLabelDist * wetVecEnd;

    // Draw circle
    FVector YAxis, ZAxis;
    FVector DirectionNorm = dirToWetEmitter.GetSafeNormal();
    DirectionNorm.FindBestAxisVectors(YAxis, ZAxis);
    const FMatrix ConeToWorld = FMatrix(DirectionNorm, YAxis, ZAxis, wetVecEnd - FVector(0, 0, 25));
    auto circleRadius = 50.0f;
    DrawDebugCircle(m_World, ConeToWorld, circleRadius, 32, arrowColor, false, -1.f, 0, 0.f, false);

    // Draw cone
    auto spread = params.TritonParams.Wet.AngularSpreadDegrees;
    auto maxConeAngle = FMath::Atan(circleRadius / c_ArrowLen);
    auto spreadConeAngle = FMath::Lerp(0.0f, maxConeAngle, spread / 360.0f);
    DrawDebugCone(
        m_World,
        vecStart,
        dirToWetEmitter,
        c_ArrowLen,
        spreadConeAngle,
        spreadConeAngle,
        8,
        arrowColor,
        false,
        -1.f,
        0,
        0.f);
    
    // Wet text
    FDebugMultiLinePrinter p2(m_Canvas, wetLabelPos, m_CameraPos, m_CameraLook);
    auto wetText = FString::Printf(TEXT("Wet:%s Spread:%f"), *info.DisplayName.ToString(), spread);
    p2.DrawText(wetText, arrowColor, true);

    // Also draw any push vector that might be being used
    if (params.InterpolationConfig.Resolver == TritonRuntime::InterpolationConfig::DisambiguationMode::Push)
    {
        FVector pushVecStart = info.SourceLocation;
        FVector pushVecEnd =
            m_Acoustics->TritonDirectionToWorld(AcousticsUtils::ToFVector(params.InterpolationConfig.PushVector));
        pushVecEnd.Normalize();
        pushVecEnd *= 100;
        pushVecEnd += pushVecStart;
        DrawDebugDirectionalArrow(
            m_World, pushVecStart, pushVecEnd, 10.0f, FColor::Blue, false, -1.0f, SDPG_Foreground, 5.0f);
    }
}

void FProjectAcousticsDebugRender::DrawSources(AcousticsDrawParameters shouldDrawSourceParameters)
{
    if (shouldDrawSourceParameters == AcousticsDrawParameters::HideAllParameters)
    {
        // No drawing any sources. Exit early
        return;
    }

    auto tritonDebug = m_Acoustics->GetTritonDebugInstance();

    for (const auto& pair : m_DebugCache)
    {
        const auto& info = pair.Value;

        // Only skip out on this source iff:
        // the individual Source says don't draw AND the global shouldDraw isn't set to ShowAll
        if (!info.ShouldDraw && shouldDrawSourceParameters != AcousticsDrawParameters::ShowAllParameters)
        {
            continue;
        }

        // Don't draw a source if it is too far. Reduces clutter.
        if (FVector::DistSquared(info.SourceLocation, info.ListenerLocation) >
            (c_MaxDebugDrawDistance * c_MaxDebugDrawDistance))
        {
            continue;
        }

        // Draw probe weights, receiver weights, and any used safety distances
        constexpr int c_MaxReceiverSamples = 8;
        float aggregateWeights[c_MaxReceiverSamples] = {};
        float safetyDist[c_MaxReceiverSamples] = {};
        int validEntries[c_MaxReceiverSamples] = {};
        FVector receiverLocs[c_MaxReceiverSamples] = {};
        float maxAggWeight = 0;

        constexpr int c_MaxProbes = 16;
        constexpr float c_MinWeight = 0.01f;
        TritonRuntime::ProbeInterpVals piv[c_MaxProbes];
        info.queryDebugInfo.GetProbeInterpWeights(piv);
        for (auto i = 0; i < c_MaxProbes; i++)
        {
            // Skip any probes with very little weight
            if (piv[i].weight < c_MinWeight)
            {
                continue;
            }

            // Draw receivers based on their weight
            TritonRuntime::ProbeMetadata probeMd;
            tritonDebug->GetProbeMetadata(piv[i].ProbeIndex, probeMd);
            TritonRuntime::ReceiverInterpolationWeights weights = info.queryDebugInfo.GetReceiverInterpWeightsForProbe(i);
            auto probeLoc = probeMd.SimRegionVoxelBbox.MinCorner;
            
            for (auto j = 0; j < c_MaxReceiverSamples; j++)
            {
                if (weights.weight[j] < c_MinWeight)
                {
                    // Too low weight, skip this sample
                    continue;
                }
                aggregateWeights[j] += weights.weight[j];
                safetyDist[j] += weights.SafetyDist[j];
                validEntries[j] += 1;

                receiverLocs[j] = m_Acoustics->TritonPositionToWorld(
                    AcousticsUtils::ToFVector(weights.ReceiverCoordsInSimRegion[j]) +
                    AcousticsUtils::ToFVector(probeMd.Location));

                if (aggregateWeights[j] > maxAggWeight)
                {
                    maxAggWeight = aggregateWeights[j];
                }
            }

            // Draw the chosen probes based on their weight
            uint8 grayScale = piv[i].weight * 255;
            FColor probeColor(grayScale, grayScale, grayScale);

            DrawDebugSolidBox(
                m_World,
                m_Acoustics->TritonPositionToWorld(AcousticsUtils::ToFVector(probeMd.Location)),
                FVector(c_ProbeBoxSize),
                m_Acoustics->GetSpaceRotation(),
                probeColor);

            DrawDebugBox(
                m_World,
                m_Acoustics->TritonPositionToWorld(AcousticsUtils::ToFVector(probeMd.Location)),
                FVector(c_ProbeBoxSize),
                m_Acoustics->GetSpaceRotation(),
                probeColor,
                false,
                -1.0f,
                0,
                2.0f);
        }

        // Draw the receiver samples
        for (int i = 0; i < c_MaxReceiverSamples; i++)
        {
            if (validEntries[i] == 0)
            {
                // None used, skip drawing
                continue;
            }

            float avgWeight = aggregateWeights[i] / maxAggWeight;

            uint8 grayScale = avgWeight * 255;
            FColor color(grayScale, grayScale, grayScale);

            // Draw a sphere the size of the safety distance
            // Safety distance is currently only populated if the interpolator used it
            // Still want to see the receivers that were picked, so have a min sphere size of 10cm
            float debugSphereSize =
                FMath::Max(10.0f, AcousticsUtils::TritonValToUnreal(safetyDist[i] / validEntries[i]));
            DrawDebugSphere(m_World, receiverLocs[i], debugSphereSize, 16, color);
        }

        const auto& params = info.objectParams;
        auto DD = params.TritonParams.Dry.PathLengthMeters;
        auto DR = params.TritonParams.Dry.LoudnessDb;
        auto ER = params.TritonParams.Wet.LoudnessDb;
        auto DT = params.TritonParams.Wet.DecayTimeSeconds;
        auto WS = params.TritonParams.Wet.AngularSpreadDegrees;

        auto occlusion = -1 * FMath::Min(DR, 0.0f) * params.Design.OcclusionMultiplier;

        FString topStr = info.DidQuerySucceed ? FString::Printf(TEXT("Occlusion: %.1f"), occlusion) : TEXT("FAILED");

        FString callingStr = FString::Printf(
            TEXT("ObjectPos(%.1f,%.1f,%.1f) ListenerPos(%.1f,%.1f,%.1f)"),
            info.SourceLocation.X,
            info.SourceLocation.Y,
            info.SourceLocation.Z,
            info.ListenerLocation.X,
            info.ListenerLocation.Y,
            info.ListenerLocation.Z);

        FString errString = "";
        int warningCount = 0;
        int errorCount = 0;
        {
            const auto& queryInfo = info.queryDebugInfo;

            errorCount = queryInfo.CountMessagesOfType(TritonRuntime::QueryDebugInfo::Error);
            warningCount = queryInfo.CountMessagesOfType(TritonRuntime::QueryDebugInfo::Warning);

            if (errorCount > 0 || warningCount > 0)
            {
                errString += FString::Printf(TEXT(" Errors: %d, Warnings: %d"), errorCount, warningCount);
            }

            int messageCount = 0;
            auto messageList = queryInfo.GetMessageList(messageCount);
            for (int mess = 0; mess < messageCount; mess++)
            {
                const auto& message = messageList[mess];
                if (message.Type == TritonRuntime::QueryDebugInfo::Warning)
                {
                    errString += FString::Printf(TEXT("\nWARN: %s"), message.MessageString);
                }
                else if (message.Type == TritonRuntime::QueryDebugInfo::Error)
                {
                    errString += FString::Printf(TEXT("\nERR: %s"), message.MessageString);
                }
            }
        }

        // Values display goes transluscent as source becomes progressively occluded.
        // Reduces clutter when viewing a large number of sources.
        float occAlpha = ParamProps::MapDBToAlpha(-1 * FMath::Max(occlusion, -ER));
        FColor occColor;
        if (!info.DidQuerySucceed || errorCount > 0)
        {
            occColor = FColor(255, 0, 0, 255);
        }
        else if (warningCount > 0)
        {
            occColor = FColor(255, 128, 0, occAlpha);
        }
        else
        {
            occColor = FColor(255, 255, 0, occAlpha);
        }

        // Draw a box indicating where acoustics was queried from
        {
            const FColor boxColor(255, 0, 0, 0);
            FVector boxSize(c_SourceBoxSize, c_SourceBoxSize, c_SourceBoxSize);
            DrawDebugBox(m_World, info.SourceLocation, FVector(c_SourceBoxSize), boxColor);
        }

        auto sourceColor = MakeRandomColor(info.SourceID);
        sourceColor.A = occAlpha;

        bool didConsiderOpenings = false, didGoThroughAnyOpening = false;
        // Draw dynamic opening information
        {
            auto* openingInfo = info.queryDebugInfo.GetDynamicOpeningDebugInfo();
            didConsiderOpenings = openingInfo != nullptr;
            if (didConsiderOpenings)
            {
                didGoThroughAnyOpening = openingInfo->DidGoThroughOpening;
                if (didGoThroughAnyOpening)
                {
                    if (tritonDebug)
                    {
                        auto* opening = reinterpret_cast<AActor*>(openingInfo->OpeningID);
                        auto openingCenter =
                            m_Acoustics->TritonPositionToWorld(AcousticsUtils::ToFVector(openingInfo->Center));

                        // Line between opening's associated probe and center of opening
                        auto boundProbePos = m_Acoustics->TritonPositionToWorld(
                            AcousticsUtils::ToFVector(tritonDebug->GetProbeLocation(openingInfo->BoundProbeID)));
                        DrawDebugLine(m_World, openingCenter, boundProbePos, FColor::White, false, -1.0f, 0, 0.5f);

                        // Box on string tightened point on portal, with text label about source routing through it
                        auto stringTightenedPointOnOpening = m_Acoustics->TritonPositionToWorld(
                            AcousticsUtils::ToFVector(openingInfo->StringTightenedPoint));
                        DrawDebugBox(
                            m_World, stringTightenedPointOnOpening, FVector(c_DynamicOpeningBoxSize), m_Acoustics->GetSpaceRotation(), sourceColor);
                        FDebugMultiLinePrinter p(m_Canvas, stringTightenedPointOnOpening, m_CameraPos, m_CameraLook);
                        p.DrawText(
                            FString::Printf(TEXT("Source: [%s]"), *info.DisplayName.ToString()), sourceColor, true);
                        p.DrawText(FString::Printf(TEXT("Opening: [%s]"), *opening->GetName()), sourceColor, true);
                        if (!openingInfo->DidProcessingSucceed)
                        {
                            auto c = FColor::Red;
                            c.A = occAlpha;
                            p.DrawText("Processing failed.", c, true);
                        }
                        else
                        {
                            p.DrawText(
                                FString::Printf(
                                    TEXT("Distance diff: %d cm"),
                                    static_cast<int>(openingInfo->DistanceDifference * 100.0f)),
                                sourceColor,
                                true);
                        }
                    }
                }
            }
        }

        // Draw directional arrow point to portalled arrival direction
        DrawDirection(info, params, sourceColor);

        // Draw parameter values
        {

            // Overlay print at the source location. This is useful to indicate where the
            // call emanates from and if the source location is inside geometry.
            FDebugMultiLinePrinter printer(m_Canvas, info.SourceLocation, m_CameraPos, m_CameraLook);

            // Compute rendered properties for each parameter
            constexpr int numParams = 5;
            ParamProps paramVals[numParams] = {ParamProps(ParamType::Distance, DD, TEXT("Dist.")),
                                               ParamProps(ParamType::Loudness, DR, TEXT("Dry")),
                                               ParamProps(ParamType::Loudness, ER, TEXT("Wet")),
                                               ParamProps(ParamType::Decay, DT, TEXT("RT60")),
                                               ParamProps(ParamType::Angle, WS, TEXT("WetSpread"))};

            if (c_DebugVerbosity >= 0)
            {
                FString shortErrString = TEXT("");

                if (warningCount > 0)
                {
                    shortErrString += FString::Printf(TEXT(" WARN: %d"), warningCount);
                }

                if (errorCount > 0)
                {
                    shortErrString += FString::Printf(TEXT(" ERR: %d"), errorCount);
                }

                printer.DrawText(info.DisplayName.ToString() + " | " + topStr + shortErrString, occColor, false);

                const auto rectHeight = printer.GetLineHeight();

                // draw limiter line
                printer.DrawRect(
                    FVector2D(c_ParamBarLen + 1, 0),
                    FVector2D(1, -numParams * rectHeight),
                    FColor(255, 255, 255, occAlpha));

                for (int i = 0; i < numParams; i++)
                {
                    const float Width = 1 + c_ParamBarLen * paramVals[i].barlen;
                    FColor BarColor = paramVals[i].barcolor;
                    BarColor.A = occAlpha;
                    printer.DrawRect(FVector2D(0, 0), FVector2D(Width, -rectHeight), BarColor);
                    FColor TextColor = paramVals[i].textcolor;
                    TextColor.A = occAlpha;
                    printer.DrawText(paramVals[i].label + TEXT(": ") + paramVals[i].dispval, TextColor);
                }

                if (didConsiderOpenings && !didGoThroughAnyOpening)
                {
                    printer.DrawText(TEXT("[no dynamic opening]"));
                }
            }

            if (c_DebugVerbosity >= 1)
            {
                printer.DrawText(callingStr, occColor, false);
            }

            if (c_DebugVerbosity >= 2)
            {
                printer.DrawText(errString, occColor, false);
            }
        }
    }
}
#endif // UE_BUILD_SHIPPING