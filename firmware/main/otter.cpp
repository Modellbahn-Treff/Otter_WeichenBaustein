// Copyright (C) 2026 Modellbahn-Treff for Kids GmbH
// SPDX-License-Identifier: GPL-3.0-or-later

#include "otter.h"

// GPIO pairs for the 8 points (Weichen), one pair per SN754410 half-bridge
// output, per the WeichenBaustein KiCad schematic (U11/U21/U31/U41).
const uint8_t Pin[16] = {4, 5, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
