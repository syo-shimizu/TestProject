/************************************************************************************************/
/*                                                                                              */
/* FILE NAME                                                                    VERSION         */
/*                                                                                              */
/*      i2c_drv.c                                                               0.00            */
/*                                                                                              */
/* DESCRIPTION:                                                                                 */
/*                                                                                              */
/*      I2C driver implimentation file                                                          */
/*                                                                                              */
/* HISTORY                                                                                      */
/*                                                                                              */
/*      NAME            DATE        REMARKS                                                     */
/*                                                                                              */
/*      0.00            2021/01/13  Version 0.00                                                */
/*                                  Created prototype version 0.00                              */
/*                                                                                              */
/************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "kernel.h"
#include "ARMv7M.h"
#include "imx8m_reg.h"
#include "code_rules_def.h"
#include "i2c_drv.h"
#include "i2c_drv_local.h"

/********************************/
/* static prototype definitions */
LOCAL int32_t I2C_SetData( int32_t ch, const i2c_param_t* param );
LOCAL int32_t I2C_Prepare( int32_t ch, int32_t adr );
LOCAL void I2C_handler( int vec );
LOCAL void I2C_TimerCallBack(int timid);
LOCAL void I2C_QueueDataInvalid( QUEUE_DATA* pQd );
LOCAL int  I2C_MuxPinSet( int ch );
LOCAL void  I2C1_IntrEntry( void );
LOCAL void  I2C2_IntrEntry( void );
LOCAL void  I2C3_IntrEntry( void );
LOCAL void  I2C4_IntrEntry( void );
LOCAL void  I2C5_IntrEntry( void );
LOCAL void  I2C6_IntrEntry( void );
LOCAL int  I2C_MasterStop(int ch);
unsigned long I2C_ProcessTime( SYSTIM start_time, SYSTIM end_time );
void I2C_wait(int time);



/*************************************************************************/
/* Internal valiable                                                     */
/*************************************************************************/
/* Valiables */
DLOCAL  ID                      s_semid_i2c[I2C_CH_NUM] = {0};      /* I2C セマフォのID番号 */
DLOCAL  ID                      s_almid_i2c[I2C_CH_NUM] = {0};      /* I2C アラームハンドラのID番号 */
DLOCAL  int32_t                 s_eIsInitialized = FALSE;           /* 初期化完了フラグ     */
DLOCAL  int32_t                 s_eModuleEnable[I2C_CH_NUM] = {I2C_DISABLE};     /* モジュール有効/無効フラグ */
DLOCAL  int32_t                 s_eIsTranseive[I2C_CH_NUM]  = {FALSE};           /* 転送中フラグ              */
DLOCAL  int32_t                 s_eStatus[I2C_CH_NUM]       = {0};               /* 通信状態             */
/*    slave address */
DLOCAL  I2C_CH_DATA             s_eChData[I2C_CH_NUM]   = {0};

DLOCAL  int32_t                 s_ActiveCh      = 0;        /* アクティブチャンネル */
DLOCAL  ER                      s_ercd          = 0;        /* エラーコード         */

DLOCAL  ENQUEUE                 s_func_enqueue = enqueue_c;
DLOCAL  DEQUEUE                 s_func_dequeue = dequeue_c;

/***************************/
/*   I2Cレジスタアドレス   */
/***************************/
DLOCAL  I2C_REG_TBL             *s_i2c_register[I2C_CH_NUM] = {
    (I2C_REG_TBL *)IMX8M_I2C1_A32_BASE_ADDR,    /* I2C1 Controller Base Address */
    (I2C_REG_TBL *)IMX8M_I2C2_A32_BASE_ADDR,    /* I2C2 Controller Base Address */
    (I2C_REG_TBL *)IMX8M_I2C3_A32_BASE_ADDR,    /* I2C3 Controller Base Address */
    (I2C_REG_TBL *)IMX8M_I2C4_A32_BASE_ADDR,    /* I2C4 Controller Base Address */
    (I2C_REG_TBL *)IMX8M_I2C5_A32_BASE_ADDR,    /* I2C5 Controller Base Address */
    (I2C_REG_TBL *)IMX8M_I2C6_A32_BASE_ADDR     /* I2C6 Controller Base Address */
};

/*    I2C clock devider define    */
DLOCAL const DIV_TBL s_DivTbl[I2C_RATE_MAX] = {
#if 0
    { IMX8M_I2C_B06_IFDR_IC_26  },              /* 400K */
    { IMX8M_I2C_B06_IFDR_IC_52  },              /* 200K */
    { IMX8M_I2C_B06_IFDR_IC_104 },              /* 100K */
    { IMX8M_I2C_B06_IFDR_IC_224 },              /* 50K */
    { IMX8M_I2C_B06_IFDR_IC_448 }               /* 25K */
#endif
//    { IMX8M_I2C_B06_IFDR_IC_60  },              /* 400K */
//    { IMX8M_I2C_B06_IFDR_IC_128 },              /* 200K */
//    { IMX8M_I2C_B06_IFDR_IC_240 },              /* 100K */
//    { IMX8M_I2C_B06_IFDR_IC_480 },              /* 50K */
//    { IMX8M_I2C_B06_IFDR_IC_960 }               /* 25K */
    { 0x26 },              /* 400K */
    { 0x08 },              /* 200K */
    { 0x0D },              /* 100K */
    { 0x11 },              /* 50K */
    { 0x15 }               /* 25K */

};

/*********************************************************/
/*  I2C semapho object                                   */
/*********************************************************/
DLOCAL const T_CSEM l_tCsem1 = {
    (TA_HLNG | TA_TFIFO),       /* セマフォ属性 */
    1,                          /* セマフォの初期値  */
    1                           /* セマフォの最大値  */
    /* B *name;                 / *  セマフォ名へのポインタ（省略可） */
}; 

#define I2C_INT_MLEVEL                  (192)
#define I2C1_INT_MLEVEL                 I2C_INT_MLEVEL
#define I2C2_INT_MLEVEL                 I2C_INT_MLEVEL
#define I2C3_INT_MLEVEL                 I2C_INT_MLEVEL
#define I2C4_INT_MLEVEL                 I2C_INT_MLEVEL
#define I2C5_INT_MLEVEL                 I2C_INT_MLEVEL
#define I2C6_INT_MLEVEL                 I2C_INT_MLEVEL

#define I2C_STOP_BUSY_TIMER             (100000)

/*********************************************************/
/* I2C割り込みベクタ番号設定                             */
/*********************************************************/
DLOCAL  I2C_INTVECT_NO_FUNC        i2c_vect_num_tbl[I2C_CH_NUM] = {
    {   
    	{	TA_HLNG,				/* ATR    isratr 割込みサービスルーチン属性         */
    		(VP_INT)I2C_CH_1,		/* VP_INT exinf  割込みサービスルーチンの拡張情報   */
			IMX8M_I2C1_VECTOR,		/* INTNO  intno  割込みサービスルーチンを付加する割込み番号 */
			I2C1_IntrEntry,			/* FP     isr    割込みサービスルーチンの起動番地     */
			I2C1_INT_MLEVEL,		/* IMASK  imask  割込みサービスルーチンの割込みレベル */
    	},
    	IMX8M_I2C1_VECTOR, I2C1_IntrEntry, 0            /* I2C1割り込みベクタ番号,処理, 登録ID */
	},
    {	
    	{	TA_HLNG,				/* ATR    isratr 割込みサービスルーチン属性         */
			(VP_INT)I2C_CH_2,		/* VP_INT exinf  割込みサービスルーチンの拡張情報   */
			IMX8M_I2C2_VECTOR,		/* INTNO  intno  割込みサービスルーチンを付加する割込み番号 */
			I2C2_IntrEntry,			/* FP     isr    割込みサービスルーチンの起動番地     */
			I2C2_INT_MLEVEL,		/* IMASK  imask  割込みサービスルーチンの割込みレベル */
    	},
    	IMX8M_I2C2_VECTOR, I2C2_IntrEntry, 0            /* I2C2割り込みベクタ番号,処理, 登録ID */
	},
    {
    	{	TA_HLNG,				/* ATR    isratr 割込みサービスルーチン属性         */
			(VP_INT)I2C_CH_3,		/* VP_INT exinf  割込みサービスルーチンの拡張情報   */
			IMX8M_I2C3_VECTOR,		/* INTNO  intno  割込みサービスルーチンを付加する割込み番号 */
			I2C3_IntrEntry,			/* FP     isr    割込みサービスルーチンの起動番地     */
			I2C3_INT_MLEVEL,		/* IMASK  imask  割込みサービスルーチンの割込みレベル */
    	},
    	IMX8M_I2C3_VECTOR, I2C3_IntrEntry, 0            /* I2C3割り込みベクタ番号,処理, 登録ID */
	},
    {
    	{	TA_HLNG,				/* ATR    isratr 割込みサービスルーチン属性         */
			(VP_INT)I2C_CH_4,		/* VP_INT exinf  割込みサービスルーチンの拡張情報   */
			IMX8M_I2C4_VECTOR,		/* INTNO  intno  割込みサービスルーチンを付加する割込み番号 */
			I2C4_IntrEntry,			/* FP     isr    割込みサービスルーチンの起動番地     */
			I2C4_INT_MLEVEL,		/* IMASK  imask  割込みサービスルーチンの割込みレベル */
    	},
    	IMX8M_I2C4_VECTOR, I2C4_IntrEntry, 0            /* I2C4割り込みベクタ番号,処理, 登録ID */
	},
    {
    	{	TA_HLNG,				/* ATR    isratr 割込みサービスルーチン属性         */
			(VP_INT)I2C_CH_5,		/* VP_INT exinf  割込みサービスルーチンの拡張情報   */
			IMX8M_I2C5_VECTOR,		/* INTNO  intno  割込みサービスルーチンを付加する割込み番号 */
			I2C5_IntrEntry,			/* FP     isr    割込みサービスルーチンの起動番地     */
			I2C5_INT_MLEVEL,		/* IMASK  imask  割込みサービスルーチンの割込みレベル */
    	},
    	IMX8M_I2C5_VECTOR, I2C5_IntrEntry, 0            /* I2C5割り込みベクタ番号,処理, 登録ID */
	},
    {
    	{	TA_HLNG,				/* ATR    isratr 割込みサービスルーチン属性         */
			(VP_INT)I2C_CH_6,		/* VP_INT exinf  割込みサービスルーチンの拡張情報   */
			IMX8M_I2C6_VECTOR,		/* INTNO  intno  割込みサービスルーチンを付加する割込み番号 */
			I2C6_IntrEntry,			/* FP     isr    割込みサービスルーチンの起動番地     */
			I2C6_INT_MLEVEL,		/* IMASK  imask  割込みサービスルーチンの割込みレベル */
    	},
    	IMX8M_I2C6_VECTOR, I2C6_IntrEntry, 0             /* I2C6割り込みベクタ番号,処理, 登録ID */
	}
};


