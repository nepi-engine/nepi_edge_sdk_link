/*
 * NEPI Dual-Use License
 * Project: nepi_edge_sdk_link
 *
 * This license applies to any user of NEPI Engine software
 *
 * Copyright (C) 2023 Numurus, LLC <https://www.numurus.com>
 * see https://github.com/numurus-nepi/nepi_edge_sdk_link
 *
 * This software is dual-licensed under the terms of either a NEPI software developer license
 * or a NEPI software commercial license.
 *
 * The terms of both the NEPI software developer and commercial licenses
 * can be found at: www.numurus.com/licensing-nepi-engine
 *
 * Redistributions in source code must retain this top-level comment block.
 * Plagiarizing this software to sidestep the license obligations is illegal.
 *
 * Contact Information:
 * ====================
 * - https://www.numurus.com/licensing-nepi-engine
 * - mailto:nepi@numurus.com
 *
 */
#ifndef __NEPI_EDGE_HB_INTERFACE_H
#define __NEPI_EDGE_HB_INTERFACE_H

#include "nepi_edge_errors.h"

NEPI_EDGE_RET_t NEPI_EDGE_HBLinkDataFolder(const char* data_folder_path);
NEPI_EDGE_RET_t NEPI_EDGE_HBUnlinkDataFolder(void);

#endif
