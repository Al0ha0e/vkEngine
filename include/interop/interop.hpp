#ifndef INTEROP_H
#define INTEROP_H

#if defined(_MSC_VER)
#define VKE_INTEROP_CDECL __cdecl
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__i386__) || defined(_M_IX86)
#define VKE_INTEROP_CDECL __attribute__((cdecl))
#else
#define VKE_INTEROP_CDECL
#endif
#else
#define VKE_INTEROP_CDECL
#endif

#endif
