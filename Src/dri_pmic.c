/************************************************************************************************/
/*                                                                                              */
/* FILE NAME                                                                    VERSION         */
/*                                                                                              */
/*      dri_pmic.c                                                              0.00            */
/*                                                                                              */
/* DESCRIPTION:                                                                                 */
/*                                                                                              */
/*      PMIC�h���C�o�\�[�X�t�@�C��                                                              */
/*                                                                                              */
/* HISTORY                                                                                      */
/*                                                                                              */
/*   NAME           DATE        REMARKS                                                         */
/*                                                                                              */
/*   K.Hatano       2021/XX/XX  Version 0.00                                                    */
/*                              �V�K�쐬                                                        */
/************************************************************************************************/

/****************************************************************************/
/*  �C���N���[�h�t�@�C��                                                    */
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
/*  �萔�E�}�N����`                                                        */
/****************************************************************************/

/* PMIC��I2C�X���[�u�A�h���X */
#define PMIC_I2C_ADDRESS    (0x25)

/* I2C���o�͎��Ɏg�p�����L���[�o�b�t�@�̈撷 */
#define PMIC_I2C_TX_BUF     (8)
#define PMIC_I2C_RX_BUF     (8)

/* ���M���̃f�[�^�o�b�t�@�� */
#define PMIC_MAX_TXDATA     (2)

/* ���o�͊����^�C���A�E�g */
#define PMIC_IO_TIMEOUT     (60000)

/* I2C�G���[�`�F�b�N�}�X�N */
#define PIMC_i2C_ERROR_MASK (I2C_ERROR_CAUSE_RECV_AL    | \
                             I2C_ERROR_CAUSE_RECV_ATHER | \
                             I2C_ERROR_CAUSE_SEND_AL    | \
                             I2C_ERROR_CAUSE_SEND_NACK  | \
                             I2C_ERROR_CAUSE_SEND_ATHER | \
                             I2C_ERROR_CAUSE_INT_TMOUT)

/****************************************************************************/
/*  �\���̒�`                                                              */
/****************************************************************************/

/* PMIC�h���C�o��� */
typedef struct PMIC_DrvInfo_tag {
    ID              tSemLockID;                     /* �Z�}�t�HID(���o�̓��b�N) */
    ID              tSemIoID;                       /* �Z�}�t�HID(���o�͊���) */

    i2c_param_t     tI2CParam;                      /* I2C�ݒ��� */

    uint8_t         aucSendData[PMIC_MAX_TXDATA];   /* ���o�͎��̃f�[�^(���W�X�^&���W�X�^�l) */
    int             iCallbackStatus;                /* ����M�������̃X�e�[�^�X */
} PMIC_DrvInfo;

/****************************************************************************/
/*  ���[�J���f�[�^                                                          */
/****************************************************************************/

/* QSPI�h���C�o�f�[�^ */
DLOCAL PMIC_DrvInfo l_tDrvInfo = { 0 };

/* I2C���o�̓L���[�o�b�t�@ */ 
DLOCAL uint8_t      l_aucTxBuf[PMIC_I2C_TX_BUF];    /* ���M���o�b�t�@ */
DLOCAL uint8_t      l_aucRxBuf[PMIC_I2C_RX_BUF];    /* ���M���o�b�t�@ */

int                 l_Pmic_I2C_Channel = PMIC_I2C_CH;

/****************************************************************************/
/*  ���[�J���֐��錾                                                        */
/****************************************************************************/

/* ���o�͗v���`�F�b�N */
LOCAL int32_t _PMIC_CheckRequest(uint32_t ulAddress, uint8_t *pucData);


/* ����M�����R�[���o�b�N */

/* ���M�����R�[���o�b�N */
LOCAL int _PMIC_TxCallback(int iStatus);

/* ��M�����R�[���o�b�N */
LOCAL int _PMIC_RxCallback(int iStatus);

/****************************************************************************/
/*  �񋟊֐�                                                                */
/****************************************************************************/

/* ������ */

/************************************************************************************************/
/* FUNCTION   : PMIC_Init                                                                       */
/*                                                                                              */
/* DESCRIPTION: ������                                                                          */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                                                                            */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : PMIC_E_SUCCESS                  ����I��                                        */
/*              PMIC_E_ERROR                    OS���\�[�X�̍쐬���ɃG���[����������            */
/*                                                                                              */
/************************************************************************************************/
int32_t PMIC_Init(void)
{
T_CSEM tCSem = { 0 };
int32_t lRet = PMIC_E_ERROR;
int iRet     = -1;

    /* �h���C�o�f�[�^������ */
    memset((void*)&l_tDrvInfo, 0, sizeof(l_tDrvInfo));

    /* �Z�}�t�H�쐬 */
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

    /* I2C�h���C�o�I�[�v�� */
    l_tDrvInfo.tI2CParam.rate        = /*I2C_RATE_400;        / * 400kHz */
                                       I2C_RATE_200;        /* (1) 200kHz */
                                       /* IMX8MP_P33A_Errata_Rev0.2.pdf : ERR007805 �ɂ�� 200kHz���g�p */
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
    /* �Z�}�t�H�폜 */
    del_sem(l_tDrvInfo.tSemIoID);

err_end1:
    /* �Z�}�t�H�폜 */
    del_sem(l_tDrvInfo.tSemLockID);

err_end:
    return lRet;
}

