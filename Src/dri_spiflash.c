/************************************************************************************************/
/*                                                                                              */
/* FILE NAME                                                                    VERSION         */
/*                                                                                              */
/*      dri_spiflash.c                                                          0.00            */
/*                                                                                              */
/* DESCRIPTION:                                                                                 */
/*                                                                                              */
/*      NORドライバソースファイル                                                               */
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

//#include <log.h>

#include "itron.h"
#include "kernel.h"
#include "ARMv7M.h"

#include "code_rules_def.h"
#include "dri_flexspi.h"
#include "dri_flexspi_lut.h"
#include "dri_spiflash.h"

/****************************************************************************/
/*  定数・マクロ定義                                                        */
/****************************************************************************/

#define MAX_WRITE_ADDR      (FROM_SIZE - 1U)    /* アドレス終端 */

/* イベントフラグビット */
#define FROM_EVFBIT_WAIT    (0x00000001U)       /* 汎用時間待ち */

/****************************************************************************/
/*  構造体定義                                                              */
/****************************************************************************/

/* NORドライバ情報 */
typedef struct FROM_DrvInfo_tag {
    FlexSPI_Type    *tpFlexSPIReg;  /* FlexSPIコントローラのレジスタベースアドレス */
    uint32_t        ulState;        /* 動作状態 */
    ID              tSemID;         /* セマフォID */
    ID              tFlgID;         /* イベントフラグID */
} FROM_DrvInfo;

/****************************************************************************/
/*  ローカルデータ                                                          */
/****************************************************************************/

/* NORドライバ情報 */
DLOCAL FROM_DrvInfo l_tDrvInfo = { 0 };

/****************************************************************************/
/*  ローカル関数宣言                                                        */
/****************************************************************************/

/* 書き込み処理 */
LOCAL int _FROM_WriteCore(unsigned int uiAddress, unsigned int uiLength, unsigned char *strWriteData);

/* 読み出し処理 */
LOCAL int _FROM_ReadCore(unsigned int uiAddress, unsigned int uiLength, unsigned char *strReadData);

/* セクタ消去処理 */
LOCAL int _FROM_SectorEraseCore(unsigned int uiAddress, unsigned int uiLength);

/* ブロック消去処理 */
LOCAL int _FROM_BlockEraseCore(unsigned int uiAddress, unsigned int uiLength);

/****************************************************************************/
/*  提供関数                                                                */
/****************************************************************************/

/************************************************************************************************/
/* FUNCTION   : FROM_Init                                                                       */
/*                                                                                              */
/* DESCRIPTION: 初期化                                                                          */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                            なし                                            */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : none                            なし                                            */
/*                                                                                              */
/************************************************************************************************/
void FROM_Init(void)
{
T_CSEM tCSem = { 0 };
T_CFLG tCFLG = { 0 };

    if (l_tDrvInfo.ulState == FROM_NONE_STATE){
        /* ドライバデータ初期化 */
        l_tDrvInfo.tpFlexSPIReg = (FlexSPI_Type*)FLEXSPI_BASE;  /* FlexSPIコントローラレジスタベースアドレス */

        /* セマフォ作成 */
        tCSem.sematr  = (TA_HLNG | TA_TFIFO);
        tCSem.isemcnt = 1;
        tCSem.maxsem  = 1;
        tCSem.name    = "FROM Semaphore";
        l_tDrvInfo.tSemID = acre_sem(&tCSem);
        if (E_OK <= l_tDrvInfo.tSemID) {
            ;   /* do nothing */
        }
        else {
            for ( ;; ) {}
        }

        /* イベントフラグ作成 */
        tCFLG.flgatr  = (TA_TFIFO | TA_WMUL);
        tCFLG.iflgptn = 0;
        tCFLG.name    = "FROM Eventflag";
        l_tDrvInfo.tFlgID = acre_flg(&tCFLG);
        if (E_OK <= l_tDrvInfo.tFlgID) {
            l_tDrvInfo.ulState = FROM_INIT_STATE;   /* 動作状態更新 */
        }
        else {
            for ( ;; ) {}
        }
    }
    else {
        ;   /* do nothing */
    }
}

