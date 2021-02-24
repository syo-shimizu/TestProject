/************************************************************************************************/
/*                                                                                              */
/* FILE NAME                                                                    VERSION         */
/*                                                                                              */
/*      dri_flexspi_local.h                                                     0.00            */
/*                                                                                              */
/* DESCRIPTION:                                                                                 */
/*                                                                                              */
/*      FlexSPIドライバヘッダファイル(ローカル定義)                                             */
/*                                                                                              */
/* HISTORY                                                                                      */
/*                                                                                              */
/*      NAME            DATE        REMARKS                                                     */
/*                                                                                              */
/*      K.Hatano        2021/01/26  Version 0.00                                                */
/*                                  新規作成(お客様提供ソースをベースに修正)                    */
/*                                                                                              */
/************************************************************************************************/
#ifndef _DRI_FLEXSPI_LOCAL_H_
#define _DRI_FLEXSPI_LOCAL_H_

/****************************************************************************/
/*  インクルードファイル                                                    */
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/****************************************************************************/
/*  定数・マクロ定義                                                        */
/****************************************************************************/

/* レジスタビット・マスク */

/* MCR0 */
#define FlexSPI_MCR0_AHBGRANTWAIT_MASK              (0xFF000000U)
#define FlexSPI_MCR0_IPGRANTWAIT_MASK               (0x00FF0000U)
#define FlexSPI_MCR0_LEARNEN_MASK                   (0x00008000U)
#define FlexSPI_MCR0_RESERVED_MASK                  (0x0000000CU)
#define FlexSPI_MCR0_MDIS_MASK                      (0x00000002U)
#define FlexSPI_MCR0_SWRESET_MASK                   (0x00000001U)
#define FlexSPI_MCR0_RXCLKSRC_SHIFT                 (4U)

/* MCR2 */
#define FlexSPI_MCR2_CLRLEARNPHASE_MASK             (0x00004000U)
#define FlexSPI_MCR2_RESERVED_MASK                  (0x00F737FFU)

/* AHBCR */
#define FlexSPI_AHBCR_PREFETCHEN_MASK               (0x00000020U)

/* INTEN */
#define FlexSPI_INTEN_IPCMDDONE                     (0x00000001U)

/* INTR */
#define FlexSPI_INTR_IPRXWA_MASK                    (0x00000020U)
#define FlexSPI_INTR_IPCMDDONE_MASK                 (0x00000001U)

/* AHBRXBUFCR0 */
#define FlexSPI_AHBRXBUFCR0_PREFETCHEN_MASK         (0x80000000U)

/* FLSHCR2 */
#define FlexSPI_FLSHCR2_AWRWAITUNIT_MASK            (0x70000000U)
#define FlexSPI_FLSHCR2_AWRWAIT_MASK                (0x0FFF0000U)
#define FlexSPI_FLSHCR2_AWRSEQNUM_MASK              (0x0000E000U)
#define FlexSPI_FLSHCR2_AWRSEQID_MASK               (0x00001F00U)
#define FlexSPI_FLSHCR2_ARDSEQNUM_MASK              (0x000000E0U)
#define FlexSPI_FLSHCR2_ARDSEQID_MASK               (0x0000001FU)

/* FLSHCR4 */
#define FlexSPI_FLSHCR4_WMENB_MASK                  (0x00000008U)
#define FlexSPI_FLSHCR4_WMENA_MASK                  (0x00000004U)
#define FlexSPI_FLSHCR4_WMOPT1_MASK                 (0x00000001U)

/* IPRXFCR */
#define FlexSPI_IPRXFCR_RXWMRK_MASK                 (0x000000FCU)
#define FlexSPI_IPRXFCR_RXDMAEN_MASK                (0x00000002U)

/* IPTXFCR */
#define FlexSPI_IPTXFCR_TXWMRK_MASK                 (0x000001FCU)
#define FlexSPI_IPTXFCR_TXDMAEN_MASK                (0x00000002U)

/* DLLCR */
#define FlexSPI_DLLCR_DLLEN_MASK                    (0x00000001U)

/* STS0 */
#define FlexSPI_STS0_ARBIDLE_MASK                   (0x00000002U)
#define FlexSPI_STS0_SEQIDLE_MASK                   (0x00000001U)

/* STS2 */
#define FlexSPI_STS2_BREFLOCK_MASK                  (0x00020000U)
#define FlexSPI_STS2_BSLVLOCK_MASK                  (0x00010000U)
#define FlexSPI_STS2_AREFLOCK_MASK                  (0x00000002U)
#define FlexSPI_STS2_ASLVLOCK_MASK                  (0x00000001U)

