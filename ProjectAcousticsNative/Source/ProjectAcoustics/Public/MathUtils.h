// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "TritonVector.h"
#include "Runtime/Launch/Resources/Version.h"

namespace AcousticsUtils
{
    // Vector type conversion
    template <typename T>
    static inline Triton::Vec3f ToTritonVector(const T& t)
    {
        return Triton::Vec3f(static_cast<float>(t.X), static_cast<float>(t.Y), static_cast<float>(t.Z));
    }

    template <typename T>
    static inline Triton::Vec3d ToTritonVectorDouble(const T& t)
    {
        return Triton::Vec3d(t.X, t.Y, t.Z);
    }

    template <typename T>
    static inline FVector ToFVector(const T& t)
    {
#if ENGINE_MAJOR_VERSION == 5
        // FVector is double in UE5
        return FVector{t.x, t.y, t.z};
#else
        // FVector is float in UE4
        return FVector{static_cast<float>(t.x), static_cast<float>(t.y), static_cast<float>(t.z)};
#endif
    }

    // Scale conversion
    static inline float DbToAmplitude(float decibels)
    {
        return pow(10.0f, decibels / 20.0f);
    }

    static inline float AmplitudeToDb(float amplitude)
    {
        // protect against 0 amplitude which throws exception - clamp at -200dB
        return 10 * log10(amplitude * amplitude + 1e-20f);
    }

    // Conversion routines:
    //    Unreal's engine is left-handed Z-up, centimeters
    //    Unreal's FBX import & export is left-handed Z-up, centimeters
    //    Triton is right-handed Z-up, meters
    //    Therefore, conversion between Triton and Unreal's imported FBX coordinates is simply:
    //        Negate Y-axis and scale
    static constexpr float c_UnrealToTritonScale = 0.01f;
    static constexpr float c_TritonToUnrealScale = 1.0f / c_UnrealToTritonScale;

    // For converting a position in Triton's coordinate system to a position in Unreal's coordinate system, including
    // scale (negate Y and convert from m to cm)
    template <typename T>
    static inline T TritonPositionToUnreal(const T& vec)
    {
        return T{vec.X * c_TritonToUnrealScale, -vec.Y * c_TritonToUnrealScale, vec.Z * c_TritonToUnrealScale};
    }

    // For converting a position in Unreal's coordinate system to a position in Triton's coordinate system, including
    // scale (negate Y and convert from cm to m)
    template <typename T>
    static inline T UnrealPositionToTriton(const T& vec)
    {
        return T{vec.X * c_UnrealToTritonScale, -vec.Y * c_UnrealToTritonScale, vec.Z * c_UnrealToTritonScale};
    }

    // For converting from Triton's scale to Unreal's scale (m to cm)
    template <typename T>
    static inline T TritonValToUnreal(const T val)
    {
        return val * c_TritonToUnrealScale;
    }

    // For converting from Unreal's scale to Triton's scale (cm to m)
    template <typename T>
    static inline T UnrealValToTriton(const T val)
    {
        return val * c_UnrealToTritonScale;
    }

    // Direction transformations. Need to preserve length of vector and assume a unit vector as input.
    // If your game has an additional transform during gameplay with respect to the transform during bake,
    // modify these functions based on ONLY the rotational component of the transform.

    // For converting a direction in Unreal's coordinate system to a position in Triton's coordinate system, NOT
    // including scale (negative Y)
    static inline FVector UnrealDirectionToTriton(const FVector& vec)
    {
        return FVector{vec.X, -vec.Y, vec.Z};
    }

    // For converting a direction in Triton's coordinate system to a position in Unreal's coordinate system, NOT
    // including scale (negative Y)
    static inline FVector TritonDirectionToUnreal(const FVector& vec)
    {
        return FVector{vec.X, -vec.Y, vec.Z};
    }

    // For converting a direction in Triton's coordinate system to a position in HRTF (Windows's) coordinate system, NOT
    // including scale
    template <typename T>
    static inline T TritonDirectionToHrtfEngine(const T& Input)
    {
        return {Input.X, Input.Z, -Input.Y};
    }

    // For converting a direction in HRTF (Window's) coordinate system to a position in Triton's coordinate system, NOT
    // including scale
    template <typename T>
    static inline T HrtfEngineDirectionToTriton(const T& Input)
    {
        return {Input.X, -Input.Z, Input.Y};
    }

    // For converting a direction in Unreal's coordinate system to a position in HRTF (Windows's) coordinate system, NOT
    // including scale
    template <typename T>
    static inline T UnrealDirectionToHrtfEngine(const T& Input)
    {
        return {Input.X, Input.Z, Input.Y};
    }

    // For converting a direction in HRTF (Window's) coordinate system to a position in Unreal's coordinate system, NOT
    // including scale
    template <typename T>
    static inline T HrtfEngineDirectionToUnreal(const T& Input)
    {
        return {Input.X, Input.Z, Input.Y};
    }
} // namespace ProjectAcousticsUtils