/************************************************************************************************/
/* FUNCTION   : FROM_Open                                                                       */
/*                                                                                              */
/* DESCRIPTION: オープン                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                            なし                                            */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : none                            なし                                            */
/*                                                                                              */
/************************************************************************************************/
int FROM_Open(void)
{
flexspi_device_config_t tConfig = { 0 };
int iRet                        = FROM_SPI_OPEN_ERROR;

    /* 動作状態チェック */
    if (l_tDrvInfo.ulState != FROM_INIT_STATE) {
        return iRet;                            /* 初期化済み状態でない */
    }
    else {
        ;   /* do nothing */
    }

    /* セマフォ取得 */
    wai_sem(l_tDrvInfo.tSemID);

    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_OPENING_STATE;    /* オープン処理中 */

    /* コンフィギュレーション情報設定 */
    tConfig.flexspiRootClk       = 40000000;
    tConfig.isSck2Enabled        = false;
    tConfig.flashSize            = (uint32_t)FROM_SIZE / 1024U; /* 設定はKB単位 */
    tConfig.CSIntervalUnit       = kFLEXSPI_CsIntervalUnit1SckCycle;
    tConfig.CSInterval           = 3;
    tConfig.CSHoldTime           = 3;
    tConfig.CSSetupTime          = 3;
    tConfig.dataValidTime        = 0;
    tConfig.columnspace          = 0;
    tConfig.enableWordAddress    = false;
    tConfig.AWRSeqIndex          = 0;
    tConfig.AWRSeqNumber         = 0;
    tConfig.ARDSeqIndex          = 0;
    tConfig.ARDSeqNumber         = 0;
    tConfig.AHBWriteWaitUnit     = kFLEXSPI_AhbWriteWaitUnit2AhbCycle;
    tConfig.AHBWriteWaitInterval = 0;
    tConfig.enableWriteMask      = false;

    /* QSPIドライバオープン */
    iRet = FlexSPI_Open(l_tDrvInfo.tpFlexSPIReg, 0, &tConfig);
    if (iRet == FLEXSPI_E_SUCCESS) {
        /* 動作状態更新 */
        l_tDrvInfo.ulState = FROM_OPEN_STATE;   /* オープン中 */
        iRet = FROM_SUCCESS;
    }
    else {
        /* 動作状態更新 */
        l_tDrvInfo.ulState = FROM_INIT_STATE;   /* 初期化済み */
        iRet = FROM_SPI_OPEN_ERROR;             /* オープン処理エラー */
    }

    /* セマフォ解放 */
    sig_sem(l_tDrvInfo.tSemID);

    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FROM_Close                                                                      */
/*                                                                                              */
/* DESCRIPTION: クローズ                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                            なし                                            */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : none                            なし                                            */
/*                                                                                              */
/************************************************************************************************/
int FROM_Close(void)
{
int iRet = FROM_SPI_CLOSE_ERROR;

    /* 動作状態チェック */
    if (l_tDrvInfo.ulState == FROM_BUSY_STATE) {
        return iRet;    /* 入出力動作中 */
    }
    else {
        ;   /* do nothng */
    }

    if ((l_tDrvInfo.ulState == FROM_NONE_STATE) ||
        (l_tDrvInfo.ulState == FROM_INIT_STATE)) {
        return FROM_SUCCESS;    /* 未オープンなので正常終了とする */
    }
    else {
        ;   /* do nothng */
    }

    /* セマフォ取得 */
    wai_sem(l_tDrvInfo.tSemID);

    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_CLOSING_STATE;    /* クローズ処理中 */

    /* QSPIドライバクローズ */
    iRet = FlexSPI_Close(l_tDrvInfo.tpFlexSPIReg);
    if (iRet == FLEXSPI_E_SUCCESS) {
        /* 動作状態更新 */
        l_tDrvInfo.ulState = FROM_INIT_STATE;    /* 初期化済み */
        iRet = FROM_SUCCESS;
    }
    else {
        /* 動作状態更新 */
        l_tDrvInfo.ulState = FROM_OPEN_STATE;    /* オープン中 */
    }

    /* セマフォ解放 */
    sig_sem(l_tDrvInfo.tSemID);

    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FROM_Write                                                                      */
/*                                                                                              */
/* DESCRIPTION: 書き込み                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : uiAddress                       書き込みを開始するアドレス                      */
/*            : uiLength                        書き込みデータ長                                */
/*            : strWriteData                    書き込みデータ                                  */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_SUCCESS                    正常終了                                        */
/*            : FROM_WRITE_ERROR                書き込みエラー                                  */
/*            : FROM_WRITE_ENABLE_ERROR         書き込み失敗                                    */
/*                                                                                              */
/************************************************************************************************/
int FROM_Write(unsigned int uiAddress, unsigned int uiLength, unsigned char *strWriteData)
{
uint32_t ulSize = 0;
int iRet        = FROM_WRITE_ERROR;

    /* パラメータチェック */
    if (FROM_SIZE <= uiAddress) {
        iRet = FROM_WRITE_ERROR;    /* 書き込み開始アドレスが範囲外 */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if (FROM_SIZE < (uiAddress + uiLength)) {
        iRet = FROM_WRITE_ERROR;    /* 書き込みサイズが範囲外 */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if (strWriteData == NULL) {
        iRet = FROM_WRITE_ERROR;    /* 書き込みデータ未設定 */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 動作状態チェック */
    if (l_tDrvInfo.ulState != FROM_OPEN_STATE) {
        iRet = FROM_WRITE_ERROR;    /* オープン中でない */
        goto err_end;
    }
    else {
        ;   /* do nothng */
    }

    /* セマフォ取得 */
    wai_sem(l_tDrvInfo.tSemID);

    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_BUSY_STATE;   /* 入出力中 */

    /* 書き込み処理 */

    while (0 < uiLength) {
        /* １回の書き込みサイズ設定 */
        ulSize = (FLEXSPI_TX_BUFFER_SIZE < uiLength) ? FLEXSPI_TX_BUFFER_SIZE : uiLength;

        /* 書き込み処理 */
        iRet = _FROM_WriteCore( uiAddress, ulSize, strWriteData );
        if (iRet != FROM_SUCCESS) {
            iRet = FROM_WRITE_ERROR;        /* 書き込みエラー */
            goto err_end1;
        }
        else {
            /* 残書き込み長・アドレス更新 */
            strWriteData += ulSize;
            uiAddress    += ulSize;
            uiLength     -= ulSize;
        }
    }

err_end1:
    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_OPEN_STATE;   /* オープン中 */

    /* セマフォ解放 */
    sig_sem(l_tDrvInfo.tSemID);

err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FROM_Read                                                                       */
/*                                                                                              */
/* DESCRIPTION: 読み出し                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : uiAddress                       読みだしを開始するアドレス                      */
/*            : uiLength                        読み出しデータ長                                */
/*            : strReadData                     読み出しデータバッファ                          */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_SUCCESS                    正常終了                                        */
/*            : FROM_READ_ERROR                 読み出しエラー                                  */
/*                                                                                              */
/************************************************************************************************/
int FROM_Read(unsigned int uiAddress, unsigned int uiLength, unsigned char *strReadData)
{
int iRet        = FROM_READ_ERROR;
uint32_t ulSize = 0;

    /* パラメータチェック */
    if (FROM_SIZE <= uiAddress) {
        iRet = FROM_READ_ERROR;     /* 読み出し開始アドレスが範囲外 */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if (FROM_SIZE < (uiAddress + uiLength)) {
        iRet = FROM_READ_ERROR;     /* 読み出しサイズが範囲外 */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if (strReadData == NULL) {
        iRet = FROM_READ_ERROR;     /* 読み出しバッファ未設定 */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 動作状態チェック */
    if (l_tDrvInfo.ulState != FROM_OPEN_STATE) {
        iRet = FROM_READ_ERROR;     /* オープン中でない */
        goto err_end;
    }
    else {
        ;   /* do nothng */
    }

    /* セマフォ取得 */
    wai_sem(l_tDrvInfo.tSemID);

    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_BUSY_STATE;

    /* 読み出し処理 */
    while (0 < uiLength) {
        /* １回の読み出しサイズ設定 */
        ulSize = (FLEXSPI_RX_BUFFER_SIZE < uiLength) ? FLEXSPI_RX_BUFFER_SIZE : uiLength;
        /* 読み出し処理 */
        iRet = _FROM_ReadCore(uiAddress, ulSize, strReadData );
        if (iRet != FROM_SUCCESS) {
            iRet = FROM_READ_ERROR; /* 読み出しエラー */
            goto err_end1;
        }
        else {
            /* 残読み出し長・アドレス更新 */
            strReadData += ulSize;
            uiAddress   += ulSize;
            uiLength    -= ulSize;
        }
    }

err_end1:
    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_OPEN_STATE;   /* オープン中 */

    /* セマフォ解放 */
    sig_sem(l_tDrvInfo.tSemID);

err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FROM_SectorErase                                                                */
/*                                                                                              */
/* DESCRIPTION: セクタ消去                                                                      */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : uiAddress                       消去を開始するアドレス                          */
/*            : uiLength                        消去するバイト数                                */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_SUCCESS                    正常終了                                        */
/*            : FROM_ERASE_ERROR                消去エラー                                      */
/*                                                                                              */
/************************************************************************************************/
int FROM_SectorErase(unsigned int uiAddress, unsigned int uiLength)
{
int iRet = FROM_ERASE_ERROR;

    /* パラメータチェック */
    if (FROM_SIZE <= uiAddress) {
        iRet = FROM_ERASE_ERROR;    /* 消去開始アドレスが範囲を超えている */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if ((uiAddress % FROM_SECT_SIZE) != 0) {
        iRet = FROM_ERASE_ERROR;    /* 消去開始アドレスがセクタ境界でない */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if (FROM_SIZE < (uiAddress + uiLength)) {
        iRet = FROM_ERASE_ERROR;    /* 消去バイト数が範囲を超えている */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if ((uiLength % FROM_SECT_SIZE) != 0) {
        iRet = FROM_ERASE_ERROR;    /* 消去バイト数がセクタ境界でない */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 動作状態チェック */
    if (l_tDrvInfo.ulState != FROM_OPEN_STATE) {
        iRet = FROM_ERASE_ERROR;    /* オープン状態でない */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* セマフォ取得 */
    wai_sem(l_tDrvInfo.tSemID);

    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_BUSY_STATE;

    while (0 < uiLength) {
        /* セクタ消去処理 */
        iRet = _FROM_SectorEraseCore(uiAddress, (unsigned int)FROM_SECT_SIZE);
        if (iRet != FROM_SUCCESS) {
            iRet = FROM_ERASE_ERROR;    /* セクタ消去エラー */
            goto err_end1;
        }
        else {
            /* 消去するアドレス／残消去サイズ更新 */
            uiAddress += FROM_SECT_SIZE;
            uiLength  -= FROM_SECT_SIZE;
        }
    }

err_end1:
    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_OPEN_STATE;

    /* セマフォ解放 */
    sig_sem(l_tDrvInfo.tSemID);

err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FROM_BlockErase                                                                 */
/*                                                                                              */
/* DESCRIPTION: ブロック消去                                                                    */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : uiAddress                       消去を開始するアドレス                          */
/*            : uiLength                        消去するバイト数                                */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_SUCCESS                    正常終了                                        */
/*            : FROM_ERASE_ERROR                消去エラー                                      */
/*                                                                                              */
/************************************************************************************************/
int FROM_BlockErase(unsigned int uiAddress, unsigned int uiLength)
{
int iRet = FROM_ERASE_ERROR;

    /* パラメータチェック */
    if (FROM_SIZE <= uiAddress) {
        iRet = FROM_ERASE_ERROR;    /* 消去開始アドレスが範囲を超えている */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if ((uiAddress % FROM_BLK_SIZE) != 0) {
        iRet = FROM_ERASE_ERROR;    /* 消去開始アドレスがブロック境界でない */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if (FROM_SIZE < (uiAddress + uiLength)) {
        iRet = FROM_ERASE_ERROR;    /* 消去バイト数が範囲を超えている */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    if ((uiLength % FROM_BLK_SIZE) != 0) {
        iRet = FROM_ERASE_ERROR;    /* 消去バイト数がブロック境界でない */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 動作状態チェック */
    if (l_tDrvInfo.ulState != FROM_OPEN_STATE) {
        iRet = FROM_ERASE_ERROR;    /* オープン状態でない */
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* セマフォ取得 */
    wai_sem(l_tDrvInfo.tSemID);

    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_BUSY_STATE;

    while (0 < uiLength) {
        /* ブロック消去処理 */
        iRet = _FROM_BlockEraseCore(uiAddress, (unsigned int)FROM_BLK_SIZE);
        if (iRet != FROM_SUCCESS) {
            iRet = FROM_ERASE_ERROR;    /* ブロック消去エラー */
            goto err_end1;
        }
        else {
            /* 消去するアドレス／残消去サイズ更新 */
            uiAddress += FROM_BLK_SIZE;
            uiLength  -= FROM_BLK_SIZE;
        }
    }

err_end1:
    /* 動作状態更新 */
    l_tDrvInfo.ulState = FROM_OPEN_STATE;

    /* セマフォ解放 */
    sig_sem(l_tDrvInfo.tSemID);

err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : FROM_getState                                                                   */
/*                                                                                              */
/* DESCRIPTION: NORドライバ動作状態取得                                                         */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                            なし                                            */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_STATUS                     NORドライバ動作状態                             */
/*                                                                                              */
/************************************************************************************************/
int FROM_getState(void)
{
    return l_tDrvInfo.ulState;
}

/****************************************************************************/
/*  ローカル関数                                                            */
/****************************************************************************/
/************************************************************************************************/
/* FUNCTION   : _FROM_WriteCore                                                                 */
/*                                                                                              */
/* DESCRIPTION: 書き込み処理                                                                    */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : uiAddress                       書き込みを開始するアドレス                      */
/*            : uiLength                        書き込みデータ長                                */
/*            : strWriteData                    書き込みデータ                                  */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_SUCCESS                    正常終了                                        */
/*            : FROM_WRITE_ERROR                書き込みエラー                                  */
/*            : FROM_WRITE_ENABLE_ERROR         書き込み失敗                                    */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _FROM_WriteCore(unsigned int uiAddress, unsigned int uiLength, unsigned char *strWriteData)
{
FLGPTN tFlgPtn         = 0;
int iRet               = FROM_WRITE_ERROR;
uint32_t i             = 0;
unsigned char ucStatus = 0x00;

    /* 書き込み許可 */
    FlexSPI_SetWriteEnableSequence(l_tDrvInfo.tpFlexSPIReg);    /* LUT設定 */
    iRet = FlexSPI_ExecCommand(l_tDrvInfo.tpFlexSPIReg, 0, 0);  /* コマンド実行 */
    if (iRet != FLEXSPI_E_SUCCESS) {
        iRet = FROM_WRITE_ENABLE_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 書き込み */
    FlexSPI_SetQuadWriteSequence(l_tDrvInfo.tpFlexSPIReg);      /* LUT設定 */
    iRet = FlexSPI_WriteTxFifo(l_tDrvInfo.tpFlexSPIReg, strWriteData, uiAddress, uiLength);
    if (iRet != FLEXSPI_E_SUCCESS) {
        iRet = FROM_WRITE_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 書き込み完了待ち */
    FlexSPI_SetReadStatusSequence(l_tDrvInfo.tpFlexSPIReg);
    for (i = 0; i < 1000U; i++) {
        /* ステータス読み出し */
        ucStatus = FlexSPI_ExecCommandAndRead(l_tDrvInfo.tpFlexSPIReg, 0, 0);
        if (ucStatus == 0xFF) {
            iRet = FROM_WRITE_ERROR;
            goto err_end;
        }
        else {
            ;   /* do nothing */
        }
        /* ステータスチェック(b0:write in progress ビットが'0'なら完了) */
        if ((ucStatus & 0x01) == 0) {
            break;
        }
        else {
            /* ウエイト */
            twai_flg(l_tDrvInfo.tFlgID, FROM_EVFBIT_WAIT, TWF_ORW, &tFlgPtn, 1);
        }
    }
    if (i == 1000U) {
        iRet = FROM_WRITE_ERROR;
        goto err_end;
    }

    FlexSPI_SetReadFlagStatusSequence(l_tDrvInfo.tpFlexSPIReg);     /* LUT設定 */
    for (i = 0; i < 1000U; i++) {
        /* フラグステータス読み出し */
        ucStatus = FlexSPI_ExecCommandAndRead(l_tDrvInfo.tpFlexSPIReg, 0, 0);
        if (ucStatus == 0xFF) {
            iRet = FROM_WRITE_ERROR;
            goto err_end;
        }
        else {
            ;   /* do nothing */
        }
        /* ステータスチェック(b7:Program or erase controller ビットが'1'なら完了) */
        if ((ucStatus & 0x80) != 0) {
            break;
        }
        else {
            /* ウエイト */
            twai_flg(l_tDrvInfo.tFlgID, FROM_EVFBIT_WAIT, TWF_ORW, &tFlgPtn, 1);
        }
    }
    if (i == 1000U) {
        iRet = FROM_WRITE_ERROR;
        goto err_end;
    }
    else {
        iRet = FROM_SUCCESS;
    }
err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : _FROM_ReadCore                                                                  */
/*                                                                                              */
/* DESCRIPTION: 読み出し処理                                                                    */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : uiAddress                       読み出しを開始するアドレス                      */
/*            : uiLength                        読み出しデータ長                                */
/*                                                                                              */
/* OUTPUT     : strReadData                     読み出しデータ格納バッファ                      */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_SUCCESS                    正常終了                                        */
/*            : FROM_READ_ERROR                 読み出しエラー                                  */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _FROM_ReadCore(unsigned int uiAddress, unsigned int uiLength, unsigned char *strReadData)
{
int iRet  = FROM_READ_ERROR;
int iRet2 = FROM_READ_ERROR;

    /* Quadモード設定 */
    FlexSPI_SetEnterQuadModeSequence(l_tDrvInfo.tpFlexSPIReg);          /* LUT設定 */
    iRet2 = FlexSPI_ExecCommand(l_tDrvInfo.tpFlexSPIReg, 0, 0);          /* コマンド実行 */
    if (iRet2 != FLEXSPI_E_SUCCESS) {
        iRet = FROM_READ_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 読み出し */
    FlexSPI_SetQuadIOReadSequence(l_tDrvInfo.tpFlexSPIReg);                     /* LUT設定 */
    iRet2 = FlexSPI_ExecCommand(l_tDrvInfo.tpFlexSPIReg, uiAddress, uiLength);  /* コマンド実行 */
    if (iRet2 != FLEXSPI_E_SUCCESS) {
        iRet = FROM_READ_ERROR;
        goto err_end1;
    }
    else {
        ;   /* do nothing */
    }

    iRet2 = FlexSPI_ReadRxFifo(l_tDrvInfo.tpFlexSPIReg, strReadData, uiLength);  /* RX FIFO読み出し */
    if (iRet2 != FLEXSPI_E_SUCCESS) {
        iRet = FROM_READ_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

err_end1:
    /* Quadモード解除 */
    FlexSPI_SetResetQuadModeSequence(l_tDrvInfo.tpFlexSPIReg);                  /* LUT設定 */
    iRet2 = FlexSPI_ExecCommand(l_tDrvInfo.tpFlexSPIReg, 0, 0);                 /* コマンド実行 */
    if (iRet2 != FLEXSPI_E_SUCCESS) {
        iRet = FROM_READ_ERROR;
        goto err_end;
    }
    else {
        iRet = FROM_SUCCESS;
    }
   
err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : _FROM_SectorEraseCore                                                           */
/*                                                                                              */
/* DESCRIPTION: セクタ消去処理                                                                  */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : uiAddress                       消去を開始するアドレス                          */
/*            : uiLength                        消去するバイト数                                */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_SUCCESS                    正常終了                                        */
/*            : FROM_ERASE_ERROR                消去エラー                                      */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _FROM_SectorEraseCore(unsigned int uiAddress, unsigned int uiLength)
{
FLGPTN tFlgPtn         = 0;
int iRet               = FROM_ERASE_ERROR;
uint32_t i             = 0;
unsigned char ucStatus = 0x00;

    /* 書き込み許可 */
    FlexSPI_SetWriteEnableSequence(l_tDrvInfo.tpFlexSPIReg);    /* LUT設定 */
    iRet = FlexSPI_ExecCommand(l_tDrvInfo.tpFlexSPIReg, 0, 0);  /* コマンド実行 */
    if (iRet != FLEXSPI_E_SUCCESS) {
        iRet = FROM_ERASE_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* セクタ消去 */
    FlexSPI_SetErase4KBSectorSequence(l_tDrvInfo.tpFlexSPIReg);                 /* LUT設定 */
    iRet = FlexSPI_ExecCommand(l_tDrvInfo.tpFlexSPIReg, uiAddress, uiLength);   /* コマンド実行 */
    if (iRet != FLEXSPI_E_SUCCESS) {
        iRet = FROM_ERASE_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 消去完了待ち */
    FlexSPI_SetReadStatusSequence(l_tDrvInfo.tpFlexSPIReg);                     /* LUT設定 */
    for (i = 0; i < 1000U; i++) {
        /* ステータス読み出し */
        ucStatus = FlexSPI_ExecCommandAndRead(l_tDrvInfo.tpFlexSPIReg, 0, 0x40001);
        if (ucStatus == 0xFF) {
            iRet = FROM_ERASE_ERROR;
            goto err_end;
        }
        else {
            ;   /* do nothing */
        }

        /* ステータスチェック(b0:write in progress ビットが'0'なら完了) */
        if ((ucStatus & 0x01) == 0) {
            break;
        }
        else {
            /* ウエイト */
            twai_flg(l_tDrvInfo.tFlgID, FROM_EVFBIT_WAIT, TWF_ORW, &tFlgPtn, 1);
        }
    }
    if (i == 1000U) {
        iRet = FROM_ERASE_ERROR;
        goto err_end;
    }
    else {
        iRet = FROM_SUCCESS;
    }
err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : _FROM_BlockEraseCore                                                            */
/*                                                                                              */
/* DESCRIPTION: ブロック消去処理                                                                */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : uiAddress                       消去を開始するアドレス                          */
/*            : uiLength                        消去するバイト数                                */
/*                                                                                              */
/* OUTPUT     : none                            なし                                            */
/*                                                                                              */
/* RESULTS    : FROM_SUCCESS                    正常終了                                        */
/*            : FROM_ERASE_ERROR                消去エラー                                      */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _FROM_BlockEraseCore(unsigned int uiAddress, unsigned int uiLength)
{
FLGPTN tFlgPtn         = 0;
int iRet               = FROM_ERASE_ERROR;
uint32_t i             = 0;
unsigned char ucStatus = 0x00;

    /* 書き込み許可 */
    FlexSPI_SetWriteEnableSequence(l_tDrvInfo.tpFlexSPIReg);    /* LUT設定 */
    iRet = FlexSPI_ExecCommand(l_tDrvInfo.tpFlexSPIReg, 0, 0);  /* コマンド実行 */
    if (iRet != FLEXSPI_E_SUCCESS) {
        iRet = FROM_ERASE_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* ブロック消去 */
    FlexSPI_SetErase32KBSectorSequence(l_tDrvInfo.tpFlexSPIReg);                /* LUT設定 */
    iRet = FlexSPI_ExecCommand(l_tDrvInfo.tpFlexSPIReg, uiAddress, uiLength);   /* コマンド実行 */
    if (iRet != FLEXSPI_E_SUCCESS) {
        iRet = FROM_ERASE_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* 消去完了待ち */
    FlexSPI_SetReadStatusSequence(l_tDrvInfo.tpFlexSPIReg);                     /* LUT設定 */
    for (i = 0; i < 1000U; i++) {
        /* ステータス読み出し */
        ucStatus = FlexSPI_ExecCommandAndRead(l_tDrvInfo.tpFlexSPIReg, 0, 0);
        if (ucStatus == 0xFF) {
            iRet = FROM_ERASE_ERROR;
            goto err_end;
        }
        else {
            ;   /* do nothing */
        }

        /* ステータスチェック(b0:write in progress ビットが'0'なら完了) */
        if ((ucStatus & 0x01) == 0) {
            break;
        }
        else {
            /* ウエイト */
            twai_flg(l_tDrvInfo.tFlgID, FROM_EVFBIT_WAIT, TWF_ORW, &tFlgPtn, 1);
        }
    }
    if (i == 1000U) {
        iRet = FROM_ERASE_ERROR;
        goto err_end;
    }
    else {
        iRet = FROM_SUCCESS;
    }
err_end:
    return iRet;
}

#ifdef _DEBUG
/****************************************************************************/
/*  デバッグ用                                                              */
/****************************************************************************/
/************************************************************************************************/
/* FUNCTION   : FROM_GetDriverData                                                              */
/*                                                                                              */
/* DESCRIPTION: ドライバデータ取得                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                                                                            */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : ドライバデータ情報                                                              */
/*                                                                                              */
/************************************************************************************************/
void * FROM_GetDriverData(void)
{
    return (void*)&l_tDrvInfo;
}

#endif  /* _DEBUG */

/************************* 以下は、動作確認用の関数  正式版のドライバでは不要 **********/

#if 0
/**
 * @fn FROM_ReadMap
 * @brief マイコンにMAPされたFROMのデータを読み出し
 * @param unsigned int uiAddress データ読み込みを開始するアドレス
 * @param unsinged int uiLength データを読み込む長さ（byte)
 * @param unsigned char *strReadData 読み込んだデータを格納する領域
 * @return int 読み込みの成功・失敗を区別できる戻り値
 * @note
 *
 */
int FROM_ReadMap( unsigned int uiFlashAddress, unsigned int uiLength, unsigned int uiSdramAddress )
{
    s_flexspi_reg->FLSHCR2[0] = 1;      // Flash Control Register 2 (FLSHA1CR2)に、シーケンス番号セット

    memcpy( (unsigned char *)uiSdramAddress, (0x08000000 + uiFlashAddress), uiLength );

    return 0;
}


void sil_wrw_mem(uint32_t *mem, uint32_t data)
{
    *((volatile uint32_t *) mem) = data;
};



int FROM_MapSet( void )
{
    int i;

    printf( "SET SPI MAP\r\n" );

    // RXCLKSRCセット
    s_flexspi_reg->MCR0 = (0x2 << FlexSPI_MCR0_RXCLKSRC_SHIFT);
    do {
        // レジスタの変化待ち
    } while( (s_flexspi_reg->MCR0 & 0x01) != 0 );

    fspi_module_disable(1);

    // AHBGRANTWAITとIPGRANTWAITに、デフォルト値をセット
    s_flexspi_reg->MCR0 = 0xFFFF0000;

    // FLASHのサイズをセット(KB単位)
    s_flexspi_reg->FLSHCR0[0] = FROM_CHIP_SIZE / 1024;  // FLSHA1CR0
    // 接続しているFLASHは１個のみなので、FLSHA1CR0以外は0
    s_flexspi_reg->FLSHCR0[1] = 0;                      // FLSHA2CR0
    s_flexspi_reg->FLSHCR0[2] = 0;                      // FLSHB1CR0
    s_flexspi_reg->FLSHCR0[3] = 0;                      // FLSHB2CR0

    // LUTセット
    FlexSPI_SetQuadOutFastRdSequence(s_flexspi_reg);

    /******** fspi_init_ahb_read() *************/

    for(i=0; i<7; i++ ) {
        s_flexspi_reg->AHBRXBUFCR0[i] = 0;
    }

    //
    s_flexspi_reg->AHBRXBUFCR0[7] = (FLEXSPI_AHB_BUFFER_SIZE / 8) | FlexSPI_AHBRXBUFCR0_PREFETCHEN_MASK;

    // AHB Read Prefetch Enable
    s_flexspi_reg->AHBCR = FlexSPI_AHBCR_PREFETCHEN_MASK;

    s_flexspi_reg->FLSHCR2[0] = 1;      // Flash Control Register 2 (FLSHA1CR2)に、シーケンス番号セット

//  PRINT2("LUT[4]:0x%x  ", s_flexspi_reg->LUT[4] );
//  PRINT2("LUT[5]:0x%x\r\n", s_flexspi_reg->LUT[5] );

    fspi_module_disable(0);

    return 0;
}

void fspi_module_disable( unsigned char disable )
{
    unsigned long mcr_val;

    mcr_val = s_flexspi_reg->MCR0;
    if (disable != 0) {
        mcr_val |= FlexSPI_MCR0_MDIS_MASK;
    }
    else {
        mcr_val &= ~FlexSPI_MCR0_MDIS_MASK;
    }

    s_flexspi_reg->MCR0 |= mcr_val;
}
#endif