/* LUT */
#define FlexSPI_LUT_OPCODE1_SHIFT                   (26U)
#define FlexSPI_LUT_NUM_PADS1_SHIFT                 (24U)
#define FlexSPI_LUT_OPERAND1_SHIFT                  (16U)
#define FlexSPI_LUT_OPCODE0_SHIFT                   (10U)
#define FlexSPI_LUT_NUM_PADS0_SHIFT                 (8U)
#define FlexSPI_LUT_OPERAND0_SHIFT                  (0U)

/* レジスタアクセス */

/* MCR0 */
#define FlexSPI_MCR0_SCKFREERUNEN_MASK              (0x00004000U)
#define FlexSPI_MCR0_SCKFREERUNEN_SHIFT             (14U)
#define FlexSPI_MCR0_SCKFREERUNEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR0_SCKFREERUNEN_SHIFT)) & FlexSPI_MCR0_SCKFREERUNEN_MASK)

#define FlexSPI_MCR0_COMBINATIONEN_MASK             (0x00002000U)
#define FlexSPI_MCR0_COMBINATIONEN_SHIFT            (13U)
#define FlexSPI_MCR0_COMBINATIONEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR0_COMBINATIONEN_SHIFT)) & FlexSPI_MCR0_COMBINATIONEN_MASK)

#define FlexSPI_MCR0_DOZEEN_MASK                    (0x00001000U)
#define FlexSPI_MCR0_DOZEEN_SHIFT                   (12U)
#define FlexSPI_MCR0_DOZEEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR0_DOZEEN_SHIFT)) & FlexSPI_MCR0_DOZEEN_MASK)

#define FlexSPI_MCR0_HSEN_MASK                      (0x00000800U)
#define FlexSPI_MCR0_HSEN_SHIFT                     (11U)
#define FlexSPI_MCR0_HSEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR0_HSEN_SHIFT)) & FlexSPI_MCR0_HSEN_MASK)

#define FlexSPI_MCR0_ATDFEN_MASK                    (0x00000080U)
#define FlexSPI_MCR0_ATDFEN_SHIFT                   (7U)
#define FlexSPI_MCR0_ATDFEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR0_ATDFEN_SHIFT)) & FlexSPI_MCR0_ATDFEN_MASK)

#define FlexSPI_MCR0_ARDFEN_MASK                    (0x00000040U)
#define FlexSPI_MCR0_ARDFEN_SHIFT                   (6U)
#define FlexSPI_MCR0_ARDFEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR0_ARDFEN_SHIFT)) & FlexSPI_MCR0_ARDFEN_MASK)

#define FlexSPI_MCR0_RXCLKSRC_MASK                  (0x00000030U)
#define FlexSPI_MCR0_RXCLKSRC_SHIFT                 (4U)
#define FlexSPI_MCR0_RXCLKSRC(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR0_RXCLKSRC_SHIFT)) & FlexSPI_MCR0_RXCLKSRC_MASK)

/* MCR1 */
#define FlexSPI_MCR1_SEQWAIT_MASK                   (0xFFFF0000U)
#define FlexSPI_MCR1_SEQWAIT_SHIFT                  (16U)
#define FlexSPI_MCR1_SEQWAIT(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR1_SEQWAIT_SHIFT)) & FlexSPI_MCR1_SEQWAIT_MASK)

#define FlexSPI_MCR1_AHBBUSWAIT_MASK                (0x0000FFFFU)
#define FlexSPI_MCR1_AHBBUSWAIT_SHIFT               (0U)
#define FlexSPI_MCR1_AHBBUSWAIT(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR1_AHBBUSWAIT_SHIFT)) & FlexSPI_MCR1_AHBBUSWAIT_MASK)

/* MCR2 */
#define FlexSPI_MCR2_RESUMEWAIT_MASK                (0xFF000000U)
#define FlexSPI_MCR2_RESUMEWAIT_SHIFT               (24U)
#define FlexSPI_MCR2_RESUMEWAIT(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR2_RESUMEWAIT_SHIFT)) & FlexSPI_MCR2_RESUMEWAIT_MASK)

#define FlexSPI_MCR2_SCKBDIFFOPT_MASK               (0x00080000U)
#define FlexSPI_MCR2_SCKBDIFFOPT_SHIFT              (19U)
#define FlexSPI_MCR2_SCKBDIFFOPT(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR2_SCKBDIFFOPT_SHIFT)) & FlexSPI_MCR2_SCKBDIFFOPT_MASK)

