/*
 * Copyright (c) 2024 Numurus, LLC <https://www.numurus.com>.
 *
 * This file is part of nepi-engine
 * (see https://github.com/nepi-engine).
 *
 * License: 3-clause BSD, see https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __NEPI_EDGE_HB_INTERFACE_H
#define __NEPI_EDGE_HB_INTERFACE_H

#include "nepi_edge_errors.h"

NEPI_EDGE_RET_t NEPI_EDGE_HBLinkDataFolder(const char* data_folder_path);
NEPI_EDGE_RET_t NEPI_EDGE_HBUnlinkDataFolder(void);

#endif
