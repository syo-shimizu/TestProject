/************************************************************************************************/
/*                                                                                              */
/* FILE NAME                                                                    VERSION         */
/*                                                                                              */
/*      dri_flexspi_lut.c                                                       0.00            */
/*                                                                                              */
/* DESCRIPTION:                                                                                 */
/*                                                                                              */
/*      FlexSPIドライバ(LUT設定)ソースファイル                                                  */
/*                                                                                              */
/* HISTORY                                                                                      */
/*                                                                                              */
/*      NAME            DATE        REMARKS                                                     */
/*                                                                                              */
/*      K.Hatano        2021/01/26  Version 0.00                                                */
/*                                  新規作成(お客様提供ソースをベースに修正)                    */
/*                                                                                              */
/************************************************************************************************/

/****************************************************************************/
/*  インクルードファイル                                                    */
/****************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "code_rules_def.h"
#include "dri_flexspi_lut.h"
#include "dri_flexspi_local.h"

/****************************************************************************/
/*  定数・マクロ定義                                                        */
/****************************************************************************/

#define FLEXSPI_LUT_KEY_VAL             (0x5AF05AF0UL)  /* LUTKEYレジスタ設定値 */
#define FLEXSPI_LUT_COMMANDSEQ_SIZE     (4U)            /* LUTシーケンスバッファの数 */

/****************************************************************************/
/* FLASHコマンド(使用するデバイス固有)                                      */
/****************************************************************************/

#define FLASH_CMD_READ_SFDP             (0x5AU)     /* Read Serial Flash Discovery Parameter */

#define FLASH_CMD_IO_FAST_READ          (0xEBU)     /* Quad Input/Output Fast Read */
#define FLASH_CMD_QUAD_OUTPUT           (0x6BU)     /* Quad Output Fast Read */
#define FLASH_4BCMD_QUAD_OUTPUT         (0x6CU)     /* 4-Byte Quad Output Fast Read */
#define FLASH_4BCMD_IO_FAST_READ        (0xECU)     /* 4-Byte Quad Input/Output Fast Read */

#define FLASH_CMD_WRITE_ENABLE          (0x06U)     /* Write Enable */
#define FLASH_CMD_WRITE_DISABLE         (0x04U)     /* Write Disable */

#define FLASH_CMD_RD_STATUS             (0x05U)     /* Read Status Register */
#define FLASH_CMD_RD_FLAG_STAT          (0x70U)     /* Read Flag Status Register */

#define FLASH_CMD_PAGE_PROGRAM          (0x02U)     /* Page Program */
#define FLASH_CMD_QUAD_INP_FST_PG       (0x32U)     /* Quad Input Fast Program */
#define FLASH_4BCMD_QUAD_I_FST_PG       (0x34U)     /* 4-Byte Quad Input Fast Program */

#define FLASH_CMD_4K_ERASE              (0x20U)     /* 4KB Subsector Erase */
#define FLASH_CMD_32K_ERASE             (0x52U)     /* 32KB Subsector Erase */
#define FLASH_4BCMD_4K_ERASE            (0x21U)     /* 4-Byte 4KB Subsector Erase */
#define FLASH_4BCMD_64K_ERASE           (0xDCU)     /* 4-Byte Sector Erase(64KB) */

#define FLASH_CMD_ENTER_QUAD            (0x35U)     /* Enter Quad Input/Output Mode */
#define FLASH_CMD_RESET_QUAD            (0xF5U)     /* Reset Quad Input/Output Mode */

/****************************************************************************/
/*  構造体定義                                                              */
/****************************************************************************/

/****************************************************************************/
/*  ローカルデータ                                                          */
/****************************************************************************/

/****************************************************************************/
/*  ローカル関数宣言                                                        */
/****************************************************************************/

/* LUT設定 */
LOCAL void _FlexSPI_SetLUT(FlexSPI_Type *base, uint32_t *lut, uint8_t size);

