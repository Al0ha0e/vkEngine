#ifndef INTEROP_MATH_H
#define INTEROP_MATH_H

namespace vke_interop
{
    template <typename T>
    struct Vector2
    {
        T x;
        T y;
    };

    template <typename T>
    struct Vector3
    {
        T x;
        T y;
        T z;
    };

    template <typename T>
    struct Vector4
    {
        T x;
        T y;
        T z;
        T w;
    };

    template <typename T>
    struct Quaternion
    {
        T x;
        T y;
        T z;
        T w;
    };
}

#endif
