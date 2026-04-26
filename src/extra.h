/*
 * src/extra.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifndef __ia16__
#define __far
#endif

#define BIT(n) BIT_TYPE(n, l)
#define BIT_TYPE(n, t) (1##t << (n))