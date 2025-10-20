#ifndef VKGS_GPU_EXPORT_API_H
#define VKGS_GPU_EXPORT_API_H

// clang-format off
#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef VKGS_GPU_EXPORTS
    #define VKGS_GPU_API __declspec(dllexport)
  #else
    #define VKGS_GPU_API __declspec(dllimport)
  #endif
#else
  #define VKGS_GPU_API
#endif
// clang-format on

#endif  // VKGS_GPU_EXPORT_API_H
