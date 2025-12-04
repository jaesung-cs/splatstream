#ifndef VKGS_COMMON_ALIGN_H
#define VKGS_COMMON_ALIGN_H

namespace vkgs {

int RoundUp(int a, int b) { return (a + b - 1) / b * b; }

}  // namespace vkgs

#endif  // VKGS_COMMON_ALIGN_H
