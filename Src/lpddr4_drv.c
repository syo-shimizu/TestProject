/************************************************************************************************/
/*                                                                                              */
/* FILE NAME                                                                    VERSION         */
/*                                                                                              */
/*      lpddr4_drv.c                                                            0.00            */
/*                                                                                              */
/* DESCRIPTION:                                                                                 */
/*                                                                                              */
/*      LPDDR4 driver                                                                           */
/*                                                                                              */
/* HISTORY                                                                                      */
/*                                                                                              */
/*      NAME            DATE        REMARKS                                                     */
/*                                                                                              */
/*      0.00            2021/01/28  Version 0.00                                                */
/*                                  Created prototype version 0.00                              */
/*                                                                                              */
/************************************************************************************************/
/************************************************************************************************/
/* Include                                                                                      */
/************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "code_rules_def.h"
#include "imx8m_reg.h"
#include "lpddr4_cfg.h"
#include "lpddr4_drv.h"

/* static prototype definitions */
LOCAL int32_t   _LPDDR4_MR_WriteBitCheck( uint32_t ulTime );
LOCAL int32_t   _LPDDR4_MR_read(int32_t iReg, uint8_t *ucReadVal);
LOCAL int32_t   _LPDDR4_MR_write(int32_t iReg, uint8_t ucDat);
LOCAL void      _LPDDR4ConfigDat_set(void);
LOCAL int       _LPDDR4_Init_MR_Set(int iDdr);
LOCAL void      _LPDDR4_Init_DDRC_Set(LPDDR4_DDRC_TABLE *pTbl);

/************************************************************************************************/
/* Internal define                                                                              */
/************************************************************************************************/
/* Check LPDDR4 contoller */
#define LPDDR4_CHK_MNFCT                (0)
#define LPDDR4_CHK_REV1                 (1)
#define LPDDR4_CHK_REV2                 (2)
#define LPDDR4_CHK_ID_NUM               (3)

/************************************************************************************************/
/* Internal valiable                                                                            */
/************************************************************************************************/
DLOCAL int32_t                  l_iDriverStatus = LPDDR4_STUS_NOTINIT;      /* LPDDR4 driver status */