/****************************************************************************/
/*  提供関数                                                                */
/****************************************************************************/

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetEnterQuadModeSequence                                                */
/*                                                                                              */
/* DESCRIPTION: LUT設定[Enter Quad Input/Output Mode]                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetEnterQuadModeSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR    << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_1PAD           << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_CMD_ENTER_QUAD    << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_STOP   << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_1PAD           << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               (0                       << FlexSPI_LUT_OPERAND1_SHIFT));

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetResetQuadModeSequence                                                */
/*                                                                                              */
/* DESCRIPTION: LUT設定[Reset Quad Input/Output Mode]                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetResetQuadModeSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR    << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_4PAD           << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_CMD_RESET_QUAD    << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_STOP   << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_4PAD           << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               (0                       << FlexSPI_LUT_OPERAND1_SHIFT));

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetWriteEnableSequence                                                  */
/*                                                                                              */
/* DESCRIPTION: LUT設定[Write Enable]                                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetWriteEnableSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR    << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_1PAD           << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_CMD_WRITE_ENABLE  << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_STOP   << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_1PAD           << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               (0                       << FlexSPI_LUT_OPERAND1_SHIFT));

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetReadStatusSequence                                                   */
/*                                                                                              */
/* DESCRIPTION: LUT設定[Read Status Register]                                                   */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetReadStatusSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR        << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_CMD_RD_STATUS         << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_READ_SDR   << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               (1                           << FlexSPI_LUT_OPERAND1_SHIFT));    /* 1バイト読み込み */

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetQuadIOReadSequence                                                   */
/*                                                                                              */
/* DESCRIPTION: LUT設定[4-Byte Quad Input/Output Fast Read]                                     */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetQuadIOReadSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR        << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_4PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_4BCMD_IO_FAST_READ    << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_RADDR_SDR  << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_4PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               ((4 * 8)                     << FlexSPI_LUT_OPERAND1_SHIFT));    /* アドレスは32bit */

    lut[1] |= ((kFLEXSPI_Command_DUMMY_SDR  << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_4PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (10                          << FlexSPI_LUT_OPERAND0_SHIFT)  |   /* ダミーサイクル */

               (kFLEXSPI_Command_READ_SDR   << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_4PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               (0                           << FlexSPI_LUT_OPERAND1_SHIFT));

    lut[2] |=  ((kFLEXSPI_Command_STOP      << FlexSPI_LUT_OPCODE0_SHIFT)   |
                (kFLEXSPI_4PAD              << FlexSPI_LUT_NUM_PADS0_SHIFT) |
                (0                          << FlexSPI_LUT_OPERAND0_SHIFT));

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetQuadWriteSequence                                                    */
/*                                                                                              */
/* DESCRIPTION: LUT設定[4-Byte Quad Input/Output Fast Read]                                     */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetQuadWriteSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR        << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_4BCMD_QUAD_I_FST_PG   << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_RADDR_SDR  << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               ((4 * 8)                     << FlexSPI_LUT_OPERAND1_SHIFT));    /* アドレスは32bit */

    lut[1] |= ((kFLEXSPI_Command_WRITE_SDR  << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_4PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (0                           << FlexSPI_LUT_OPERAND0_SHIFT));

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetErase4KBSectorSequence                                               */
/*                                                                                              */
/* DESCRIPTION: LUT設定[4-Byte 4KB Subsector Erase]                                             */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetErase4KBSectorSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR        << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_4BCMD_4K_ERASE        << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_RADDR_SDR  << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               ((4 * 8)                     << FlexSPI_LUT_OPERAND1_SHIFT));    /* アドレスは32bit */

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetErase4KBSectorSequence                                               */
/*                                                                                              */
/* DESCRIPTION: LUT設定[4-Byte Sector Erase(64KB)]                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetErase32KBSectorSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR        << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_4BCMD_64K_ERASE       << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_RADDR_SDR  << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               ((4 * 8)                     << FlexSPI_LUT_OPERAND1_SHIFT));    /* アドレスは32bit */

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetReadFlagStatusSequence                                               */
/*                                                                                              */
/* DESCRIPTION: LUT設定[Read Flag Status Register]                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetReadFlagStatusSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= ((kFLEXSPI_Command_SDR        << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_CMD_RD_FLAG_STAT      << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_READ_SDR   << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               (1                           << FlexSPI_LUT_OPERAND1_SHIFT));    /* １バイト読み込み */

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_SetQuadOutFastRdSequence                                                */
/*                                                                                              */
/* DESCRIPTION: LUT設定[4-Byte Quad Output Fast Read]                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
void FlexSPI_SetQuadOutFastRdSequence(FlexSPI_Type *base)
{
uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};
int i = 0;

    lut[0] |= ((kFLEXSPI_Command_SDR        << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (FLASH_4BCMD_QUAD_OUTPUT     << FlexSPI_LUT_OPERAND0_SHIFT)  |

               (kFLEXSPI_Command_RADDR_SDR  << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_1PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               ((4 * 8)                     << FlexSPI_LUT_OPERAND1_SHIFT));    /* アドレスは32bit */

    lut[1] |= ((kFLEXSPI_Command_DUMMY_SDR  << FlexSPI_LUT_OPCODE0_SHIFT)   |
               (kFLEXSPI_4PAD               << FlexSPI_LUT_NUM_PADS0_SHIFT) |
               (10                          << FlexSPI_LUT_OPERAND0_SHIFT)  |   /* ダミークロック */

               (kFLEXSPI_Command_READ_SDR   << FlexSPI_LUT_OPCODE1_SHIFT)   |
               (kFLEXSPI_4PAD               << FlexSPI_LUT_NUM_PADS1_SHIFT) |
               (0                           << FlexSPI_LUT_OPERAND1_SHIFT));

    /* LUTアンロック */
    base->LUTKEY = FLEXSPI_LUT_KEY_VAL;
    base->LUTCR  = FlexSPI_LUTCR_LOCK(0) | FlexSPI_LUTCR_UNLOCK(1);

    /* 設定しているLUTテーブルが他と異なることに注意 */
    for (i = 0; i < FLEXSPI_LUT_COMMANDSEQ_SIZE; i++) {
        base->LUT[4+i] = lut[i];
    }
    /* LUTロック */
    base->LUTKEY = FLEXSPI_LUT_KEY_VAL;
    base->LUTCR  = FlexSPI_LUTCR_LOCK(1) | FlexSPI_LUTCR_UNLOCK(0);
}

