#ifndef __NEPI_EDGE_SDK_H
#define __NEPI_EDGE_SDK_H

#include "nepi_edge_errors.h"
#include "nepi_edge_lb_consts.h"

NEPI_EDGE_RET_t NEPI_EDGE_SetBotBaseFilePath(const char* path);
const char* NEPI_EDGE_GetBotBaseFilePath(void);

#endif //__NEPI_EDGE_SDK_H