/*************************************************************************************/
/*   I2C Interrupt hander define object                                              */
/*************************************************************************************/
//DLOCAL       T_DINH  l_tDinh = {
//    TA_HLNG,                        /* ATR inhatr;  割込みハンドラ属性 */
//    0,                              /* FP  inthdr;  割込みハンドラとする関数へのポインタ */
//    0x20  /*192U*/                            /* UINT imask;  割込みマスク(一部のプロセッサのみ) */
//};

/*************************************************************************************/
/*   I2C Alarm hander define object                                                  */
/*************************************************************************************/
DLOCAL       T_CALM     l_tCalm = {
    TA_HLNG,                        /* ATR almatr; アラームハンドラ属性 */
    0,                              /* VP_INT exinf; 拡張情報           */
    0                               /* FP almhdr; アラームハンドラとする関数へのポインタ */
}; 

/*************************************************************************************/
/*   I2C_IOMUX                                                                       */
/*************************************************************************************/
DLOCAL       I2C_IOMUX   l_tMuxTbl[I2C_CH_NUM] = {
    { IMX8M_I2C_A32_I2C1_IOMUX_SCL, 0x10, IMX8M_I2C_A32_I2C1_IOMUX_SDA, 0x10 },
    { IMX8M_I2C_A32_I2C2_IOMUX_SCL, 0x10, IMX8M_I2C_A32_I2C2_IOMUX_SDA, 0x10 },
    { IMX8M_I2C_A32_I2C3_IOMUX_SCL, 0x10, IMX8M_I2C_A32_I2C3_IOMUX_SDA, 0x10 },
    { IMX8M_I2C_A32_I2C4_IOMUX_SCL, 0x10, IMX8M_I2C_A32_I2C4_IOMUX_SDA, 0x10 },
    {                            0,    0,                            0,    0 }, /* T.B.D. */
    {                            0,    0,                            0,    0 }  /* T.B.D. */
};