#define FlexSPI_MCR2_SAMEDEVICEEN_MASK              (0x00008000U)
#define FlexSPI_MCR2_SAMEDEVICEEN_SHIFT             (15U)
#define FlexSPI_MCR2_SAMEDEVICEEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR2_SAMEDEVICEEN_SHIFT)) & FlexSPI_MCR2_SAMEDEVICEEN_MASK)

#define FlexSPI_MCR2_CLRAHBBUFOPT_MASK              (0x00000800U)
#define FlexSPI_MCR2_CLRAHBBUFOPT_SHIFT             (11U)
#define FlexSPI_MCR2_CLRAHBBUFOPT(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_MCR2_CLRAHBBUFOPT_SHIFT)) & FlexSPI_MCR2_CLRAHBBUFOPT_MASK)

/* INTEN */
#define FlexSPI_INTEN_IPCMDDONEEN_MASK              (0x00000001U)
#define FlexSPI_INTEN_IPCMDDONEEN_SHIFT             (0U)
#define FlexSPI_INTEN_IPCMDDONEEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_INTEN_IPCMDDONEEN_SHIFT)) & FlexSPI_INTEN_IPCMDDONEEN_MASK)

/* INTR */
#define FlexSPI_INTR_IPTXWE_MASK                    (0x00000040U)
#define FlexSPI_INTR_IPTXWE_SHIFT                   (6U)
#define FlexSPI_INTR_IPTXWE(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_INTR_IPTXWE_SHIFT)) & FlexSPI_INTR_IPTXWE_MASK)

#define FlexSPI_INTR_IPRXWA_MASK                    (0x00000020U)
#define FlexSPI_INTR_IPRXWA_SHIFT                   (5U)
#define FlexSPI_INTR_IPRXWA(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_INTR_IPRXWA_SHIFT)) & FlexSPI_INTR_IPRXWA_MASK)

#define FlexSPI_INTR_IPCMDDONE_MASK                 (0x00000001U)
#define FlexSPI_INTR_IPCMDDONE_SHIFT                (5U)
#define FlexSPI_INTR_IPCMDDONE(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_INTR_IPCMDDONE_SHIFT)) & FlexSPI_INTR_IPCMDDONE_MASK)

/* LUTCR */
#define FlexSPI_LUTCR_LOCK_MASK                     (0x00000001U)
#define FlexSPI_LUTCR_LOCK_SHIFT                    (0U)
#define FlexSPI_LUTCR_LOCK(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_LUTCR_LOCK_SHIFT)) & FlexSPI_LUTCR_LOCK_MASK)

#define FlexSPI_LUTCR_UNLOCK_MASK                   (0x00000002U)
#define FlexSPI_LUTCR_UNLOCK_SHIFT                  (1U)
#define FlexSPI_LUTCR_UNLOCK(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_LUTCR_UNLOCK_SHIFT)) & FlexSPI_LUTCR_UNLOCK_MASK)

/* FLSHCR1 */
#define FlexSPI_FLSHCR1_CSINTERVAL_MASK             (0xFFFF0000U)
#define FlexSPI_FLSHCR1_CSINTERVAL_SHIFT            (16U)
#define FlexSPI_FLSHCR1_CSINTERVAL(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR1_CSINTERVAL_SHIFT)) & FlexSPI_FLSHCR1_CSINTERVAL_MASK)

#define FlexSPI_FLSHCR1_CSINTERVALUNIT_MASK         (0x00008000U)
#define FlexSPI_FLSHCR1_CSINTERVALUNIT_SHIFT        (15U)
#define FlexSPI_FLSHCR1_CSINTERVALUNIT(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR1_CSINTERVALUNIT_SHIFT)) & FlexSPI_FLSHCR1_CSINTERVALUNIT_MASK)

#define FlexSPI_FLSHCR1_CAS_MASK                    (0x00007800U)
#define FlexSPI_FLSHCR1_CAS_SHIFT                   (11U)
#define FlexSPI_FLSHCR1_CAS(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR1_CAS_SHIFT)) & FlexSPI_FLSHCR1_CAS_MASK)

#define FlexSPI_FLSHCR1_WA_MASK                     (0x00000400U)
#define FlexSPI_FLSHCR1_WA_SHIFT                    (10U)
#define FlexSPI_FLSHCR1_WA(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR1_WA_SHIFT)) & FlexSPI_FLSHCR1_WA_MASK)

