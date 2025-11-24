#ifndef VKGS_VIEWER_EXPORT_API_H
#define VKGS_VIEWER_EXPORT_API_H

// clang-format off
#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(VKGS_VIEWER_STATIC)
    #define VKGS_VIEWER_API
  #elif defined(VKGS_VIEWER_EXPORTS)
    #define VKGS_VIEWER_API __declspec(dllexport)
  #else
    #define VKGS_VIEWER_API __declspec(dllimport)  
  #endif
#else
  #define VKGS_VIEWER_API
#endif
// clang-format on

#endif  // VKGS_VIEWER_EXPORT_API_H