/************************************************************************************************/
/* FUNCTION   : I2C_Init                                                                        */
/*                                                                                              */
/* DESCRIPTION: Initialize I2C driver                                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                                                                            */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                       Successfully return                                     */
/*              -1                      Error return                                            */
/*                                                                                              */
/************************************************************************************************/
int I2C_Init( void )
{
int32_t                         iCh;
ID                              err;
ER_ID                           iErr_Id;
int                             iRet = 0;

    for( iCh = 0; iCh < I2C_CH_NUM; iCh++)
    {
        /* 1. 初期化済み状態(s_eIsInitialized==TRUE)である場合、そのまま正常終了(0)でreturnする */
        if(s_eIsInitialized == TRUE)
        {
            return 0;
        }
        else
        {
            /* nothing */
        }
        if(s_semid_i2c[iCh] == 0)         /* I2C セマフォは未作成かチェック */
        {
            /* 2. acre_semサービスコールを呼び出してセマフォを作成する */
            iErr_Id = acre_sem((T_CSEM *)&l_tCsem1);                       /* セマフォ生成 */ 
            if(iErr_Id <= E_OK)
            {
                iRet = -1;    /* = iErr_Id; */
                goto err_end;
            }
            else 
            {    /* セマフォのIDは、変数s_semid_i2cに保存する */
                s_semid_i2c[iCh] = iErr_Id;
            }
        }
        else 
        {
            /* nothing */
        }
        /* 3. CPUをロックする(loc_cpu()) */
        loc_cpu();

        /* 4. I2Cx_I2SRレジスタをクリアする */
        s_i2c_register[iCh]->I2SR &= ~(IMX8M_I2C_B01_I2SR_IAL | IMX8M_I2C_B01_I2SR_IIF);

        /* 5. I2Cx_I2CRレジスタをクリアする */
        s_i2c_register[iCh]->I2CR = 0x0000;

        /* 6. 割り込みハンドラ関数を登録する(def_inh()) */
//        l_tDinh.inthdr = i2c_vect_num_tbl[iCh].func;
//        err = def_inh(i2c_vect_num_tbl[iCh].int_no, &l_tDinh);
        err = acre_isr( &i2c_vect_num_tbl[iCh].t_Cisr);
        if ( err < E_OK)
        {
            iRet = -1;    /* ISR作成エラー */
            goto err_end;
        }
        else
        {
            i2c_vect_num_tbl[iCh].isrId = err;
        }

        /* 7. アラームハンドラ関数を登録する(acre_alm()) */
        l_tCalm.exinf = (VP_INT)iCh;
        l_tCalm.almhdr = (FP)I2C_TimerCallBack;
        err = acre_alm(&l_tCalm);
        if(err <= E_OK)
        {
            iRet = -1;          /* system error */
            break;
        }
        else
        {
            s_almid_i2c[iCh] = err;      /* I2C アラームハンドラのID番号 */
        }
        /* 8. CPUをアンロックする(unl_cpu()) */
        unl_cpu();

        /* 9. 初期化済みフラグ有効 */
        /* s_eIsInitialized    = TRUE; */
        /* 10. チャンネル単位の情報を初期化 */
        s_eStatus[iCh]           = I2C_STATUS_IDLE;
        s_eModuleEnable[iCh]     = I2C_DISABLE;
        s_eIsTranseive[iCh]      = FALSE;
        memset(&s_eChData[iCh], 0, sizeof(I2C_CH_DATA));
    }    /***  end of for() loop ***/
    if(iRet == E_OK)
    {
        s_eIsInitialized    = TRUE;    /* 初期化済みフラグ有効 */
    }
err_end:
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : I2C_Open                                                                        */
/*                                                                                              */
/* DESCRIPTION: Open I2C driver                                                                 */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      Using I2C channel no.                                   */
/*              param                   parameter for I2C Open                                  */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                       Successfully return                                     */
/*              -1                      Error return                                            */
/*                                                                                              */
/************************************************************************************************/
int I2C_Open( int ch, const i2c_param_t* param )
{
int                             iRet = 0;

    /* 1. 初期化されていない場合は、-1でreturnする(s_eIsInitialized == FALSE) */
    if( FALSE == s_eIsInitialized )
    {
        return -1;          /* no initialize error */
    } 
    else
    {
        /* nothing */
    }
    /* 2. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
    if((ch < 0) || (I2C_CH_NUM <= ch))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 3. param->rateの値が不正である場合は、-1でreturnする(表 2 3に示す値ではない) */
    if((param->rate < I2C_RATE_400) || (param->rate > I2C_RATE_25))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 4. param->rxbuf_sizeの値が正でない場合は、-1でreturnする(param->rxbuf_size < 1) */
    if(param->rxbuf_size < 1)
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 5. param->txbuf_sizeの値が正でない場合は、-1でreturnする(param->txbuf_size < 1) */
    if(param->txbuf_size < 1)
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 6. chに設定したI2Cチャンネルがオープン状態であるかどうかを確認する */
    if(TRUE == s_eChData[ch].is_opened)
    {
        /* 6-1. TRUEである場合、-1でreturnする */
        return -1;
    }
    else
    {
        /* 6-2. FALSEである場合、引数paramで渡した内容をs_eChDataに格納する(I2cSetData()) */
        iRet = I2C_SetData( ch, param );
    }
    /* 7. モジュールフラグ(s_eModuleEnable[ch])の内容をチェックし、DISABLEであれば以下の処理を実行する */
    if(s_eModuleEnable[ch] == I2C_DISABLE)
    {
        /* 7-1. CPUをロックする(loc_cpu()) */
        loc_cpu();
        /* 7-2. チャンネル割り込みを許可する( ena_int() ) */
        iRet = ena_int(i2c_vect_num_tbl[ch].int_no);
        if(iRet < 0)
        {
            iRet = -1;
        }
        else 
        {
        /* 7-3. モジュールフラグをENABLEに設定(s_eModuleEnable[ch] = ENABLE)*/
            s_eModuleEnable[ch] = I2C_ENABLE;
            s_eChData[ch].is_opened = TRUE;                  /* Open状態にする */

        /* MUX pin 設定 */
            I2C_MuxPinSet( ch );
        }
        /* 7-4. CPUをアンロックする(loc_cpu()) */
        unl_cpu();
    }
    else
    {
        /* nothing */
    }
    /* 8. 正常終了(0)でreturnする */
    return iRet;
}


/************************************************************************************************/
/* FUNCTION   : I2C_Close                                                                       */
/*                                                                                              */
/* DESCRIPTION: Close I2C driver                                                                */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      Used I2C channel no.                                    */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                       Successfully return                                     */
/*              -1                      Error return                                            */
/*                                                                                              */
/************************************************************************************************/
int  I2C_Close( int ch )
{
    /* 1. 初期化されていない場合は、-1でreturnする(s_eIsInitialized == FALSE) */
    if(s_eIsInitialized == FALSE)
    {
        return -1;          /* no initialize error */
    }
    else
    {
        /* nothing */
    }
    /* 2. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
    if((ch < 0) || (I2C_CH_NUM <= ch))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 3. chに設定したI2Cチャンネルがオープン状態であるかどうかを確認する */
    if(FALSE == s_eChData[ch].is_opened)
    {
        return 0;     /* 3-1.オープン状態ではない場合、正常終了(0)で returnする */
    }
    else
    {
        /* nothing */
    }
    s_i2c_register[ch]->I2CR &= ~IMX8M_I2C_B01_I2CR_IEN;   /* I2C reset */

    /* 4. Open済フラグ(s_eChData[ch].is_opened)をFALSEにする */
    s_eChData[ch].is_opened = FALSE;
    /* 5. チャンネル割り込みを禁止する( dis_int() ) */
    dis_int(i2c_vect_num_tbl[ch].int_no);
    /* 6. 通信ステータスをSTATUS_IDLEに設定(s_eStatus[ch] = STATUS_IDLE) */
    s_eStatus[ch] = I2C_STATUS_IDLE;
    /* 7. モジュールフラグをDISABLEに設定(s_eModuleEnable[ch] = DISABLE) */
    s_eModuleEnable[ch] = I2C_DISABLE;

    stp_alm(s_almid_i2c[ch]);               /* アラームハンドラの動作停止 */

    /* 8. 正常終了(0)でreturnする */
    return 0;
}


/************************************************************************************************/
/* FUNCTION   : I2C_Send                                                                        */
/*                                                                                              */
/* DESCRIPTION: Send data to I2C driver                                                         */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      Using I2C channel no.                                   */
/*              adr                     Slave address( 8-bits notation )                        */
/*              data                    Send data                                               */
/*              sz                      Send size                                               */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                       Successfully return                                     */
/*              -1                      Error return                                            */
/*                                                                                              */
/************************************************************************************************/
int  I2C_Send( int ch, int adr, const unsigned char *data, int sz )
{
int                             iWk_sz = sz;
int                             iRet = 0;

    /* 1. 初期化されていない場合は、-1でreturnする(s_eIsInitialized == FALSE) */
    if(s_eIsInitialized == FALSE)
    {
        return -1;          /* no initialize error */
    }
    else
    {
        /* nothing */
    }
    /* 2. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
    if((ch < 0) || (I2C_CH_NUM <= ch))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 3. スレーブアドレスが不正である場合は、-1でreturnする */
    if((adr < 0) ||  (I2C_ADR_MAX < adr))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 4. 送信データサイズが正でない場合は、-1でreturnする(sz < 1) */
    if(sz < 1)
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 5. 通信ステータスがSTATUS_IDLEでなければ、-1でreturnする */
    if(s_eStatus[ch] != I2C_STATUS_IDLE)
    {
        return -1;          /* status error */
    }
    else
    {
        /* nothing */
    }
    /* 6. chに設定したI2Cチャンネルがオープン状態であるかどうかを確認する */
    if(TRUE != s_eChData[ch].is_opened)
    {
        return -1;      /* 5-1. オープン状態ではない場合、-1でreturnする */
    }
    else
    {
        /* nothing */
    }
    /* 7. 内部変数を設定する */
    /* 7-1.通信ステータスを送信中(STATUS_TRANSMIT)に設定 */
    s_eStatus[ch] = I2C_STATUS_TRANSMIT;
    /* 7-2.送受信同時処理フラグをFALSEに設定(s_eIsTranseive[ch] = FALSE) */
    s_eIsTranseive[ch] = FALSE;
    /* 8. 送信キューをクリアする */
    I2C_QueueDataInvalid( &s_eChData[ch].TxQue );
    /* 9. 送信データを送信データサイズ分キューに追加する(enqueue_c()) */
    s_ActiveCh = ch;
    while( sz-- )
    {
        /* Queに追加 */
        if( -1 == s_func_enqueue( &(s_eChData[s_ActiveCh].TxQue), (uint8_t *)data++ ) )
        {
        /* 9-1.エラーの場合、通信ステータスをSTATUS_IDLEに設定し、-1でreturnする */
            s_eStatus[ch] = I2C_STATUS_IDLE;        /* 通信状態 */
            return -1;                              /* 内部QueバッファFull */
        }
        else
        {
            /* nothing */
        }
    }
    /* 10. I2Cの送信設定を行う(I2C_Prepare(ch, MODE_SEND)) */
    if (I2C_Prepare(ch, adr) == TRUE)
    {
        /* 以下の処理はI2C_Prepare()内で行うため、処理なし */
        /* 10-1. チャンネル割り込みを無効にする(dis_int()) */
        /* 10-2. スレーブアドレスをs_eChDataに保存(s_eChData[ch].adr = adr) */
        /* 10-3. I2C_Open()呼び出し時に作成したs_eChDataを用いて、I2Cレジスタへ設定を行う */
        /* 10-4. チャンネル割り込みを有効にする(dis_int()) */
    }
    else 
    {   /* 10-5. I2C_Prepare()がエラーの場合、通信ステータスをSTATUS_IDLEに設定し、 */
        s_eStatus[ch] = I2C_STATUS_IDLE;
        return -1;    /*      -1でreturnする */
    }
    /* 11. チャンネル割り込みを無効にする(dis_int()) */
    iRet = dis_int(i2c_vect_num_tbl[ch].int_no);
    /* 12. 送信データ数をI2C内部管理エリアに設定(s_eChData[ch].send_count == sz) */
    s_eChData[ch].send_count = iWk_sz;

    /* I2SRのIIFとIALをクリア */
    s_i2c_register[ch]->I2SR &= ~(IMX8M_I2C_B01_I2SR_IIF | IMX8M_I2C_B01_I2SR_IAL);
	
    /* 13. I2Cレジスタ(I2CR)をマスターモード送信、割り込み許可に設定 */
    /*     REG_I2C_I2CR |= (I2C_I2CR_IIEN_MASK | I2C_I2CR_MSTA_MASK | I2C_I2CR_MTX_MASK); */
    s_i2c_register[ch]->I2CR &= ~(IMX8M_I2C_B01_I2CR_IEN);   /* I2C reset */
    s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;

    s_i2c_register[ch]->I2CR |= ( IMX8M_I2C_B01_I2CR_IIEN | 
                                  IMX8M_I2C_B01_I2CR_MSTA | 
                                  IMX8M_I2C_B01_I2CR_MTX  );
    s_i2c_register[ch]->I2DR = ((adr << 1) | 0);     // 起動させるため スレーブアドレス書込み
	
    /* 14. チャンネルに対応するアラームハンドラを起動(sta_alm()) */
    sta_alm(s_almid_i2c[ch], I2C_INT_TIME_OUT);
    /* 15. チャンネル割り込みを有効にする(ena_int()) */
    iRet = ena_int(i2c_vect_num_tbl[ch].int_no);

//	s_i2c_register[ch]->I2DR = ((adr << 1) | 0);     // 起動させるため スレーブアドレス書込み

    /* 16. 正常終了(0)でreturnする */
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : I2C_RecvStart                                                                   */
/*                                                                                              */
/* DESCRIPTION: Start data recive to I2C driver                                                 */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      I2C channel Number                                      */
/*              adr                     Slave address( 8-bits notation )                        */
/*              sz                      Recive data size                                        */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                       Successfully return                                     */
/*              -1                      Error return                                            */
/*                                                                                              */
/************************************************************************************************/
int  I2C_RecvStart( int ch, int adr, int sz )
{
int                             iRet = 0;
volatile int                    iWk = 0;

    iWk++;           /* for warning fix */
    /* 1. 初期化されていない場合は、-1でreturnする(s_eIsInitialized == FALSE) */
    if(s_eIsInitialized == FALSE)
    {
        return -1;          /* no initialize error */
    }
    else
    {
        /* nothing */
    }
    /* 2. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
    if((ch < 0) || (I2C_CH_NUM <= ch))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 3. スレーブアドレスが不正である場合は、-1でreturnする */
    if((adr < 0) ||  (I2C_ADR_MAX < adr))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 4. 受信データサイズが正でない場合は、-1でreturnする(sz < 1) */
    if(sz < 1)
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 5. 通信ステータスがSTATUS_IDLEでなければ、-1でreturnする */
    if(s_eStatus[ch] != I2C_STATUS_IDLE)
    {
        return -1;          /* status error */
    }
    else
    {
        /* nothing */
    }
    /* 6. chに設定したI2Cチャンネルがオープン状態であるかどうかを確認する */
    if(TRUE != s_eChData[ch].is_opened)
    {
        return -1;      /* 5-1. オープン状態ではない場合、-1でreturnする */
    }
    else
    {
        /* nothing */
    }
    /* 7. 内部変数設定 */
    /* 7-1.通信ステータスを受信中(STATUS_RECEIVE)に設定 */
    s_eStatus[ch] = I2C_STATUS_RECEIVE;
    /* 7-2.送受信同時処理フラグをFALSEに設定(s_eIsTranseive[ch] = FALSE) */
    s_eIsTranseive[ch] = FALSE;
    /* 8. 受信データ数をI2C内部管理エリア(s_eChData[].recv_size)に設定 */
    s_eChData[ch].recv_size = sz;
    s_eChData[ch].recv_cnt  = 0;
    /* 9. 受信キューデータをクリアする(I2C_QueueDataInvalid(RxQue) */
    I2C_QueueDataInvalid(&(s_eChData[ch].RxQue));	/* Queクリア */
    /* 10. I2Cの受信設定を行う(I2C_Prepare(ch, MODE_RECV)) */
    if (I2C_Prepare(ch, adr) == TRUE)
    {
        /* 以下の処理はI2C_Prepare()内で行うため、処理なし */
        /* 10-1. チャンネル割り込みを無効にする(dis_int()) */
        /* 10-2. スレーブアドレスをs_eChDataに保存(s_eChData[ch].adr = adr) */
        /* 10-3. I2C_Open()呼び出し時に作成したs_eChDataを用いて、I2Cレジスタへ設定を行う */
        /* 10-4. チャンネル割り込みを有効にする(dis_int()) */
    }
    else 
    {
        /* 10-5. I2C_Prepare()がエラーの場合、通信ステータスをSTATUS_IDLEに設定し、 */
        s_eStatus[ch] = I2C_STATUS_IDLE;
        return -1;    /*      -1でreturnする */
    }
    /* 11. CPUをロックする(loc_cpu()) */
    loc_cpu();

    /* I2SRのIIFとIALをクリア */
    s_i2c_register[ch]->I2SR &= ~(IMX8M_I2C_B01_I2SR_IIF | IMX8M_I2C_B01_I2SR_IAL);

    s_i2c_register[ch]->I2CR &= ~(IMX8M_I2C_B01_I2CR_IEN);
    s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */

    /* 12. 送信サイズが１であれば、NAckを出す設定を行う */
    if( s_eChData[ch].recv_size == 1)
    {
        s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_TXAK;
    }
    /* 13. 内部管理情報の受信カウンタクリア */
    s_eChData[ch].receive_count = 0;
    /* 14. CPUをアンロックする(unl_cpu()) */
    unl_cpu();
    /* 15. チャンネル割り込みを無効にする(dis_int()) */
    iRet = dis_int(i2c_vect_num_tbl[ch].int_no);
    /* 16. I2Cレジスタ(I2CR)をマスターモード受信、割り込み許可に設定 */
    s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
    s_i2c_register[ch]->I2CR &= ~(IMX8M_I2C_B01_I2CR_MTX);    /* Receive : 0 */
    s_i2c_register[ch]->I2CR |= (IMX8M_I2C_B01_I2CR_IIEN | IMX8M_I2C_B01_I2CR_MSTA);
    /* 17. チャンネルに対応するアラームハンドラを起動(sta_alm()) */
    sta_alm(s_almid_i2c[ch], I2C_INT_TIME_OUT);
    /* 18. チャンネル割り込みを有効にする(ena_int()) */
    iRet = ena_int(i2c_vect_num_tbl[ch].int_no);

    iWk = s_i2c_register[ch]->I2DR;                  /* dummy read */
    s_i2c_register[ch]->I2DR = ((adr << 1) | 1);     // 起動させるため スレーブアドレス書込み
    iWk = s_i2c_register[ch]->I2DR;                  /* dummy read */

    iRet = 0;
    /* 19. 正常終了(0)でreturnする */
    return iRet;
}


/************************************************************************************************/
/* FUNCTION   : I2C_Send_RecvStart                                                              */
/*                                                                                              */
/* DESCRIPTION: Send & Recive data to I2C driver                                                */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      I2C channel Number                                      */
/*              adr                     Slave address( 8-bits notation )                        */
/*              data                    Send data                                               */
/*              sz                      Send data size                                          */
/*              rcv_sz                  Recive data size                                        */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                       Successfully return                                     */
/*              -1                      Error return                                            */
/*                                                                                              */
/************************************************************************************************/
int  I2C_Send_RecvStart( int ch, int adr, const unsigned char *data, int snd_sz, int rcv_sz )
{
int                             iWk_snd_sz = snd_sz;
int                             iRet = 0;

    /* 1. 初期化されていない場合は、-1でreturnする(s_eIsInitialized == FALSE) */
    if(s_eIsInitialized == FALSE)
    {
        return -1;          /* no initialize error */
    }
    else
    {
        /* nothing */
    }
    /* 2. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
    if((ch < 0) || (I2C_CH_NUM <= ch))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 3. スレーブアドレスが不正である場合は、-1でreturnする */
    if((adr < 0) ||  (I2C_ADR_MAX < adr))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 4. 送信データサイズが正でない場合は、-1でreturnする(sz < 1) */
    if(snd_sz < 1)
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 5. 受信データサイズが正でない場合は、-1でreturnする(rcv_sz < 1) */
    if(rcv_sz < 1)
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 6. 通信ステータスがSTATUS_IDLEでなければ、-1でreturnする */
    if(s_eStatus[ch] != I2C_STATUS_IDLE)
    {
        return -1;          /* status error */
    }
    else
    {
        /* nothing */
    }
    /* 7. chに設定したI2Cチャンネルがオープン状態であるかどうかを確認する */
    if(TRUE != s_eChData[ch].is_opened)
    {
        /* 6-1. オープン状態ではない場合、-1でreturnする */
        return -1;
    }
    /* 8. 内部変数設定 */
    /* 8-1.通信ステータスを送信中(STATUS_ TRANSMIT)に設定 */
    s_eStatus[ch] = I2C_STATUS_TRANSMIT;
    /* 8-2. 送受信同時処理フラグをTRUEに設定(s_eIsTranseive[ch] = TRUE) */
    s_eIsTranseive[ch] = TRUE;
    /* 9. 受信データ数をI2C内部管理エリア(s_eChData[ch].recv_size)に設定 */
    s_eChData[ch].recv_size = rcv_sz;
    /* 10.  受信データカウンタをクリア(s_eChData[ch].recv_cnt = 0) */
    s_eChData[ch].recv_cnt = 0;
    /* 11. 受信キューデータをクリア(I2C_QueueDataInvalid(RxQue) */
    I2C_QueueDataInvalid(&(s_eChData[ch].RxQue));	/* Queクリア */
    /* 12. 送信データを送信データサイズ分キューに追加する(enqueue_c()) */
    s_ActiveCh = ch;
    while( snd_sz-- )
    {
        /* Queに追加 */
        if( -1 == s_func_enqueue( &(s_eChData[s_ActiveCh].TxQue), (uint8_t *)data++ ) )
        {
        /* 12-1.エラーの場合、通信ステータスをSTATUS_IDLEに設定し、-1でreturnする */
            s_eStatus[ch] = I2C_STATUS_IDLE;        /* 通信状態 */
            return -1;                              /* 内部QueバッファFull */
        }
        else
        {
            /* nothing */
        }
    }
    /* 13. I2Cの送受信設定(I2C_Prepare(ch, MODE_SEND_RECV)) */
    if (I2C_Prepare(ch, adr) == TRUE)
    {
        /* 以下の処理はI2C_Prepare()内で行うため、処理なし */
        /* 13-1. チャンネル割り込みを無効にする(dis_int()) */
        /* 13-2. スレーブアドレスをs_eChDataに保存(s_eChData[ch].adr = adr) */
        /* 13-3. I2C_Open()呼び出し時に作成したs_eChDataを用いて、I2Cレジスタへ設定を行う */
        /* 13-4. チャンネル割り込みを有効にする(dis_int()) */
    }
    else 
    {
        /* 13-5. I2C_Prepare()がエラーの場合、通信ステータスをSTATUS_IDLEに設定し、 */
        s_eStatus[ch] = I2C_STATUS_IDLE;
        return -1;    /*       -1でreturnする */
    }

    s_i2c_register[ch]->I2CR &= ~(IMX8M_I2C_B01_I2CR_IEN);   /* I2C reset */
    /* 14. 送信サイズが１であれば、NAckを出す設定を行う */
    if(iWk_snd_sz == 1)
    {
        /* 14-1. CPUをロックする(loc_cpu()) */
        loc_cpu();
        s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
        /* 14-2. I2CRレジスタのTXAKビットを’1’に設定する */
        s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_TXAK;
        /* 14-3. CPUをアンロックする(unl_cpu()) */
        unl_cpu();
    }
    /* 15. チャンネル割り込みを無効にする(dis_int()) */
    iRet = dis_int(i2c_vect_num_tbl[ch].int_no);
    /* 16. 送信データ数をI2C内部管理エリアに設定(s_eChData[ch].send_count == sz) */
    s_eChData[ch].send_count = iWk_snd_sz;
    /* 17. I2Cレジスタ(I2CR)をマスターモード、送信モード、割り込み許可に設定 */
    s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
//    s_i2c_register[ch]->I2CR |= ( IMX8M_I2C_B01_I2CR_IIEN | 
//                                  IMX8M_I2C_B01_I2CR_MSTA | 
//                                  IMX8M_I2C_B01_I2CR_MTX );
    s_i2c_register[ch]->I2CR |= ( IMX8M_I2C_B01_I2CR_IIEN );
    /* 18. チャンネルに対応するアラームハンドラを起動(sta_alm()) */
    sta_alm(s_almid_i2c[ch], I2C_INT_TIME_OUT);
    /* 19. チャンネル割り込みを有効にする(ena_int()) */
    iRet = ena_int(i2c_vect_num_tbl[ch].int_no);

    /* I2SRのIIFとIALをクリア */
//    s_i2c_register[ch]->I2SR &= ~(IMX8M_I2C_B01_I2SR_IIF | IMX8M_I2C_B01_I2SR_IAL);
    
    s_i2c_register[ch]->I2CR |= ( IMX8M_I2C_B01_I2CR_MSTA | IMX8M_I2C_B01_I2CR_MTX );
    
//	s_i2c_register[ch]->I2CR &= ~IMX8M_I2C_B01_I2CR_IIEN;
    s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IIEN;
    
    s_i2c_register[ch]->I2DR = ((adr << 1) | 0);     // 起動させるため スレーブアドレス書込み
    
    /* 20. 正常終了(0)でreturnする */
    
    return iRet;
}


/************************************************************************************************/
/* FUNCTION   : I2C_Recv                                                                        */
/*                                                                                              */
/* DESCRIPTION: Send data to I2C driver                                                         */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      I2C channel Number                                      */
/*              adr                     Slave address( 8-bits notation )                        */
/*              data                    Recive data address                                     */
/*              sz                      Recive data size                                        */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0<=                     Received data length (unit: bytes)                      */
/*              -1                      Error return                                            */
/*                                                                                              */
/************************************************************************************************/
int  I2C_Recv( int ch, int adr, unsigned char *data, int sz )
{
int32_t                         cnt = 0;
int                             iRet = 0;

    /* 1. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
    if((ch < 0) || (I2C_CH_NUM <= ch))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 2. スレーブアドレスが不正である場合は、-1でreturnする */
    if((adr < 0) ||  (I2C_ADR_MAX < adr))
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 3. 受信データサイズが正でない場合は、-1でreturnする(rcv_sz < 1) */
    if(sz < 1)
    {
        return -1;          /* parameter error */
    }
    else
    {
        /* nothing */
    }
    /* 4. chに設定したI2Cチャンネルがオープン状態であるかどうかを確認する */
    if(TRUE != s_eChData[ch].is_opened)
    {
        return -1;    /* 3-1. オープン状態ではない場合、-1でreturnする */
    }
    else
    {
        /* nothing */
    }
    /* 5. 受信取りこぼし(s_eChData[ch].RxOver)チェック */
    if(s_eChData[ch].RxOver == TRUE)
    {
        return -1;    /* 4-1. ありの場合、-1でreturnする(TRUE == s_eChData[ch].RxOver) */
    }
    else
    {
        /* nothing */
    }
    /* 6. 直前に受信動作を行ったスレーブアドレスであるかどうかを確認(s_eChData[ch].adr == adr) */
    if(s_eChData[ch].adr == adr)
    {
        /* nothing */
    }
    else 
    {    /* 6-1. スレーブアドレスが異なる場合、-1でreturnする */
        return -1;
    }
    /* 7. チャンネル割り込みを無効にする(dis_int()) */
    iRet++;    /* for warning fix */
    iRet = dis_int(i2c_vect_num_tbl[ch].int_no);
    /* 8. 以下の処理を受信サイズ分繰り返す */
    /* 8-1. データを受信キューから取り出し、受信データ格納先アドレスへコピーする(dequeue_c) */
    while( sz-- >= 1 )
    {
        /* 受信データの取り出し */
        if( -1 == s_func_dequeue( &(s_eChData[ch].RxQue), data++ ) )
        {
            break;
        }
        else
        {
            /* nothing */
        }
    /* 8-2. 取り出し個数(cnt)に１を加える */
        cnt++;
    }
    /* 9. チャンネル割り込みを有効にする(ena_int()) */
    iRet = ena_int(i2c_vect_num_tbl[ch].int_no);
    stp_alm(s_almid_i2c[ch]);               /* アラームハンドラの動作停止 */
	
    /* 10. cntを受信数としてreturnする */
    return cnt;
}


/************************************************************************************************/
/* FUNCTION   : I2C_isBusy                                                                      */
/*                                                                                              */
/* DESCRIPTION: I2C busy check                                                                  */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      I2C channel Number                                      */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : I2C_IDLE                Idle State                                              */
/*              I2C_BUSY                Busy State                                              */
/*              -1                      Error return                                            */
/*                                                                                              */
/************************************************************************************************/
int  I2C_isBusy( int ch )
{
int                             iRet = 0;

    /* 1. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
    if((ch < 0) || (I2C_CH_NUM <= ch))
    {
        return -1;              /* not initialize */
    }
    else
    {
        /* nothing */
    }
    /* 2. 通信ステータス(s_eStatus[ch])をチェックする */
    if(s_eStatus[ch] == I2C_STATUS_IDLE)
    {
        /* 2-1. STATUS_IDLEであれば I2C_IDLE でreturnする */
        iRet = I2C_IDLE;
    }
    else
    {   /* 2-2. それ以外は、I2C_BUSYでreturnする */
        iRet = I2C_BUSY;
    }
    return iRet;
}


/************************************************************************************************/
/* FUNCTION   : I2C_Lock                                                                        */
/*                                                                                              */
/* DESCRIPTION: I2C lock with mutex                                                             */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      I2C channel Number                                      */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                       Normal End                                              */
/*              -1                      Error Occured                                           */
/*                                                                                              */
/************************************************************************************************/
int  I2C_Lock(int ch)
{
int                             iRet = 0;

    /* 1. 初期化されていない場合は、-1でreturnする(s_eIsInitialized == FALSE) */
    if(s_eIsInitialized == FALSE)
    {
        iRet = -1;              /* not initialize */
    }
    else
    {   /* 2. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
        if( (ch < 0) || (I2C_CH_NUM <= ch) )
        {
            iRet = -1;          /* parameter error */
        }
        else 
        {    /* wai_sem(s_semid_i2c[ch])でロックする */
            s_ercd = wai_sem(s_semid_i2c[ch]);
        }
    }
    /* 正常終了(0)でreturnする */
    return iRet;
}