/****************************************************************************/
/*  ローカル関数                                                            */
/****************************************************************************/
/************************************************************************************************/
/* FUNCTION   : _FlexSPI_setLUT                                                                 */
/*                                                                                              */
/* DESCRIPTION: LUT設定                                                                         */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーベースアドレス             */
/*            : lut                             LUTテーブル                                     */
/*            : size                            設定数                                          */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void _FlexSPI_SetLUT(FlexSPI_Type *base, uint32_t *lut, uint8_t size)
{
int i = 0;

    /* LUTアンロック */
    base->LUTKEY = FLEXSPI_LUT_KEY_VAL;
    base->LUTCR  = FlexSPI_LUTCR_LOCK(0) | FlexSPI_LUTCR_UNLOCK(1);

    /* LUT設定 */
    for (i = 0; i < size; i++) {
        base->LUT[i] = lut[i];
    }

    /* LUTロック */
    base->LUTKEY = FLEXSPI_LUT_KEY_VAL;
    base->LUTCR  = FlexSPI_LUTCR_LOCK(1) | FlexSPI_LUTCR_UNLOCK(0);

#if 0
    PRINT("LUT: ");
    for (int i = 0; i < size; i++)
    {
        PRINT2("0x%x ", base->LUT[i]);
    }
    PRINT("\r\n");
#endif
}

#if 0
/*******************************************************************
 * @fn FlexSPI_SetWriteDisableSequence
 * @brief LUT登録  WRITE DISABLE
 * @param FlexSPI_Type 対象のFlexSPIレジスタ
 * @note 
 *
 ******************************************************************/
