#ifndef VKGS_CORE_EXPORT_API_H
#define VKGS_CORE_EXPORT_API_H

// clang-format off
#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(VKGS_CORE_STATIC)
    #define VKGS_CORE_API
  #elif defined(VKGS_CORE_EXPORTS)
    #define VKGS_CORE_API __declspec(dllexport)
  #else
    #define VKGS_CORE_API __declspec(dllimport)  
  #endif
#else
  #define VKGS_CORE_API
#endif
// clang-format on

#endif  // VKGS_CORE_EXPORT_API_H