/************************************************************************************************/
/* FUNCTION   : I2C_UnLock                                                                      */
/*                                                                                              */
/* DESCRIPTION: I2C un-lock with mutex                                                          */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      I2C channel Number                                      */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                       Normal End                                              */
/*              -1                      Error Occured                                           */
/*                                                                                              */
/************************************************************************************************/
int  I2C_UnLock(int ch)
{
int                             iRet = 0;

    /* 1. 初期化されていない場合は、-1でreturnする(s_eIsInitialized == FALSE) */
    if(s_eIsInitialized == FALSE)
    {
        iRet = -1;              /* not initialize */
    }
    else
    {
        /* 2. チャンネル番号が不正である場合は、-1でreturnする((ch < 0) || (I2C_CH_NUM <=ch)) */
        if( (ch < 0) || (I2C_CH_NUM <= ch) )
        {
            iRet = -1;          /* parameter error */
        }
        else 
        {    /* 3. sig_sem(s_semid_i2c[ch])でアンロックする */
            s_ercd = sig_sem(s_semid_i2c[ch]);
        }
    }
    /* 4. 正常終了(0)でreturnする */
    return iRet;
}

/*--------------------------以下は非公開---------------------------------*/

/************************************************************************************************/
/* FUNCTION   : I2C_SetData                                                                     */
/*                                                                                              */
/* DESCRIPTION: Store param contents in s_eChData.                                              */
/*                                                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                              I2C channel number                              */
/*              param                           Setting parameter                               */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : 0                               Normal end                                      */
/*              -1                              error return(already Open)                      */
/*                                                                                              */
/************************************************************************************************/
LOCAL int32_t I2C_SetData( int32_t ch, const i2c_param_t* param )
{
int32_t                         iRet = 0;

    if( s_eChData[ch].is_opened == FALSE )
    {
        s_eChData[ch].adr = 0;
        initqueue( &(s_eChData[ch].RxQue), param->rxbuf_ptr, param->rxbuf_size, sizeof(unsigned char) );
        initqueue( &(s_eChData[ch].TxQue), param->txbuf_ptr, param->txbuf_size, sizeof(unsigned char) );
        s_eChData[ch].RxOver      = FALSE;
        s_eChData[ch].rx_callback = param->rx_callback;
        s_eChData[ch].tx_callback = param->tx_callback;
        s_eChData[ch].ifdr_IC     = s_DivTbl[param->rate].ifdr_devider;  /* I2Cx_IFDR IC : I2C clock rate */
        s_eChData[ch].is_opened   = TRUE;
    }
    else
    {
        iRet = -1;
    }
    return iRet;
}


