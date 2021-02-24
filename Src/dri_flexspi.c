/************************************************************************************************/
/*                                                                                              */
/* FILE NAME                                                                    VERSION         */
/*                                                                                              */
/*      dri_flexspi.c                                                           0.00            */
/*                                                                                              */
/* DESCRIPTION:                                                                                 */
/*                                                                                              */
/*      FlexSPIドライバソースファイル                                                           */
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

#include "itron.h"
#include "kernel.h"
#include "ARMv7M.h"
#include "imx8mplus_uC3.h"
#include "code_rules_def.h"
#include "dri_flexspi.h"
#include "dri_flexspi_lut.h"
#include "dri_flexspi_local.h"

/****************************************************************************/
/*  定数・マクロ定義                                                        */
/****************************************************************************/

#define INTNO_QSPI              (INT_FLEXSPI)

#define FREQ_1MHz               (1000000UL)
#define FLEXSPI_DLLCR_DEFAULT   (0x100UL)
#define FLEXSPI_WATERMARK_BITS  (8U)
#define FLEXSPI_AHB_BUFFER_SIZE (0x800U)

#define FLEXSPI_MAX_RETRY       (1000U)             /* 最大リトライ回数 */

/* イベントフラグビット */
#define FLEXSPI_EVFBIT_DONE     (0x00000001U)       /* コマンド実行完了 */
#define FLEXSPI_EVFBIT_RX       (0x00000002U)       /* RX FIFO待ち */
#define FLEXSPI_EVFBIT_TX       (0x00000004U)       /* TX FIFO待ち */
#define FLEXSPI_EVFBIT_WAIT     (0x80000000U)       /* 汎用時間待ち */

/* DLLレジスタ設定値最小・最大 */
enum {
    kFLEXSPI_DelayCellUnitMin = 75,  /* 75ps. */
    kFLEXSPI_DelayCellUnitMax = 225, /* 225ps. */
};

enum {
    kFlexSPI_FlashASampleClockSlaveDelayLocked =
        FlexSPI_STS2_ASLVLOCK_MASK, /* Flash A sample clock slave delay line locked. */
    kFlexSPI_FlashASampleClockRefDelayLocked =
        FlexSPI_STS2_AREFLOCK_MASK, /* Flash A sample clock reference delay line locked. */
    kFlexSPI_FlashBSampleClockSlaveDelayLocked =
        FlexSPI_STS2_BSLVLOCK_MASK, /* Flash B sample clock slave delay line locked. */
    kFlexSPI_FlashBSampleClockRefDelayLocked =
        FlexSPI_STS2_BREFLOCK_MASK, /* Flash B sample clock reference delay line locked. */
};

/****************************************************************************/
/*  構造体定義                                                              */
/****************************************************************************/

/* QSPIドライバ情報 */
typedef struct FlexSPI_DrvInfo_tag {
    FlexSPI_Type    *tpBase;    /* FlexSPIコントローラーレジスタベースアドレス */
    ID              tIsrID;     /* 割り込みサービスルーチンID */
    ID              tFlgID;     /* イベントフラグID */
} FlexSPI_DrvInfo;

/****************************************************************************/
/*  ローカルデータ                                                          */
/****************************************************************************/

/* QSPIドライバデータ */
DLOCAL FlexSPI_DrvInfo  l_tDrvInfo = { 0 };

/****************************************************************************/
/*  ローカル関数宣言                                                        */
/****************************************************************************/

/* ソフトリセット */
LOCAL int _FlexSPI_SoftwareReset(FlexSPI_Type *base);

/* レジスタ設定 */
LOCAL void _FlexSPI_SetConfig(FlexSPI_Type *base);

/* レジスタ設定 */
LOCAL int _FlexSPI_SetCS(FlexSPI_Type *base, int chip_select, const flexspi_device_config_t *config);

/* DLLレジスタ値取得 */
LOCAL int _FlexSPI_GetDLLValue(FlexSPI_Type *base, int chip_select, const flexspi_device_config_t *config);

/* AHB設定 */
LOCAL void _FlexSPI_SetAHBConfig(FlexSPI_Type *base);

/* 割り込みサービスルーチン */
LOCAL void _FlexSPI_ISR(VP_INT exinf);

/****************************************************************************/
/*  提供関数                                                                */
/****************************************************************************/

/* 初期化 */