/* Initial DDR4 setting value */
LOCAL const LPDDR4_DDRC_TABLE   l_atLPDDR4ConfigTable[LPDDR4_UNIT_LIST_NUM] = {
{
    /* DDR Initial setting value （LPDDR4_DDRC_TABLE）for MT53E256M32 */
    IMX8M_VINIT_MSTR_MT53,             /* DDRC_MSTR            */
    IMX8M_VINIT_MSTR2_MT53,            /* DDRC_MSTR2           */
    IMX8M_VINIT_ADDRMAP0_MT53,         /* DDRC_ADDRMAP0        */
    IMX8M_VINIT_ADDRMAP1_MT53,         /* DDRC_ADDRMAP1        */
    IMX8M_VINIT_ADDRMAP3_MT53,         /* DDRC_ADDRMAP3        */
    IMX8M_VINIT_ADDRMAP4_MT53,         /* DDRC_ADDRMAP4        */
    IMX8M_VINIT_ADDRMAP5_MT53,         /* DDRC_ADDRMAP5        */
    IMX8M_VINIT_ADDRMAP6_MT53,         /* DDRC_ADDRMAP6        */
    IMX8M_VINIT_ADDRMAP7_MT53,         /* DDRC_ADDRMAP7        */
    IMX8M_VINIT_RFSHCTL0_MT53,         /* DDRC_RFSHCTL0        */
    IMX8M_VINIT_RFSHTMG_MT53,          /* DDRC_RFSHTMG         */
    IMX8M_VINIT_INIT0_MT53,            /* DDRC_INIT0           */
    IMX8M_VINIT_INIT1_MT53,            /* DDRC_INIT1           */
    IMX8M_VINIT_RANKCTL_MT53,          /* DDRC_RANKCTL         */
    IMX8M_VINIT_DRAMTMG0_MT53,         /* DDRC_DRAMTMG0        */
    IMX8M_VINIT_DRAMTMG1_MT53,         /* DDRC_DRAMTMG1        */
    IMX8M_VINIT_DRAMTMG2_MT53,         /* DDRC_DRAMTMG2        */
    IMX8M_VINIT_DRAMTMG3_MT53,         /* DDRC_DRAMTMG3        */
    IMX8M_VINIT_DRAMTMG4_MT53,         /* DDRC_DRAMTMG4        */
    IMX8M_VINIT_DRAMTMG5_MT53,         /* DDRC_DRAMTMG5        */
    IMX8M_VINIT_DRAMTMG6_MT53,         /* DDRC_DRAMTMG6        */
    IMX8M_VINIT_DRAMTMG7_MT53,         /* DDRC_DRAMTMG7        */
    IMX8M_VINIT_DRAMTMG12_MT53,        /* DDRC_DRAMTMG12       */
    IMX8M_VINIT_DRAMTMG13_MT53,        /* DDRC_DRAMTMG13       */
    IMX8M_VINIT_DRAMTMG14_MT53,        /* DDRC_DRAMTMG14       */
    IMX8M_VINIT_DRAMTMG17_MT53,        /* DDRC_DRAMTMG17       */
    IMX8M_VINIT_ZQCTL0_MT53,           /* DDRC_ZQCTL0          */
    IMX8M_VINIT_ZQCTL1_MT53,           /* DDRC_ZQCTL1          */
    IMX8M_VINIT_DERATEEN_MT53,         /* DDRC_DERATEEN        */
    IMX8M_VINIT_DERATEINT_MT53,        /* DDRC_DERATEINT       */
    IMX8M_VINIT_ODTMAP_MT53,           /* DDRC_ODTMAP          */
    IMX8M_VINIT_PWRCTL_MT53,           /* DDRC_PWRCTL          */
    IMX8M_VINIT_INIT3_MT53,            /* DDRC_INIT3           */
    IMX8M_VINIT_INIT4_MT53,            /* DDRC_INIT4           */
    IMX8M_VINIT_INIT6_MT53,            /* DDRC_INIT6           */
    IMX8M_VINIT_INIT7_MT53,            /* DDRC_INIT7           */
    IMX8M_VINIT_ECCCFG0_MT53,          /* DDRC_ECCCFG0         */
    IMX8M_VINIT_ECCCFG1_MT53,          /* DDRC_ECCCFG1         */
    IMX8M_VINIT_DFITMG0_MT53,          /* DDRC_DFITMG0         */
    IMX8M_VINIT_DFITMG1_MT53,          /* DDRC_DFITMG1         */
    IMX8M_VINIT_DFITMG2_MT53,          /* DDRC_DFITMG2         */
    IMX8M_VINIT_DFIMISC_MT53,          /* DDRC_DFIMISC         */
    IMX8M_VINIT_DFIUPD0_MT53,          /* DDRC_DFIUPD0         */
    IMX8M_VINIT_DFIUPD1_MT53,          /* DDRC_DFIUPD1         */
    IMX8M_VINIT_DFIUPD2_MT53,          /* DDRC_DFIUPD2         */
    IMX8M_VINIT_DBICTL_MT53,           /* DDRC_DBICTL          */
    IMX8M_VINIT_DFIPHYMSTR_MT53,       /* DDRC_DFIPHYMSTR      */
    IMX8M_VINIT_SCHED_MT53,            /* DDRC_SCHED           */
    IMX8M_VINIT_SCHED1_MT53,           /* DDRC_SCHED1          */
    IMX8M_VINIT_PERFHPR1_MT53,         /* DDRC_PERFHPR1        */
    IMX8M_VINIT_PERFLPR1_MT53,         /* DDRC_PERFLPR1        */
    IMX8M_VINIT_PERFWR1_MT53,          /* DDRC_PERFWR1         */
    IMX8M_VINIT_PCCFG_MT53,            /* DDRC_PCCFG           */
    IMX8M_VINIT_PCFGR_0_MT53,          /* DDRC_PCFGR_0         */
    IMX8M_VINIT_PCFGW_0_MT53,          /* DDRC_PCFGW_0         */
    IMX8M_VINIT_PCFGQOS0_0_MT53,       /* DDRC_PCFGQOS0_0      */
    IMX8M_VINIT_PCFGQOS1_0_MT53,       /* DDRC_PCFGQOS1_0      */
    IMX8M_VINIT_PCFGWQOS0_0_MT53,      /* DDRC_PCFGWQOS0_0     */
    IMX8M_VINIT_PCFGWQOS1_0_MT53,      /* DDRC_PCFGWQOS1_0     */

    /* LPDDR4 Contoller MR configuration */
    (uint8_t)MT53_VINIT_MR4,           /* MR4                  */
    (uint8_t)MT53_VINIT_MR16,          /* MR16                 */
    (uint8_t)MT53_VINIT_MR17,          /* MR17                 */
    (uint8_t)MT53_VINIT_MR24,          /* MR24                 */
    (uint8_t)0x00,                     /* MR5                  */
    (uint8_t)0x00,                     /* MR6                  */
    (uint8_t)0x00,                     /* MR7                  */
    (uint8_t)0x00                      /* Padding              */
},
{
/* DDR Initial setting value （LPDDR4_DDRC_TABLE）for NT6AN256T32 */
    IMX8M_VINIT_MSTR_NT6A,             /* DDRC_MSTR            */
    IMX8M_VINIT_MSTR2_NT6A,            /* DDRC_MSTR2           */
    IMX8M_VINIT_ADDRMAP0_NT6A,         /* DDRC_ADDRMAP0        */
    IMX8M_VINIT_ADDRMAP1_NT6A,         /* DDRC_ADDRMAP1        */
    IMX8M_VINIT_ADDRMAP3_NT6A,         /* DDRC_ADDRMAP3        */
    IMX8M_VINIT_ADDRMAP4_NT6A,         /* DDRC_ADDRMAP4        */
    IMX8M_VINIT_ADDRMAP5_NT6A,         /* DDRC_ADDRMAP5        */
    IMX8M_VINIT_ADDRMAP6_NT6A,         /* DDRC_ADDRMAP6        */
    IMX8M_VINIT_ADDRMAP7_NT6A,         /* DDRC_ADDRMAP7        */
    IMX8M_VINIT_RFSHCTL0_NT6A,         /* DDRC_RFSHCTL0        */
    IMX8M_VINIT_RFSHTMG_NT6A,          /* DDRC_RFSHTMG         */
    IMX8M_VINIT_INIT0_NT6A,            /* DDRC_INIT0           */
    IMX8M_VINIT_INIT1_NT6A,            /* DDRC_INIT1           */
    IMX8M_VINIT_RANKCTL_NT6A,          /* DDRC_RANKCTL         */
    IMX8M_VINIT_DRAMTMG0_NT6A,         /* DDRC_DRAMTMG0        */
    IMX8M_VINIT_DRAMTMG1_NT6A,         /* DDRC_DRAMTMG1        */
    IMX8M_VINIT_DRAMTMG2_NT6A,         /* DDRC_DRAMTMG2        */
    IMX8M_VINIT_DRAMTMG3_NT6A,         /* DDRC_DRAMTMG3        */
    IMX8M_VINIT_DRAMTMG4_NT6A,         /* DDRC_DRAMTMG4        */
    IMX8M_VINIT_DRAMTMG5_NT6A,         /* DDRC_DRAMTMG5        */
    IMX8M_VINIT_DRAMTMG6_NT6A,         /* DDRC_DRAMTMG6        */
    IMX8M_VINIT_DRAMTMG7_NT6A,         /* DDRC_DRAMTMG7        */
    IMX8M_VINIT_DRAMTMG12_NT6A,        /* DDRC_DRAMTMG12       */
    IMX8M_VINIT_DRAMTMG13_NT6A,        /* DDRC_DRAMTMG13       */
    IMX8M_VINIT_DRAMTMG14_NT6A,        /* DDRC_DRAMTMG14       */
    IMX8M_VINIT_DRAMTMG17_NT6A,        /* DDRC_DRAMTMG17       */
    IMX8M_VINIT_ZQCTL0_NT6A,           /* DDRC_ZQCTL0          */
    IMX8M_VINIT_ZQCTL1_NT6A,           /* DDRC_ZQCTL1          */
    IMX8M_VINIT_DERATEEN_NT6A,         /* DDRC_DERATEEN        */
    IMX8M_VINIT_DERATEINT_NT6A,        /* DDRC_DERATEINT       */
    IMX8M_VINIT_ODTMAP_NT6A,           /* DDRC_ODTMAP          */
    IMX8M_VINIT_PWRCTL_NT6A,           /* DDRC_PWRCTL          */
    IMX8M_VINIT_INIT3_NT6A,            /* DDRC_INIT3           */
    IMX8M_VINIT_INIT4_NT6A,            /* DDRC_INIT4           */
    IMX8M_VINIT_INIT6_NT6A,            /* DDRC_INIT6           */
    IMX8M_VINIT_INIT7_NT6A,            /* DDRC_INIT7           */
    IMX8M_VINIT_ECCCFG0_NT6A,          /* DDRC_ECCCFG0         */
    IMX8M_VINIT_ECCCFG1_NT6A,          /* DDRC_ECCCFG1         */
    IMX8M_VINIT_DFITMG0_NT6A,          /* DDRC_DFITMG0         */
    IMX8M_VINIT_DFITMG1_NT6A,          /* DDRC_DFITMG1         */
    IMX8M_VINIT_DFITMG2_NT6A,          /* DDRC_DFITMG2         */
    IMX8M_VINIT_DFIMISC_NT6A,          /* DDRC_DFIMISC         */
    IMX8M_VINIT_DFIUPD0_NT6A,          /* DDRC_DFIUPD0         */
    IMX8M_VINIT_DFIUPD1_NT6A,          /* DDRC_DFIUPD1         */
    IMX8M_VINIT_DFIUPD2_NT6A,          /* DDRC_DFIUPD2         */
    IMX8M_VINIT_DBICTL_NT6A,           /* DDRC_DBICTL          */
    IMX8M_VINIT_DFIPHYMSTR_NT6A,       /* DDRC_DFIPHYMSTR      */
    IMX8M_VINIT_SCHED_NT6A,            /* DDRC_SCHED           */
    IMX8M_VINIT_SCHED1_NT6A,           /* DDRC_SCHED1          */
    IMX8M_VINIT_PERFHPR1_NT6A,         /* DDRC_PERFHPR1        */
    IMX8M_VINIT_PERFLPR1_NT6A,         /* DDRC_PERFLPR1        */
    IMX8M_VINIT_PERFWR1_NT6A,          /* DDRC_PERFWR1         */
    IMX8M_VINIT_PCCFG_NT6A,            /* DDRC_PCCFG           */
    IMX8M_VINIT_PCFGR_0_NT6A,          /* DDRC_PCFGR_0         */
    IMX8M_VINIT_PCFGW_0_NT6A,          /* DDRC_PCFGW_0         */
    IMX8M_VINIT_PCFGQOS0_0_NT6A,       /* DDRC_PCFGQOS0_0      */
    IMX8M_VINIT_PCFGQOS1_0_NT6A,       /* DDRC_PCFGQOS1_0      */
    IMX8M_VINIT_PCFGWQOS0_0_NT6A,      /* DDRC_PCFGWQOS0_0     */
    IMX8M_VINIT_PCFGWQOS1_0_NT6A,      /* DDRC_PCFGWQOS1_0     */

    /* LPDDR4 Contoller MR configuration */
    (uint8_t)NT6A_VINIT_MR4,           /* MR4                  */
    (uint8_t)NT6A_VINIT_MR16,          /* MR16                 */
    (uint8_t)NT6A_VINIT_MR17,          /* MR17                 */
    (uint8_t)NT6A_VINIT_MR24,          /* MR24                 */
    (uint8_t)0x00,                     /* MR5                  */
    (uint8_t)0x00,                     /* MR6                  */
    (uint8_t)0x00,                     /* MR7                  */
    (uint8_t)0x00                      /* Padding              */
}
};

