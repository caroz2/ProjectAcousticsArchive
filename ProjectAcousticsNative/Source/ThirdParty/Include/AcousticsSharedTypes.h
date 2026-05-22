// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//! \file AcousticsSharedTypes.h
//! \brief Common vector types to be used with the acoustics toolkit

#pragma once

//! A simple vector of doubles
#pragma pack(push, 1)
typedef struct ATKVectorD
{
#ifdef __cplusplus
    constexpr ATKVectorD() : x(0.0), y(0.0), z(0.0)
    {
    }
    ATKVectorD(double _x, double _y, double _z) : x(_x), y(_y), z(_z)
    {
    }
#endif
    //! The x-component of the vector
    double x;
    //! The y-component of the vector
    double y;
    //! The z-component of the vector
    double z;
} VectorD;
#pragma pack(pop)

//! A simple vector of floats
#pragma pack(push, 1)
typedef struct ATKVectorF
{
#ifdef __cplusplus
    constexpr ATKVectorF() : x(0.0f), y(0.0f), z(0.0f)
    {
    }
    ATKVectorF(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
    {
    }
#endif
    //! The x-component of the vector
    float x;
    //! The y-component of the vector
    float y;
    //! The z-component of the vector
    float z;
} VectorF;
#pragma pack(pop)

//! A simple vector of ints
#pragma pack(push, 1)
typedef struct ATKVectorI
{
#ifdef __cplusplus
    constexpr ATKVectorI() : x(0), y(0), z(0)
    {
    }
    ATKVectorI(int _x, int _y, int _z) : x(_x), y(_y), z(_z)
    {
    }
#endif
    //! The x-component of the vector
    int x;
    //! The y-component of the vector
    int y;
    //! The z-component of the vector
    int z;
} VectorI;
#pragma pack(pop)

//! A simple vector of unsigned ints
#pragma pack(push, 1)
typedef struct ATKVectorU32
{
#ifdef __cplusplus
    constexpr ATKVectorU32() : x(0), y(0), z(0)
    {
    }
    ATKVectorU32(unsigned int _x, unsigned int _y, unsigned int _z) : x(_x), y(_y), z(_z)
    {
    }
#endif
    //! The x-component of the vector
    unsigned int x;
    //! The y-component of the vector
    unsigned int y;
    //! The z-component of the vector
    unsigned int z;
} VectorU32;
#pragma pack(pop)

//! A matrix of floats, with row-column index
#pragma pack(push, 1)
typedef struct ATKMatrix4x4
{
    //! Component in row 1, column 1
    float m11;
    //! Component in row 1, column 2
    float m12;
    //! Component in row 1, column 3
    float m13;
    //! Component in row 1, column 4
    float m14;
    //! Component in row 2, column 1
    float m21;
    //! Component in row 2, column 2
    float m22;
    //! Component in row 2, column 3
    float m23;
    //! Component in row 2, column 4
    float m24;
    //! Component in row 3, column 1
    float m31;
    //! Component in row 3, column 2
    float m32;
    //! Component in row 3, column 3
    float m33;
    //! Component in row 3, column 4
    float m34;
    //! Component in row 4, column 1
    float m41;
    //! Component in row 4, column 2
    float m42;
    //! Component in row 4, column 3
    float m43;
    //! Component in row 4, column 4
    float m44;
} ATKMatrix4x4;
#pragma pack(pop)

//! A pointer to an object returned from this API.
//! ObjectHandles are always validated internally before use
typedef const void* ObjectHandle;

//! RAII helper class
#ifdef __cplusplus
#include <utility>

template <typename deleter>
class UniqueObjectHandle final
{
public:
    UniqueObjectHandle()
    {
    }
    //! Constructs a UniqueObjectHandle that owns the provided handle
    //! \param o The object handle for which to own the lifetime
    UniqueObjectHandle(ObjectHandle o) : m_ObjectHandle(o)
    {
    }
    //! Move constructor that takes over ownership from another UniqueObjectHandle
    //! \param o The rvalue reference from which to take ownership
    UniqueObjectHandle(UniqueObjectHandle&& o) : m_ObjectHandle(o.m_ObjectHandle)
    {
        o.m_ObjectHandle = nullptr;
    }
    ~UniqueObjectHandle()
    {
        m_Deleter(m_ObjectHandle);
    }

    //! Transfers ownership of the passed handle to this instance
    //! \return The value of the handle held by this instance
    UniqueObjectHandle& operator=(UniqueObjectHandle&& o)
    {
        m_ObjectHandle = std::move(o.m_ObjectHandle);
        return *this;
    }

    //! Returns the value of the handle held by this instance
    //! \return The value of the handle held by this instance
    inline ObjectHandle Get() const
    {
        return m_ObjectHandle;
    }

    //! Useful for outparam allocation functions.
    //! \return A non-const pointer to the object handle member this object owns
    inline ObjectHandle* operator&()
    {
        return &m_ObjectHandle;
    }

private:
    ObjectHandle m_ObjectHandle;
    deleter m_Deleter;
};
#endif

#ifndef EXPORT_API
#ifdef _MSC_VER
#define EXPORT_API __cdecl
#else
#define EXPORT_API __attribute__((__visibility__("default")))
#endif
#endif
