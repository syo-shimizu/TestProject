/************************************************************************************************/
/*                                                                                              */
/* FILE NAME                                                                    VERSION         */
/*                                                                                              */
/*      dri_pmic.c                                                              0.00            */
/*                                                                                              */
/* DESCRIPTION:                                                                                 */
/*                                                                                              */
/*      PMICドライバソースファイル                                                              */
/*                                                                                              */
/* HISTORY                                                                                      */
/*                                                                                              */
/*   NAME           DATE        REMARKS                                                         */
/*                                                                                              */
/*   K.Hatano       2021/XX/XX  Version 0.00                                                    */
/*                              新規作成                                                        */
/************************************************************************************************/

/****************************************************************************/
/*  インクルードファイル                                                    */
/****************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

//#include "M7/OS/Kernel/Standard/inc/uC3sys.h"

//#include "Common/code_rules_def.h"
//#include "M7/include/driver/dri_i2c.h"
//#include "M7/include/driver/dri_pmic.h"
#include "uC3sys.h"
#include "code_rules_def.h"
#include "i2c_drv.h"
#include "dri_pmic.h"


/****************************************************************************/
/*  定数・マクロ定義                                                        */
/****************************************************************************/

/* PMICのI2Cスレーブアドレス */
#define PMIC_I2C_ADDRESS    (0x25)

/* I2C入出力時に使用されるキューバッファ領域長 */
#define PMIC_I2C_TX_BUF     (8)
#define PMIC_I2C_RX_BUF     (8)

/* 送信時のデータバッファ長 */
#define PMIC_MAX_TXDATA     (2)

/* 入出力完了タイムアウト */
#define PMIC_IO_TIMEOUT     (60000)

/* I2Cエラーチェックマスク */
#define PIMC_i2C_ERROR_MASK (I2C_ERROR_CAUSE_RECV_AL    | \
                             I2C_ERROR_CAUSE_RECV_ATHER | \
                             I2C_ERROR_CAUSE_SEND_AL    | \
                             I2C_ERROR_CAUSE_SEND_NACK  | \
                             I2C_ERROR_CAUSE_SEND_ATHER | \
                             I2C_ERROR_CAUSE_INT_TMOUT)

/****************************************************************************/
/*  構造体定義                                                              */
/****************************************************************************/

/* PMICドライバ情報 */
typedef struct PMIC_DrvInfo_tag {
    ID              tSemLockID;                     /* セマフォID(入出力ロック) */
    ID              tSemIoID;                       /* セマフォID(入出力完了) */

    i2c_param_t     tI2CParam;                      /* I2C設定情報 */

    uint8_t         aucSendData[PMIC_MAX_TXDATA];   /* 入出力時のデータ(レジスタ&レジスタ値) */
    int             iCallbackStatus;                /* 送受信完了時のステータス */
} PMIC_DrvInfo;

/****************************************************************************/
/*  ローカルデータ                                                          */
/****************************************************************************/

/* QSPIドライバデータ */
DLOCAL PMIC_DrvInfo l_tDrvInfo = { 0 };

/* I2C入出力キューバッファ */ 
DLOCAL uint8_t      l_aucTxBuf[PMIC_I2C_TX_BUF];    /* 送信時バッファ */
DLOCAL uint8_t      l_aucRxBuf[PMIC_I2C_RX_BUF];    /* 送信時バッファ */

int                 l_Pmic_I2C_Channel = PMIC_I2C_CH;

/****************************************************************************/
/*  ローカル関数宣言                                                        */
/****************************************************************************/

/* 入出力要求チェック */
LOCAL int32_t _PMIC_CheckRequest(uint32_t ulAddress, uint8_t *pucData);


/* 送受信完了コールバック */

/* 送信完了コールバック */
LOCAL int _PMIC_TxCallback(int iStatus);

/* 受信完了コールバック */
LOCAL int _PMIC_RxCallback(int iStatus);

/****************************************************************************/
/*  提供関数                                                                */
/****************************************************************************/

/* 初期化 */

