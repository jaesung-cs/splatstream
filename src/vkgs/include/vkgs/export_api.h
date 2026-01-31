#ifndef VKGS_EXPORT_API_H
#define VKGS_EXPORT_API_H

// clang-format off
#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef VKGS_STATIC
    #define VKGS_API
  #elif defined(VKGS_EXPORTS)
    #define VKGS_API __declspec(dllexport)
  #else
    #define VKGS_API __declspec(dllimport)
  #endif
#else
  #define VKGS_API
#endif
// clang-format on

#endif  // VKGS_EXPORT_API_H