/************************************************************************************************/
/* FUNCTION   : I2C_Prepare                                                                     */
/*                                                                                              */
/* DESCRIPTION: Make settings for using I2C                                                     */
/*                                                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                      I2C Channel number                                      */
/*              adr                     Slave address( 8-bits notation )                        */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : TRUE                    Normal end                                              */
/*                                                                                              */
/************************************************************************************************/
LOCAL int32_t I2C_Prepare( int32_t ch, int32_t adr )
{
    /* 1. チャンネル割り込みを無効にする(dis_int()) */
    dis_int(i2c_vect_num_tbl[ch].int_no);

    /* 2. スレーブアドレスをs_eChDataに保存(s_eChData[ch].adr = adr) */
    s_eChData[ch].adr = adr;            /* Slave address setting */

    /* 3. I2C_Open()呼び出し時に作成したs_eChDataを用いて、I2Cレジスタへ設定を行う */
    s_i2c_register[ch]->IFDR = s_eChData[ch].ifdr_IC;
    s_i2c_register[ch]->IADR = (s_eChData[ch].adr << 1);
	
    s_i2c_register[ch]->I2CR |= (IMX8M_I2C_B01_I2CR_IEN);

    /* 4. チャンネル割り込みを有効にする(ena_int()) */
    ena_int(i2c_vect_num_tbl[ch].int_no);

    return TRUE;
}

