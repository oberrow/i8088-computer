/*
 * src/extra.h
 *
 * Copyright (c) 2026 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

#define BIT(n) BIT_TYPE(n, l)
#define BIT_TYPE(n, t) (1##t << (n))