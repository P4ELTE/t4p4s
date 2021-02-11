// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Eotvos Lorand University, Budapest, Hungary

#pragma once

#include <stdio.h>

void add_translation(const char* key, const char* value);
const char* translate(const char* key);
void print_translations();
