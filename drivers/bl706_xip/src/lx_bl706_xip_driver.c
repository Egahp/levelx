/**
 ******************************************************************************
 * @file          lx_bl706_xip_driver.c
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

/* include -------------------------------------------------------------------*/
#include "lx_bl706_xip_driver.h"
#include "lx_api.h"

#include "string.h"
#include "bflb_platform.h"
#include "hal_flash.h"
#include "bl702_sflash.h"
#include "bl702_sf_ctrl.h"
#include "bl702_sf_cfg.h"
#include "bl702_sf_cfg_ext.h"
#include "bl702_xip_sflash.h"
#include "bl702_xip_sflash_ext.h"
/* marco ---------------------------------------------------------------------*/
#define LX_BL706_WRITE_DEBUG_DISABLE
#define LX_BL706_READ_DEBUG_DISABLE
#define LX_BL706_ERASE_DEBUG_DISABLE

// #ifndef LX_DIRECT_READ
static uint8_t xip_sector_memory[LX_BL706_XIP_DISK_BYTES_PER_BLOCK/4];
// #endif

/* typedef -------------------------------------------------------------------*/

/* declare -------------------------------------------------------------------*/
static UINT lx_bl706_xip_driver_read(ULONG *flash_address, ULONG *destination, ULONG words);
static UINT lx_bl706_xip_driver_write(ULONG *flash_address, ULONG *source, ULONG words);
static UINT lx_bl706_xip_driver_block_erase(ULONG block, ULONG erase_count);
static UINT lx_bl706_xip_driver_block_erased_verify(ULONG block);
static UINT lx_bl706_xip_driver_system_error(UINT error_code);

/* variable ------------------------------------------------------------------*/


/* code ----------------------------------------------------------------------*/

/**
 *   @brief         初始化  
 *   @param  nor_flash              要初始化的nor flash
 *   @return UINT 
 */
UINT lx_bl706_xip_driver_initialize(LX_NOR_FLASH *nor_flash)
{
    UINT ret = LX_SUCCESS;

    nor_flash->lx_nor_flash_base_address = (ULONG *)LX_BL706_XIP_DISK_BASE_ADDRESS;

    nor_flash->lx_nor_flash_total_blocks = LX_BL706_XIP_DISK_TOTAL_BLOCK;
    nor_flash->lx_nor_flash_words_per_block = LX_BL706_XIP_DISK_WORDS_PER_BLOCK;

    nor_flash->lx_nor_flash_driver_read = lx_bl706_xip_driver_read;
    nor_flash->lx_nor_flash_driver_write = lx_bl706_xip_driver_write;
    nor_flash->lx_nor_flash_driver_block_erase = lx_bl706_xip_driver_block_erase;
    nor_flash->lx_nor_flash_driver_block_erased_verify = lx_bl706_xip_driver_block_erased_verify;

    nor_flash->lx_nor_flash_driver_system_error = lx_bl706_xip_driver_system_error;

#ifndef LX_DIRECT_READ
        nor_flash->lx_nor_flash_sector_buffer = (ULONG *)xip_sector_memory;
#endif

    return ret;
}

/**
 *   @brief         读取
 *   @param  flash_address          要读取的地址
 *   @param  destination            数据缓冲区
 *   @param  words                  要读取的字数
 *   @return UINT 
 */
static UINT lx_bl706_xip_driver_read(ULONG *flash_address, ULONG *destination, ULONG words)
{
    UINT ret = LX_SUCCESS;

#ifndef LX_BL706_READ_DEBUG_DISABLE
    LOG_D("levelx driver read, addr [0x%08x], size [%d]\r\n", (uint32_t)flash_address, (words * 4));
#endif

    /*!< DONE 通过系统总线访问方法因为cache 原因，flash无法通过cache写策略更新脏数据，需要每次清除cache，反而降低速度 */
    // volatile uint32_t *ptr = (uint32_t*)(0x23000000 + ((uint32_t)flash_address) - 0x2000);

    // for (uint32_t i=0;i<words;i++){
    //     *destination++ = *ptr++;
    // }

    if (flash_read((uint32_t)flash_address, (uint8_t*)destination, (words * 4)) !=  SUCCESS)
    {
        ret = LX_ERROR;
    }

    return ret;
}

/**
 *   @brief         写入
 *   @param  flash_address          要写入的地址
 *   @param  source                 要写入的数据
 *   @param  words                  要写入的字数
 *   @return UINT 
 */
static UINT lx_bl706_xip_driver_write(ULONG *flash_address, ULONG *source, ULONG words)
{
    UINT ret = LX_SUCCESS;

#ifndef LX_BL706_WRITE_DEBUG_DISABLE
    LOG_D("levelx driver write, addr [0x%08x], size [%d]\r\n", (uint32_t)flash_address, (words * 4));
#endif

    if (flash_write((uint32_t)flash_address, (uint8_t*)source, (words * 4)) !=  SUCCESS)
    {
        ret = LX_ERROR;
    }

    return ret;
}

/**
 *   @brief         擦除指定块
 *   @param  block                  要擦除的块
 *   @param  erase_count            这个块被擦除的次数
 *   @return UINT 
 */
static UINT lx_bl706_xip_driver_block_erase(ULONG block, ULONG erase_count)
{
    UINT ret = LX_SUCCESS;

#ifndef LX_BL706_ERASE_DEBUG_DISABLE
    LOG_D("levelx driver erase, block [%d], erase count [%d]\r\n", block, erase_count);
#endif

    if (flash_erase(LX_BL706_XIP_DISK_BASE_ADDRESS + block * LX_BL706_XIP_DISK_BYTES_PER_BLOCK, LX_BL706_XIP_DISK_BYTES_PER_BLOCK) !=  SUCCESS)
    {
        ret = LX_ERROR;
    }

    return ret;
}

/**
 *   @brief         擦除校验
 *   @param  block                  要校验的快
 *   @return UINT 
 */
static UINT lx_bl706_xip_driver_block_erased_verify(ULONG block)
{
    UINT ret = LX_SUCCESS;

#ifndef LX_BL706_ERASE_DEBUG_DISABLE
    LOG_D("levelx driver erase verify, block [%d]\r\n", block);
#endif

    return ret;
}

/**
 *   @brief         错误处理
 *   @param  error_code             
 *   @return UINT 
 */
static UINT  lx_bl706_xip_driver_system_error(UINT error_code)
{
    UINT ret = LX_ERROR;
    LOG_E("levelx xip driver system error [ %d ]\r\n", error_code);
    return ret;
}


/************************ (C) COPYRIGHT 2021 Egahp *****END OF FILE*************/