#define FlexSPI_FLSHCR1_TCSH_MASK                   (0x000003E0U)
#define FlexSPI_FLSHCR1_TCSH_SHIFT                  (5U)
#define FlexSPI_FLSHCR1_TCSH(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR1_TCSH_SHIFT)) & FlexSPI_FLSHCR1_TCSH_MASK)

#define FlexSPI_FLSHCR1_TCSS_MASK                   (0x0000001FU)
#define FlexSPI_FLSHCR1_TCSS_SHIFT                  (0U)
#define FlexSPI_FLSHCR1_TCSS(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR1_TCSS_SHIFT)) & FlexSPI_FLSHCR1_TCSS_MASK)

/* FLSHCR2 */
#define FlexSPI_FLSHCR2_AWRWAITUNIT_MASK            (0x70000000U)
#define FlexSPI_FLSHCR2_AWRWAITUNIT_SHIFT           (28U)
#define FlexSPI_FLSHCR2_AWRWAITUNIT(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR2_AWRWAITUNIT_SHIFT)) & FlexSPI_FLSHCR2_AWRWAITUNIT_MASK)

#define FlexSPI_FLSHCR2_AWRWAIT_MASK                (0x0FFF0000U)
#define FlexSPI_FLSHCR2_AWRWAIT_SHIFT               (16U)
#define FlexSPI_FLSHCR2_AWRWAIT(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR2_AWRWAIT_SHIFT)) & FlexSPI_FLSHCR2_AWRWAIT_MASK)

#define FlexSPI_FLSHCR2_AWRSEQNUM_MASK              (0x0000E000U)
#define FlexSPI_FLSHCR2_AWRSEQNUM_SHIFT             (13U)
#define FlexSPI_FLSHCR2_AWRSEQNUM(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR2_AWRSEQNUM_SHIFT)) & FlexSPI_FLSHCR2_AWRSEQNUM_MASK)

#define FlexSPI_FLSHCR2_AWRSEQID_MASK               (0x00001F00U)
#define FlexSPI_FLSHCR2_AWRSEQID_SHIFT              (8U)
#define FlexSPI_FLSHCR2_AWRSEQID(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR2_AWRSEQID_SHIFT)) & FlexSPI_FLSHCR2_AWRSEQID_MASK)

#define FlexSPI_FLSHCR2_ARDSEQNUM_MASK              (0x000000E0U)
#define FlexSPI_FLSHCR2_ARDSEQNUM_SHIFT             (5U)
#define FlexSPI_FLSHCR2_ARDSEQNUM(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR2_ARDSEQNUM_SHIFT)) & FlexSPI_FLSHCR2_ARDSEQNUM_MASK)

#define FlexSPI_FLSHCR2_ARDSEQID_MASK               (0x0000001FU)
#define FlexSPI_FLSHCR2_ARDSEQID_SHIFT              (0U)
#define FlexSPI_FLSHCR2_ARDSEQID(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR2_ARDSEQID_SHIFT)) & FlexSPI_FLSHCR2_ARDSEQID_MASK)

/* FLSHCR4 */
#define FlexSPI_FLSHCR4_WMENB_MASK                  (0x00000008U)
#define FlexSPI_FLSHCR4_WMENB_SHIFT                 (3U)
#define FlexSPI_FLSHCR4_WMENB(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR4_WMENB_SHIFT)) & FlexSPI_FLSHCR4_WMENB_MASK)

#define FlexSPI_FLSHCR4_WMENA_MASK                  (0x00000004U)
#define FlexSPI_FLSHCR4_WMENA_SHIFT                 (2U)
#define FlexSPI_FLSHCR4_WMENA(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_FLSHCR4_WMENA_SHIFT)) & FlexSPI_FLSHCR4_WMENA_MASK)

/* IPCR1 */
#define FlexSPI_IPCR1_IDATSZ_MASK                   (0x0000FFFFU)
#define FlexSPI_IPCR1_IDATSZ_SHIFT                  (0U)
#define FlexSPI_IPCR1_IDATSZ(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_IPCR1_IDATSZ_SHIFT)) & FlexSPI_IPCR1_IDATSZ_MASK)

/* IPCMD */
#define FlexSPI_IPCMD_TRG_MASK                      (0x00000001U)
#define FlexSPI_IPCMD_TRG_SHIFT                     (0U)
#define FlexSPI_IPCMD_TRG(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_IPCMD_TRG_SHIFT)) & FlexSPI_IPCMD_TRG_MASK)

/* IPRXFCR */
#define FlexSPI_IPRXFCR_RXWMRK_MASK                 (0x000000FCU)
#define FlexSPI_IPRXFCR_RXWMRK_SHIFT                (2U)
#define FlexSPI_IPRXFCR_RXWMRK(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_IPRXFCR_RXWMRK_SHIFT)) & FlexSPI_IPRXFCR_RXWMRK_MASK)

