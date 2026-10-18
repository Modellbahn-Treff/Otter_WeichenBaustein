// Copyright (C) 2026 Modellbahn-Treff for Kids GmbH
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <stdint.h>
#include <stdbool.h>

extern bool    AmSchalten[8];
extern int64_t AusSchaltZeitStempel[8];

void WM_Trigger(uint8_t number, const char *richtung);
void WM_Loop(uint8_t number);
void WM_Start(uint8_t number);
