#pragma once


#include <stddef.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "TeslaUtils.h"

typedef size_t uint64_t;

#ifdef _DEBUG
#define DEBUG_ASSERT(x) \
    do                  \
    {                   \
        assert(x);      \
    } while (false)
#else
#define DEBUG_ASSERT(x)
#endif

#if true
#define SAFE_ASSERT(x) \
    do                 \
    {                  \
        assert(x);     \
    } while (false)
#endif