#define FlexSPI_IPRXFCR_RXDMAEN_MASK                (0x00000002U)
#define FlexSPI_IPRXFCR_RXDMAEN_SHIFT               (1U)
#define FlexSPI_IPRXFCR_RXDMAEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_IPRXFCR_RXDMAEN_SHIFT)) & FlexSPI_IPRXFCR_RXDMAEN_MASK)

#define FlexSPI_IPRXFCR_CLRIPRXF_MASK               (0x00000001U)
#define FlexSPI_IPRXFCR_CLRIPRXF_SHIFT              (0U)
#define FlexSPI_IPRXFCR_CLRIPRXF(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_IPRXFCR_CLRIPRXF_SHIFT)) & FlexSPI_IPRXFCR_CLRIPRXF_MASK)

/* IPTXFCR */
#define FlexSPI_IPTXFCR_TXWMRK_MASK                 (0x000001FCU)
#define FlexSPI_IPTXFCR_TXWMRK_SHIFT                (2U)
#define FlexSPI_IPTXFCR_TXWMRK(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_IPTXFCR_TXWMRK_SHIFT)) & FlexSPI_IPTXFCR_TXWMRK_MASK)

#define FlexSPI_IPTXFCR_TXDMAEN_MASK                (0x00000002U)
#define FlexSPI_IPTXFCR_TXDMAEN_SHIFT               (1U)
#define FlexSPI_IPTXFCR_TXDMAEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_IPTXFCR_TXDMAEN_SHIFT)) & FlexSPI_IPTXFCR_TXDMAEN_MASK)

#define FlexSPI_IPTXFCR_CLRIPTXF_MASK               (0x00000001U)
#define FlexSPI_IPTXFCR_CLRIPTXF_SHIFT              (0U)
#define FlexSPI_IPTXFCR_CLRIPTXF(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_IPTXFCR_CLRIPTXF_SHIFT)) & FlexSPI_IPTXFCR_CLRIPTXF_MASK)

/* DLLCR */
#define FlexSPI_DLLCR_OVRDVAL_MASK                  (0x00007E00U)
#define FlexSPI_DLLCR_OVRDVAL_SHIFT                 (9U)
#define FlexSPI_DLLCR_OVRDVAL(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_DLLCR_OVRDVAL_SHIFT)) & FlexSPI_DLLCR_OVRDVAL_MASK)

#define FlexSPI_DLLCR_OVRDEN_MASK                   (0x00000100U)
#define FlexSPI_DLLCR_OVRDEN_SHIFT                  (8U)
#define FlexSPI_DLLCR_OVRDEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_DLLCR_OVRDEN_SHIFT)) & FlexSPI_DLLCR_OVRDEN_MASK)

#define FlexSPI_DLLCR_SLVDLYTARGET_MASK             (0x00000078U)
#define FlexSPI_DLLCR_SLVDLYTARGET_SHIFT            (3U)
#define FlexSPI_DLLCR_SLVDLYTARGET(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_DLLCR_SLVDLYTARGET_SHIFT)) & FlexSPI_DLLCR_SLVDLYTARGET_MASK)

#define FlexSPI_DLLCR_SLVDLYTARGET_MASK             (0x00000078U)
#define FlexSPI_DLLCR_SLVDLYTARGET_SHIFT            (3U)
#define FlexSPI_DLLCR_SLVDLYTARGET(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_DLLCR_SLVDLYTARGET_SHIFT)) & FlexSPI_DLLCR_SLVDLYTARGET_MASK)

#define FlexSPI_DLLCR_DLLEN_MASK                    (0x00000001U)
#define FlexSPI_DLLCR_DLLEN_SHIFT                   (0U)
#define FlexSPI_DLLCR_DLLEN(x) \
    (((uint32_t)(((uint32_t)(x)) << FlexSPI_DLLCR_DLLEN_SHIFT)) & FlexSPI_DLLCR_DLLEN_MASK)

#define __NOP()                                     __asm volatile ("nop")

#if 0
int FROM_ReadMap( unsigned int uiFlashAddress, unsigned int uiLength, unsigned int uiSdramAddress );

#define FLEXSPI_AHB_BUFFER_SIZE     0x800
#define FROM_CHIP_SIZE      (32 * 1024 * 1024)          /* FLASHのサイズ   32M */
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _DRI_FLEXSPI_LOCAL_H_ */
