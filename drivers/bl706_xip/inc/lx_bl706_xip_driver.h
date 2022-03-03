/**
  ******************************************************************************
  * @file          lx_bl706_xip_driver.h
  * @brief         levelx bl706 xip driver
  * @author        Egahp
  *                2687434412@qq.com
  * @version       1.0
  * @date          2022.02.22
  ******************************************************************************
  * @attention
  * 
  * <h2><center>&copy; Copyright 2021 Egahp.
  * All rights reserved.</center></h2>
  * 
  * @htmlonly 
  * <span style='font-weight: bold'>History</span> 
  * @endhtmlonly
  * 版本|作者|时间|描述
  * ----|----|----|----
  * 1.0|Egahp|2022.02.22|创建文件
  ******************************************************************************
  */

#ifndef __LX_BL706_XIP_DRIVER_H__
#define __LX_BL706_XIP_DRIVER_H__
#ifdef __cplusplus
extern "C" {
#endif
/* include -------------------------------------------------------------------*/
#include "lx_api.h"

extern uint32_t __levelx_start__;
extern uint32_t __levelx_end__;
extern uint32_t __levelx_start_offset__;

#define LX_BL706_XIP_DISK_SIZE                ((uint32_t)( (((uint32_t)(&__levelx_end__) - (uint32_t)(&__levelx_start__))) & 0xFFFFC000))
#define LX_BL706_XIP_DISK_BYTES_PER_BLOCK     ((ULONG)(4096))
#define LX_BL706_XIP_DISK_WORDS_PER_BLOCK     ((ULONG)(LX_BL706_XIP_DISK_BYTES_PER_BLOCK / 4))
#define LX_BL706_XIP_DISK_TOTAL_BLOCK         ((ULONG)(LX_BL706_XIP_DISK_SIZE / LX_BL706_XIP_DISK_BYTES_PER_BLOCK))

#define LX_BL706_XIP_DISK_BASE_ADDRESS        ((uint32_t)(&__levelx_start__) - (uint32_t)(&__levelx_start_offset__))
#define LX_BL706_XIP_DISK_BASE_BLOCK          ((uint32_t)LX_BL706_XIP_DISK_BASE_ADDRESS / LX_BL706_XIP_DISK_BYTES_PER_BLOCK)

UINT lx_bl706_xip_driver_initialize(LX_NOR_FLASH *nor_flash);

#ifdef __cplusplus
}
#endif
#endif
/************************ (C) COPYRIGHT 2021 Egahp *****END OF FILE*************/