/************************************************************************************************/
/* FUNCTION   : LPDDR4_Init                                                                     */
/*                                                                                              */
/* DESCRIPTION: Initialize LPDDR4 memory                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                                                                            */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : LPDDR4_ERCD_SUCCESS     Successfully return                                     */
/*              LPDDR4_ERCD_CANTEXEC    Already called the function                             */
/*              LPDDR4_ERCD_UNIT        Invalid LPDDR4 controller                               */
/*              LPDDR4_ERCD_HARDFAULT   LPDDR4 hardware access fault                            */
/*                                                                                              */
/************************************************************************************************/
int32_t LPDDR4_Init(void)
{
int32_t                         iStatus       = LPDDR4_ERCD_UNIT;
int32_t                         iDdr          = 0;
int32_t                         iWk           = l_iDriverStatus;        /* save current DriveStatus */
uint8_t                         aucChk_val[3] = {0};

    /* status check .... */
    if(LPDDR4_STUS_NOTINIT != l_iDriverStatus)     /* setting state to none-init */
    {
        return LPDDR4_ERCD_CANTEXEC;                    /* Already called the function */
    }
    else
    {
        /* nothing */
    }

    /* all bank suspend disable */
    iStatus = LPDDR4_SuspMask(LPDDR4_BANK_ALL);
    if(LPDDR4_ERCD_SUCCESS != iStatus)
    {
        return iStatus;
    }
    else
    {
        /* nothing */
    }

    /* DDR4 Controller Identification Reading */
    iStatus = _LPDDR4_MR_read(LPDDR4_MR_ManufacturerID, &aucChk_val[LPDDR4_CHK_MNFCT]);
    if(LPDDR4_ERCD_SUCCESS == iStatus)
    {
        iStatus = _LPDDR4_MR_read(LPDDR4_MR_RevisionID1, &aucChk_val[LPDDR4_CHK_REV1]);
        if(LPDDR4_ERCD_SUCCESS == iStatus)
        {
            iStatus = _LPDDR4_MR_read(LPDDR4_MR_RevisionID2, &aucChk_val[LPDDR4_CHK_REV2]);
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
    if(LPDDR4_ERCD_SUCCESS != iStatus)
    {
        return iStatus;
    }
    else 
    {
        /* nothing */
    }

    /* MT53E256M32 controler check */
    if(((uint8_t)LPDDR4_MT53_MNFCT == aucChk_val[LPDDR4_CHK_MNFCT]) &&
       ((uint8_t)LPDDR4_MT53_REV1  == aucChk_val[LPDDR4_CHK_REV1]) &&
       ((uint8_t)LPDDR4_MT53_REV2  == aucChk_val[LPDDR4_CHK_REV2]))
    {
        /* Select MT53E256M32 */
        iDdr = LPDDR4_UNIT_MT53;
    }
    /* NT6AN256T32  controler check  */
    else if(((uint8_t)LPDDR4_NT6A_MNFCT == aucChk_val[LPDDR4_CHK_MNFCT]) &&
            ((uint8_t)LPDDR4_NT6A_REV1  == aucChk_val[LPDDR4_CHK_REV1]) &&
            ((uint8_t)LPDDR4_NT6A_REV2  == aucChk_val[LPDDR4_CHK_REV2]))
    {
        /* Select MT53E256M32 */
        iDdr = LPDDR4_UNIT_NT6A;
    }
    else
    {
        /* Invalid LPDDR4 controller */
        return LPDDR4_ERCD_UNIT;
    }

    /* set to LPDDR4 controller */
    iStatus = _LPDDR4_Init_MR_Set(iDdr);

    /* LPDDR4 driver status set */
    if (LPDDR4_ERCD_SUCCESS == iStatus)
    {
        l_iDriverStatus = LPDDR4_STUS_ACTIVE;      /* normal state     */
    }

    return iStatus; 
}

/************************************************************************************************/
/* FUNCTION   : LPDDR4_SuspMask                                                                 */
/*                                                                                              */
/* DESCRIPTION: Set suspend area                                                                */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : usBank                  Bit[15:0] Suspend bank pattern                          */
/*                                      '0':Enable suspend  '1':Disable suspend                 */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : LPDDR4_ERCD_SUCCESS     Successfully return                                     */
/*              LPDDR4_ERCD_CANTEXEC    Not initialized or already suspended                    */
/*              LPDDR4_ERCD_HARDFAULT   LPDDR4 hardware access fault                            */
/*                                                                                              */
/************************************************************************************************/
int32_t LPDDR4_SuspMask(uint16_t usBank)
{
int32_t                         iStatus = LPDDR4_ERCD_CANTEXEC;
uint32_t                        ulReg;

    if(l_iDriverStatus != LPDDR4_STUS_ACTIVE)       /* normal state     */
    {
        return LPDDR4_ERCD_CANTEXEC;                     /* Not initialized or already suspended  */
    }
    else
    {
        /* nothing */
    }
    /* bank 0 ~ 7 */
    DDRC_R32_WR(IMX8M_DDRC_A32_MRCTRL2, IMX8M_VBANK_MRCTRL2_BANK0_7);
    iStatus = _LPDDR4_MR_write(LPDDR4_MR_16, (uint8_t)(LPDDR4_SUSPENDMASK_BANK & usBank));
    if(iStatus == LPDDR4_ERCD_SUCCESS)
    {
        /* bank 8 ~ 15 */
        DDRC_R32_WR(IMX8M_DDRC_A32_MRCTRL2, IMX8M_VBANK_MRCTRL2_BANK8_15);
        iStatus = _LPDDR4_MR_write(LPDDR4_MR_16, (uint8_t)(usBank >> 8));
    }
    else 
    {
        /* nothing */
    }
    return iStatus; 
}


/************************************************************************************************/
/* FUNCTION   : LPDDR4_Suspend                                                                  */
/*                                                                                              */
/* DESCRIPTION: Suspend LPDDR4 memory                                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ulWait                  Wait time(Unit:Input clockx32 Range:0-31)               */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : LPDDR4_ERCD_SUCCESS     Successfully return                                     */
/*              LPDDR4_ERCD_PARAM       Invalid parameter                                       */
/*              LPDDR4_ERCD_CANTEXEC    Not initialized or already suspended                    */
/*                                                                                              */
/************************************************************************************************/
int32_t LPDDR4_Suspend(uint32_t ulWait)
{
uint32_t                        ulReg;

    /* ulWait range check */
    if( (LPDDR4_SUSPENDWAIT_LOWER <= ulWait) && (ulWait <= LPDDR4_SUSPENDWAIT_UPPER) )
    {
        /* nothing */
    }
    else
    {
        return LPDDR4_ERCD_PARAM;                    /* Invalid parameter */
    }
    if( l_iDriverStatus != LPDDR4_STUS_ACTIVE)       /* normal state     */
    {
        return LPDDR4_ERCD_CANTEXEC;                     /* Not initialized or already suspended */
    }
    else
    {
        /* nothing */
    }
    /* PWRTMG setting */
    ulReg =     DDRC_R32_RD(IMX8M_DDRC_A32_PWRTMG);
    ulReg =     (ulReg & (~LPDDR4_SUSPENDMASK_WAIT)) & (LPDDR4_SUSPENDMASK_WAIT & ulWait);
    DDRC_R32_WR(IMX8M_DDRC_A32_PWRTMG, ulReg);
    /* DRAM PLL CLK E = 0  */
    ulReg =     CCM_R32_RD(IMX8M_CCMANA_A32_DRAMPLLGENCTRL);
    ulReg &=    ~IMX8M_B01_PLL_CLKE;
    CCM_R32_WR(IMX8M_CCMANA_A32_DRAMPLLGENCTRL, ulReg);
    /* Status setting */
    l_iDriverStatus = LPDDR4_STUS_PWSAVE;            /* power saving mode  */
    
    /* Successfully return */
    return LPDDR4_ERCD_SUCCESS; 
}

/************************************************************************************************/
/* FUNCTION   : LPDDR4_Resume                                                                   */
/*                                                                                              */
/* DESCRIPTION: Resume LPDDR4 memory                                                            */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : none                                                                            */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : LPDDR4_ERCD_SUCCESS     Successfully return                                     */
/*              LPDDR4_ERCD_CANTEXEC    Not initialized or not suspended                        */
/*                                                                                              */
/************************************************************************************************/
int32_t LPDDR4_Resume(void)
{
uint32_t                        ulReg;

    if( LPDDR4_STUS_PWSAVE != l_iDriverStatus)              /* power saving mode  */
    {
        return LPDDR4_ERCD_CANTEXEC;                            /* Not initialized or not suspended */
    }
    else
    {
        /* nothing */
    }
    /* DRAM PLL CLK E = 1  */
    ulReg =     CCM_R32_RD(IMX8M_CCMANA_A32_DRAMPLLGENCTRL);
    ulReg |=    IMX8M_B01_PLL_CLKE;
    CCM_R32_WR(IMX8M_CCMANA_A32_DRAMPLLGENCTRL, ulReg);

    /* Successfully return */
    return LPDDR4_ERCD_SUCCESS; 
}

/************************************************************************************************/
/*  Internal functions                                                                          */
/************************************************************************************************/

/************************************************************************************************/
/* FUNCTION   : _LPDDR4_MR_WriteBitCheck                                                         */
/*                                                                                              */
/* DESCRIPTION: LPDDR4 DDRC_MRCTRL0 MR bit Write result check                                   */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : ulTime                  TimeOut check count value                               */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : LPDDR4_ERCD_SUCCESS     Successfully return                                     */
/*              LPDDR4_ERCD_HARDFAULT   LPDDR4 hardware access fault                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL int32_t _LPDDR4_MR_WriteBitCheck( uint32_t ulTime )
{
int32_t                         iRet = LPDDR4_ERCD_SUCCESS;
uint32_t                        ulReg;

    /* 1st read */
    ulReg = DDRC_R32_RD(IMX8M_DDRC_A32_MRCTRL0);

    while((ulReg & IMX8M_B01_MR_WR) != 0)
    {
        if(0 < ulTime)
        {
            /* Timeout check */
            ulTime--;
            if(0 == ulTime)
            {
                iRet     = LPDDR4_ERCD_HARDFAULT;        /* LPDDR Time-Out Occured */
                break;
            }
        }
        else
        {
            /* Infinity wait, no operation */
        }

        /* read */
        ulReg = DDRC_R32_RD(IMX8M_DDRC_A32_MRCTRL0);
    }
    return  iRet;
}

/************************************************************************************************/
/* FUNCTION   : LPDDR4_MR_read                                                                  */
/*                                                                                              */
/* DESCRIPTION: LPDDR4 MR register read                                                         */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : iReg                     MR register                                            */
/*              ucReadVal                Stored address of Read value                           */
/* OUTPUT     : ucReadVal                Read value (when result is LPDDR4_ERCD_SUCCESS)        */
/*                                                                                              */
/* RESULTS    : LPDDR4_ERCD_SUCCESS      Successfully return                                    */
/*              LPDDR4_ERCD_HARDFAULT    LPDDR4 hardware access fault                           */
/*                                                                                              */
/************************************************************************************************/
LOCAL int32_t _LPDDR4_MR_read(int32_t iReg, uint8_t *ucReadVal)
{
int32_t                         iRet  = LPDDR4_ERCD_SUCCESS;
uint32_t                        ulReg0;

    /* MR register set */
    DDRC_R32_WR(IMX8M_DDRC_A32_MRCTRL1, (IMX8M_VMASK_MRCTRL1_MRADDR & (iReg << DDRC_MRCTRL1_SHIFT_ADRS)));
    ulReg0 =    DDRC_R32_RD(IMX8M_DDRC_A32_MRCTRL0);
    ulReg0 |=   IMX8M_B01_MR_TYPE;
    DDRC_R32_WR(IMX8M_DDRC_A32_MRCTRL0, ulReg0);    
    /* check IMX8M_DDRC_A32_MRCTRL0 :  IMX8M_B01_MR_WR bit */
    iRet = _LPDDR4_MR_WriteBitCheck(LPDDR4_TIMEOOUT_VALUE);
    if(LPDDR4_ERCD_SUCCESS == iRet)
    {
        /* read MR register value & result setting */
        *ucReadVal        = (uint8_t)(DDRC_R32_RD(IMX8M_DDRC_A32_MRCTRL1) & IMX8M_VMASK_MRCTRL1_MRDATA);
    }
    else {
        /* nothing */
    }
    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : LPDDR4_MR_write                                                                 */
/*                                                                                              */
/* DESCRIPTION: LPDDR4 MR register write                                                        */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : iReg                    MR register                                             */
/*              ucDat                   Data of writting                                        */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : LPDDR4_ERCD_SUCCESS     Successfully return                                     */
/*              LPDDR4_ERCD_HARDFAULT   LPDDR4 hardware access fault                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL int32_t _LPDDR4_MR_write(int32_t iReg, uint8_t ucDat)
{
int32_t                         iRet  = LPDDR4_ERCD_SUCCESS;
uint32_t                        ulReg;

    /* MR register & data set */
    DDRC_R32_WR(IMX8M_DDRC_A32_MRCTRL1, ((iReg << DDRC_MRCTRL1_SHIFT_ADRS) | (IMX8M_VMASK_MRCTRL1_MRDATA & ucDat)));
    ulReg =     DDRC_R32_RD(IMX8M_DDRC_A32_MRCTRL0);
    ulReg &=    ~(IMX8M_B01_MR_TYPE);                     /* write mode */
    ulReg |=    IMX8M_B01_MR_WR;                          /* write execute */
    DDRC_R32_WR(IMX8M_DDRC_A32_MRCTRL0, ulReg);
    /* check IMX8M_DDRC_A32_MRCTRL0 :  IMX8M_B01_MR_WR bit */
    iRet = _LPDDR4_MR_WriteBitCheck(LPDDR4_TIMEOOUT_VALUE);

    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : _LPDDR4_Init_MR_Set                                                              */
/*                                                                                              */
/* DESCRIPTION: LPDDR4 Controller MR register setting                                           */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : iDdr                    LPDDR contoller unit ID                                 */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : LPDDR4_ERCD_SUCCESS     Successfully return                                     */
/*              LPDDR4_ERCD_HARDFAULT   LPDDR4 hardware access fault                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL int _LPDDR4_Init_MR_Set(int iDdr)
{
int                             iRet;                                   /* Return code */

    /* set to LPDDR4 controller */
    iRet = _LPDDR4_MR_write(LPDDR4_MR_4 , l_atLPDDR4ConfigTable[iDdr].ulMR4);    /* MR4   */
    if(LPDDR4_ERCD_SUCCESS != iRet)
    {
        iRet = _LPDDR4_MR_write(LPDDR4_MR_16, l_atLPDDR4ConfigTable[iDdr].ulMR16);    /* MR16  */
        if(LPDDR4_ERCD_SUCCESS != iRet)
        {
            /* nothing */
        }
        else
        {
            iRet = _LPDDR4_MR_write(LPDDR4_MR_17, l_atLPDDR4ConfigTable[iDdr].ulMR17);    /* MR17  */
            if(LPDDR4_ERCD_SUCCESS != iRet)
            {
                /* nothing */
            }
            else
            {
                iRet = _LPDDR4_MR_write(LPDDR4_MR_24, l_atLPDDR4ConfigTable[iDdr].ulMR24);    /* MR24  */
                if(LPDDR4_ERCD_SUCCESS != iRet)
                {
                    /* nothing */
                }
                else
                {
                    /* i.MX 8M Plus DDR controller setting  */
                    _LPDDR4_Init_DDRC_Set((LPDDR4_DDRC_TABLE *)&l_atLPDDR4ConfigTable[iDdr]);
                }
            }
        }
    }

    return iRet;
}

/************************************************************************************************/
/* FUNCTION   : _LPDDR4_Init_DDRC_Set                                                           */
/*                                                                                              */
/* DESCRIPTION: i.MX 8M Plus DDR Controller register setting                                    */
/*----------------------------------------------------------------------------------------------*/
/* INPUT      : LPDDR4_DDRC_TABLE *pTbl    Pointer to LPDDR4_DDRC_TABLE data                    */
/*                                                                                              */
/* OUTPUT     : none                                                                            */
/*                                                                                              */
/* RESULTS    : none                                                                            */
/*                                                                                              */
/************************************************************************************************/
LOCAL void _LPDDR4_Init_DDRC_Set(LPDDR4_DDRC_TABLE *pTbl)
{
    if(NULL == pTbl)
    {
        /* nothing */
    }
    else
    {
        /* Set registers */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ0LNSEL2, pTbl->ulDq0LnSel_2);    /* DDR_PHY_Dq0LnSel_2   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ1LNSEL2, pTbl->ulDq1LnSel_2);    /* DDR_PHY_Dq1LnSel_2   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ2LNSEL2, pTbl->ulDq2LnSel_2);    /* DDR_PHY_Dq2LnSel_2   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ3LNSEL2, pTbl->ulDq3LnSel_2);    /* DDR_PHY_Dq3LnSel_2   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ4LNSEL2, pTbl->ulDq4LnSel_2);    /* DDR_PHY_Dq4LnSel_2   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ5LNSEL2, pTbl->ulDq5LnSel_2);    /* DDR_PHY_Dq5LnSel_2   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ6LNSEL2, pTbl->ulDq6LnSel_2);    /* DDR_PHY_Dq6LnSel_2   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ7LNSEL2, pTbl->ulDq7LnSel_2);    /* DDR_PHY_Dq7LnSel_2   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ0LNSEL3, pTbl->ulDq0LnSel_3);    /* DDR_PHY_Dq0LnSel_3   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ1LNSEL3, pTbl->ulDq1LnSel_3);    /* DDR_PHY_Dq1LnSel_3   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ2LNSEL3, pTbl->ulDq2LnSel_3);    /* DDR_PHY_Dq2LnSel_3   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ3LNSEL3, pTbl->ulDq3LnSel_3);    /* DDR_PHY_Dq3LnSel_3   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ4LNSEL3, pTbl->ulDq4LnSel_3);    /* DDR_PHY_Dq4LnSel_3   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ5LNSEL3, pTbl->ulDq5LnSel_3);    /* DDR_PHY_Dq5LnSel_3   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ6LNSEL3, pTbl->ulDq6LnSel_3);    /* DDR_PHY_Dq6LnSel_3   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ7LNSEL3, pTbl->ulDq7LnSel_3);    /* DDR_PHY_Dq7LnSel_3   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ0LNSEL1, pTbl->ulDq0LnSel_1);    /* DDR_PHY_Dq0LnSel_1   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ1LNSEL1, pTbl->ulDq1LnSel_1);    /* DDR_PHY_Dq1LnSel_1   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ2LNSEL1, pTbl->ulDq2LnSel_1);    /* DDR_PHY_Dq2LnSel_1   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ3LNSEL1, pTbl->ulDq3LnSel_1);    /* DDR_PHY_Dq3LnSel_1   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ4LNSEL1, pTbl->ulDq4LnSel_1);    /* DDR_PHY_Dq4LnSel_1   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ5LNSEL1, pTbl->ulDq5LnSel_1);    /* DDR_PHY_Dq5LnSel_1   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ6LNSEL1, pTbl->ulDq6LnSel_1);    /* DDR_PHY_Dq6LnSel_1   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ7LNSEL1, pTbl->ulDq7LnSel_1);    /* DDR_PHY_Dq7LnSel_1   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ0LNSEL0, pTbl->ulDq0LnSel_0);    /* DDR_PHY_Dq0LnSel_0   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ1LNSEL0, pTbl->ulDq1LnSel_0);    /* DDR_PHY_Dq1LnSel_0   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ2LNSEL0, pTbl->ulDq2LnSel_0);    /* DDR_PHY_Dq2LnSel_0   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ3LNSEL0, pTbl->ulDq3LnSel_0);    /* DDR_PHY_Dq3LnSel_0   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ4LNSEL0, pTbl->ulDq4LnSel_0);    /* DDR_PHY_Dq4LnSel_0   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ5LNSEL0, pTbl->ulDq5LnSel_0);    /* DDR_PHY_Dq5LnSel_0   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ6LNSEL0, pTbl->ulDq6LnSel_0);    /* DDR_PHY_Dq6LnSel_0   */
        DDRC_R32_WR(IMX8M_DDRPHY_A32_DQ7LNSEL0, pTbl->ulDq7LnSel_0);    /* DDR_PHY_Dq7LnSel_0   */
        DDRC_R32_WR(IMX8M_DDRC_A32_MSTR,        pTbl->ulMSTR);          /* DDRC_MSTR            */
        DDRC_R32_WR(IMX8M_DDRC_A32_MSTR2,       pTbl->ulMSTR2);         /* DDRC_MSTR2           */
        DDRC_R32_WR(IMX8M_DDRC_A32_ADDRMAP0,    pTbl->ulADDRMAP0);      /* DDRC_ADDRMAP0        */
        DDRC_R32_WR(IMX8M_DDRC_A32_ADDRMAP1,    pTbl->ulADDRMAP1);      /* DDRC_ADDRMAP1        */
        DDRC_R32_WR(IMX8M_DDRC_A32_ADDRMAP3,    pTbl->ulADDRMAP3);      /* DDRC_ADDRMAP3        */
        DDRC_R32_WR(IMX8M_DDRC_A32_ADDRMAP4,    pTbl->ulADDRMAP4);      /* DDRC_ADDRMAP4        */
        DDRC_R32_WR(IMX8M_DDRC_A32_ADDRMAP5,    pTbl->ulADDRMAP5);      /* DDRC_ADDRMAP5        */
        DDRC_R32_WR(IMX8M_DDRC_A32_ADDRMAP6,    pTbl->ulADDRMAP6);      /* DDRC_ADDRMAP6        */
        DDRC_R32_WR(IMX8M_DDRC_A32_ADDRMAP7,    pTbl->ulADDRMAP7);      /* DDRC_ADDRMAP7        */
        DDRC_R32_WR(IMX8M_DDRC_A32_RFSHCTL0,    pTbl->ulRFSHCTL0);      /* DDRC_RFSHCTL0        */
        DDRC_R32_WR(IMX8M_DDRC_A32_RFSHTMG,     pTbl->ulRFSHTMG0);      /* DDRC_RFSHTMG         */
        DDRC_R32_WR(IMX8M_DDRC_A32_INIT0,       pTbl->ulINIT0);         /* DDRC_INIT0           */
        DDRC_R32_WR(IMX8M_DDRC_A32_INIT1,       pTbl->ulINIT1);         /* DDRC_INIT1           */
        DDRC_R32_WR(IMX8M_DDRC_A32_RANKCTL,     pTbl->ulRANKCTL);       /* DDRC_RANKCTL         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG0,    pTbl->ulDRAMTMG0);      /* DDRC_DRAMTMG0        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG1,    pTbl->ulDRAMTMG1);      /* DDRC_DRAMTMG1        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG2,    pTbl->ulDRAMTMG2);      /* DDRC_DRAMTMG2        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG3,    pTbl->ulDRAMTMG3);      /* DDRC_DRAMTMG3        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG4,    pTbl->ulDRAMTMG4);      /* DDRC_DRAMTMG4        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG5,    pTbl->ulDRAMTMG5);      /* DDRC_DRAMTMG5        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG6,    pTbl->ulDRAMTMG6);      /* DDRC_DRAMTMG6        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG7,    pTbl->ulDRAMTMG7);      /* DDRC_DRAMTMG7        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG12,   pTbl->ulDRAMTMG12);     /* DDRC_DRAMTMG12       */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG13,   pTbl->ulDRAMTMG13);     /* DDRC_DRAMTMG13       */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG14,   pTbl->ulDRAMTMG14);     /* DDRC_DRAMTMG14       */
        DDRC_R32_WR(IMX8M_DDRC_A32_DRAMTMG17,   pTbl->ulDRAMTMG17);     /* DDRC_DRAMTMG17       */
        DDRC_R32_WR(IMX8M_DDRC_A32_ZQCTL0,      pTbl->ulZQCTL0);        /* DDRC_ZQCTL0          */
        DDRC_R32_WR(IMX8M_DDRC_A32_ZQCTL1,      pTbl->ulZQCTL1);        /* DDRC_ZQCTL1          */
        DDRC_R32_WR(IMX8M_DDRC_A32_DERATEEN,    pTbl->ulDERATEEN);      /* DDRC_DERATEEN        */
        DDRC_R32_WR(IMX8M_DDRC_A32_DERATEINT,   pTbl->ulDERATEINT);     /* DDRC_DERATEINT       */
        DDRC_R32_WR(IMX8M_DDRC_A32_ODTMAP,      pTbl->ulODTMAP);        /* DDRC_ODTMAP          */
        DDRC_R32_WR(IMX8M_DDRC_A32_PWRCTL,      pTbl->ulPWRCTL);        /* DDRC_PWRCTL          */
        DDRC_R32_WR(IMX8M_DDRC_A32_INIT3,       pTbl->ulINIT3);         /* DDRC_INIT3           */
        DDRC_R32_WR(IMX8M_DDRC_A32_INIT4,       pTbl->ulINIT4);         /* DDRC_INIT4           */
        DDRC_R32_WR(IMX8M_DDRC_A32_INIT6,       pTbl->ulINIT6);         /* DDRC_INIT6           */
        DDRC_R32_WR(IMX8M_DDRC_A32_INIT7,       pTbl->ulINIT7);         /* DDRC_INIT7           */
        DDRC_R32_WR(IMX8M_DDRC_A32_ECCCFG0,     pTbl->ulECCCFG0);       /* DDRC_ECCCFG0         */
        DDRC_R32_WR(IMX8M_DDRC_A32_ECCCFG1,     pTbl->ulECCCFG1);       /* DDRC_ECCCFG1         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DFITMG0,     pTbl->ulDFITMG0);       /* DDRC_DFITMG0         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DFITMG1,     pTbl->ulDFITMG1);       /* DDRC_DFITMG1         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DFITMG2,     pTbl->ulDFITMG2);       /* DDRC_DFITMG2         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DFIMISC,     pTbl->ulDFIMISC);       /* DDRC_DFIMISC         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DFIUPD0,     pTbl->ulDFIUPD0);       /* DDRC_DFIUPD0         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DFIUPD1,     pTbl->ulDFIUPD1);       /* DDRC_DFIUPD1         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DFIUPD2,     pTbl->ulDFIUPD2);       /* DDRC_DFIUPD2         */
        DDRC_R32_WR(IMX8M_DDRC_A32_DBICTL,      pTbl->ulDBICTL);        /* DDRC_DBICTL          */
        DDRC_R32_WR(IMX8M_DDRC_A32_DFIPHYMSTR,  pTbl->ulDFIPHYMSTR);    /* DDRC_DFIPHYMSTR      */
        DDRC_R32_WR(IMX8M_DDRC_A32_SCHED,       pTbl->ulSCHED);         /* DDRC_SCHED           */
        DDRC_R32_WR(IMX8M_DDRC_A32_SCHED1,      pTbl->ulSCHED1);        /* DDRC_SCHED1          */
        DDRC_R32_WR(IMX8M_DDRC_A32_PERFHPR1,    pTbl->ulPERFHPR1);      /* DDRC_PERFHPR1        */
        DDRC_R32_WR(IMX8M_DDRC_A32_PERFLPR1,    pTbl->ulPERFLPR1);      /* DDRC_PERFLPR1        */
        DDRC_R32_WR(IMX8M_DDRC_A32_PERFWR1,     pTbl->ulPERFWR1);       /* DDRC_PERFWR1         */
        DDRC_R32_WR(IMX8M_DDRC_A32_PCCFG,       pTbl->ulPCCFG);         /* DDRC_PCCFG           */
        DDRC_R32_WR(IMX8M_DDRC_A32_PCFGR_0,     pTbl->ulPCFGR_0);       /* DDRC_PCFGR_0         */
        DDRC_R32_WR(IMX8M_DDRC_A32_PCFGW_0,     pTbl->ulPCFGW_0);       /* DDRC_PCFGW_0         */
        DDRC_R32_WR(IMX8M_DDRC_A32_PCFGQOS0_0,  pTbl->ulPCFGQOS_0);     /* DDRC_PCFGQOS0_0      */
        DDRC_R32_WR(IMX8M_DDRC_A32_PCFGQOS1_0,  pTbl->ulPCFGQOS_1);     /* DDRC_PCFGQOS1_0      */
        DDRC_R32_WR(IMX8M_DDRC_A32_PCFGWQOS0_0, pTbl->ulPCFGWQOS_0);    /* DDRC_PCFGWQOS0_0     */
        DDRC_R32_WR(IMX8M_DDRC_A32_PCFGWQOS1_0, pTbl->ulPCFGWQOS_1);    /* DDRC_PCFGWQOS1_0     */
    }
}