void FlexSPI_SetWriteDisableSequence(FlexSPI_Type *base)
{
    uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= kFLEXSPI_Command_SDR      << FlexSPI_LUT_OPCODE0_SHIFT |
              kFLEXSPI_1PAD             << FlexSPI_LUT_NUM_PADS0_SHIFT |
              FLASH_CMD_WRITE_DISABLE   << FlexSPI_LUT_OPERAND0_SHIFT |
              kFLEXSPI_Command_STOP     << FlexSPI_LUT_OPCODE1_SHIFT |
              kFLEXSPI_1PAD             << FlexSPI_LUT_NUM_PADS1_SHIFT |
              0                         << FlexSPI_LUT_OPERAND1_SHIFT;

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/*******************************************************************
 * @fn FlexSPI_SetReadSFDPSequence
 * @brief LUT登録  READ SERIAL FLASH DISCOVERY PARAMETER
 * @param FlexSPI_Type 対象のFlexSPIレジスタ
 * @note 
 *
 ******************************************************************/
void FlexSPI_SetReadSFDPSequence(FlexSPI_Type *base, flexspi_pad_t pad)
{
    uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= kFLEXSPI_Command_SDR          << FlexSPI_LUT_OPCODE0_SHIFT |
              pad                           << FlexSPI_LUT_NUM_PADS0_SHIFT |
              FLASH_CMD_READ_SFDP           << FlexSPI_LUT_OPERAND0_SHIFT |
              kFLEXSPI_Command_RADDR_SDR    << FlexSPI_LUT_OPCODE1_SHIFT |
              pad                           << FlexSPI_LUT_NUM_PADS1_SHIFT |
              (4 * 8)                       << FlexSPI_LUT_OPERAND1_SHIFT; // アドレスは4バイト

    lut[1] |= kFLEXSPI_Command_DUMMY_SDR    << FlexSPI_LUT_OPCODE0_SHIFT |
              pad                           << FlexSPI_LUT_NUM_PADS0_SHIFT |
              8                             << FlexSPI_LUT_OPERAND0_SHIFT | // ダミーサイクル8
              kFLEXSPI_Command_READ_SDR     << FlexSPI_LUT_OPCODE1_SHIFT |
              pad                           << FlexSPI_LUT_NUM_PADS1_SHIFT |
              0                             << FlexSPI_LUT_OPERAND1_SHIFT;

    lut[2] |= kFLEXSPI_Command_STOP         << FlexSPI_LUT_OPCODE0_SHIFT |
              pad                           << FlexSPI_LUT_NUM_PADS0_SHIFT |
              0                             << FlexSPI_LUT_OPERAND0_SHIFT;

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}

/*******************************************************************
 * @fn FlexSPI_SetQuadIOWriteSequence
 * @brief LUT登録  FLASH_CMD_PAGE_PROGRAM
 * @param FlexSPI_Type 対象のFlexSPIレジスタ
 * @note 
 *
 ******************************************************************/
void FlexSPI_SetQuadIOWriteSequence(FlexSPI_Type *base)
{
    uint32_t lut[FLEXSPI_LUT_COMMANDSEQ_SIZE] = {0};

    lut[0] |= kFLEXSPI_Command_SDR          << FlexSPI_LUT_OPCODE0_SHIFT |
              kFLEXSPI_4PAD                 << FlexSPI_LUT_NUM_PADS0_SHIFT |
              FLASH_CMD_PAGE_PROGRAM        << FlexSPI_LUT_OPERAND0_SHIFT |
              kFLEXSPI_Command_RADDR_SDR    << FlexSPI_LUT_OPCODE1_SHIFT |
              kFLEXSPI_4PAD                 << FlexSPI_LUT_NUM_PADS1_SHIFT |
              (4 * 8)                       << FlexSPI_LUT_OPERAND1_SHIFT; // アドレスは4バイト

    lut[1] |= kFLEXSPI_Command_WRITE_SDR    << FlexSPI_LUT_OPCODE0_SHIFT |
              kFLEXSPI_4PAD                 << FlexSPI_LUT_NUM_PADS0_SHIFT |
              0                             << FlexSPI_LUT_OPERAND0_SHIFT |
              kFLEXSPI_Command_STOP         << FlexSPI_LUT_OPCODE1_SHIFT |
              kFLEXSPI_4PAD                 << FlexSPI_LUT_NUM_PADS1_SHIFT |
              0                             << FlexSPI_LUT_OPERAND1_SHIFT;

    _FlexSPI_SetLUT(base, lut, FLEXSPI_LUT_COMMANDSEQ_SIZE);
}
#endif
