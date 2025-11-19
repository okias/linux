/*
 * Copyright (C) 2014-2020 NXP Semiconductors, All Rights Reserved.
 * Copyright 2021 GOODIX
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


typedef struct TfaBfName {
   unsigned short bfEnum;
   char  *bfName;
} tfaBfName_t;

typedef struct TfaIrqName {
        unsigned short irqEnum;
        char  *irqName;
} tfaIrqName_t;

#include "tfa2_tfafieldnames_N1C.h"
/* diffs for specific devices */
#include "tfa9872_tfafieldnames.h"
#include "tfa9912_tfafieldnames.h"
#include "tfa9873_tfafieldnames.h"
#include "tfa9873_tfafieldnames_B0.h"
#include "tfa9874_tfafieldnames.h"
#include "tfa9878_tfafieldnames.h"
#include "tfa9894_tfafieldnames.h"
#include "tfa9894_tfafieldnames_N2.h"
#include "tfa9875_tfafieldnames.h"
#include "tfa9875_tfafieldnames_A1.h"

/* missing 'common' defs break the build */
#define TFA2_BF_CFSM -1

/* MTP access uses registers
 *  defs are derived from corresponding bitfield names as used in the BF macros
 */
#define MTPKEY2         MTPK            /* unlock key2 MTPK */
#define MTP0            MTPOTC  /* MTP data */
#define MTP_CONTROL CIMTP       /* copy i2c to mtp */

/* interrupt enable register uses HW name in TFA2 */
#define TFA2_BF_INTENVDDS TFA2_BF_IEVDDS

/* TFA9872 specific bit field names */
#define TFA2_BF_IELP0 TFA9872_BF_IELP0
#define TFA2_BF_ISTLP0 TFA9872_BF_ISTLP0
#define TFA2_BF_IPOLP0 TFA9872_BF_IPOLP0
#define TFA2_BF_IELP1 TFA9872_BF_IELP1
#define TFA2_BF_ISTLP1 TFA9872_BF_ISTLP1
#define TFA2_BF_IPOLP1 TFA9872_BF_IPOLP1
#define TFA2_BF_LP0 TFA9872_BF_LP0
#define TFA2_BF_LP1 TFA9872_BF_LP1
#define TFA2_BF_R25C TFA9872_BF_R25C
#define TFA2_BF_SAMMODE TFA9872_BF_SAMMODE