/************************************************************************************************/
/* FUNCTION   : I2C_handler                                                                     */
/*                                                                                              */
/* DESCRIPTION: I2CのIRQハンドラ処理                                                            */
/*                                                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                              I2C Channel number                              */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void I2C_handler( int ch )
{
unsigned int                   icivr;
int                            ret;
int                            errCause;
unsigned char                  data;

    /* 1. 割り込み要因取得(icivr = s_i2c_register[ch]->I2SR) */
    icivr = s_i2c_register[ch]->I2SR;
    /* 2. IIFビットをクリア(s_i2c_register[ch]->I2SR &= ~I2C_I2SR_IIF_MASK) */
    s_i2c_register[ch]->I2SR &= ~IMX8M_I2C_B01_I2SR_IIF;
	
    /* 3. ICF、IAL、RXAKのすべてが0である場合、returnする */
    if((icivr & ( IMX8M_I2C_B01_I2SR_ICF |
                  IMX8M_I2C_B01_I2SR_IAL |
                  IMX8M_I2C_B01_I2SR_RXAK )) == 0)
    {
        return;
    }
    else 
    {
        if( (icivr & IMX8M_I2C_B01_I2SR_IAL) != 0 )
        {
            s_i2c_register[ch]->I2SR &= ~IMX8M_I2C_B01_I2SR_IAL;
        }
        else 
        {
            /* nothing */
        }
    }
    if( (icivr & IMX8M_I2C_B01_I2SR_ICF) == 0 )   /* 転送途中 */
    {
//        return;
    }

    /* ------------------------- 受信 ------------------------------*/
    /* 4. 通信ステータス(s_eStatus[ch])が受信(STATUS_RECEIVE)の場合 */
    if( I2C_STATUS_RECEIVE == s_eStatus[ch] )              /* 通信状態 */
    {
        /* 4.1 受信完了の場合(icivr & I2C_I2SR_ICF_MASK) != 0) */
        if ((icivr & IMX8M_I2C_B01_I2SR_ICF) != 0)
        {
            data = (unsigned char)s_i2c_register[ch]->I2DR;
            /* 4.1.1 受信カウント＋１(s_eChData[ch].recv_cnt++) */
            s_eChData[ch].recv_cnt++;
            /* 4.1.2 受信カウントが受信要求サイズであれば */
            if( s_eChData[ch].recv_cnt == s_eChData[ch].recv_size )
            {
                /* 4.1.2.1 割り込み要因クリアREG_I2C_I2SR = 0x00; */
                s_i2c_register[ch]->I2SR = 0x00;
                /* 4.1.2.2 受信データを受信キューに登録する */
                ret = s_func_enqueue( &(s_eChData[ch].RxQue), &data);
                /* 4.1.2.3 登録エラーの場合 */
                if(ret < 0)
                {  /* 4.1.2.3.1 受信Overフラグセット(s_eChData[ch].RxOver = TRUE) */
                    s_eChData[ch].RxOver = TRUE;
                }
                else 
                {
                    /* nothing */
                }
                /* 4.1.2.4 STOPコンディション出力 */
                //s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
                //s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_MSTA | IMX8M_I2C_B01_I2CR_MTX  | 
                //                               IMX8M_I2C_B01_I2CR_TXAK);
                I2C_MasterStop(ch);
                /* 4.1.2.5 受信Callback関数が登録されていれば、受信完了コールバックする */
                if(s_eChData[ch].rx_callback != 0)
                {
                    s_eChData[ch].rx_callback( s_eChData[ch].adr );
                }
                else 
                {
                    /* nothing */
                }
                /* 4.1.2.6 通信ステータスをSTATUS_IDLEにする(s_eStatus[ch] = STATUS_IDLE) */
                s_eStatus[ch] = I2C_STATUS_IDLE;
                s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_IEN | IMX8M_I2C_B01_I2CR_IIEN ); /* add by s.shimizu */
            }
            else 
            {
                /* nothing */
            }
        }
        else 
        {  /* 4.2 受信完了でない(受信途中)場合 */
            /* 4.2.1 最後の１個かチェック */
            if(s_eChData[ch].recv_cnt == (s_eChData[ch].recv_size - 1))
            { /* 4.2.2 最後の１個であれば、*/
                /* 次受信でNAckを返すように設定する(s_i2c_register[ch]->I2CR |= I2C_I2CR_TXAK) */
                s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_TXAK;
                /* 4.2.3 受信データを読み出し(data = (unsigned char)s_i2c_register[ch]->I2DR)、*/
                /* 受信キューに登録する→enqueue_c( &(s_eChData[ch].RxQue), &data)　関数で登録 */
                data = (unsigned char)s_i2c_register[ch]->I2DR;
                if( -1 == s_func_enqueue( &(s_eChData[ch].RxQue), &data) )
                {
                    s_eChData[ch].RxOver = TRUE;
                }
                else
                {
                    /* nothing */
                }
                /*---------------------------   shimizu add start ----------------------*/
                /* 4.2.4 STOPコンディション出力 */
                //s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
                //s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_MSTA | IMX8M_I2C_B01_I2CR_MTX  | 
                //                               IMX8M_I2C_B01_I2CR_TXAK);
                I2C_MasterStop(ch);
                /* 4.2.5 受信Callback関数が登録されていれば、受信完了コールバックする */
                if(s_eChData[ch].rx_callback != 0)
                {
                    s_eChData[ch].rx_callback( s_eChData[ch].adr );
                }
                else 
                {
                    /* nothing */
                }
                /* 4.2.6 通信ステータスをSTATUS_IDLEにする(s_eStatus[ch] = STATUS_IDLE) */
                s_eStatus[ch] = I2C_STATUS_IDLE;
                s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_IEN | IMX8M_I2C_B01_I2CR_IIEN ); /* add by s.shimizu */
                /*---------------------------   shimizu add end   ----------------------*/
            
            }
            else
            {
                /* 4.3 受信でNGの場合 */
                /* 4.3.1 icivrがArbitorationLossの場合((icivr & I2C_I2SR_IAL_MASK) != 0) */
                if((icivr & IMX8M_I2C_B01_I2SR_IAL) != 0)
                {
                    errCause = I2C_ERROR_CAUSE_RECV_AL;
                }
                else 
                {  /* 4.3.2 icivrが上記以外の場合 */
                    errCause = I2C_ERROR_CAUSE_RECV_ATHER;
                }
                /* 4.3.3 STOPコンディション出力 */
                //s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
                //s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_MSTA | IMX8M_I2C_B01_I2CR_MTX | 
                //                               IMX8M_I2C_B01_I2CR_TXAK);
                I2C_MasterStop(ch);
                /* 4.3.4 受信Callback関数が登録されていれば、関数実行する */
                if(s_eChData[ch].rx_callback != 0)
                {
                    s_eChData[ch].rx_callback( errCause );
                }
                else
                {
                    /* nothing */
                }
                /* 4.3.5 通信ステータスをSTATUS_IDLEに設定する(s_eStatus[ch] = STATUS_IDLE)を */
                s_eStatus[ch] = I2C_STATUS_IDLE;
                s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_IEN | IMX8M_I2C_B01_I2CR_IIEN ); /* add by s.shimizu */
            }
        }
    }
    /* ------------------------- 送信 ------------------------------*/
    /* 5. 通信ステータス(s_eStatus[ch])が送信(STATUS_TRANSMIT, STATUS_TRANSSTART)の場合 */
    else if( (I2C_STATUS_TRANSMIT   == s_eStatus[ch]) ||        /* 通信状態 */
             (I2C_STATUS_TRANSSTART == s_eStatus[ch]) )
    {
        /* 5.1 送信完了であれば(icivr & I2C_I2SR_ICF) != 0) */
        if((icivr & IMX8M_I2C_B01_I2SR_ICF) != 0)
        {
            /* 5.1.1 通信ステータス(s_eStatus[ch])が送信中(STATUS_TRANSMIT)の時 */
            if(s_eStatus[ch] == I2C_STATUS_TRANSMIT)
            {
                /* 5.1.1.1 送信キューよりデータを取り出す */
                if(-1 == s_func_dequeue( &(s_eChData[ch].TxQue), &data ))
                {
                    /* nothing */
                }
                /* 5.1.1.2 送信レジスタにセット(s_i2c_register[ch]->I2DR = (unsigned short)data) */
                s_i2c_register[ch]->I2DR = (unsigned short)data;
                /* 5.1.1.3 最後のデータであれば、*/
                if(s_eChData[ch].TxQue.front == s_eChData[ch].TxQue.last)
                {
                    /* 5.1.1.3.1 送受信同時要求であるかどうかをチェック */
                    if(TRUE == s_eIsTranseive[ch])
                    { /* 5.1.1.3.1.1 送受信同時要求の時 */
                        /* 5.1.1.3.1.1.1 通信ステータスをSTATUS_RECVSTARTとする */
                        s_eStatus[ch] = I2C_STATUS_RECVSTART;
                        /* 5.1.1.3.1.1.2 転送中フラグOFFとする(s_eIsTranseive[ch] = FALSE) */
                        s_eIsTranseive[ch] = FALSE;
                    }
                    else 
                    { /* 5.1.1.3.1.2 送信要求の時 */
                        /* 5.1.1.3.1.2.1 通信ステータスをSTATUS_STOPとする */
                        s_eStatus[ch] = I2C_STATUS_STOP;
                    }
                }
                else
                {
                    /* nothing */
                }
            }
            else
            {
                /* nothing */
            }
        }
        else 
        {  /* 5.2 送信でNGの場合 */
            /* 5.2.1 icivrがArbitorationLossの場合((icivr & I2C_I2SR_IAL_MASK) != 0) */
            if((icivr & IMX8M_I2C_B01_I2SR_IAL) != 0)
            {
                errCause = I2C_ERROR_CAUSE_SEND_AL;
            }
            /* 5.2.2 icivrが NAck割り込みの場合((icivr & I2C_I2SR_RXAK_MASK) != 0) */
            else if((icivr & IMX8M_I2C_B01_I2SR_RXAK) != 0)
            {
                errCause = I2C_ERROR_CAUSE_SEND_NACK;
            }
            else 
            { /* 5.2.3 icivrが上記以外の場合 */
                errCause = I2C_ERROR_CAUSE_SEND_ATHER;
            }
            /* 5.2.4 STOPコンディション出力 */
            //s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
            //s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_MSTA | IMX8M_I2C_B01_I2CR_MTX | 
            //                               IMX8M_I2C_B01_I2CR_TXAK );
            I2C_MasterStop(ch);
            /* 5.2.5 送信QUEUEデータ廃棄 */
            I2C_QueueDataInvalid( &(s_eChData[ch].TxQue) );
            /* 5.2.6 送信Callback関数が登録されていれば、関数実行する */
            if(s_eChData[ch].tx_callback != 0)
            {
                s_eChData[ch].tx_callback( errCause );
            }
            else
            {
                /* nothing */
            }
            /* 5.2.7 通信ステータスをSTATUS_IDLEに設定する(s_eStatus[ch] = STATUS_IDLE)  */
            s_eStatus[ch] = I2C_STATUS_IDLE;
        }
    }
    /* ------------------------- 受信開始 ------------------------------*/
    /* 6. 通信ステータス(s_eStatus[ch])が受信開始(STATUS_RECVSTART)の場合 */
    else if ( I2C_STATUS_RECVSTART == s_eStatus[ch] )			/* 通信状態 */
    {
        /* 6.1 IALビットをクリアする */
        s_i2c_register[ch]->I2SR &= (~IMX8M_I2C_B01_I2SR_IAL);
        /* 6.2	IENビットをクリアする */
//        s_i2c_register[ch]->I2CR &= (~IMX8M_I2C_B01_I2CR_IEN);  /* I2C reset */
        s_i2c_register[ch]->I2CR |= (IMX8M_I2C_B01_I2CR_IEN);
    
        /* 6.3	受信データ数を0に設定する(s_eChData[ch].receive_count = 0) */
        s_eChData[ch].receive_count = 0;
        /* 6.4	通信ステータスをSTATUS_RECEIVEに設定(s_eStatus[ch]= STATUS_RECEIVE) */
        s_eStatus[ch] = I2C_STATUS_RECEIVE;
        
        /* I2SRのIIFとIALをクリア */
        s_i2c_register[ch]->I2SR &= ~(IMX8M_I2C_B01_I2SR_IIF | IMX8M_I2C_B01_I2SR_IAL);
        
        /* 6.5	I2Cレジスタ(I2CR)をマスターモード、受信モードに設定する */
//        s_i2c_register[ch]->I2CR &= ~(IMX8M_I2C_B01_I2CR_MTX);    /* Receive : 0 */
        s_i2c_register[ch]->I2CR |= ( IMX8M_I2C_B01_I2CR_IIEN | IMX8M_I2C_B01_I2CR_MSTA );
        I2C_wait(1);
        s_i2c_register[ch]->I2CR |= ( IMX8M_I2C_B01_I2CR_RSTA | IMX8M_I2C_B01_I2CR_MTX );

        /* I2SRのIIFとIALをクリア */
        s_i2c_register[ch]->I2SR &= ~(IMX8M_I2C_B01_I2SR_IIF | IMX8M_I2C_B01_I2SR_IAL);

        for(int readcount = 0; readcount < 10; readcount++)
        {
            data = s_i2c_register[ch]->I2DR;         // Dummy Read
        }
        s_i2c_register[ch]->I2DR = ((s_eChData[ch].adr << 1) | 1);     // 起動させるため スレーブアドレス書込み
        
        data = s_i2c_register[ch]->I2DR;         // Dummy Read
        
    }
    /* ------------------------- 停止 ------------------------------*/
    /* 7. 通信ステータス(s_eStatus)が停止(STATUS_STOP)の場合 */
    if ( I2C_STATUS_STOP == s_eStatus[ch] )            /* 通信状態 */
    {
        /* 7.1 STOPコンディション出力 */
        //s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
        //s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_MSTA | 
        //                               IMX8M_I2C_B01_I2CR_MTX  | 
        //                               IMX8M_I2C_B01_I2CR_TXAK );
        I2C_MasterStop(ch);
        /* 7.2 送信QUEUEデータ廃棄(I2C_QueueDataInvalid()) */
        I2C_QueueDataInvalid( &(s_eChData[ch].TxQue) );
        /* 7.3 送信Callback関数が登録されていれば、送信Callback関数を実行する */
        if(s_eChData[ch].tx_callback != 0)
        {
            s_eChData[ch].tx_callback( s_eChData[ch].adr );
        }
        else
        {
            /* nothing */
        }
        /* 7.4 通信ステータスをSTATUS_IDLEにする(s_eStatus[ch] = STATUS_IDLE)  */
        s_eStatus[ch] = I2C_STATUS_IDLE;
    }
    /* 8. return する */
    return;
}


