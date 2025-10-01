//
// Created by Alexey Logachev on 08.09.2025.
//

#ifndef HANDMADE_HERO_COMMON_H
#define HANDMADE_HERO_COMMON_H

#include <stdint.h>

#define local_persist static
#define global_variable static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

#define pi32 3.14159265359f

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))


#endif //HANDMADE_HERO_COMMON_H