// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

namespace Triton
{
#pragma pack(push, 4)
    template <typename T>
    union Vector3 {
        struct
        {
            T x;
            T y;
            T z;
        };
        T arr[3];

        constexpr Vector3() : x(0), y(0), z(0) {}
        constexpr Vector3(T a, T b, T c) : x(a), y(b), z(c) {}
        
        static constexpr Vector3 Zero()
        {
            return Vector3(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
        }
    };
#pragma pack(pop)

    using Vec3i = Vector3<int>;
    using Vec3f = Vector3<float>;
    using Vec3d = Vector3<double>;
    using Vec3u = Vector3<uint32_t>;

#pragma pack(push, 4)
    template <typename T>
    struct AABox {
        Vector3<T> MinCorner;
        Vector3<T> MaxCorner;
    };
#pragma pack(pop)
} // namespace Triton