/************************************************************************************************/
/* FUNCTION   : I2C_TimerCallBack                                                               */
/*                                                                                              */
/* DESCRIPTION: I2Cのタイマーコールバック関数                                                   */
/*                                                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : exinf                           I2C Channel number                              */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void I2C_TimerCallBack(int exinf)
{
int                             ch = exinf;

    /* 1. 通信中であるかどうかを判定する */
    /* 1-1. STATUS_IDLEまたはSTATUS_STOPならば、returnする */
    if((s_eStatus[ch] == I2C_STATUS_IDLE) || (s_eStatus[ch] == I2C_STATUS_STOP))
    {
        return;
    }
    else 
    {
        /* nothing */
    }
    /* 2. I2Cx_I2SRレジスタをクリアする */
    s_i2c_register[ch]->I2SR = 0x00;
    /* 3. STOPコンディション出力 */
    //s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
    //s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_MSTA | IMX8M_I2C_B01_I2CR_MTX | 
    //                               IMX8M_I2C_B01_I2CR_TXAK );
    I2C_MasterStop(ch);
    /* 4. 送信キューデータ廃棄( I2C_QueueDataInvalid(&(s_eChData[exinf].TxQue)) ) */
    I2C_QueueDataInvalid( &(s_eChData[ch].TxQue) );
    /* 5. 受信callback関数が登録されていれば、処理をCallする */
    if( (s_eStatus[ch] == I2C_STATUS_RECEIVE) || (s_eStatus[ch] == I2C_STATUS_RECVSTART) )
    {
        if(s_eChData[ch].rx_callback != 0)
        {
            s_eChData[ch].rx_callback( I2C_ERROR_CAUSE_INT_TMOUT );
        }
        else 
        {
            /* nothing */
        }
    }
    /* 送信callback関数が登録されていれば、処理をCallする */
    else if( s_eStatus[ch] == I2C_STATUS_TRANSMIT )
    {
        if(s_eChData[ch].tx_callback != 0)
        {
            s_eChData[ch].tx_callback( I2C_ERROR_CAUSE_INT_TMOUT );
        }
        else 
        {
            /* nothing */
        }
    }
    else 
    {
        /* nothing */
    }

    /* 6. ステータス更新( s_eStatus[exinf] = STATUS_IDLE ) */
    s_eStatus[ch] = I2C_STATUS_IDLE;

    /* I2SRのIIFとIALをクリア */
    s_i2c_register[ch]->I2SR &= ~(IMX8M_I2C_B01_I2SR_IIF | IMX8M_I2C_B01_I2SR_IAL);

    s_i2c_register[ch]->I2CR &= ~(IMX8M_I2C_B01_I2CR_IEN);  /* I2C reset */
    s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
//    s_i2c_register[ch]->I2SR &= ~( IMX8M_I2C_B01_I2SR_IAL );    /* IAL clear */
    s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_IEN | IMX8M_I2C_B01_I2CR_IIEN ); /* add by s.shimizu */
	
    /* 7. returnする */
    return;
}