/* �ǂݏo���E�������� */

/************************************************************************************************/
/* FUNCTION   : PMIC_Read                                                                       */
/*                                                                                              */
/* DESCRIPTION: �ǂݏo��                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ulAddress                       PMIC���W�X�^�A�h���X                            */
/*                                                                                              */
/* OUTPUT     : pucData                         �ǂݏo���f�[�^�i�[�o�b�t�@                      */
/*                                                                                              */
/* RESULTS    : PMIC_E_SUCCESS                  ����I��                                        */
/*              PMIC_E_ERROR                    I2C�ʐM�G���[                                   */
/*              PMIC_E_PARAM                    �p�����[�^�Ɍ�肪����                          */
/*                                                                                              */
/************************************************************************************************/
int32_t PMIC_Read(uint32_t ulAddress, uint8_t *pucData)
{
int32_t lRet = PMIC_E_ERROR;
ER tRet      = E_SYS;
int     iRet     = -1;
int     iReadSize = 0;

    /* �p�����[�^�`�F�b�N */
    lRet = _PMIC_CheckRequest(ulAddress, pucData);
    if (lRet != PMIC_E_SUCCESS) {
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* �Z�}�t�H�ɂ�郍�b�N */
    tRet = wai_sem(l_tDrvInfo.tSemLockID);
    if (tRet != E_OK) {
        lRet = PMIC_E_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    l_tDrvInfo.iCallbackStatus = 0;
    /* ����M�p�����[�^�ݒ� */
    l_tDrvInfo.aucSendData[0] = ulAddress & 0xFF;

    /* I2C���b�N */
    I2C_Lock(l_Pmic_I2C_Channel);

    iReadSize = 1;
    /* I2C����M�v�� */
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

    /* ���o�͊����Z�}�t�H�҂� */
    tRet = twai_sem(l_tDrvInfo.tSemIoID, (TMO)PMIC_IO_TIMEOUT);
    if (tRet < E_OK) {
        lRet = PMIC_E_ERROR;
        goto err_end1;
    }
    else {
        /* ����M�����X�e�[�^�X�`�F�b�N */
        if ((l_tDrvInfo.iCallbackStatus & PIMC_i2C_ERROR_MASK) != 0) {
            lRet = PMIC_E_ERROR;
            goto err_end1;
        }
        else {
            ;   /* do nothing */
        }
    }

    /* I2C�f�[�^��M */
    iRet = I2C_Recv(l_Pmic_I2C_Channel, (int)PMIC_I2C_ADDRESS, 
                      (unsigned char*)pucData, iReadSize );
    if (iRet < 0) {
        lRet = PMIC_E_ERROR;
        goto err_end1;
    }
    else {
        lRet = PMIC_E_SUCCESS;  /* ����I�� */
    }

err_end1:
    /* I2C�A�����b�N */
    I2C_UnLock(l_Pmic_I2C_Channel);

    /* �Z�}�t�H�A�����b�N */
    sig_sem(l_tDrvInfo.tSemLockID);

err_end:
    return lRet;
}

/************************************************************************************************/
/* FUNCTION   : PMIC_Write                                                                      */
/*                                                                                              */
/* DESCRIPTION: ��������                                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ulAddress                       PMIC���W�X�^�A�h���X                            */
/*              pucData                         �������݃f�[�^�o�b�t�@                          */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : PMIC_E_SUCCESS                  ����I��                                        */
/*              PMIC_E_ERROR                    I2C�ʐM�G���[                                   */
/*              PMIC_E_PARAM                    �p�����[�^�Ɍ�肪����                          */
/*                                                                                              */
/************************************************************************************************/
int32_t PMIC_Write(uint32_t ulAddress, uint8_t *pucData)
{
int32_t lRet = PMIC_E_ERROR;
ER tRet      = E_SYS;
int iRet     = -1;

    /* �p�����[�^�`�F�b�N */
    lRet = _PMIC_CheckRequest(ulAddress, pucData);
    if (lRet != PMIC_E_SUCCESS) {
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    /* �Z�}�t�H�ɂ�郍�b�N */
    tRet = wai_sem(l_tDrvInfo.tSemLockID);
    if (tRet != E_OK) {
        lRet = PMIC_E_ERROR;
        goto err_end;
    }
    else {
        ;   /* do nothing */
    }

    l_tDrvInfo.iCallbackStatus = 0;
    /* ����M�p�����[�^�ݒ� */
    l_tDrvInfo.aucSendData[0] = ulAddress & 0xFF;
    l_tDrvInfo.aucSendData[1] = *pucData;

    /* I2C���b�N */
    I2C_Lock(l_Pmic_I2C_Channel);

    /* I2C���M�v�� */
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

    /* ���o�͊����Z�}�t�H�҂� */
    tRet = twai_sem(l_tDrvInfo.tSemIoID, (TMO)PMIC_IO_TIMEOUT);
    if (tRet != E_OK) {
        lRet = PMIC_E_ERROR;
        goto err_end1;
    }
    else {
        /* ���M�����X�e�[�^�X�`�F�b�N */
        if ((l_tDrvInfo.iCallbackStatus & PIMC_i2C_ERROR_MASK) != 0) {
            lRet = PMIC_E_ERROR;
            goto err_end1;
        }
        else {
            lRet = PMIC_E_SUCCESS;  /* ����I�� */
        }
    }

err_end1:
    /* I2C�A�����b�N */
    I2C_UnLock(l_Pmic_I2C_Channel);

    /* �Z�}�t�H�A�����b�N */
    sig_sem(l_tDrvInfo.tSemLockID);

err_end:
    return lRet;
}

/****************************************************************************/
/*  ���[�J���֐�                                                            */
/****************************************************************************/
/************************************************************************************************/
/* FUNCTION   : _PMIC_CheckRequest                                                              */
/*                                                                                              */
/* DESCRIPTION: ���o�͗v���`�F�b�N                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ulAddress                       PMIC���W�X�^�A�h���X                            */
/*            : pucData                         ���o�̓f�[�^�o�b�t�@                            */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : PMIC_E_SUCCESS                  ����I��                                        */
/*              PMIC_E_PARAM                    �p�����[�^�Ɍ�肪����                          */
/*                                                                                              */
/************************************************************************************************/
LOCAL int32_t _PMIC_CheckRequest(uint32_t ulAddress, uint8_t* pucData)
{
int32_t lRet = PMIC_E_PARAM;

    /* PMIC���W�X�^�A�h���X�`�F�b�N */
    if ((((uint32_t)PMIC_O08_DEVICE_ID   <= ulAddress) && (ulAddress <= (uint32_t)PMIC_O08_CONFIG2))    ||
        (((uint32_t)PMIC_O08_BUCK123_DVS <= ulAddress) && (ulAddress <= (uint32_t)PMIC_O08_BUCK6OUT))   ||
        (((uint32_t)PMIC_O08_LDO_AD_CTRL <= ulAddress) && (ulAddress <= (uint32_t)PMIC_O08_LDO5CTRL_H)) ||
        (((uint32_t)PMIC_O08_LOADSW_CTRL <= ulAddress) && (ulAddress <= (uint32_t)PMIC_O08_VRFLT2_MASK))) {
        ;   /* do nothing */
    }
    else {
        goto err_end;
    }

    /* �f�[�^�o�b�t�@�`�F�b�N */
    if (pucData != NULL) {
        ;   /* do nothing */
    }
    else {
        goto err_end;
    }

    lRet = PMIC_E_SUCCESS;  /* ����I�� */

err_end:
    return lRet;
}

/* I2C���M�E��M�R�[���o�b�N */

/************************************************************************************************/
/* FUNCTION   : _PMIC_TxCallback                                                                */
/*                                                                                              */
/* DESCRIPTION: I2C�h���C�o����̑��M�����R�[���o�b�N                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : iStatus                         ���M�����X�e�[�^�X                              */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                               ��Ƀ[��                                        */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _PMIC_TxCallback(int iStatus)
{
    /* ���M�����X�e�[�^�X�ۑ� */
    l_tDrvInfo.iCallbackStatus = iStatus;

    /* ���o�͊����Z�}�t�H���|�X�g */
    isig_sem(l_tDrvInfo.tSemIoID);

    return 0;
}

/************************************************************************************************/
/* FUNCTION   : _PMIC_RxCallback                                                                */
/*                                                                                              */
/* DESCRIPTION: I2C�h���C�o����̎�M�����R�[���o�b�N                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : iStatus                         ��M�����X�e�[�^�X                              */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                               ��Ƀ[��                                        */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _PMIC_RxCallback(int iStatus)
{
    /* ��M�����X�e�[�^�X�ۑ� */
    l_tDrvInfo.iCallbackStatus = iStatus;

    /* ���o�͊����Z�}�t�H���|�X�g */
    isig_sem(l_tDrvInfo.tSemIoID);

    return 0;
}