/************************************************************************************************/
/* FUNCTION   : PMIC_Init                                                                       */
/*                                                                                              */
/* DESCRIPTION: 初期化                                                                          */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                                                                            */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : PMIC_E_SUCCESS                  正常終了                                        */
/*              PMIC_E_ERROR                    OSリソースの作成時にエラーが発生した            */
/*                                                                                              */
/************************************************************************************************/
int32_t PMIC_Init(void)
{
T_CSEM tCSem = { 0 };
int32_t lRet = PMIC_E_ERROR;
int iRet     = -1;

    /* ドライバデータ初期化 */
    memset((void*)&l_tDrvInfo, 0, sizeof(l_tDrvInfo));

    /* セマフォ作成 */
    tCSem.sematr  = (TA_HLNG | TA_TFIFO);
    tCSem.isemcnt = 1;
    tCSem.maxsem  = 1;
    tCSem.name    = "PMIC Lock";
    l_tDrvInfo.tSemLockID = acre_sem(&tCSem);
    if (l_tDrvInfo.tSemLockID < 0) {
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    tCSem.sematr  = (TA_HLNG | TA_TFIFO);
    tCSem.isemcnt = 0;
    tCSem.maxsem  = 1;
    tCSem.name    = "PMIC IO";
    l_tDrvInfo.tSemIoID = acre_sem(&tCSem);
    if (l_tDrvInfo.tSemIoID < 0) {
        goto err_end1;
    }
    else {
        ;   /* do nothing */
    }

    /* I2Cドライバオープン */
    l_tDrvInfo.tI2CParam.rate        = /*I2C_RATE_400;        / * 400kHz */
                                       I2C_RATE_200;        /* (1) 200kHz */
                                       /* IMX8MP_P33A_Errata_Rev0.2.pdf : ERR007805 により 200kHzを使用 */
    l_tDrvInfo.tI2CParam.rxbuf_ptr   = (unsigned char*)l_aucRxBuf;
    l_tDrvInfo.tI2CParam.rxbuf_size  = PMIC_I2C_RX_BUF;
    l_tDrvInfo.tI2CParam.rx_callback = _PMIC_RxCallback;
    l_tDrvInfo.tI2CParam.txbuf_ptr   = (unsigned char*)l_aucTxBuf;
    l_tDrvInfo.tI2CParam.txbuf_size  = PMIC_I2C_TX_BUF;
    l_tDrvInfo.tI2CParam.tx_callback = _PMIC_TxCallback;
    /*iRet = I2C_Open((int)PMIC_I2C_ADDRESS, &l_tDrvInfo.tI2CParam); s.shimizu del */
    iRet = I2C_Open( l_Pmic_I2C_Channel, &l_tDrvInfo.tI2CParam);
    if (iRet != 0) {
        goto err_end2;
    }

    lRet = PMIC_E_SUCCESS;
    return lRet;

err_end2:
    /* セマフォ削除 */
    del_sem(l_tDrvInfo.tSemIoID);

err_end1:
    /* セマフォ削除 */
    del_sem(l_tDrvInfo.tSemLockID);

err_end:
    return lRet;
}

/* 読み出し・書き込み */

/************************************************************************************************/
/* FUNCTION   : PMIC_Read                                                                       */
/*                                                                                              */
/* DESCRIPTION: 読み出し                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ulAddress                       PMICレジスタアドレス                            */
/*                                                                                              */
/* OUTPUT     : pucData                         読み出しデータ格納バッファ                      */
/*                                                                                              */
/* RESULTS    : PMIC_E_SUCCESS                  正常終了                                        */
/*              PMIC_E_ERROR                    I2C通信エラー                                   */
/*              PMIC_E_PARAM                    パラメータに誤りがある                          */
/*                                                                                              */
/************************************************************************************************/
int32_t PMIC_Read(uint32_t ulAddress, uint8_t *pucData)
{
int32_t lRet = PMIC_E_ERROR;
ER tRet      = E_SYS;
int     iRet     = -1;
int     iReadSize = 0;

    /* パラメータチェック */
    lRet = _PMIC_CheckRequest(ulAddress, pucData);
    if (lRet != PMIC_E_SUCCESS) {
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* セマフォによるロック */
    tRet = wai_sem(l_tDrvInfo.tSemLockID);
    if (tRet != E_OK) {
        lRet = PMIC_E_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    l_tDrvInfo.iCallbackStatus = 0;
    /* 送受信パラメータ設定 */
    l_tDrvInfo.aucSendData[0] = ulAddress & 0xFF;

    /* I2Cロック */
    I2C_Lock(l_Pmic_I2C_Channel);

    iReadSize = 1;
    /* I2C送受信要求 */
    iRet = I2C_Send_RecvStart(l_Pmic_I2C_Channel, (int)PMIC_I2C_ADDRESS,
                              (unsigned char*)&l_tDrvInfo.aucSendData,
                              1,
                              iReadSize);
    if (iRet < 0) {
        lRet = PMIC_E_ERROR;
        goto err_end1;
    }
    else {
        ;   /* do nothing */
    }

    /* 入出力完了セマフォ待ち */
    tRet = twai_sem(l_tDrvInfo.tSemIoID, (TMO)PMIC_IO_TIMEOUT);
    if (tRet < E_OK) {
        lRet = PMIC_E_ERROR;
        goto err_end1;
    }
    else {
        /* 送受信完了ステータスチェック */
        if ((l_tDrvInfo.iCallbackStatus & PIMC_i2C_ERROR_MASK) != 0) {
            lRet = PMIC_E_ERROR;
            goto err_end1;
        }
        else {
            ;   /* do nothing */
        }
    }

    /* I2Cデータ受信 */
    iRet = I2C_Recv(l_Pmic_I2C_Channel, (int)PMIC_I2C_ADDRESS, 
                      (unsigned char*)pucData, iReadSize );
    if (iRet < 0) {
        lRet = PMIC_E_ERROR;
        goto err_end1;
    }
    else {
        lRet = PMIC_E_SUCCESS;  /* 正常終了 */
    }

err_end1:
    /* I2Cアンロック */
    I2C_UnLock(l_Pmic_I2C_Channel);

    /* セマフォアンロック */
    sig_sem(l_tDrvInfo.tSemLockID);

err_end:
    return lRet;
}

/************************************************************************************************/
/* FUNCTION   : PMIC_Write                                                                      */
/*                                                                                              */
/* DESCRIPTION: 書き込み                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ulAddress                       PMICレジスタアドレス                            */
/*              pucData                         書き込みデータバッファ                          */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : PMIC_E_SUCCESS                  正常終了                                        */
/*              PMIC_E_ERROR                    I2C通信エラー                                   */
/*              PMIC_E_PARAM                    パラメータに誤りがある                          */
/*                                                                                              */
/************************************************************************************************/
int32_t PMIC_Write(uint32_t ulAddress, uint8_t *pucData)
{
int32_t lRet = PMIC_E_ERROR;
ER tRet      = E_SYS;
int iRet     = -1;

    /* パラメータチェック */
    lRet = _PMIC_CheckRequest(ulAddress, pucData);
    if (lRet != PMIC_E_SUCCESS) {
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* セマフォによるロック */
    tRet = wai_sem(l_tDrvInfo.tSemLockID);
    if (tRet != E_OK) {
        lRet = PMIC_E_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    l_tDrvInfo.iCallbackStatus = 0;
    /* 送受信パラメータ設定 */
    l_tDrvInfo.aucSendData[0] = ulAddress & 0xFF;
    l_tDrvInfo.aucSendData[1] = *pucData;

    /* I2Cロック */
    I2C_Lock(l_Pmic_I2C_Channel);

    /* I2C送信要求 */
    iRet = I2C_Send(l_Pmic_I2C_Channel, (int)PMIC_I2C_ADDRESS,
                    (const unsigned char*)&l_tDrvInfo.aucSendData,
                    2);
    if (iRet < 0) {
        lRet = PMIC_E_ERROR;
        goto err_end1;
    }
    else {
        ;   /* do nothing */
    }

    /* 入出力完了セマフォ待ち */
    tRet = twai_sem(l_tDrvInfo.tSemIoID, (TMO)PMIC_IO_TIMEOUT);
    if (tRet != E_OK) {
        lRet = PMIC_E_ERROR;
        goto err_end1;
    }
    else {
        /* 送信完了ステータスチェック */
        if ((l_tDrvInfo.iCallbackStatus & PIMC_i2C_ERROR_MASK) != 0) {
            lRet = PMIC_E_ERROR;
            goto err_end1;
        }
        else {
            lRet = PMIC_E_SUCCESS;  /* 正常終了 */
        }
    }

err_end1:
    /* I2Cアンロック */
    I2C_UnLock(l_Pmic_I2C_Channel);

    /* セマフォアンロック */
    sig_sem(l_tDrvInfo.tSemLockID);

err_end:
    return lRet;
}

/****************************************************************************/
/*  ローカル関数                                                            */
/****************************************************************************/
/************************************************************************************************/
/* FUNCTION   : _PMIC_CheckRequest                                                              */
/*                                                                                              */
/* DESCRIPTION: 入出力要求チェック                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ulAddress                       PMICレジスタアドレス                            */
/*            : pucData                         入出力データバッファ                            */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : PMIC_E_SUCCESS                  正常終了                                        */
/*              PMIC_E_PARAM                    パラメータに誤りがある                          */
/*                                                                                              */
/************************************************************************************************/
LOCAL int32_t _PMIC_CheckRequest(uint32_t ulAddress, uint8_t* pucData)
{
int32_t lRet = PMIC_E_PARAM;

    /* PMICレジスタアドレスチェック */
    if ((((uint32_t)PMIC_O08_DEVICE_ID   <= ulAddress) && (ulAddress <= (uint32_t)PMIC_O08_CONFIG2))    ||
        (((uint32_t)PMIC_O08_BUCK123_DVS <= ulAddress) && (ulAddress <= (uint32_t)PMIC_O08_BUCK6OUT))   ||
        (((uint32_t)PMIC_O08_LDO_AD_CTRL <= ulAddress) && (ulAddress <= (uint32_t)PMIC_O08_LDO5CTRL_H)) ||
        (((uint32_t)PMIC_O08_LOADSW_CTRL <= ulAddress) && (ulAddress <= (uint32_t)PMIC_O08_VRFLT2_MASK))) {
        ;   /* do nothing */
    }
    else {
        goto err_end;
    }

    /* データバッファチェック */
    if (pucData != NULL) {
        ;   /* do nothing */
    }
    else {
        goto err_end;
    }

    lRet = PMIC_E_SUCCESS;  /* 正常終了 */

err_end:
    return lRet;
}

/* I2C送信・受信コールバック */

/************************************************************************************************/
/* FUNCTION   : _PMIC_TxCallback                                                                */
/*                                                                                              */
/* DESCRIPTION: I2Cドライバからの送信完了コールバック                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : iStatus                         送信完了ステータス                              */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                               常にゼロ                                        */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _PMIC_TxCallback(int iStatus)
{
    /* 送信完了ステータス保存 */
    l_tDrvInfo.iCallbackStatus = iStatus;

    /* 入出力完了セマフォをポスト */
    isig_sem(l_tDrvInfo.tSemIoID);

    return 0;
}

/************************************************************************************************/
/* FUNCTION   : _PMIC_RxCallback                                                                */
/*                                                                                              */
/* DESCRIPTION: I2Cドライバからの受信完了コールバック                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : iStatus                         受信完了ステータス                              */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                               常にゼロ                                        */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _PMIC_RxCallback(int iStatus)
{
    /* 受信完了ステータス保存 */
    l_tDrvInfo.iCallbackStatus = iStatus;

    /* 入出力完了セマフォをポスト */
    isig_sem(l_tDrvInfo.tSemIoID);

    return 0;
}