/************************************************************************************************/
/* FUNCTION   : FlexSPI_Init                                                                    */
/*                                                                                              */
/* DESCRIPTION: 初期化                                                                          */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                                                                            */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : FLEXSPI_E_SUCCESS               正常終了                                        */
/*              FLEXSPI_E_ERROR                 OSリソースの作成時にエラーが発生した            */
/*                                                                                              */
/************************************************************************************************/
int FlexSPI_Init(void)
{
int iRet     = FLEXSPI_E_ERROR;
T_CISR tCISR = { 0 };
T_CFLG tCFLG = { 0 };

    /* FlexSPIコントローラーのレジスタベースアドレス設定 */
    l_tDrvInfo.tpBase = (FlexSPI_Type*)FLEXSPI_BASE;

    /* 割り込みハンドラ設定 */
    tCISR.isratr = TA_HLNG;
    tCISR.intno  = INTNO_QSPI;
    tCISR.isr    = (FP)_FlexSPI_ISR;
    tCISR.exinf  = (VP_INT)&l_tDrvInfo;
    tCISR.imask  = 192U;            /* 0xC0 */
    l_tDrvInfo.tIsrID = acre_isr(&tCISR);
    if (l_tDrvInfo.tIsrID < E_OK) {
        iRet = FLEXSPI_E_ERROR;    /* ISR作成エラー */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* イベントフラグ作成 */
    tCFLG.flgatr  = (TA_TFIFO | TA_WMUL);   /* 属性 */
    tCFLG.iflgptn = 0;                      /* 初期値 */
    tCFLG.name    = "Dri_FlexSPI_Event";    /* 名称 */
    l_tDrvInfo.tFlgID = acre_flg(&tCFLG);
    if (l_tDrvInfo.tFlgID < E_OK) {
        iRet = FLEXSPI_E_ERROR;     /* イベントフラグ作成エラー */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* モジュールディスエーブル */
    l_tDrvInfo.tpBase->MCR0 |= FlexSPI_MCR0_MDIS_MASK;

    iRet = FLEXSPI_E_SUCCESS;

err_end:
    return iRet;
}

/* オープン・クローズ */

/************************************************************************************************/
/* FUNCTION   : FlexSPI_Open                                                                    */
/*                                                                                              */
/* DESCRIPTION: オープン                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*            : chip_select                     論理QSPIデバイス番号(0 only)                    */
/*            : config                          デバイスコンフィギュレーション情報              */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : FLEXSPI_E_SUCCESS               正常終了                                        */
/*              FLEXSPI_E_ERROR                 OSリソースの作成時にエラーが発生した            */
/*              FLEXSPI_E_PARAM                 パラメータに誤りがある                          */
/*                                                                                              */
/************************************************************************************************/
int FlexSPI_Open(FlexSPI_Type *base, int chip_select, const flexspi_device_config_t *config)
{
int iRet = FLEXSPI_E_ERROR;

    /* パラメータチェック */
    if ((base        == NULL) ||    /* レジスタベースアドレス未設定 */
        (chip_select != 0)    ||    /* 論理デバイス番号がゼロでない */
        (config      == NULL)) {    /* デバイスコンフィギュレーション情報未設定 */
        iRet = FLEXSPI_E_PARAM;     
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* FlexSPIコントローラーレジスタ設定 */

    /* 1)ソフトウェアリセット */
    base->MCR0 &= ~FlexSPI_MCR0_MDIS_MASK;  /* モジュールイネーブル */
    iRet = _FlexSPI_SoftwareReset(base);    /* ソフトウェアリセット */
    if (iRet != FLEXSPI_E_SUCCESS) {
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* レジスタ設定(MCR0 - 2) */
    _FlexSPI_SetConfig(base);

    /* コンフィギュレーション指定 */
    _FlexSPI_SetCS(base, chip_select, config);

    /* AHB制御レジスタの構成 */
    _FlexSPI_SetAHBConfig(base);

    /* 19)MCR0(モジュールイネーブル) */
    base->MCR0 &= ~FlexSPI_MCR0_MDIS_MASK;

    iRet = FLEXSPI_E_SUCCESS;

err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_Close                                                                   */
/*                                                                                              */
/* DESCRIPTION: クローズ                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : FLEXSPI_E_SUCCESS               正常終了                                        */
/*              FLEXSPI_E_ERROR                 FlexSPIコントローラーへのアクセス時にエラー     */
/*              FLEXSPI_E_PARAM                 パラメータに誤りがある                          */
/*                                                                                              */
/************************************************************************************************/
int FlexSPI_Close(FlexSPI_Type *base)
{
int iRet = FLEXSPI_E_ERROR;

    /* パラメータチェック */
    if (base == NULL) {
        return FLEXSPI_E_PARAM;     /* パラメータエラー */
    }
    else {
        ;   /* do nothing */
    }

    /* FlexSPIコントローラーレジスタ設定 */

    /* ソフトウェアリセット */
    iRet = _FlexSPI_SoftwareReset(base);

    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_ReadRxFifo                                                              */
/*                                                                                              */
/* DESCRIPTION: RX FIFOより読み出し                                                             */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*            : size                            読み出し長                                      */
/*                                                                                              */
/* OUTPUT     : buf                             データ格納バッファ                              */
/*                                                                                              */
/* RESULTS    : FLEXSPI_E_SUCCESS               正常終了                                        */
/*              FLEXSPI_E_ERROR                 FlexSPIコントローラーへのアクセス時にエラー     */
/*              FLEXSPI_E_PARAM                 パラメータに誤りがある                          */
/*                                                                                              */
/************************************************************************************************/
int FlexSPI_ReadRxFifo(FlexSPI_Type *base, unsigned char *buf, uint32_t size)
{
uint32_t ulReadByte = 0;
uint32_t ulReadData = 0;
uint32_t ulRetry    = 0;
FLGPTN tFlgPtn      = 0;

    /* パラメータチェック */
    if ((base == NULL) ||           /* レジスタベースアドレス未設定 */ 
        (buf  == NULL) ||           /* 読み出しバッファ未設定 */
        (size == 0)) {              /* 読み出し長がゼロ */
        return FLEXSPI_E_PARAM;     /* パラメータエラー */
    }
    else {
        ;   /* do nothing */
    }

    /* FlexSPIコントローラーレジスタアクセス */

    /* 指定されたサイズまでデータ取得 */
    while (size--) {
        if ((ulReadByte % 4) == 0) {
            /* RX FIFOデータ待ち */
            for (ulRetry = 0; ulRetry < FLEXSPI_MAX_RETRY; ulRetry++) {
                if ((base->INTR & FlexSPI_INTR_IPRXWA_MASK) != 0U) {
                    break;
                }
                else {
                    twai_flg(l_tDrvInfo.tFlgID, FLEXSPI_EVFBIT_RX, TWF_ORW, &tFlgPtn, 1U);
                }
            }
            if (ulRetry == FLEXSPI_MAX_RETRY) {
                return FLEXSPI_E_ERROR; /* リトライアウト */
            }
            else {
                ;   /* do nothing */
            }

            /* RX FIFOよりデータ取得 */
            ulReadData = base->RFDR[ulReadByte / 4];
        }
        else {
            ;   /* do nothing */
        }

        /* データを格納 */
        *buf = ulReadData & 0xFF;

        ulReadData >>= 8;
        buf++;
        ulReadByte++;
    }

    /* 2)INTR(RX FIFOデータクリア) */
    base->INTR |= FlexSPI_INTR_IPRXWA(1);

    /* 3)IPRXFCR(RX FIFOクリア) */
    base->IPRXFCR |= FlexSPI_IPRXFCR_CLRIPRXF(1);

    /* 4)INTR(RX FIFOデータクリア) */
    base->INTR |= FlexSPI_INTR_IPRXWA(1);

    return FLEXSPI_E_SUCCESS;
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_WriteTxFifo                                                             */
/*                                                                                              */
/* DESCRIPTION: TX FIFO書き込み＆コマンド実行                                                   */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*            : txbuf                           送信データ格納バッファ                          */
/*            : address                         SPI転送デバイスアドレス                         */
/*            : byteLength                      書き込み長                                      */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : FLEXSPI_E_SUCCESS               正常終了                                        */
/*              FLEXSPI_E_ERROR                 FlexSPIコントローラーへのアクセス時にエラー     */
/*              FLEXSPI_E_PARAM                 パラメータに誤りがある                          */
/*                                                                                              */
/************************************************************************************************/
int FlexSPI_WriteTxFifo(FlexSPI_Type *base, unsigned char *txbuf, unsigned int address, uint32_t byteLength)
{
uint32_t ulCount  = 0;
uint32_t ulRemain = 0;
uint32_t ulRetry  = 0;
FLGPTN tFlgPtn    = 0;
ER tRet           = E_SYS;
int iRet          = FLEXSPI_E_ERROR;

    /* パラメータチェック */
    if ((base       == NULL) ||     /* レジスタベースアドレス未設定 */ 
        (txbuf      == NULL) ||     /* 送信データ格納バッファ未設定 */
        (byteLength == 0)) {        /* 書き込み長がゼロ */
        return FLEXSPI_E_PARAM;     /* パラメータエラー */
    }
    else {
        ;   /* do nothing */
    }

    /* FlexSPIコントローラーレジスタアクセス */

    /* 1)INTR(TX FIFO空きフラグクリア) */
    base->INTR    |= FlexSPI_INTR_IPTXWE(1);

    /* 2)IPTXFCR(TX FIFOクリア) */
    base->IPTXFCR |= FlexSPI_IPTXFCR_CLRIPTXF(1);

    /* 3)IPCR0(QSPIデバイスアドレス設定) */
    base->IPCR0 = address;

    /* TX FIFOへの書き込み単位は64bit(=8Bytes) */
    ulCount  = byteLength / 8;
    ulRemain = byteLength % 8;

    /* TX FIFO書き込み */

    while (ulCount--) {
        /* 4)INTR(TX FIFO空きフラグチェック) */
        for (ulRetry = 0; ulRetry < FLEXSPI_MAX_RETRY; ulRetry++) {
            if ((base->INTR & FlexSPI_INTR_IPTXWE_MASK) != 0) {
                break;
            }
            else {
                twai_flg(l_tDrvInfo.tFlgID, FLEXSPI_EVFBIT_TX, TWF_ORW, &tFlgPtn, 1);
            }
        }
        if (ulRetry == FLEXSPI_MAX_RETRY) {
            return FLEXSPI_E_ERROR; /* リトライアウト */
        }
        else {
            ;   /* do nothing */
        }

        /* 5)TFDR(TX FIFO書き込み) */
        memcpy((void*)&base->TFDR, txbuf, 8);
        txbuf += 8;

        /* 6)INTR(TX FIFO空きフラグクリア) */
        base->INTR |= FlexSPI_INTR_IPTXWE_MASK;
    }

    if (ulRemain != 0) {
        /* 4)INTR(TX FIFO空きフラグチェック) */
        for (ulRetry = 0; ulRetry < FLEXSPI_MAX_RETRY; ulRetry++) {
            if ((base->INTR & FlexSPI_INTR_IPTXWE_MASK) != 0) {
                break;
            }
            else {
                twai_flg(l_tDrvInfo.tFlgID, FLEXSPI_EVFBIT_TX, TWF_ORW, &tFlgPtn, 1);
            }
        }
        if (ulRetry == FLEXSPI_MAX_RETRY) {
            return FLEXSPI_E_ERROR; /* リトライアウト */
        }
        else {
            ;   /* do nothing */
        }

        /* 5)TFDR(TX FIFO書き込み) */
        memcpy((void*)&base->TFDR, txbuf, ulRemain);

        /* 6)INTR(TX FIFO空きフラグクリア) */
        base->INTR |= FlexSPI_INTR_IPTXWE_MASK;
    }
    else {
        ;   /* do nothing */
    }

    /* 7)IPCR1(SPI転送サイズ設定) */
    base->IPCR1 &= ~(uint32_t)FlexSPI_IPCR1_IDATSZ_MASK;
    base->IPCR1 |= FlexSPI_IPCR1_IDATSZ(byteLength);

    /* イベントフラグクリア */
    clr_flg(l_tDrvInfo.tFlgID, ~FLEXSPI_EVFBIT_DONE);

    /* 割り込みステータスクリア */
    base->INTR |= FlexSPI_INTR_IPCMDDONE(1);

    /* 割り込みイネーブル */
    base->INTEN |= FlexSPI_INTEN_IPCMDDONEEN(1);

    /* 割り込み許可 */
    tRet = ena_int(INTNO_QSPI);
    if (tRet != E_OK) {
        goto err_end;   /* 割り込み許可エラー */
    }
    else {
        ;   /* do nothing */
    }

    /* 8)IPCMD(コマンド実行) */
    base->IPCMD |= FlexSPI_IPCMD_TRG(1);

    /* 9)INTR(SPI転送完了チェック) */
    if ((base->INTR & FlexSPI_INTR_IPCMDDONE_MASK) == 0U) {
        wai_flg(l_tDrvInfo.tFlgID, FLEXSPI_EVFBIT_DONE, TWF_ORW, &tFlgPtn);
    }
    else {
        ;   /* do nothing */
    }

    /* 10)IPTXFCR(TX FIFOクリア) */
    base->IPTXFCR |= FlexSPI_IPTXFCR_CLRIPTXF(1);

    /* 11)INTR(TX FIFO空きフラグクリア) */
    base->INTR |= FlexSPI_INTR_IPRXWA(1);

    /* 割り込み禁止 */
    tRet = dis_int(INTNO_QSPI);
    if (tRet != E_OK) {
        goto err_end;   /* 割り込み禁止エラー */
    }
    else {
        ;   /* do nothing */
    }

    iRet = FLEXSPI_E_SUCCESS;
err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_ExecCommand                                                             */
/*                                                                                              */
/* DESCRIPTION: コマンド実行                                                                    */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*            : address                         SPI転送デバイスアドレス                         */
/*            : byteLength                      書き込み長                                      */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : FLEXSPI_E_SUCCESS               正常終了                                        */
/*              FLEXSPI_E_ERROR                 FlexSPIコントローラーへのアクセス時にエラー     */
/*              FLEXSPI_E_PARAM                 パラメータに誤りがある                          */
/*                                                                                              */
/************************************************************************************************/
int FlexSPI_ExecCommand(FlexSPI_Type *base, uint32_t address, uint32_t byteLength)
{
FLGPTN tFlgPtn = 0;
ER tRet        = E_SYS;
int iRet       = FLEXSPI_E_ERROR;

    /* パラメータチェック */
    if (base == NULL) {         /* レジスタベースアドレス未設定 */ 
        return FLEXSPI_E_PARAM; /* パラメータエラー */
    }
    else {
        ;   /* do nothing */
    }

    /* FlexSPIコントローラーレジスタアクセス */

    /* 1)IPCR0(SPI転送アドレス設定) */
    base->IPCR0 = address;

    /* 2)IPCR1(SPI転送サイズ設定) */
    base->IPCR1 &= ~(uint32_t)FlexSPI_IPCR1_IDATSZ_MASK;
    base->IPCR1 |= FlexSPI_IPCR1_IDATSZ(byteLength);

    /* 3)IPRXFCR(RX FIFOクリア) */
    base->IPRXFCR |= FlexSPI_IPRXFCR_CLRIPRXF(1);

    /* 4)INTR(RX FIFOフルフラグクリア) */
    base->INTR |= FlexSPI_INTR_IPRXWA(1);

    /* イベントフラグクリア */
    clr_flg(l_tDrvInfo.tFlgID, ~FLEXSPI_EVFBIT_DONE);

    /* 割り込みステータスクリア */
    base->INTR |= FlexSPI_INTR_IPCMDDONE(1);

    /* 割り込みイネーブル */
    base->INTEN |= FlexSPI_INTEN_IPCMDDONEEN(1);

    /* 割り込み許可 */
    tRet = ena_int(INTNO_QSPI);
    if (tRet != E_OK) {
        goto err_end;   /* 割り込み許可エラー */
    }
    else {
        ;   /* do nothing */
    }

    /* 5)IPCMD(コマンド実行) */
    base->IPCMD |= FlexSPI_IPCMD_TRG(1);

    /* 6)INTR(SPI転送完了チェック) */
    if ((base->INTR & FlexSPI_INTR_IPCMDDONE_MASK) == 0U) {
        wai_flg(l_tDrvInfo.tFlgID, FLEXSPI_EVFBIT_DONE, TWF_ORW, &tFlgPtn);
    }
    else {
        ;   /* do nothing */
    }

    /* 割り込み禁止 */
    tRet = dis_int(INTNO_QSPI);
    if (tRet != E_OK) {
        goto err_end;   /* 割り込み禁止エラー */
    }
    else {
        ;   /* do nothing */
    }

    iRet = FLEXSPI_E_SUCCESS;

err_end:
	return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FlexSPI_ExecCommandAndRead                                                      */
/*                                                                                              */
/* DESCRIPTION: コマンド実行&読み出し                                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*            : address                         SPI転送デバイスアドレス                         */
/*            : byteLength                      書き込み長                                      */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0xFF以外                        正常終了(戻り値は読み出しデータ)                */
/*              0xFF                            エラー                                          */
/*                                                                                              */
/************************************************************************************************/
unsigned char FlexSPI_ExecCommandAndRead(FlexSPI_Type *base, uint32_t address, uint32_t byteLength)
{
FLGPTN tFlgPtn         = 0;
ER  tRet               = E_SYS;
int iRet               = 0;
unsigned char ucStatus = 0xFF;

    /* パラメータチェック */
    if (base == NULL) {         /* レジスタベースアドレス未設定 */ 
        return ucStatus;        /* パラメータエラー */
    }
    else {
        ;   /* do nothing */
    }

    /* FlexSPIコントローラーレジスタアクセス */

    /* 1)IPCR0(SPI転送アドレス設定) */
    base->IPCR0 = address;

    /* 2)IPCR1(SPI転送サイズ設定) */
    base->IPCR1 &= ~(uint32_t)FlexSPI_IPCR1_IDATSZ_MASK;
    base->IPCR1 |= FlexSPI_IPCR1_IDATSZ(byteLength);

    /* 3)IPRXFCR(RX FIFOクリア) */
    base->IPRXFCR |= FlexSPI_IPRXFCR_CLRIPRXF(1);

    /* 4)INTR(RX FIFOフルフラグクリア) */
    base->INTR |= FlexSPI_INTR_IPRXWA(1);

    /* イベントフラグクリア */
    clr_flg(l_tDrvInfo.tFlgID, ~FLEXSPI_EVFBIT_DONE);

    /* 割り込みステータスクリア */
    base->INTR |= FlexSPI_INTR_IPCMDDONE(1);

    /* 割り込みイネーブル */
    base->INTEN |= FlexSPI_INTEN_IPCMDDONEEN(1);

    /* 割り込み許可 */
    tRet = ena_int(INTNO_QSPI);
    if (tRet != E_OK) {
        goto err_end;   /* 割り込み許可エラー */
    }
    else {
        ;   /* do nothing */
    }

    /* 5)IPCMD(コマンド実行) */
    base->IPCMD |= FlexSPI_IPCMD_TRG(1);

    /* 6)INTR(SPI転送完了チェック) */
    if ((base->INTR & FlexSPI_INTR_IPCMDDONE_MASK) == 0U) {
        wai_flg(l_tDrvInfo.tFlgID, FLEXSPI_EVFBIT_DONE, TWF_ORW, &tFlgPtn);
    }
    else {
        ;   /* do nothing */
    }

    /* 割り込み禁止 */
    tRet = dis_int(INTNO_QSPI);
    if (tRet != E_OK) {
        goto err_end;   /* 割り込み禁止エラー */
    }
    else {
        ;   /* do nothing */
    }

    /* RX FIFO読み出し */
	iRet = FlexSPI_ReadRxFifo(base, &ucStatus, 1U);
    if (iRet != FLEXSPI_E_SUCCESS) {
        ucStatus = 0xFF;
    }
    else {
        ;   /* do nothing */
    }

err_end:
	return ucStatus;
}

/****************************************************************************/
/*  ローカル関数                                                            */
/****************************************************************************/
/************************************************************************************************/
/* FUNCTION   : __FlexSPI_SoftwareReset                                                         */
/*                                                                                              */
/* DESCRIPTION: ソフトウェアリセット                                                            */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : FLEXSPI_E_SUCCESS               正常終了                                        */
/*              FLEXSPI_E_ERROR                 エラー                                          */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _FlexSPI_SoftwareReset(FlexSPI_Type *base)
{
FLGPTN tFlgPtn = 0;
uint32_t i     = 0;

    /* ソフトウェアリセット */
    base->MCR0 |= FlexSPI_MCR0_SWRESET_MASK;

    for (i = 0; i < FLEXSPI_MAX_RETRY; i++) {
        /* フラグチェック */
        if ((base->MCR0 & FlexSPI_MCR0_SWRESET_MASK) == 0) {
            return FLEXSPI_E_SUCCESS;
        }
        else {
            twai_flg(l_tDrvInfo.tFlgID, FLEXSPI_EVFBIT_WAIT, TWF_ORW, &tFlgPtn, 1);
        }
    }

    return FLEXSPI_E_ERROR;     /* FlexSPIコントローラー異常 */
}

/************************************************************************************************/
/* FUNCTION   : __FlexSPI_SetConfig                                                             */
/*                                                                                              */
/* DESCRIPTION: レジスタ設定(MCR0 - 2)                                                          */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void _FlexSPI_SetConfig(FlexSPI_Type *base)
{
uint32_t ulReg = 0;

    /* 2)MCR0(レジスタ設定) */
    ulReg =  base->MCR0;
    ulReg &= ~(FlexSPI_MCR0_LEARNEN_MASK      |
               FlexSPI_MCR0_SCKFREERUNEN_MASK |
               FlexSPI_MCR0_RESERVED_MASK     |
               FlexSPI_MCR0_MDIS_MASK         |
               FlexSPI_MCR0_SWRESET_MASK);
    ulReg |= (FlexSPI_MCR0_AHBGRANTWAIT_MASK |      /* NOTE: Debug only Please default value */
              FlexSPI_MCR0_IPGRANTWAIT_MASK  |      /* NOTE: Debug only Please default value */
              FlexSPI_MCR0_SCKFREERUNEN(0)   |
              FlexSPI_MCR0_COMBINATIONEN(0)  |
              FlexSPI_MCR0_DOZEEN(0)         |
              FlexSPI_MCR0_HSEN(1)           |
              FlexSPI_MCR0_ATDFEN(0)         |
              FlexSPI_MCR0_ARDFEN(0)         |
              FlexSPI_MCR0_RXCLKSRC(0));
    base->MCR0 = ulReg;

    /* 3)MCR1(レジスタ設定e) */
    ulReg =  base->MCR1;
    ulReg &= ~(FlexSPI_MCR1_SEQWAIT_MASK | FlexSPI_MCR1_AHBBUSWAIT_MASK);
    ulReg |= (FlexSPI_MCR1_SEQWAIT(0x00FFU) |
              FlexSPI_MCR1_AHBBUSWAIT(0xFFFFU));
    base->MCR1 = ulReg;

    /* 4)MCR2(レジスタ設定) */
    ulReg =  base->MCR2;
    ulReg &= ~(FlexSPI_MCR2_RESUMEWAIT_MASK   |
               FlexSPI_MCR2_SCKBDIFFOPT_MASK  |
               FlexSPI_MCR2_SAMEDEVICEEN_MASK |
               FlexSPI_MCR2_CLRAHBBUFOPT_MASK);
    ulReg |= (FlexSPI_MCR2_RESUMEWAIT(0x20U) |
              FlexSPI_MCR2_SCKBDIFFOPT(0)    |
              FlexSPI_MCR2_SAMEDEVICEEN(0)   |
              FlexSPI_MCR2_CLRAHBBUFOPT(0));
    base->MCR2 = ulReg;
}

/************************************************************************************************/
/* FUNCTION   : __FlexSPI_SetCS                                                                 */
/*                                                                                              */
/* DESCRIPTION: レジスタ設定(MCR0 - 2)                                                          */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : FLEXSPI_E_SUCCESS               正常終了                                        */
/*            : FLEXSPI_E_ERROR                 エラー                                          */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _FlexSPI_SetCS(FlexSPI_Type *base, int chip_select, const flexspi_device_config_t *config)
{
FLGPTN tFlgPtn       = 0;
uint32_t configValue = 0;
uint32_t statusValue = 0;
uint32_t i           = 0;
uint8_t index        = (uint8_t)chip_select >> 1U; /* PortA with index 0, PortB with index 1. */
uint8_t delay        = 0;

    /* 5)STS0(アイドル待ち) */
    for (i = 0; i < FLEXSPI_MAX_RETRY; i++) {
        if (((base->STS0 & FlexSPI_STS0_ARBIDLE_MASK) != 0) &&
            ((base->STS0 & FlexSPI_STS0_SEQIDLE_MASK) != 0)) {
            break;
        }
        else {
            twai_flg(l_tDrvInfo.tFlgID, FLEXSPI_EVFBIT_WAIT, TWF_ORW, &tFlgPtn, 1);
        }
    }
    if (i == FLEXSPI_MAX_RETRY) {
        return FLEXSPI_E_ERROR;
    }
    else {
        ;   /* do nothing */
    }

    /* 6)FLSHCR0[n](コンフィギュレーション情報設定) */
    base->FLSHCR0[chip_select] = config->flashSize;

    /* 7)FLSHCR1[n](コンフィギュレーション情報設定) */
    base->FLSHCR1[chip_select] = (FlexSPI_FLSHCR1_CSINTERVAL(config->CSInterval)         |
                                  FlexSPI_FLSHCR1_CSINTERVALUNIT(config->CSIntervalUnit) |
                                  FlexSPI_FLSHCR1_CAS(config->columnspace)               |
                                  FlexSPI_FLSHCR1_WA(config->enableWordAddress)          |
                                  FlexSPI_FLSHCR1_TCSH(config->CSHoldTime)               |
                                  FlexSPI_FLSHCR1_TCSS(config->CSSetupTime));
	
    /* 8)FLSHCR2[n](コンフィギュレーション情報設定) */
    configValue = base->FLSHCR2[chip_select];

    configValue &= ~(FlexSPI_FLSHCR2_AWRWAITUNIT_MASK |
                     FlexSPI_FLSHCR2_AWRWAIT_MASK     |
                     FlexSPI_FLSHCR2_AWRSEQNUM_MASK   |
                     FlexSPI_FLSHCR2_AWRSEQID_MASK    |
                     FlexSPI_FLSHCR2_ARDSEQNUM_MASK   |
                     FlexSPI_FLSHCR2_ARDSEQID_MASK);

    configValue |= (FlexSPI_FLSHCR2_AWRWAITUNIT(config->AHBWriteWaitUnit) |
                    FlexSPI_FLSHCR2_AWRWAIT(config->AHBWriteWaitInterval));

    if (0 < config->AWRSeqNumber) {
        configValue |= (FlexSPI_FLSHCR2_AWRSEQID((uint32_t)config->AWRSeqIndex) |
                        FlexSPI_FLSHCR2_AWRSEQNUM((uint32_t)config->AWRSeqNumber - 1U));
    }
    else {
        ;   /* do nothing */
    }

    if (0 < config->ARDSeqNumber) {
        configValue |= (FlexSPI_FLSHCR2_ARDSEQID((uint32_t)config->ARDSeqIndex) |
                        FlexSPI_FLSHCR2_ARDSEQNUM((uint32_t)config->ARDSeqNumber - 1U));
    }
    else {
        ;   /* do nothing */
    }

    base->FLSHCR2[chip_select] = configValue;

    /* 9)DLLCR[n/2](レジスタ設定) */
    configValue        = _FlexSPI_GetDLLValue(base, chip_select, config);
    base->DLLCR[index] = configValue;

    /* 10)FLSHCR4(コンフィギュレーション情報設定) */
    if (config->enableWriteMask) {
        base->FLSHCR4 &= ~FlexSPI_FLSHCR4_WMOPT1_MASK;
    }
    else {
        base->FLSHCR4 |= FlexSPI_FLSHCR4_WMOPT1_MASK;
    }

    if (index == 0U) {
         /*PortA*/
        base->FLSHCR4 &= ~FlexSPI_FLSHCR4_WMENA_MASK;
        base->FLSHCR4 |= FlexSPI_FLSHCR4_WMENA(config->enableWriteMask);
    }
    else {
        base->FLSHCR4 &= ~FlexSPI_FLSHCR4_WMENB_MASK;
        base->FLSHCR4 |= FlexSPI_FLSHCR4_WMENB(config->enableWriteMask);
    }

    /* 11)MCR0(モジュールイネーブル) */
    base->MCR0 &= ~FlexSPI_MCR0_MDIS_MASK;

    /* According to ERR011377, need to delay at least 100 NOPs to ensure the DLL is locked. */
    statusValue = (index == 0U) ?
                   ((uint32_t)kFlexSPI_FlashASampleClockSlaveDelayLocked |
                    (uint32_t)kFlexSPI_FlashASampleClockRefDelayLocked) :
                   ((uint32_t)kFlexSPI_FlashBSampleClockSlaveDelayLocked |
                    (uint32_t)kFlexSPI_FlashBSampleClockRefDelayLocked);

    if ((configValue & FlexSPI_DLLCR_DLLEN_MASK) != 0U) {
        /* Wait slave delay line locked and slave reference delay line locked. */
        while ((base->STS2 & statusValue) != statusValue) {
            //PRINT("SLAVE WAIT\r\n");
        }

        /* Wait at least 100 NOPs*/
        for (delay = 100U; delay > 0U; delay--) {
            __NOP();
        }
    }
    else {
        ;   /* do nothing */
    }
    return FLEXSPI_E_SUCCESS;
}

/************************************************************************************************/
/* FUNCTION   : __FlexSPI_GetDLLValue                                                           */
/*                                                                                              */
/* DESCRIPTION: DLLレジスタ値取得                                                               */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : DLL設定値                                                                       */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _FlexSPI_GetDLLValue(FlexSPI_Type *base, int chip_select, const flexspi_device_config_t *config)
{
uint32_t isUnifiedConfig = 1;
uint32_t flexspiDllValue = 0;
uint32_t dllValue        = 0;
uint32_t temp            = 0;

    if (isUnifiedConfig != 0) {
        flexspiDllValue = FLEXSPI_DLLCR_DEFAULT; /* 1 fixed delay cells in DLL delay chain) */
    }
    else {
        if ((100U * FREQ_1MHz) <= config->flexspiRootClk) {
            /* DLLEN = 1, SLVDLYTARGET = 0xF, */
            flexspiDllValue = FlexSPI_DLLCR_DLLEN(1) | FlexSPI_DLLCR_SLVDLYTARGET(0x0F);
        }
        else {
            temp     = (uint32_t)config->dataValidTime * 1000U; /* Convert data valid time in ns to ps. */
            dllValue = temp / (uint32_t)kFLEXSPI_DelayCellUnitMin;
            if (dllValue * (uint32_t)kFLEXSPI_DelayCellUnitMin < temp) {
                dllValue++;
            }
            else {
                ;   /* do nothing */
            }
            flexspiDllValue = FlexSPI_DLLCR_OVRDEN(1) | FlexSPI_DLLCR_OVRDVAL(dllValue);
        }
    }
    return flexspiDllValue;
}

/************************************************************************************************/
/* FUNCTION   : __FlexSPI_SetAHBConfig                                                          */
/*                                                                                              */
/* DESCRIPTION: AHB設定                                                                         */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : base                            FlexSPIコントローラーレジスタベースアドレス     */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void _FlexSPI_SetAHBConfig(FlexSPI_Type *base)
{
int i = 0;

	// FLASHのサイズをセット(KB単位)
//	base->FLSHCR0[0] = FROM_CHIP_SIZE / 1024;	// FLSHA1CR0 
	// 接続しているFLASHは１個のみなので、FLSHA1CR0以外は0
	base->FLSHCR0[1] = 0;						// FLSHA2CR0
	base->FLSHCR0[2] = 0;						// FLSHB1CR0
	base->FLSHCR0[3] = 0;						// FLSHB2CR0

	/* 12)LUT(LUT設定 4-Byte Quad Output Fast Read) */
	FlexSPI_SetQuadOutFastRdSequence(base);

    /* 13)AHBRXBUFCR0[0]-[7](レジスタクリア) */
	for (i = 0; i < 7; i++ ) {
		base->AHBRXBUFCR0[i] = 0;
	}

    /* 14)AHBRXBUFCR0[7](バッファ設定) */
	base->AHBRXBUFCR0[7] = (FLEXSPI_AHB_BUFFER_SIZE / 8) | FlexSPI_AHBRXBUFCR0_PREFETCHEN_MASK;

	/* 15)AHBCR(プリフェッチ設定) */
	base->AHBCR = FlexSPI_AHBCR_PREFETCHEN_MASK;

    /* 16)FLSHCR2[n](LUTシーケンス設定) */
	base->FLSHCR2[0] = 1;		// Flash Control Register 2 (FLSHA1CR2)に、シーケンス番号セット
	
    /* 17)IPRXFCR(RX FIFO設定) */
    base->IPRXFCR &= ~FlexSPI_IPRXFCR_RXDMAEN_MASK;
    base->IPRXFCR &= ~FlexSPI_IPRXFCR_RXWMRK_MASK;
    base->IPRXFCR |= FlexSPI_IPRXFCR_RXWMRK((uint32_t)FLEXSPI_WATERMARK_BITS / 8U - 1U);

    /* 18)IPTXFCR(TX FIFO設定) */
    base->IPTXFCR &= ~FlexSPI_IPTXFCR_TXDMAEN_MASK;
    base->IPTXFCR &= ~FlexSPI_IPTXFCR_TXWMRK_MASK;
    base->IPTXFCR |= FlexSPI_IPTXFCR_TXWMRK((uint32_t)FLEXSPI_WATERMARK_BITS / 8U - 1U);
}

/* 割り込みサービス */

/************************************************************************************************/
/* FUNCTION   : _FlexSPI_ISR                                                                    */
/*                                                                                              */
/* DESCRIPTION: 割り込みサービスルーチン                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : exinf                           拡張情報                                        */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void _FlexSPI_ISR(VP_INT exinf)
{
FlexSPI_DrvInfo *ptDrvInfo = (FlexSPI_DrvInfo*)exinf;
uint32_t ulStatus          = 0;

    /* 割り込みステータス取得 */
    ulStatus = ptDrvInfo->tpBase->INTR;

    /* 割り込み分類 */
    if ((ulStatus & FlexSPI_INTEN_IPCMDDONEEN_MASK) != 0) {
        /* 割り込みマスク */
        ptDrvInfo->tpBase->INTEN &= ~FlexSPI_INTEN_IPCMDDONEEN_MASK;

        /* イベントフラグセット */
        iset_flg(ptDrvInfo->tFlgID, 0x00000001);
    }
    else {
        ;   /* do nothing */
    }
}