/************************************************************************************************/
/* FUNCTION   : I2C_QueueDataInvalid                                                            */
/*                                                                                              */
/* DESCRIPTION: I2Cの QUEUEデータ廃棄(内容を０にする)処理                                       */
/*                                                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : pQd                             Pointer of QUEUE_DATA data                      */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void I2C_QueueDataInvalid( QUEUE_DATA* pQd )
{
    if(pQd != 0)
    {
        pQd->front = pQd->last;
    }
    else
    {
        /* nothing */
    }
}

#if 0
以下のQueue関数は、(株)キーエンス様から提供されている標準ライブラリを使用→util_public.cへ

LOCAL ER  initQueue( QUEUE_DATA *Que, unsigned char *buf_ptr, int buf_size, int type_size );
LOCAL ER  enqueue_c( QUEUE_DATA *Que, unsigned char *pdata );
LOCAL ER  dequeue_c( QUEUE_DATA *Que, unsigned char *pdata );

#endif

/************************************************************************************************/
/* FUNCTION   : I2C_MuxPinSet                                                                   */
/*                                                                                              */
/* DESCRIPTION: I2Cの Mux Pin Setting 処理                                                      */
/*                                                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ch                              Channel Number                                  */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    :  0 :                            Normal End                                      */
/*              -1 :                            Parameter Error                                 */
/*                                                                                              */
/************************************************************************************************/
LOCAL int I2C_MuxPinSet( int ch )
{
    if((ch < 0) || (I2C_CH_NUM <= ch))
    {
        return -1;          /* parameter error */
    }
    else
    {
    /* MUX pin 設定 */
        if(l_tMuxTbl[ch].MUX_SCL_adrs != 0)
        {
            *((uint32_t *)l_tMuxTbl[ch].MUX_SCL_adrs) = l_tMuxTbl[ch].MUX_SCL_value;
            *((uint32_t *)l_tMuxTbl[ch].MUX_SDA_adrs) = l_tMuxTbl[ch].MUX_SDA_value;
        }
    }
    return 0;
}

/************************************************************************************************/
/* FUNCTION   : I2C1_IntrEntry                                                                  */
/*              I2C2_IntrEntry                                                                  */
/*              I2C3_IntrEntry                                                                  */
/*              I2C4_IntrEntry                                                                  */
/*              I2C5_IntrEntry                                                                  */
/*              I2C6_IntrEntry                                                                  */
/*                                                                                              */
/* DESCRIPTION: I2C割り込み発生時のエントリー処理                                               */
/*                                                                                              */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                                                                            */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void  I2C1_IntrEntry( void )
{
    I2C_handler( I2C_CH_1 );       /* I2C1 割り込み */
}

LOCAL void  I2C2_IntrEntry( void )
{
    I2C_handler( I2C_CH_2 );       /* I2C2 割り込み */
}
LOCAL void  I2C3_IntrEntry( void )
{
    I2C_handler( I2C_CH_3 );       /* I2C3 割り込み */
}
LOCAL void  I2C4_IntrEntry( void )
{
    I2C_handler( I2C_CH_4 );       /* I2C4 割り込み */
}
LOCAL void  I2C5_IntrEntry( void )
{
    I2C_handler( I2C_CH_5 );       /* I2C5 割り込み */
}
LOCAL void  I2C6_IntrEntry( void )
{
    I2C_handler( I2C_CH_6 );       /* I2C6 割り込み */
}

/************************************************************************************************/
/* FUNCTION   : I2C_GetDebugInfo                                                                */
/************************************************************************************************/
int I2C_GetDebugInfo( I2C_Debug_Type *debugInfo)
{
    debugInfo->ps_eStatus = s_eStatus;
    debugInfo->s_eChData  = s_eChData;

    return 0;
}

/************************************************************************************************/
/* FUNCTION   : I2C_MasterStop                                                                  */
/************************************************************************************************/
LOCAL int  I2C_MasterStop(int ch)
{
int                             iRet = 0;
uint32_t                        uiTime = I2C_STOP_BUSY_TIMER;
#if 0
SYSTIM                          sTime1 = {0};
SYSTIM                          sTime2 = {0};
#endif
long                            lDiffTime = 0;
ER                              err = 0;

    err++;          /* for warning fix */
    /*  STOPコンディション出力 */
    s_i2c_register[ch]->I2CR |= IMX8M_I2C_B01_I2CR_IEN;    /* I2CRを事前にセットが必要 */
    s_i2c_register[ch]->I2CR &= ~( IMX8M_I2C_B01_I2CR_MSTA | IMX8M_I2C_B01_I2CR_MTX | 
                                   IMX8M_I2C_B01_I2CR_TXAK );
#if 0
    err = get_tim((SYSTIM *)&sTime1);   /* この関数は割り込み処理内では使用不可 */
#endif
    while(1)
    {
        if( (s_i2c_register[ch]->I2SR & IMX8M_I2C_B01_I2SR_IBB) == 0)
        {
            break;
        }
#if 0
        err = get_tim((SYSTIM *)&sTime2);   /* この関数は割り込み処理内では使用不可 */
        lDiffTime = (long)I2C_ProcessTime( sTime1, sTime2 );
#else
        lDiffTime++;
#endif
        if( (long)lDiffTime >= (long)uiTime )
        {
            iRet = -1;
            break;
        }
    }
    return iRet;
}


/************************************************************************************************/
/* FUNCTION   : I2C_ProcessTime                                                                 */
/************************************************************************************************/
unsigned long I2C_ProcessTime( SYSTIM start_time, SYSTIM end_time )
{
unsigned long                           ul_ResultTime;

    ul_ResultTime = end_time.ltime - start_time.ltime;
    return ul_ResultTime;
}

void I2C_wait(int time)
{
	while(time > 0)
	{
		time--;
	}
}

#if 0
/***** for debug ******************/
void  debug_I2C1_IntrEntry( void )
{
    I2C_handler( I2C_CH_1 );       /* I2C1 割り込み */
}
#endif
/********************** end of file ( i2c_drv.c ) ****************************************/
