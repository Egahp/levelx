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

#ifndef LX_DIRECT_READ
static uint32_t xip_sector_memory[LX_BL706_XIP_DISK_BYTES_PER_BLOCK / 4];
#endif

/* typedef -------------------------------------------------------------------*/

/* declare -------------------------------------------------------------------*/
static UINT lx_bl706_xip_driver_read(ULONG *flash_address, ULONG *destination, ULONG words);
static UINT lx_bl706_xip_driver_write(ULONG *flash_address, ULONG *source, ULONG words);
static UINT lx_bl706_xip_driver_block_erase(ULONG block, ULONG erase_count);
static UINT lx_bl706_xip_driver_block_erased_verify(ULONG block);
static UINT lx_bl706_xip_driver_system_error(UINT error_code);

/*!< xip dirver glue */
static BL_Err_Type flash_erase_glue(uint32_t block, uint32_t erase_count);

/* variable ------------------------------------------------------------------*/
static UINT is_initialized = LX_FALSE;

/* code ----------------------------------------------------------------------*/
UINT lx_bl706_xip_driver_initialize(LX_NOR_FLASH *nor_flash)
{
    UINT ret = LX_SUCCESS;

    if (is_initialized == LX_FALSE)
    {
        nor_flash->lx_nor_flash_base_address = (ULONG *)LX_BL706_XIP_DISK_BASE_ADDRESS;

        nor_flash->lx_nor_flash_total_blocks = LX_BL706_XIP_DISK_TOTAL_BLOCK;
        nor_flash->lx_nor_flash_words_per_block = LX_BL706_XIP_DISK_WORDS_PER_BLOCK;

        nor_flash->lx_nor_flash_driver_read = lx_bl706_xip_driver_read;
        nor_flash->lx_nor_flash_driver_write = lx_bl706_xip_driver_write;
        nor_flash->lx_nor_flash_driver_block_erase = lx_bl706_xip_driver_block_erase;
        nor_flash->lx_nor_flash_driver_block_erased_verify = lx_bl706_xip_driver_block_erased_verify;

        nor_flash->lx_nor_flash_driver_system_error = lx_bl706_xip_driver_system_error;

#ifndef LX_DIRECT_READ
        nor_flash->lx_nor_flash_sector_buffer = xip_sector_memory;
#endif
        is_initialized = LX_TRUE;
    }

    return ret;
}

static UINT lx_bl706_xip_driver_read(ULONG *flash_address, ULONG *destination, ULONG words)
{
    UINT ret = LX_SUCCESS;

    if (flash_read((uint32_t)flash_address, (uint8_t)destination, (words * 4)) !=  SUCCESS)
    {
        ret = LX_ERROR;
    }

    return ret;
}

static UINT lx_bl706_xip_driver_write(ULONG *flash_address, ULONG *source, ULONG words)
{
    UINT ret = LX_SUCCESS;

    if (flash_write((uint32_t)flash_address, (uint8_t)source, (words * 4)) !=  SUCCESS)
    {
        ret = LX_ERROR;
    }

    return ret;
}

static UINT lx_bl706_xip_driver_block_erase(ULONG block, ULONG erase_count)
{
    UINT ret = LX_SUCCESS;
    uint32_t start_address = block * LX_BL706_XIP_DISK_BYTES_PER_BLOCK;
    uint32_t erase_byte = erase_count * LX_BL706_XIP_DISK_BYTES_PER_BLOCK;

    if (flash_erase_glue(block, erase_count) !=  SUCCESS)
    {
        ret = LX_ERROR;
    }

    return ret;
}

static UINT lx_bl706_xip_driver_block_erased_verify(ULONG block)
{
    UINT ret = LX_SUCCESS;

    return ret;
}

static UINT  lx_bl706_xip_driver_system_error(UINT error_code)
{
    UINT ret = LX_ERROR;
    LOG_E("levelx xip driver system error [ %d ]\r\n", error_code);
    return ret;
}

/*!< xip dirver glue */

extern SPI_Flash_Cfg_Type g_flash_cfg;

static BL_Err_Type ATTR_TCM_SECTION SFlash_Erase_glue(SPI_Flash_Cfg_Type *flashCfg, uint32_t block, uint32_t erase_count)
{
    BL_Err_Type ret = SUCCESS;
    uint32_t end_block = block + erase_count;

    while (block < end_block)
    {
        erase_count = end_block - block;
        /*!< 支持64k擦除, block 64k对齐且剩余擦除长度大于64K */
        if ((flashCfg->blk64EraseCmd != BFLB_SPIFLASH_CMD_INVALID)
            && ((block & 0xF) == 0)
            && (erase_count >= 16))
        {
            ret = SFlash_Blk64_Erase(flashCfg, block);
            block += 16;
        }

        /*!< 支持32k擦除, block 32k对齐且剩余擦除长度大于32K */
        else if ((flashCfg->blk32EraseCmd != BFLB_SPIFLASH_CMD_INVALID)
            && ((block & 0x7) == 0)
            && (erase_count >= 8))
        {
            ret = SFlash_Blk32_Erase(flashCfg, block);
            block += 8;
        }

        /*!< 擦除一个block */
        else{
            ret = SFlash_Sector_Erase(flashCfg, block);
            block++;
        }

        if (ret != SUCCESS)
        {
            return ERROR;
        }
    }

    return SUCCESS;
}

static BL_Err_Type ATTR_TCM_SECTION XIP_SFlash_Erase_Need_Lock_glue(SPI_Flash_Cfg_Type *pFlashCfg, SF_Ctrl_IO_Type ioMode, uint32_t block, uint32_t erase_count)
{
    BL_Err_Type stat;
    uint32_t offset;

    stat = XIP_SFlash_State_Save(pFlashCfg, &offset);

    if (stat != SUCCESS) {
        SFlash_Set_IDbus_Cfg(pFlashCfg, ioMode, 1, 0, 32);
    } else {
        stat = SFlash_Erase_glue(pFlashCfg, block, erase_count);
        XIP_SFlash_State_Restore(pFlashCfg, ioMode, offset);
    }

    return stat;
}

static BL_Err_Type ATTR_TCM_SECTION flash_erase_glue(uint32_t block, uint32_t erase_count)
{
    BL_Err_Type ret = ERROR;

    cpu_global_irq_disable();
    XIP_SFlash_Opt_Enter();
    ret = XIP_SFlash_Erase_Need_Lock_glue(&g_flash_cfg, g_flash_cfg.ioMode & 0xf, block, erase_count);
    XIP_SFlash_Opt_Exit();
    cpu_global_irq_enable();

    return ret;
}


/************************ (C) COPYRIGHT 2021 Egahp *****END OF FILE*************/
