/*
 * Bink video 2 decoder
 * Copyright (c) 2014 Konstantin Shishkov
 * Copyright (c) 2019 Paul B Mahol
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_BINK2G_H
#define AVCODEC_BINK2G_H

#include <stdint.h>
#include "avcodec.h"
#include "get_bits.h"
#include "bink2.h"

static const uint8_t bink2g_scan[64] = {
     0,   8,   1,   2,  9,  16,  24,  17,
    10,   3,   4,  11, 18,  25,  32,  40,
    33,  26,  19,  12,  5,   6,  13,  20,
    27,  34,  41,  48, 56,  49,  42,  35,
    28,  21,  14,   7, 15,  22,  29,  36,
    43,  50,  57,  58, 51,  44,  37,  30,
    23,  31,  38,  45, 52,  59,  60,  53,
    46,  39,  47,  54, 61,  62,  55,  63,
};

static const uint16_t bink2g_ac_skip_codes[2][NUM_AC_SKIPS] = {
    {
        0x01, 0x00, 0x004, 0x02C, 0x06C, 0x0C, 0x4C,
        0xAC, 0xEC, 0x12C, 0x16C, 0x1AC, 0x02, 0x1C,
    },
    {
        0x01, 0x04, 0x00, 0x08, 0x02, 0x32, 0x0A,
        0x12, 0x3A, 0x7A, 0xFA, 0x72, 0x06, 0x1A,
    },
};

static const uint8_t bink2g_ac_skip_bits[2][NUM_AC_SKIPS] = {
    { 1, 3, 4, 9, 9, 7, 7, 9, 8, 9, 9, 9, 2, 5 },
    { 1, 3, 4, 4, 5, 7, 5, 6, 7, 8, 8, 7, 3, 6 },
};

static const uint8_t bink2g_mv_codes[] = {
    0x01, 0x06, 0x0C, 0x1C, 0x18, 0x38, 0x58, 0x78,
    0x68, 0x48, 0x28, 0x08, 0x14, 0x04, 0x02, 0x00,
};

static const uint8_t bink2g_mv_bits[] = {
    1, 3, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 3, 4,
};

static const uint8_t bink2g_skips[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 64, 0, 0, 0,
};

static uint8_t bink2g_chroma_cbp_pat[16] = {
    0x00, 0x00, 0x00, 0x0F,
    0x00, 0x0F, 0x0F, 0x0F,
    0x00, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F,
};

static const int32_t bink2g_dc_pat[] = {
    1024, 1218, 1448, 1722, 2048,
    2435, 2896, 3444, 4096, 4871,
    5793, 6889, 8192, 9742, 11585, 13777, 16384,
    19484, 23170, 27555, 32768, 38968, 46341,
    55109, 65536, 77936, 92682, 110218, 131072,
    155872, 185364, 220436, 262144, 311744,
    370728, 440872, 524288,
};

static const uint16_t bink2g_luma_intra_qmat[4][64] = {
    {
     1024,   1432,   1506,   1181,
     1843,   2025,   5271,   8592,
     1313,   1669,   1630,   1672,
     2625,   3442,   8023,  12794,
     1076,   1755,   1808,   1950,
     3980,   4875,   8813,  11909,
     1350,   1868,   2127,   2016,
     4725,   4450,   7712,   9637,
     2458,   3103,   4303,   4303,
     6963,   6835,  11079,  13365,
     3375,   5704,   5052,   6049,
     9198,   7232,  10725,   9834,
     5486,   7521,   7797,   7091,
    11079,  10016,  13559,  12912,
     7279,   7649,   7020,   6097,
     9189,   9047,  12661,  13768,
    },
    {
     1218,   1703,   1791,   1405,
     2192,   2408,   6268,  10218,
     1561,   1985,   1938,   1988,
     3122,   4093,   9541,  15215,
     1279,   2087,   2150,   2319,
     4733,   5798,  10481,  14162,
     1606,   2222,   2530,   2398,
     5619,   5292,   9171,  11460,
     2923,   3690,   5117,   5118,
     8281,   8128,  13176,  15894,
     4014,   6783,   6008,   7194,
    10938,   8600,  12755,  11694,
     6524,   8944,   9272,   8433,
    13176,  11911,  16125,  15354,
     8657,   9096,   8348,   7250,
    10927,  10759,  15056,  16373,
    },
    {
     1448,   2025,   2130,   1671,
     2607,   2864,   7454,  12151,
     1856,   2360,   2305,   2364,
     3713,   4867,  11346,  18094,
     1521,   2482,   2557,   2758,
     5628,   6894,  12464,  16841,
     1909,   2642,   3008,   2852,
     6683,   6293,  10906,  13629,
     3476,   4388,   6085,   6086,
     9847,   9666,  15668,  18901,
     4773,   8066,   7145,   8555,
    13007,  10227,  15168,  13907,
     7758,  10637,  11026,  10028,
    15668,  14165,  19175,  18259,
    10294,  10817,   9927,   8622,
    12995,  12794,  17905,  19470,
    },
    {
     1722,   2408,   2533,   1987,
     3100,   3406,   8864,  14450,
     2208,   2807,   2741,   2811,
     4415,   5788,  13493,  21517,
     1809,   2951,   3041,   3280,
     6693,   8199,  14822,  20028,
     2271,   3142,   3578,   3391,
     7947,   7484,  12969,  16207,
     4133,   5218,   7236,   7238,
    11711,  11495,  18633,  22478,
     5677,   9592,   8497,  10174,
    15469,  12162,  18038,  16538,
     9226,  12649,  13112,  11926,
    18633,  16845,  22804,  21715,
    12242,  12864,  11806,  10254,
    15454,  15215,  21293,  23155,
    },
};

static const uint16_t bink2g_chroma_intra_qmat[4][64] = {
    {
     1024,   1193,   1434,   2203,
     5632,   4641,   5916,   6563,
     1193,   1622,   1811,   3606,
     6563,   5408,   6894,   7649,
     1434,   1811,   3515,   4875,
     5916,   4875,   6215,   6894,
     2203,   3606,   4875,   3824,
     4641,   3824,   4875,   5408,
     5632,   6563,   5916,   4641,
     5632,   4641,   5916,   6563,
     4641,   5408,   4875,   3824,
     4641,   3824,   4875,   5408,
     5916,   6894,   6215,   4875,
     5916,   4875,   6215,   6894,
     6563,   7649,   6894,   5408,
     6563,   5408,   6894,   7649,
    },
    {
     1218,   1419,   1706,   2620,
     6698,   5519,   7035,   7805,
     1419,   1929,   2153,   4288,
     7805,   6432,   8199,   9096,
     1706,   2153,   4180,   5798,
     7035,   5798,   7390,   8199,
     2620,   4288,   5798,   4548,
     5519,   4548,   5798,   6432,
     6698,   7805,   7035,   5519,
     6698,   5519,   7035,   7805,
     5519,   6432,   5798,   4548,
     5519,   4548,   5798,   6432,
     7035,   8199,   7390,   5798,
     7035,   5798,   7390,   8199,
     7805,   9096,   8199,   6432,
     7805,   6432,   8199,   9096,
    },
    {
     1448,   1688,   2028,   3116,
     7965,   6563,   8367,   9282,
     1688,   2294,   2561,   5099,
     9282,   7649,   9750,  10817,
     2028,   2561,   4971,   6894,
     8367,   6894,   8789,   9750,
     3116,   5099,   6894,   5408,
     6563,   5408,   6894,   7649,
     7965,   9282,   8367,   6563,
     7965,   6563,   8367,   9282,
     6563,   7649,   6894,   5408,
     6563,   5408,   6894,   7649,
     8367,   9750,   8789,   6894,
     8367,   6894,   8789,   9750,
     9282,  10817,   9750,   7649,
     9282,   7649,   9750,  10817,
    },
    {
     1722,   2007,   2412,   3706,
     9472,   7805,   9950,  11038,
     2007,   2729,   3045,   6064,
    11038,   9096,  11595,  12864,
     2412,   3045,   5912,   8199,
     9950,   8199,  10452,  11595,
     3706,   6064,   8199,   6432,
     7805,   6432,   8199,   9096,
     9472,  11038,   9950,   7805,
     9472,   7805,   9950,  11038,
     7805,   9096,   8199,   6432,
     7805,   6432,   8199,   9096,
     9950,  11595,  10452,   8199,
     9950,   8199,  10452,  11595,
    11038,  12864,  11595,   9096,
    11038,   9096,  11595,  12864,
    },
};

static const uint16_t bink2g_inter_qmat[4][64] = {
    {
     1024,   1193,   1076,    844,
     1052,    914,   1225,   1492,
     1193,   1391,   1254,    983,
     1227,   1065,   1463,   1816,
     1076,   1254,   1161,    936,
     1195,   1034,   1444,   1741,
      844,    983,    936,    811,
     1055,    927,   1305,   1584,
     1052,   1227,   1195,   1055,
     1451,   1336,   1912,   2354,
      914,   1065,   1034,    927,
     1336,   1313,   1945,   2486,
     1225,   1463,   1444,   1305,
     1912,   1945,   3044,   4039,
     1492,   1816,   1741,   1584,
     2354,   2486,   4039,   5679,
    },
    {
     1218,   1419,   1279,   1003,
     1252,   1087,   1457,   1774,
     1419,   1654,   1491,   1169,
     1459,   1267,   1739,   2159,
     1279,   1491,   1381,   1113,
     1421,   1230,   1717,   2070,
     1003,   1169,   1113,    965,
     1254,   1103,   1552,   1884,
     1252,   1459,   1421,   1254,
     1725,   1589,   2274,   2799,
     1087,   1267,   1230,   1103,
     1589,   1562,   2313,   2956,
     1457,   1739,   1717,   1552,
     2274,   2313,   3620,   4803,
     1774,   2159,   2070,   1884,
     2799,   2956,   4803,   6753,
    },
    {
     1448,   1688,   1521,   1193,
     1488,   1293,   1732,   2110,
     1688,   1967,   1773,   1391,
     1735,   1507,   2068,   2568,
     1521,   1773,   1642,   1323,
     1690,   1462,   2042,   2462,
     1193,   1391,   1323,   1147,
     1492,   1311,   1845,   2241,
     1488,   1735,   1690,   1492,
     2052,   1889,   2704,   3328,
     1293,   1507,   1462,   1311,
     1889,   1857,   2751,   3515,
     1732,   2068,   2042,   1845,
     2704,   2751,   4306,   5712,
     2110,   2568,   2462,   2241,
     3328,   3515,   5712,   8031,
    },
    {
     1722,   2007,   1809,   1419,
     1770,   1537,   2060,   2509,
     2007,   2339,   2108,   1654,
     2063,   1792,   2460,   3054,
     1809,   2108,   1953,   1574,
     2010,   1739,   2428,   2928,
     1419,   1654,   1574,   1364,
     1774,   1559,   2195,   2664,
     1770,   2063,   2010,   1774,
     2440,   2247,   3216,   3958,
     1537,   1792,   1739,   1559,
     2247,   2209,   3271,   4181,
     2060,   2460,   2428,   2195,
     3216,   3271,   5120,   6793,
     2509,   3054,   2928,   2664,
     3958,   4181,   6793,   9550,
    },
};

static inline void bink2g_idct_1d(int16_t *blk, int step, int shift)
{
#define idct_mul_a(val) ((val) + ((val) >> 2))
#define idct_mul_b(val) ((val) >> 1)
#define idct_mul_c(val) ((val) - ((val) >> 2) - ((val) >> 4))
#define idct_mul_d(val) ((val) + ((val) >> 2) - ((val) >> 4))
#define idct_mul_e(val) ((val) >> 2)
    int tmp00 =  blk[3*step] + blk[5*step];
    int tmp01 =  blk[3*step] - blk[5*step];
    int tmp02 =  idct_mul_a(blk[2*step]) + idct_mul_b(blk[6*step]);
    int tmp03 =  idct_mul_b(blk[2*step]) - idct_mul_a(blk[6*step]);
    int tmp0  = (blk[0*step] + blk[4*step]) + tmp02;
    int tmp1  = (blk[0*step] + blk[4*step]) - tmp02;
    int tmp2  =  blk[0*step] - blk[4*step];
    int tmp3  =  blk[1*step] + tmp00;
    int tmp4  =  blk[1*step] - tmp00;
    int tmp5  =  tmp01 + blk[7*step];
    int tmp6  =  tmp01 - blk[7*step];
    int tmp7  =  tmp4 + idct_mul_c(tmp6);
    int tmp8  =  idct_mul_c(tmp4) - tmp6;
    int tmp9  =  idct_mul_d(tmp3) + idct_mul_e(tmp5);
    int tmp10 =  idct_mul_e(tmp3) - idct_mul_d(tmp5);
    int tmp11 =  tmp2 + tmp03;
    int tmp12 =  tmp2 - tmp03;

    blk[0*step] = (tmp0  + tmp9)  >> shift;
    blk[1*step] = (tmp11 + tmp7)  >> shift;
    blk[2*step] = (tmp12 + tmp8)  >> shift;
    blk[3*step] = (tmp1  + tmp10) >> shift;
    blk[4*step] = (tmp1  - tmp10) >> shift;
    blk[5*step] = (tmp12 - tmp8)  >> shift;
    blk[6*step] = (tmp11 - tmp7)  >> shift;
    blk[7*step] = (tmp0  - tmp9)  >> shift;
}

static void bink2g_idct_put(uint8_t *dst, int stride, int16_t *block)
{
    for (int i = 0; i < 8; i++)
        bink2g_idct_1d(block + i, 8, 0);
    for (int i = 0; i < 8; i++)
        bink2g_idct_1d(block + i * 8, 1, 6);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++)
            dst[j] = av_clip_uint8(block[j * 8 + i]);
        dst += stride;
    }
}

static void bink2g_idct_add(uint8_t *dst, int stride, int16_t *block)
{
    for (int i = 0; i < 8; i++)
        bink2g_idct_1d(block + i, 8, 0);
    for (int i = 0; i < 8; i++)
        bink2g_idct_1d(block + i * 8, 1, 6);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++)
            dst[j] = av_clip_uint8(dst[j] + block[j * 8 + i]);
        dst += stride;
    }
}

static int bink2g_get_type(GetBitContext *gb, int *lru)
{
    int val;

    switch (get_unary(gb, 1, 3)) {
    case 0:
        val = lru[0];
        break;
    case 1:
        val = lru[1];
        FFSWAP(int, lru[0], lru[1]);
        break;
    case 2:
        val = lru[3];
        FFSWAP(int, lru[2], lru[3]);
        break;
    case 3:
        val = lru[2];
        FFSWAP(int, lru[1], lru[2]);
        break;
    }

    return val;
}

static int bink2g_decode_dq(GetBitContext *gb)
{
    int dq = get_unary(gb, 1, 4);

    if (dq == 3)
        dq += get_bits1(gb);
    else if (dq == 4)
        dq += get_bits(gb, 5) + 1;
    if (dq && get_bits1(gb))
        dq = -dq;

    return dq;
}

static unsigned bink2g_decode_cbp_luma(Bink2Context *c,
                                       GetBitContext *gb, unsigned prev_cbp)
{
    unsigned ones = 0, cbp, mask;

    for (int i = 0; i < 16; i++) {
        if (prev_cbp & (1 << i))
            ones += 1;
    }

    cbp = 0;
    mask = 0;
    if (ones > 7) {
        ones = 16 - ones;
        mask = 0xFFFF;
    }

    if (get_bits1(gb) == 0) {
        if (ones < 4) {
            for (int j = 0; j < 16; j += 4)
                if (!get_bits1(gb))
                    cbp |= get_bits(gb, 4) << j;
        } else {
            cbp = get_bits(gb, 16);
        }
    }

    cbp ^= mask;
    if (!(c->frame_flags & 0x40000) || cbp) {
        if (get_bits1(gb))
            cbp = cbp | cbp << 16;
    }

    return cbp;
}

static unsigned bink2g_decode_cbp_chroma(GetBitContext *gb, unsigned prev_cbp)
{
    unsigned cbp;

    cbp = prev_cbp & 0xF0000 | bink2g_chroma_cbp_pat[prev_cbp & 0xF];
    if (get_bits1(gb) == 0) {
        cbp = get_bits(gb, 4);
        if (get_bits1(gb))
            cbp |= cbp << 16;
    }

    return cbp;
}

static void bink2g_predict_dc(Bink2Context *c,
                              int is_luma, int mindc, int maxdc,
                              int flags, int tdc[16])
{
    int *LTdc = c->prev_idc[FFMAX(c->mb_pos - 1, 0)].dc[c->comp];
    int *Tdc = c->prev_idc[c->mb_pos].dc[c->comp];
    int *Ldc = c->current_idc[FFMAX(c->mb_pos - 1, 0)].dc[c->comp];
    int *dc = c->current_idc[c->mb_pos].dc[c->comp];

    if (is_luma && (flags & 0x20) && (flags & 0x80)) {
        dc[0]  = av_clip((mindc < 0 ? 0 : 1024) + tdc[0], mindc, maxdc);
        dc[1]  = av_clip(dc[0] + tdc[1], mindc, maxdc);
        dc[2]  = av_clip(DC_MPRED2(dc[0], dc[1]) + tdc[2], mindc, maxdc);
        dc[3]  = av_clip(DC_MPRED(dc[0], dc[2], dc[1]) + tdc[3], mindc, maxdc);
        dc[4]  = av_clip(DC_MPRED2(dc[1], dc[3]) + tdc[4], mindc, maxdc);
        dc[5]  = av_clip(dc[4] + tdc[5], mindc, maxdc);
        dc[6]  = av_clip(DC_MPRED(dc[1], dc[3], dc[4]) + tdc[6], mindc, maxdc);
        dc[7]  = av_clip(DC_MPRED(dc[4], dc[6], dc[5]) + tdc[7], mindc, maxdc);
        dc[8]  = av_clip(DC_MPRED2(dc[2], dc[3]) + tdc[8], mindc, maxdc);
        dc[9]  = av_clip(DC_MPRED(dc[2], dc[8], dc[3]) + tdc[9], mindc, maxdc);
        dc[10] = av_clip(DC_MPRED2(dc[8], dc[9]) + tdc[10], mindc, maxdc);
        dc[11] = av_clip(DC_MPRED(dc[8], dc[10], dc[9]) + tdc[11], mindc, maxdc);
        dc[12] = av_clip(DC_MPRED(dc[3], dc[9], dc[6]) + tdc[12], mindc, maxdc);
        dc[13] = av_clip(DC_MPRED(dc[6], dc[12], dc[7]) + tdc[13], mindc, maxdc);
        dc[14] = av_clip(DC_MPRED(dc[9], dc[11], dc[12]) + tdc[14], mindc, maxdc);
        dc[15] = av_clip(DC_MPRED(dc[12], dc[14], dc[13]) + tdc[15], mindc, maxdc);
    } else if (is_luma && (flags & 0x80)) {
        dc[0]  = av_clip(DC_MPRED2(Ldc[5], Ldc[7]) + tdc[0], mindc, maxdc);
        dc[1]  = av_clip(dc[0] + tdc[1], mindc, maxdc);
        dc[2]  = av_clip(DC_MPRED(Ldc[5], Ldc[7], dc[0]) + tdc[2], mindc, maxdc);
        dc[3]  = av_clip(DC_MPRED(dc[0], dc[2], dc[1]) + tdc[3], mindc, maxdc);
        dc[4]  = av_clip(DC_MPRED2(dc[1], dc[3]) + tdc[4], mindc, maxdc);
        dc[5]  = av_clip(dc[4] + tdc[5], mindc, maxdc);
        dc[6]  = av_clip(DC_MPRED(dc[1], dc[3], dc[4]) + tdc[6], mindc, maxdc);
        dc[7]  = av_clip(DC_MPRED(dc[4], dc[6], dc[5]) + tdc[7], mindc, maxdc);
        dc[8]  = av_clip(DC_MPRED(Ldc[7], Ldc[13], dc[2]) + tdc[8], mindc, maxdc);
        dc[9]  = av_clip(DC_MPRED(dc[2], dc[8], dc[3]) + tdc[9], mindc, maxdc);
        dc[10] = av_clip(DC_MPRED(Ldc[13], Ldc[15], dc[8]) + tdc[10], mindc, maxdc);
        dc[11] = av_clip(DC_MPRED(dc[8], dc[10], dc[9]) + tdc[11], mindc, maxdc);
        dc[12] = av_clip(DC_MPRED(dc[3], dc[9], dc[6]) + tdc[12], mindc, maxdc);
        dc[13] = av_clip(DC_MPRED(dc[6], dc[12], dc[7]) + tdc[13], mindc, maxdc);
        dc[14] = av_clip(DC_MPRED(dc[9], dc[11], dc[12]) + tdc[14], mindc, maxdc);
        dc[15] = av_clip(DC_MPRED(dc[12], dc[14], dc[13]) + tdc[15], mindc, maxdc);
    } else if (is_luma && (flags & 0x20)) {
        dc[0]  = av_clip(DC_MPRED2(Tdc[10], Tdc[11]) + tdc[0], mindc, maxdc);
        dc[1]  = av_clip(DC_MPRED(Tdc[10], dc[0], Tdc[11]) + tdc[1], mindc, maxdc);
        dc[2]  = av_clip(DC_MPRED2(dc[0], dc[1]) + tdc[2], mindc, maxdc);
        dc[3]  = av_clip(DC_MPRED(dc[0], dc[2], dc[1]) + tdc[3], mindc, maxdc);
        dc[4]  = av_clip(DC_MPRED(Tdc[11], dc[1], Tdc[14]) + tdc[4], mindc, maxdc);
        dc[5]  = av_clip(DC_MPRED(Tdc[14], dc[4], Tdc[15]) + tdc[5], mindc, maxdc);
        dc[6]  = av_clip(DC_MPRED(dc[1], dc[3], dc[4]) + tdc[6], mindc, maxdc);
        dc[7]  = av_clip(DC_MPRED(dc[4], dc[6], dc[5]) + tdc[7], mindc, maxdc);
        dc[8]  = av_clip(DC_MPRED2(dc[2], dc[3]) + tdc[8], mindc, maxdc);
        dc[9]  = av_clip(DC_MPRED(dc[2], dc[8], dc[3]) + tdc[9], mindc, maxdc);
        dc[10] = av_clip(DC_MPRED2(dc[8], dc[9]) + tdc[10], mindc, maxdc);
        dc[11] = av_clip(DC_MPRED(dc[8], dc[10], dc[9]) + tdc[11], mindc, maxdc);
        dc[12] = av_clip(DC_MPRED(dc[3], dc[9], dc[6]) + tdc[12], mindc, maxdc);
        dc[13] = av_clip(DC_MPRED(dc[6], dc[12], dc[7]) + tdc[13], mindc, maxdc);
        dc[14] = av_clip(DC_MPRED(dc[9], dc[11], dc[12]) + tdc[14], mindc, maxdc);
        dc[15] = av_clip(DC_MPRED(dc[12], dc[14], dc[13]) + tdc[15], mindc, maxdc);
    } else if (is_luma) {
        dc[0]  = av_clip(DC_MPRED(LTdc[15], Ldc[5], Tdc[10]) + tdc[0], mindc, maxdc);
        dc[1]  = av_clip(DC_MPRED(Tdc[10], dc[0], Tdc[11]) + tdc[1], mindc, maxdc);
        dc[2]  = av_clip(DC_MPRED(Ldc[5], Ldc[7], dc[0]) + tdc[2], mindc, maxdc);
        dc[3]  = av_clip(DC_MPRED(dc[0], dc[2], dc[1]) + tdc[3], mindc, maxdc);
        dc[4]  = av_clip(DC_MPRED(Tdc[11], dc[1], Tdc[14]) + tdc[4], mindc, maxdc);
        dc[5]  = av_clip(DC_MPRED(Tdc[14], dc[4], Tdc[15]) + tdc[5], mindc, maxdc);
        dc[6]  = av_clip(DC_MPRED(dc[1], dc[3], dc[4]) + tdc[6], mindc, maxdc);
        dc[7]  = av_clip(DC_MPRED(dc[4], dc[6], dc[5]) + tdc[7], mindc, maxdc);
        dc[8]  = av_clip(DC_MPRED(Ldc[7], Ldc[13], dc[2]) + tdc[8], mindc, maxdc);
        dc[9]  = av_clip(DC_MPRED(dc[2], dc[8], dc[3]) + tdc[9], mindc, maxdc);
        dc[10] = av_clip(DC_MPRED(Ldc[13], Ldc[15], dc[8]) + tdc[10], mindc, maxdc);
        dc[11] = av_clip(DC_MPRED(dc[8], dc[10], dc[9]) + tdc[11], mindc, maxdc);
        dc[12] = av_clip(DC_MPRED(dc[3], dc[9], dc[6]) + tdc[12], mindc, maxdc);
        dc[13] = av_clip(DC_MPRED(dc[6], dc[12], dc[7]) + tdc[13], mindc, maxdc);
        dc[14] = av_clip(DC_MPRED(dc[9], dc[11], dc[12]) + tdc[14], mindc, maxdc);
        dc[15] = av_clip(DC_MPRED(dc[12], dc[14], dc[13]) + tdc[15], mindc, maxdc);
    } else if (!is_luma && (flags & 0x20) && (flags & 0x80)) {
        dc[0] = av_clip((mindc < 0 ? 0 : 1024) + tdc[0], mindc, maxdc);
        dc[1] = av_clip(dc[0] + tdc[1], mindc, maxdc);
        dc[2] = av_clip(DC_MPRED2(dc[0], dc[1]) + tdc[2], mindc, maxdc);
        dc[3] = av_clip(DC_MPRED(dc[0], dc[2], dc[1]) + tdc[3], mindc, maxdc);
    } else if (!is_luma && (flags & 0x80)) {
        dc[0] = av_clip(DC_MPRED2(Ldc[1], Ldc[3]) + tdc[0], mindc, maxdc);
        dc[1] = av_clip(dc[0] + tdc[1], mindc, maxdc);
        dc[2] = av_clip(DC_MPRED(Ldc[1], Ldc[3], dc[0]) + tdc[2], mindc, maxdc);
        dc[3] = av_clip(DC_MPRED(dc[0], dc[2], dc[1]) + tdc[3], mindc, maxdc);
    } else if (!is_luma && (flags & 0x20)) {
        dc[0] = av_clip(DC_MPRED2(Tdc[2], Tdc[3]) + tdc[0], mindc, maxdc);
        dc[1] = av_clip(DC_MPRED(Tdc[2], dc[0], Tdc[3]) + tdc[1], mindc, maxdc);
        dc[2] = av_clip(DC_MPRED2(dc[0], dc[1]) + tdc[2], mindc, maxdc);
        dc[3] = av_clip(DC_MPRED(dc[0], dc[2], dc[1]) + tdc[3], mindc, maxdc);
    } else if (!is_luma) {
        dc[0] = av_clip(DC_MPRED(LTdc[3], Ldc[1], Tdc[2]) + tdc[0], mindc, maxdc);
        dc[1] = av_clip(DC_MPRED(Tdc[2], dc[0], Tdc[3]) + tdc[1], mindc, maxdc);
        dc[2] = av_clip(DC_MPRED(Ldc[1], Ldc[3], dc[0]) + tdc[2], mindc, maxdc);
        dc[3] = av_clip(DC_MPRED(dc[0], dc[2], dc[1]) + tdc[3], mindc, maxdc);
    }
}

static void bink2g_decode_dc(Bink2Context *c, GetBitContext *gb, int *dc,
                             int is_luma, int q, int mindc, int maxdc,
                             int flags)
{
    const int num_dc = is_luma ? 16 : 4;
    int tdc[16];
    int pat;

    q = FFMAX(q, 8);
    pat = bink2g_dc_pat[q];

    memset(tdc, 0, sizeof(tdc));

    if (get_bits1(gb)) {
        for (int i = 0; i < num_dc; i++) {
            int cnt = get_unary(gb, 0, 12);

            if (cnt > 3)
                cnt = (1 << (cnt - 3)) + get_bits(gb, cnt - 3) + 2;
            if (cnt && get_bits1(gb))
                cnt = -cnt;
            tdc[i] = (cnt * pat + 0x1FF) >> 10;
        }
    }

    bink2g_predict_dc(c, is_luma, mindc, maxdc, flags, tdc);
}

static int bink2g_decode_ac(GetBitContext *gb, const uint8_t scan[64],
                            int16_t block[4][64], unsigned cbp,
                            int q, const uint16_t qmat[4][64],
                            uint8_t ac_count[4])
{
    int idx, next, val, skip;
    VLC *skip_vlc;

    for (int i = 0; i < 4; i++) {
        memset(block[i], 0, sizeof(int16_t) * 64);
        ac_count[i] = 0;
    }

    if ((cbp & 0xf) == 0)
        return 0;

    skip_vlc = &bink2g_ac_skip0_vlc;
    if (cbp & 0xffff0000)
        skip_vlc = &bink2g_ac_skip1_vlc;

    for (int i = 0; i < 4; i++, cbp >>= 1) {
        if (!(cbp & 1))
            continue;

        next = 0;
        idx  = 1;
        while (idx < 64) {
            next--;
            if (next < 1) {
                skip = get_vlc2(gb, skip_vlc->table, skip_vlc->bits, 1);
                if (skip < 0)
                    return AVERROR_INVALIDDATA;
                next = bink2_next_skips[skip];
                skip = bink2g_skips[skip];
                if (skip == 11)
                    skip = get_bits(gb, 6);
                idx += skip;
                if (idx >= 64)
                    break;
            }

            val = get_unary(gb, 0, 12) + 1;
            if (val > 3)
                val = get_bits(gb, val - 3) + (1 << (val - 3)) + 2;
            if (get_bits1(gb))
                val = -val;
            block[i][scan[idx]] = ((val * qmat[q & 3][scan[idx]] * (1 << (q >> 2))) + 64) >> 7;
            ac_count[i]++;
            idx++;
        }
    }

    return 0;
}

static inline uint16_t bink2g_block_state(int dc, unsigned ac_count, int intra)
{
    const unsigned strength = (ac_count < 8) + (ac_count < 4) + !ac_count;

    return (dc & 0x1FFF) | (strength << 13) | (intra ? 0x8000 : 0);
}

static int bink2g_decode_intra_luma(Bink2Context *c,
                                    GetBitContext *gb, int16_t block[4][64],
                                    unsigned *prev_cbp, int q,
                                    BlockDSPContext *dsp, uint8_t *dst, int stride,
                                    int flags)
{
    int *dc = c->current_idc[c->mb_pos].dc[c->comp];
    uint16_t *state = c->current_idc[c->mb_pos].state[c->comp];
    unsigned cbp;
    int ret;

    *prev_cbp = cbp = bink2g_decode_cbp_luma(c, gb, *prev_cbp);

    bink2g_decode_dc(c, gb, dc, 1, q, 0, 2047, flags);

    for (int i = 0; i < 4; i++) {
        uint8_t ac_count[4];

        ret = bink2g_decode_ac(gb, bink2g_scan, block, cbp >> (4*i),
                               q, bink2g_luma_intra_qmat, ac_count);
        if (ret < 0)
            return ret;

        for (int j = 0; j < 4; j++) {
            const int block_index = i * 4 + j;
            const int raster_index = luma_repos[block_index];

            state[raster_index] = bink2g_block_state(dc[block_index], ac_count[j], 1);
            block[j][0] = dc[block_index] * 8 + 32;
            bink2g_idct_put(dst + (raster_index & 3) * 8 +
                            (raster_index >> 2) * 8 * stride, stride, block[j]);
        }
    }

    return 0;
}

static int bink2g_decode_intra_chroma(Bink2Context *c,
                                      GetBitContext *gb, int16_t block[4][64],
                                      unsigned *prev_cbp, int q,
                                      BlockDSPContext *dsp, uint8_t *dst, int stride,
                                      int flags)
{
    int *dc = c->current_idc[c->mb_pos].dc[c->comp];
    uint16_t *state = c->current_idc[c->mb_pos].state[c->comp];
    uint8_t ac_count[4];
    unsigned cbp;
    int ret;

    *prev_cbp = cbp = bink2g_decode_cbp_chroma(gb, *prev_cbp);

    bink2g_decode_dc(c, gb, dc, 0, q, 0, 2047, flags);

    ret = bink2g_decode_ac(gb, bink2g_scan, block, cbp,
                           q, bink2g_chroma_intra_qmat, ac_count);
    if (ret < 0)
        return ret;

    for (int j = 0; j < 4; j++) {
        state[j] = bink2g_block_state(dc[j], ac_count[j], 1);
        block[j][0] = dc[j] * 8 + 32;
        bink2g_idct_put(dst + (j & 1) * 8 +
                        (j >> 1) * 8 * stride, stride, block[j]);
    }

    return 0;
}

static int bink2g_decode_inter_luma(Bink2Context *c,
                                    GetBitContext *gb, int16_t block[4][64],
                                    unsigned *prev_cbp, int q,
                                    BlockDSPContext *dsp, uint8_t *dst, int stride,
                                    int flags)
{
    int *dc = c->current_idc[c->mb_pos].dc[c->comp];
    uint16_t *state = c->current_idc[c->mb_pos].state[c->comp];
    const int maxdc = c->frame_flags & 0x40000 ? 2047 : 1023;
    unsigned cbp;
    int ret;

    *prev_cbp = cbp = bink2g_decode_cbp_luma(c, gb, *prev_cbp);

    bink2g_decode_dc(c, gb, dc, 1, q, -maxdc, maxdc, 0xA8);

    for (int i = 0; i < 4; i++) {
        uint8_t ac_count[4];

        ret = bink2g_decode_ac(gb, bink2g_scan, block, cbp >> (4 * i),
                               q, bink2g_inter_qmat, ac_count);
        if (ret < 0)
            return ret;

        for (int j = 0; j < 4; j++) {
            const int block_index = i * 4 + j;
            const int raster_index = luma_repos[block_index];

            state[raster_index] = bink2g_block_state(dc[block_index], ac_count[j], 0);
            block[j][0] = dc[block_index] * 8 + 32;
            bink2g_idct_add(dst + (raster_index & 3) * 8 +
                            (raster_index >> 2) * 8 * stride,
                            stride, block[j]);
        }
    }

    return 0;
}

static int bink2g_decode_inter_chroma(Bink2Context *c,
                                      GetBitContext *gb, int16_t block[4][64],
                                      unsigned *prev_cbp, int q,
                                      BlockDSPContext *dsp, uint8_t *dst, int stride,
                                      int flags)
{
    int *dc = c->current_idc[c->mb_pos].dc[c->comp];
    uint16_t *state = c->current_idc[c->mb_pos].state[c->comp];
    uint8_t ac_count[4];
    const int maxdc = c->frame_flags & 0x40000 ? 2047 : 1023;
    unsigned cbp;
    int ret;

    *prev_cbp = cbp = bink2g_decode_cbp_chroma(gb, *prev_cbp);

    bink2g_decode_dc(c, gb, dc, 0, q, -maxdc, maxdc, 0xA8);

    ret = bink2g_decode_ac(gb, bink2g_scan, block, cbp,
                           q, bink2g_inter_qmat, ac_count);
    if (ret < 0)
        return ret;

    for (int j = 0; j < 4; j++) {
        state[j] = bink2g_block_state(dc[j], ac_count[j], 0);
        block[j][0] = dc[j] * 8 + 32;
        bink2g_idct_add(dst + (j & 1) * 8 +
                        (j >> 1) * 8 * stride, stride, block[j]);
    }

    return 0;
}

static void bink2g_predict_mv(Bink2Context *c, int x, int y, int flags, MVectors mv)
{
    MVectors *c_mv = &c->current_mv[c->mb_pos].mv;
    MVectors *l_mv = &c->current_mv[FFMAX(c->mb_pos - 1, 0)].mv;
    MVectors *lt_mv = &c->prev_mv[FFMAX(c->mb_pos - 1, 0)].mv;
    MVectors *t_mv = &c->prev_mv[c->mb_pos].mv;

    if (mv.nb_vectors == 1) {
        if (flags & 0x80) {
            if (!(flags & 0x20)) {
                mv.v[0][0] += mid_pred(l_mv->v[0][0], l_mv->v[1][0], l_mv->v[3][0]);
                mv.v[0][1] += mid_pred(l_mv->v[0][1], l_mv->v[1][1], l_mv->v[3][1]);
            }
        } else {
            if (!(flags & 0x20)) {
                mv.v[0][0] += mid_pred(lt_mv->v[3][0], t_mv->v[2][0], l_mv->v[1][0]);
                mv.v[0][1] += mid_pred(lt_mv->v[3][1], t_mv->v[2][1], l_mv->v[1][1]);
            } else {
                mv.v[0][0] += mid_pred(t_mv->v[0][0], t_mv->v[2][0], t_mv->v[3][0]);
                mv.v[0][1] += mid_pred(t_mv->v[0][1], t_mv->v[2][1], t_mv->v[3][1]);
            }
        }

        c_mv->v[0][0] = mv.v[0][0];
        c_mv->v[0][1] = mv.v[0][1];
        c_mv->v[1][0] = mv.v[0][0];
        c_mv->v[1][1] = mv.v[0][1];
        c_mv->v[2][0] = mv.v[0][0];
        c_mv->v[2][1] = mv.v[0][1];
        c_mv->v[3][0] = mv.v[0][0];
        c_mv->v[3][1] = mv.v[0][1];

        return;
    }

    if (!(flags & 0x80)) {
        if (flags & 0x20) {
            c_mv->v[0][0] = mv.v[0][0] + mid_pred(t_mv->v[0][0], t_mv->v[2][0], t_mv->v[3][0]);
            c_mv->v[0][1] = mv.v[0][1] + mid_pred(t_mv->v[0][1], t_mv->v[2][1], t_mv->v[3][1]);
            c_mv->v[1][0] = mv.v[1][0] + mid_pred(t_mv->v[2][0], t_mv->v[3][0], c_mv->v[0][0]);
            c_mv->v[1][1] = mv.v[1][1] + mid_pred(t_mv->v[2][1], t_mv->v[3][1], c_mv->v[0][1]);
            c_mv->v[2][0] = mv.v[2][0] + mid_pred(t_mv->v[2][0], c_mv->v[0][0], c_mv->v[1][0]);
            c_mv->v[2][1] = mv.v[2][1] + mid_pred(t_mv->v[2][1], c_mv->v[0][1], c_mv->v[1][1]);
            c_mv->v[3][0] = mv.v[3][0] + mid_pred(c_mv->v[0][0], c_mv->v[1][0], c_mv->v[2][0]);
            c_mv->v[3][1] = mv.v[3][1] + mid_pred(c_mv->v[0][1], c_mv->v[1][1], c_mv->v[2][1]);
        } else {
            c_mv->v[0][0] = mv.v[0][0] + mid_pred(t_mv->v[2][0], lt_mv->v[3][0], l_mv->v[1][0]);
            c_mv->v[0][1] = mv.v[0][1] + mid_pred(t_mv->v[2][1], lt_mv->v[3][1], l_mv->v[1][1]);
            c_mv->v[1][0] = mv.v[1][0] + mid_pred(t_mv->v[2][0], t_mv->v[3][0],  c_mv->v[0][0]);
            c_mv->v[1][1] = mv.v[1][1] + mid_pred(t_mv->v[2][1], t_mv->v[3][1],  c_mv->v[0][1]);
            c_mv->v[2][0] = mv.v[2][0] + mid_pred(l_mv->v[1][0], l_mv->v[3][0],  c_mv->v[0][0]);
            c_mv->v[2][1] = mv.v[2][1] + mid_pred(l_mv->v[1][1], l_mv->v[3][1],  c_mv->v[0][1]);
            c_mv->v[3][0] = mv.v[3][0] + mid_pred(c_mv->v[0][0], c_mv->v[1][0],  c_mv->v[2][0]);
            c_mv->v[3][1] = mv.v[3][1] + mid_pred(c_mv->v[0][1], c_mv->v[1][1],  c_mv->v[2][1]);
        }
    } else {
        if (flags & 0x20) {
            c_mv->v[0][0] = mv.v[0][0];
            c_mv->v[0][1] = mv.v[0][1];
            c_mv->v[1][0] = mv.v[1][0] + mv.v[0][0];
            c_mv->v[1][1] = mv.v[1][1] + mv.v[0][1];
            c_mv->v[2][0] = mv.v[2][0] + mv.v[0][0];
            c_mv->v[2][1] = mv.v[2][1] + mv.v[0][1];
            c_mv->v[3][0] = mv.v[3][0] + mid_pred(c_mv->v[0][0], c_mv->v[1][0], c_mv->v[2][0]);
            c_mv->v[3][1] = mv.v[3][1] + mid_pred(c_mv->v[0][1], c_mv->v[1][1], c_mv->v[2][1]);
        } else {
            const int left_v1 = mv.nb_vectors == 0 ? 3 : 1;

            c_mv->v[0][0] = mv.v[0][0] + mid_pred(l_mv->v[0][0], l_mv->v[1][0], l_mv->v[3][0]);
            c_mv->v[0][1] = mv.v[0][1] + mid_pred(l_mv->v[0][1], l_mv->v[1][1], l_mv->v[3][1]);
            c_mv->v[2][0] = mv.v[2][0] + mid_pred(l_mv->v[1][0], l_mv->v[3][0], c_mv->v[0][0]);
            c_mv->v[2][1] = mv.v[2][1] + mid_pred(l_mv->v[1][1], l_mv->v[3][1], c_mv->v[0][1]);
            c_mv->v[1][0] = mv.v[1][0] + mid_pred(l_mv->v[left_v1][0], c_mv->v[0][0], c_mv->v[2][0]);
            c_mv->v[1][1] = mv.v[1][1] + mid_pred(l_mv->v[left_v1][1], c_mv->v[0][1], c_mv->v[2][1]);
            c_mv->v[3][0] = mv.v[3][0] + mid_pred(c_mv->v[0][0], c_mv->v[1][0], c_mv->v[2][0]);
            c_mv->v[3][1] = mv.v[3][1] + mid_pred(c_mv->v[0][1], c_mv->v[1][1], c_mv->v[2][1]);
        }
    }
}

static int bink2g_decode_mv(Bink2Context *c, GetBitContext *gb, int x, int y,
                            MVectors *mv)
{
    int num_mvs = get_bits1(gb) ? 1 : 4;

    mv->nb_vectors = num_mvs;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < num_mvs; j++) {
            int val = get_vlc2(gb, bink2g_mv_vlc.table, bink2g_mv_vlc.bits, 1);

            if (val < 0)
                return AVERROR_INVALIDDATA;
            if (val >= 8 && val != 15)
                val = val - 15;
            if (val == 15) {
                int bits = get_unary(gb, 1, 12) + 4;
                val = get_bits(gb, bits) + (1 << bits) - 1;
                if (val & 1)
                    val = (-(val >> 1) - 1);
                else
                    val =    val >> 1;
            }
            mv->v[j][i] = val;
        }
    }

    return 0;
}

static void update_intra_q(Bink2Context *c, int8_t *intra_q, int dq, int flags)
{
    if (flags & 0x20 && flags & 0x80)
        *intra_q = 16 + dq;
    else if (flags & 0x80)
        *intra_q = c->current_q[c->mb_pos - 1].intra_q + dq;
    else if (flags & 0x20)
        *intra_q = c->prev_q[c->mb_pos].intra_q + dq;
    else
        *intra_q = mid_pred(c->prev_q[c->mb_pos].intra_q,
                            c->current_q[c->mb_pos - 1].intra_q,
                            c->prev_q[c->mb_pos - 1].intra_q) + dq;
}

static void update_inter_q(Bink2Context *c, int8_t *inter_q, int dq, int flags)
{
    if (flags & 0x20 && flags & 0x80)
        *inter_q = 16 + dq;
    else if (flags & 0x80)
        *inter_q = c->current_q[c->mb_pos - 1].inter_q + dq;
    else if (flags & 0x20)
        *inter_q = c->prev_q[c->mb_pos].inter_q + dq;
    else
        *inter_q = mid_pred(c->prev_q[c->mb_pos].inter_q,
                            c->current_q[c->mb_pos - 1].inter_q,
                            c->prev_q[c->mb_pos - 1].inter_q) + dq;
}

static const uint8_t bink2g_chroma_weights[4][2] = {
    { 1, 0 },
    { 3, 1 },
    { 1, 1 },
    { 1, 3 },
};

static const uint8_t bink2g_chroma_shifts[4] = { 0, 2, 1, 2 };

static void bink2g_c_mc(Bink2Context *c, int x, int y,
                        uint8_t *dst, int stride,
                        uint8_t *src, int sstride,
                        int width, int height,
                        int mv_x, int mv_y,
                        int mode)
{
    const uint8_t *msrc;
    int hphase, vphase, shift, rounding;

    if (mv_x < 0 || mv_x >= width ||
        mv_y < 0 || mv_y >= height)
        return;

    msrc = src + mv_x + mv_y * sstride;

    if (mode == 0) {
        copy_block8(dst, msrc, stride, sstride, 8);
        return;
    }

    hphase = mode & 3;
    vphase = mode >> 2;
    shift = bink2g_chroma_shifts[hphase] + bink2g_chroma_shifts[vphase];
    rounding = 1 << (shift - 1);

    for (int y = 0; y < 8; y++) {
        const uint8_t *top = msrc + y * sstride;
        const uint8_t *bottom = top + (vphase != 0) * sstride;

        for (int x = 0; x < 8; x++) {
            const int top_value = bink2g_chroma_weights[hphase][0] * top[x] +
                                  bink2g_chroma_weights[hphase][1] * top[x + (hphase != 0)];
            const int bottom_value = bink2g_chroma_weights[hphase][0] * bottom[x] +
                                     bink2g_chroma_weights[hphase][1] * bottom[x + (hphase != 0)];
            const int value = bink2g_chroma_weights[vphase][0] * top_value +
                              bink2g_chroma_weights[vphase][1] * bottom_value;

            dst[x] = (value + rounding) >> shift;
        }
        dst += stride;
    }
}

static void bink2g_filter_vertical_edge(uint8_t *p, int stride,
                                        unsigned left, unsigned right)
{
    const unsigned index = left | (right << 2);

    if (!index || index == 15)
        return;

    for (int y = 0; y < 8; y++, p += stride) {
        const int b = p[-1];
        const int c = p[0];
        const int delta = c - b;
        const int outer = (2 * delta + 8) >> 4;
        const int inner = (4 * delta + 8) >> 4;

        if (left >= 2)
            p[-2] = av_clip_uint8(p[-2] + outer);
        if (left >= 1)
            p[-1] = av_clip_uint8(b + inner);
        if (right >= 1)
            p[0] = av_clip_uint8(c - inner);
        if (right >= 2)
            p[1] = av_clip_uint8(p[1] - outer);
    }
}

static void bink2g_filter_horizontal_edge(uint8_t *p, int stride,
                                          unsigned top, unsigned bottom)
{
    const unsigned index = top | (bottom << 2);

    if (!index || index == 15)
        return;

    for (int x = 0; x < 8; x++, p++) {
        const int b = p[-stride];
        const int c = p[0];
        const int delta = c - b;
        const int outer = (2 * delta + 8) >> 4;
        const int inner = (4 * delta + 8) >> 4;

        if (top >= 2)
            p[-2 * stride] = av_clip_uint8(p[-2 * stride] + outer);
        if (top >= 1)
            p[-stride] = av_clip_uint8(b + inner);
        if (bottom >= 1)
            p[0] = av_clip_uint8(c - inner);
        if (bottom >= 2)
            p[stride] = av_clip_uint8(p[stride] - outer);
    }
}

static int bink2g_mcompensate_chroma(Bink2Context *c, int x, int y,
                                     uint8_t *dst, int stride,
                                     uint8_t *src, int sstride,
                                     int width, int height)
{
    MVectors *mv = &c->current_mv[c->mb_pos].mv;
    int mv_x, mv_y, mode;

    mv_x  = (mv->v[0][0] >> 2) + x;
    mv_y  = (mv->v[0][1] >> 2) + y;
    mode  =  mv->v[0][0] & 3;
    mode |= (mv->v[0][1] & 3) << 2;
    bink2g_c_mc(c, x, y, dst + x, stride, src, sstride, width, height, mv_x, mv_y, mode);

    mv_x = (mv->v[1][0] >> 2) + x + 8;
    mv_y = (mv->v[1][1] >> 2) + y;
    mode  =  mv->v[1][0] & 3;
    mode |= (mv->v[1][1] & 3) << 2;
    bink2g_c_mc(c, x, y, dst + x + 8, stride, src, sstride, width, height, mv_x, mv_y, mode);

    mv_x = (mv->v[2][0] >> 2) + x;
    mv_y = (mv->v[2][1] >> 2) + y + 8;
    mode  =  mv->v[2][0] & 3;
    mode |= (mv->v[2][1] & 3) << 2;
    bink2g_c_mc(c, x, y, dst + x + 8 * stride, stride, src, sstride, width, height, mv_x, mv_y, mode);

    mv_x = (mv->v[3][0] >> 2) + x + 8;
    mv_y = (mv->v[3][1] >> 2) + y + 8;
    mode  =  mv->v[3][0] & 3;
    mode |= (mv->v[3][1] & 3) << 2;
    bink2g_c_mc(c, x, y, dst + x + 8 + 8 * stride, stride, src, sstride, width, height, mv_x, mv_y, mode);

    return 0;
}

static void bink2g_y_mc(Bink2Context *c, int x, int y,
                        uint8_t *dst, int stride,
                        uint8_t *src, int sstride,
                        int width, int height,
                        int mv_x, int mv_y, int mode)
{
    uint8_t *msrc;

    if (mv_x < 0 || mv_x >= width ||
        mv_y < 0 || mv_y >= height)
        return;

    msrc = src + mv_x + mv_y * sstride;

    if (mode == 0) {
        copy_block16(dst, msrc, stride, sstride, 16);
    } else if (mode == 1) {
        for (int j = 0; j < 16; j++) {
            for (int i = 0; i < 16; i++)
                dst[i] = av_clip_uint8(LHFILTER(msrc + i));
            dst  += stride;
            msrc += sstride;
        }
    } else if (mode == 2) {
        for (int j = 0; j < 16; j++) {
            for (int i = 0; i < 16; i++)
                dst[i*stride] = av_clip_uint8(LVFILTER(msrc + i*sstride, sstride));
            dst  += 1;
            msrc += 1;
        }
    } else if (mode == 3) {
        int16_t temp[21 * 16];

        msrc -= 2 * sstride;
        for (int i = 0; i < 21; i++) {
            for (int j = 0; j < 16; j++)
                temp[i*16+j] = LHFILTER(msrc + j);
            msrc += sstride;
        }
        for (int j = 0; j < 16; j++) {
            for (int i = 0; i < 16; i++)
                dst[i] = av_clip_uint8(LVFILTER(temp+(j+2)*16+i, 16));
            dst  += stride;
        }
    }
}

static int bink2g_mcompensate_luma(Bink2Context *c, int x, int y,
                                   uint8_t *dst, int stride,
                                   uint8_t *src, int sstride,
                                   int width, int height)
{
    MVectors *mv = &c->current_mv[c->mb_pos].mv;
    int mv_x, mv_y, mode;

    mv_x  = (mv->v[0][0] >> 1) + x;
    mv_y  = (mv->v[0][1] >> 1) + y;
    mode  =  mv->v[0][0] & 1;
    mode |= (mv->v[0][1] & 1) << 1;
    bink2g_y_mc(c, x, y, dst + x, stride, src, sstride, width, height, mv_x, mv_y, mode);

    mv_x  = (mv->v[1][0] >> 1) + x + 16;
    mv_y  = (mv->v[1][1] >> 1) + y;
    mode  =  mv->v[1][0] & 1;
    mode |= (mv->v[1][1] & 1) << 1;
    bink2g_y_mc(c, x, y, dst + x + 16, stride, src, sstride, width, height, mv_x, mv_y, mode);

    mv_x  = (mv->v[2][0] >> 1) + x;
    mv_y  = (mv->v[2][1] >> 1) + y + 16;
    mode  =  mv->v[2][0] & 1;
    mode |= (mv->v[2][1] & 1) << 1;
    bink2g_y_mc(c, x, y, dst + x + 16 * stride, stride, src, sstride, width, height, mv_x, mv_y, mode);

    mv_x  = (mv->v[3][0] >> 1) + x + 16;
    mv_y  = (mv->v[3][1] >> 1) + y + 16;
    mode  =  mv->v[3][0] & 1;
    mode |= (mv->v[3][1] & 1) << 1;
    bink2g_y_mc(c, x, y, dst + x + 16 + 16 * stride, stride, src, sstride, width, height, mv_x, mv_y, mode);

    return 0;
}

static int bink2g_average_block(uint8_t *src, int stride)
{
    int sum = 0;

    for (int i = 0; i < 8; i++) {
        int avg_a = (src[i+0*stride] + src[i+1*stride] + 1) >> 1;
        int avg_b = (src[i+2*stride] + src[i+3*stride] + 1) >> 1;
        int avg_c = (src[i+4*stride] + src[i+5*stride] + 1) >> 1;
        int avg_d = (src[i+6*stride] + src[i+7*stride] + 1) >> 1;
        int avg_e = (avg_a + avg_b + 1) >> 1;
        int avg_f = (avg_c + avg_d + 1) >> 1;
        int avg_g = (avg_e + avg_f + 1) >> 1;
        sum += avg_g;
    }

    return sum;
}

static void bink2g_average_chroma(Bink2Context *c, int x, int y,
                                  uint8_t *src, int stride,
                                  int *dc)
{
    for (int i = 0; i < 4; i++) {
        int X = i & 1;
        int Y = i >> 1;
        dc[i] = bink2g_average_block(src + x + X * 8 + (y + Y * 8) * stride, stride);
    }
}

static void bink2g_average_luma(Bink2Context *c, int x, int y,
                                uint8_t *src, int stride,
                                int *dc)
{
    for (int i = 0; i < 16; i++) {
        int I = luma_repos[i];
        int X = I & 3;
        int Y = I >> 2;
        dc[i] = bink2g_average_block(src + x + X * 8 + (y + Y * 8) * stride, stride);
    }
}

static inline unsigned bink2g_filter_strength(uint16_t state)
{
    return (state >> 13) & 3;
}

static inline int bink2g_filter_map_skip(const uint8_t *map, int position)
{
    const unsigned bit = position >> 3;

    return (map[bit >> 3] >> (bit & 7)) & 1;
}

static inline int bink2g_mv_discontinuity(const MVectors *a, int ai,
                                          const MVectors *b, int bi)
{
    return FFABS(a->v[ai][0] - b->v[bi][0]) > 1 ||
           FFABS(a->v[ai][1] - b->v[bi][1]) > 1;
}

static inline void bink2g_force_mv_edge(const MVectors *a, int ai,
                                        const MVectors *b, int bi,
                                        uint16_t left_state,
                                        uint16_t right_state,
                                        unsigned *left, unsigned *right)
{
    if (!(left_state & 0x8000) && !(right_state & 0x8000) &&
        bink2g_mv_discontinuity(a, ai, b, bi))
        *left = *right = 2;
}

static inline void bink2g_select_coding_edge(uint16_t left_state,
                                             uint16_t right_state,
                                             unsigned *left,
                                             unsigned *right)
{
    const int left_intra  = !!(left_state  & 0x8000);
    const int right_intra = !!(right_state & 0x8000);

    if (!left_intra)
        *left = 0;
    if (!right_intra)
        *right = 0;
}

static inline int bink2g_recon_near(uint16_t a, uint16_t b, int threshold)
{
    const int16_t ab = (int16_t)(uint16_t)(a - b);
    const int16_t ba = (int16_t)(uint16_t)(b - a);

    return FFMAX(ab, ba) < threshold;
}

static inline int bink2g_recon_mulhi(int k, int v)
{
    return ((int32_t)(int16_t)k * (int16_t)v) >> 16;
}

static inline int bink2g_recon_curve(int base, int left, int right, int pos)
{
    switch (pos) {
    case 0: return base + bink2g_recon_mulhi(26624, left);
    case 1: return base + bink2g_recon_mulhi(18432, left) + (right >> 7);
    case 2: return base + bink2g_recon_mulhi(12288, left) + (right >> 5);
    case 3: return base + bink2g_recon_mulhi( 7168, left) + (right >> 4);
    case 4: return base + bink2g_recon_mulhi( 7168, right) + (left >> 4);
    case 5: return base + bink2g_recon_mulhi(12288, right) + (left >> 5);
    case 6: return base + bink2g_recon_mulhi(18432, right) + (left >> 7);
    default:return base + bink2g_recon_mulhi(26624, right);
    }
}

static inline uint16_t bink2g_recon_state(const DCIPredict *row, int comp,
                                          int blocks_per_mb, int blocks_w,
                                          int block_x, int block_y)
{
    block_x = av_clip(block_x, 0, blocks_w - 1);
    return row[block_x / blocks_per_mb].state[comp]
              [block_y * blocks_per_mb + block_x % blocks_per_mb];
}

static void bink2g_reconstruct_block(uint8_t *dst, int stride,
                                     int width, int height,
                                     uint16_t left, uint16_t target,
                                     uint16_t right, uint16_t up_left,
                                     uint16_t up, uint16_t up_right,
                                     uint16_t down_left, uint16_t down,
                                     uint16_t down_right, int threshold)
{
    int center[8], top[8], bottom[8];
    int d, l, r, u, v, ul, ur, dl, dr;

    if (target < 0xE000 || width <= 0 || height <= 0)
        return;

    d  = target & 0x1FFF;
    l  = bink2g_recon_near(target, left,       threshold) ? left       & 0x1FFF : d;
    r  = bink2g_recon_near(target, right,      threshold) ? right      & 0x1FFF : d;
    u  = bink2g_recon_near(target, up,         threshold) ? up         & 0x1FFF : d;
    v  = bink2g_recon_near(target, down,       threshold) ? down       & 0x1FFF : d;
    ul = bink2g_recon_near(target, up_left,    threshold) ? up_left    & 0x1FFF : l;
    ur = bink2g_recon_near(target, up_right,   threshold) ? up_right   & 0x1FFF : r;
    dl = bink2g_recon_near(target, down_left,  threshold) ? down_left  & 0x1FFF : l;
    dr = bink2g_recon_near(target, down_right, threshold) ? down_right & 0x1FFF : r;

    for (int x = 0; x < 8; x++) {
        center[x] = bink2g_recon_curve(d + 4, l - d, r - d, x);
        top[x]    = bink2g_recon_curve(u + 4, ul - u, ur - u, x) - center[x];
        bottom[x] = bink2g_recon_curve(v + 4, dl - v, dr - v, x) - center[x];
    }

    for (int y = 0; y < FFMIN(height, 8); y++) {
        for (int x = 0; x < FFMIN(width, 8); x++)
            dst[x] = av_clip_uint8(bink2g_recon_curve(center[x], top[x],
                                                      bottom[x], y) >> 3);
        dst += stride;
    }
}

static void bink2g_reconstruct_blocks(Bink2Context *c, uint8_t *dst,
                                       int stride, int comp, int global_y,
                                       const DCIPredict *up_row, int up_by,
                                       const DCIPredict *row, int by,
                                       const DCIPredict *down_row, int down_by)
{
    const int chroma = comp == 1 || comp == 2;
    const int blocks_per_mb = chroma ? 2 : 4;
    const int plane_w = chroma ? (c->avctx->width  + 1) >> 1 : c->avctx->width;
    const int plane_h = chroma ? (c->avctx->height + 1) >> 1 : c->avctx->height;
    const int visible_blocks_w = (plane_w + 7) >> 3;
    const int coded_blocks_w = ((c->avctx->width + 31) >> 5) * blocks_per_mb;
    const int coded_plane_h = ((c->avctx->height + 31) >> 5) *
                              blocks_per_mb * 8;
    const int threshold = (c->frame_flags & 0xFF) << 3;
    const int have_up = global_y >= 8;
    const int have_down = global_y + 8 < coded_plane_h;

    for (int bx = 0; bx < visible_blocks_w; bx++) {
        const uint16_t target = bink2g_recon_state(row, comp, blocks_per_mb,
                                                   coded_blocks_w, bx, by);
        const uint16_t left = bx ?
                               bink2g_recon_state(row, comp, blocks_per_mb,
                                                  coded_blocks_w, bx - 1, by) : target;
        const uint16_t right = bx + 1 < coded_blocks_w ?
                                bink2g_recon_state(row, comp, blocks_per_mb,
                                                   coded_blocks_w, bx + 1, by) : target;
        const uint16_t up_left = have_up && bx ?
                                     bink2g_recon_state(up_row, comp, blocks_per_mb,
                                                        coded_blocks_w, bx - 1, up_by) : 0;
        const uint16_t up = have_up ?
                             bink2g_recon_state(up_row, comp, blocks_per_mb,
                                                coded_blocks_w, bx, up_by) : target;
        const uint16_t up_right = have_up && bx + 1 < coded_blocks_w ?
                                       bink2g_recon_state(up_row, comp, blocks_per_mb,
                                                         coded_blocks_w, bx + 1, up_by) : 0;
        const uint16_t down_left = bx ?
                                        have_down ?
                                            bink2g_recon_state(down_row, comp, blocks_per_mb,
                                                               coded_blocks_w, bx - 1, down_by) :
                                            (target & 0xE000) | (left & 0x1FFF) : 0;
        const uint16_t down = have_down ?
                               bink2g_recon_state(down_row, comp, blocks_per_mb,
                                                  coded_blocks_w, bx, down_by) : target;
        const uint16_t down_right = bx + 1 < coded_blocks_w ?
                                         have_down ?
                                             bink2g_recon_state(down_row, comp, blocks_per_mb,
                                                                coded_blocks_w, bx + 1, down_by) :
                                             (target & 0xE000) | (right & 0x1FFF) : 0;

        bink2g_reconstruct_block(dst + bx * 8, stride,
                                 plane_w - bx * 8, plane_h - global_y,
                                 left, target, right, up_left, up, up_right,
                                 down_left, down, down_right, threshold);
    }
}

static void bink2g_reconstruct_row(Bink2Context *c, uint8_t *dst[4],
                                   const int stride[4], int y,
                                   const DCIPredict *prev_idc,
                                   const DCIPredict *current_idc,
                                   int is_last)
{
    for (int comp = 0; comp < 3 + c->has_alpha; comp++) {
        const int chroma = comp == 1 || comp == 2;
        const int plane = comp == 1 ? 2 : comp == 2 ? 1 : comp;
        const int blocks_per_mb = chroma ? 2 : 4;
        const int plane_y = chroma ? y >> 1 : y;

        if (y > 0) {
            const int by = blocks_per_mb - 1;

            bink2g_reconstruct_blocks(c, dst[plane] - 8 * stride[plane],
                                      stride[plane], comp, plane_y - 8,
                                      prev_idc, by - 1,
                                      prev_idc, by,
                                      current_idc, 0);
        }

        for (int by = 0; by < blocks_per_mb - 1; by++) {
            const DCIPredict *up_row = by ? current_idc :
                                           y > 0 ? prev_idc : current_idc;
            const int up_by = by ? by - 1 :
                              y > 0 ? blocks_per_mb - 1 : 0;

            bink2g_reconstruct_blocks(c, dst[plane] + by * 8 * stride[plane],
                                      stride[plane], comp, plane_y + by * 8,
                                      up_row, up_by,
                                      current_idc, by,
                                      current_idc, by + 1);
        }

        if (is_last) {
            const int by = blocks_per_mb - 1;

            bink2g_reconstruct_blocks(c, dst[plane] + by * 8 * stride[plane],
                                      stride[plane], comp, plane_y + by * 8,
                                      current_idc, by - 1,
                                      current_idc, by,
                                      current_idc, by);
        }
    }
}

static void bink2g_filter_vertical_row(Bink2Context *c, uint8_t *dst[4],
                                       const int stride[4], int comp,
                                       const DCIPredict *row,
                                       const MVPredict *mv_row,
                                       int by, int rel_y)
{
    const int chroma = comp != 0;
    const int plane = comp == 1 ? 2 : comp == 2 ? 1 : 0;
    const int blocks_per_mb = chroma ? 2 : 4;
    const int plane_w = chroma ? (c->avctx->width + 1) >> 1 : c->avctx->width;
    const int blocks_w = (plane_w + 7) >> 3;

    if (c->frame_flags & 0x4000)
        return;

    for (int bx = 1; bx < blocks_w; bx++) {
        const int mb = bx / blocks_per_mb;
        const int local_x = bx % blocks_per_mb;
        const int left_mb = local_x ? mb : mb - 1;
        const int left_x = local_x ? local_x - 1 : blocks_per_mb - 1;
        const uint16_t left_state = row[left_mb].state[comp]
                                       [by * blocks_per_mb + left_x];
        const uint16_t right_state = row[mb].state[comp]
                                        [by * blocks_per_mb + local_x];
        unsigned left = bink2g_filter_strength(left_state);
        unsigned right = bink2g_filter_strength(right_state);
        const int edge_x = bx * (chroma ? 16 : 8);

        bink2g_select_coding_edge(left_state, right_state, &left, &right);

        if (edge_x >= c->avctx->width ||
            bink2g_filter_map_skip(c->col_cbp, edge_x))
            continue;

        if (chroma || !(bx & 1)) {
            const int qrow = chroma ? by : by >> 1;
            const int left_q = 2 * qrow + (local_x ? 0 : 1);
            const int right_q = 2 * qrow + (local_x ? 1 : 0);

            bink2g_force_mv_edge(&mv_row[left_mb].mv, left_q,
                                 &mv_row[mb].mv, right_q,
                                 left_state, right_state, &left, &right);
        }

        bink2g_filter_vertical_edge(dst[plane] + bx * 8 +
                                   rel_y * stride[plane],
                                   stride[plane], left, right);
    }
}

static void bink2g_filter_horizontal_row(Bink2Context *c, uint8_t *dst[4],
                                         const int stride[4], int comp,
                                         const DCIPredict *top_row, int top_by,
                                         const MVPredict *top_mv,
                                         const DCIPredict *bottom_row, int bottom_by,
                                         const MVPredict *bottom_mv,
                                         int rel_y, int edge_y, int slice_start)
{
    const int chroma = comp != 0;
    const int plane = comp == 1 ? 2 : comp == 2 ? 1 : 0;
    const int blocks_per_mb = chroma ? 2 : 4;
    const int plane_w = chroma ? (c->avctx->width + 1) >> 1 : c->avctx->width;
    const int blocks_w = (plane_w + 7) >> 3;

    if ((c->frame_flags & 0x8000) || edge_y == slice_start ||
        edge_y >= c->avctx->height ||
        bink2g_filter_map_skip(c->row_cbp, edge_y))
        return;

    for (int bx = 0; bx < blocks_w; bx++) {
        const int mb = bx / blocks_per_mb;
        const int local_x = bx % blocks_per_mb;
        const uint16_t top_state = top_row[mb].state[comp]
                                      [top_by * blocks_per_mb + local_x];
        const uint16_t bottom_state = bottom_row[mb].state[comp]
                                         [bottom_by * blocks_per_mb + local_x];
        unsigned top = bink2g_filter_strength(top_state);
        unsigned bottom = bink2g_filter_strength(bottom_state);

        bink2g_select_coding_edge(top_state, bottom_state, &top, &bottom);

        if (chroma || bottom_by == 0 || bottom_by == 2) {
            const int qcol = chroma ? local_x : local_x >> 1;
            const int top_q = (bottom_by ? 0 : 2) + qcol;
            const int bottom_q = (bottom_by ? 2 : 0) + qcol;

            bink2g_force_mv_edge(&top_mv[mb].mv, top_q,
                                 &bottom_mv[mb].mv, bottom_q,
                                 top_state, bottom_state, &top, &bottom);
        }

        bink2g_filter_horizontal_edge(dst[plane] + bx * 8 +
                                     rel_y * stride[plane],
                                     stride[plane], top, bottom);
    }
}

static void bink2g_postprocess_row(Bink2Context *c, uint8_t *dst[4],
                                   const int stride[4], int y,
                                   int frame_end,
                                   const DCIPredict *prev_idc,
                                   const DCIPredict *current_idc,
                                   const MVPredict *prev_mv,
                                   const MVPredict *current_mv)
{
    const int is_last = y + 32 >= frame_end;

    bink2g_reconstruct_row(c, dst, stride, y,
                           prev_idc, current_idc, is_last);

    for (int comp = 0; comp < 3; comp++) {
        const int chroma = comp != 0;
        const int blocks_per_mb = chroma ? 2 : 4;

        if (y > 0)
            bink2g_filter_vertical_row(c, dst, stride, comp, prev_idc,
                                       prev_mv,
                                       blocks_per_mb - 1, -8);
        for (int by = 0; by < blocks_per_mb - 1; by++)
            bink2g_filter_vertical_row(c, dst, stride, comp, current_idc,
                                       current_mv, by, by * 8);
        if (is_last)
            bink2g_filter_vertical_row(c, dst, stride, comp, current_idc,
                                       current_mv,
                                       blocks_per_mb - 1,
                                       (blocks_per_mb - 1) * 8);
    }

    if (y > 0) {
        bink2g_filter_horizontal_row(c, dst, stride, 0,
                                     prev_idc, 2, prev_mv,
                                     prev_idc, 3, prev_mv,
                                     -8, y - 8, 0);
        bink2g_filter_horizontal_row(c, dst, stride, 0,
                                     prev_idc, 3, prev_mv,
                                     current_idc, 0, current_mv,
                                     0, y, 0);
        for (int comp = 1; comp <= 2; comp++) {
            bink2g_filter_horizontal_row(c, dst, stride, comp,
                                         prev_idc, 0, prev_mv,
                                         prev_idc, 1, prev_mv,
                                         -8, y - 16, 0);
            bink2g_filter_horizontal_row(c, dst, stride, comp,
                                         prev_idc, 1, prev_mv,
                                         current_idc, 0, current_mv,
                                         0, y, 0);
        }
    }

    bink2g_filter_horizontal_row(c, dst, stride, 0,
                                 current_idc, 0, current_mv,
                                 current_idc, 1, current_mv,
                                 8, y + 8, 0);
    bink2g_filter_horizontal_row(c, dst, stride, 0,
                                 current_idc, 1, current_mv,
                                 current_idc, 2, current_mv,
                                 16, y + 16, 0);

    if (is_last) {
        bink2g_filter_horizontal_row(c, dst, stride, 0,
                                     current_idc, 2, current_mv,
                                     current_idc, 3, current_mv,
                                     24, y + 24, 0);
        for (int comp = 1; comp <= 2; comp++)
            bink2g_filter_horizontal_row(c, dst, stride, comp,
                                         current_idc, 0, current_mv,
                                         current_idc, 1, current_mv,
                                         8, y + 16, 0);
    }
}

static void bink2g_postprocess_frame(Bink2Context *c, uint8_t *dst[4],
                                     const int stride[4])
{
    const int mb_width  = (c->avctx->width  + 31) >> 5;
    const int mb_height = (c->avctx->height + 31) >> 5;
    const int frame_end = mb_height << 5;

    for (int mb_y = 0; mb_y < mb_height; mb_y++) {
        const int y = mb_y << 5;
        const int prev_y = mb_y ? mb_y - 1 : 0;
        const DCIPredict *prev_idc = c->frame_idc + prev_y * mb_width;
        const DCIPredict *current_idc = c->frame_idc + mb_y * mb_width;
        const MVPredict *prev_mv = c->frame_mv + prev_y * mb_width;
        const MVPredict *current_mv = c->frame_mv + mb_y * mb_width;
        uint8_t *row_dst[4] = {
            dst[0] + y * stride[0],
            dst[1] + (y >> 1) * stride[1],
            dst[2] + (y >> 1) * stride[2],
            c->has_alpha ? dst[3] + y * stride[3] : NULL,
        };

        bink2g_postprocess_row(c, row_dst, stride, y, frame_end,
                               prev_idc, current_idc, prev_mv, current_mv);
    }
}

static int bink2g_decode_slice(Bink2Context *c,
                               uint8_t *dst[4], int stride[4],
                               uint8_t *src[4], int sstride[4],
                               int is_kf, int start, int end)
{
    GetBitContext *gb = &c->gb;
    int w = c->avctx->width;
    int h = c->avctx->height;
    const int mb_width = (w + 31) / 32;
    int ret = 0, dq, flags;

    memset(c->prev_q, 0, mb_width * sizeof(*c->prev_q));
    memset(c->prev_mv, 0, mb_width * sizeof(*c->prev_mv));
    memset(c->prev_idc, 0, mb_width * sizeof(*c->prev_idc));
    memset(c->current_idc, 0, mb_width * sizeof(*c->current_idc));

    for (int y = start; y < end; y += 32) {
        int types_lru[4] = { MOTION_BLOCK, RESIDUE_BLOCK, SKIP_BLOCK, INTRA_BLOCK };
        unsigned y_cbp_intra = 0, u_cbp_intra = 0, v_cbp_intra = 0, a_cbp_intra = 0;
        unsigned y_cbp_inter = 0, u_cbp_inter = 0, v_cbp_inter = 0, a_cbp_inter = 0;

        memset(c->current_q, 0, ((c->avctx->width + 31) / 32) * sizeof(*c->current_q));
        memset(c->current_mv, 0, ((c->avctx->width + 31) / 32) * sizeof(*c->current_mv));

        for (int x = 0; x < c->avctx->width; x += 32) {
            int type = is_kf ? INTRA_BLOCK : bink2g_get_type(gb, types_lru);
            int8_t *intra_q = &c->current_q[x / 32].intra_q;
            int8_t *inter_q = &c->current_q[x / 32].inter_q;
            MVectors mv = { 0 };

            c->mb_pos = x / 32;
            memset(c->current_idc[c->mb_pos].state, 0,
                   sizeof(c->current_idc[c->mb_pos].state));
            c->current_idc[c->mb_pos].block_type = type;
            flags = 0;
            if (y == start)
                flags |= 0x80;
            if (!x)
                flags |= 0x20;
            if (x == 32)
                flags |= 0x200;
            if (x + 32 >= c->avctx->width)
                flags |= 0x40;
            switch (type) {
            case INTRA_BLOCK:
                if (!(flags & 0xA0) && c->prev_idc[c->mb_pos - 1].block_type != INTRA_BLOCK) {
                    bink2g_average_luma  (c, x  -32, -32, dst[0], stride[0], c->prev_idc[c->mb_pos - 1].dc[0]);
                    bink2g_average_chroma(c, x/2-16, -16, dst[2], stride[2], c->prev_idc[c->mb_pos - 1].dc[1]);
                    bink2g_average_chroma(c, x/2-16, -16, dst[1], stride[1], c->prev_idc[c->mb_pos - 1].dc[2]);
                    if (c->has_alpha)
                        bink2g_average_luma(c, x-32, -32, dst[3], stride[3], c->prev_idc[c->mb_pos - 1].dc[3]);
                }
                if (!(flags & 0x20) && c->current_idc[c->mb_pos - 1].block_type != INTRA_BLOCK) {
                    bink2g_average_luma  (c, x  -32, 0, dst[0], stride[0], c->current_idc[c->mb_pos - 1].dc[0]);
                    bink2g_average_chroma(c, x/2-16, 0, dst[2], stride[2], c->current_idc[c->mb_pos - 1].dc[1]);
                    bink2g_average_chroma(c, x/2-16, 0, dst[1], stride[1], c->current_idc[c->mb_pos - 1].dc[2]);
                    if (c->has_alpha)
                        bink2g_average_luma(c, x-32, 0, dst[3], stride[3], c->current_idc[c->mb_pos - 1].dc[3]);
                }
                if ((flags & 0x20) && !(flags & 0x80) && c->prev_idc[c->mb_pos + 1].block_type != INTRA_BLOCK) {
                    bink2g_average_luma  (c, x  +32, -32, dst[0], stride[0], c->prev_idc[c->mb_pos + 1].dc[0]);
                    bink2g_average_chroma(c, x/2+16, -16, dst[2], stride[2], c->prev_idc[c->mb_pos + 1].dc[1]);
                    bink2g_average_chroma(c, x/2+16, -16, dst[1], stride[1], c->prev_idc[c->mb_pos + 1].dc[2]);
                    if (c->has_alpha)
                        bink2g_average_luma(c, x+32, -32, dst[3], stride[3], c->prev_idc[c->mb_pos + 1].dc[3]);
                }
                if (!(flags & 0x80) && c->prev_idc[c->mb_pos].block_type != INTRA_BLOCK) {
                    bink2g_average_luma  (c, x,   -32, dst[0], stride[0], c->prev_idc[c->mb_pos].dc[0]);
                    bink2g_average_chroma(c, x/2, -16, dst[2], stride[2], c->prev_idc[c->mb_pos].dc[1]);
                    bink2g_average_chroma(c, x/2, -16, dst[1], stride[1], c->prev_idc[c->mb_pos].dc[2]);
                    if (c->has_alpha)
                        bink2g_average_luma(c, x, -32, dst[3], stride[3], c->prev_idc[c->mb_pos].dc[3]);
                }

                bink2g_predict_mv(c, x, y, flags, mv);
                update_inter_q(c, inter_q, 0, flags);
                dq = bink2g_decode_dq(gb);
                update_intra_q(c, intra_q, dq, flags);
                if (*intra_q < 0 || *intra_q >= 37) {
                    ret = AVERROR_INVALIDDATA;
                    goto fail;
                }
                c->comp = 0;
                ret = bink2g_decode_intra_luma(c, gb, c->iblock, &y_cbp_intra, *intra_q, &c->dsp,
                                               dst[0] + x, stride[0], flags);
                if (ret < 0)
                    goto fail;
                c->comp = 1;
                ret = bink2g_decode_intra_chroma(c, gb, c->iblock, &u_cbp_intra, *intra_q, &c->dsp,
                                                 dst[2] + x/2, stride[2], flags);
                if (ret < 0)
                    goto fail;
                c->comp = 2;
                ret = bink2g_decode_intra_chroma(c, gb, c->iblock, &v_cbp_intra, *intra_q, &c->dsp,
                                                 dst[1] + x/2, stride[1], flags);
                if (ret < 0)
                    goto fail;
                if (c->has_alpha) {
                    c->comp = 3;
                    ret = bink2g_decode_intra_luma(c, gb, c->iblock, &a_cbp_intra, *intra_q, &c->dsp,
                                                   dst[3] + x, stride[3], flags);
                    if (ret < 0)
                        goto fail;
                }
                break;
            case SKIP_BLOCK:
                update_inter_q(c, inter_q, 0, flags);
                update_intra_q(c, intra_q, 0, flags);
                copy_block16(dst[0] + x, src[0] + x + sstride[0] * y,
                             stride[0], sstride[0], 32);
                copy_block16(dst[0] + x + 16, src[0] + x + 16 + sstride[0] * y,
                             stride[0], sstride[0], 32);
                copy_block16(dst[1] + (x/2), src[1] + (x/2) + sstride[1] * (y/2),
                             stride[1], sstride[1], 16);
                copy_block16(dst[2] + (x/2), src[2] + (x/2) + sstride[2] * (y/2),
                             stride[2], sstride[2], 16);
                if (c->has_alpha) {
                    copy_block16(dst[3] + x, src[3] + x + sstride[3] * y,
                                 stride[3], sstride[3], 32);
                    copy_block16(dst[3] + x + 16, src[3] + x + 16 + sstride[3] * y,
                                 stride[3], sstride[3], 32);
                }
                break;
            case MOTION_BLOCK:
                update_intra_q(c, intra_q, 0, flags);
                update_inter_q(c, inter_q, 0, flags);
                ret = bink2g_decode_mv(c, gb, x, y, &mv);
                if (ret < 0)
                    goto fail;
                bink2g_predict_mv(c, x, y, flags, mv);
                c->comp = 0;
                ret = bink2g_mcompensate_luma(c, x, y,
                                              dst[0], stride[0],
                                              src[0], sstride[0],
                                              w, h);
                if (ret < 0)
                    goto fail;
                c->comp = 1;
                ret = bink2g_mcompensate_chroma(c, x/2, y/2,
                                                dst[2], stride[2],
                                                src[2], sstride[2],
                                                w/2, h/2);
                if (ret < 0)
                    goto fail;
                c->comp = 2;
                ret = bink2g_mcompensate_chroma(c, x/2, y/2,
                                                dst[1], stride[1],
                                                src[1], sstride[1],
                                                w/2, h/2);
                if (ret < 0)
                    goto fail;
                if (c->has_alpha) {
                    c->comp = 3;
                    ret = bink2g_mcompensate_luma(c, x, y,
                                                  dst[3], stride[3],
                                                  src[3], sstride[3],
                                                  w, h);
                    if (ret < 0)
                        goto fail;
                }
                break;
            case RESIDUE_BLOCK:
                update_intra_q(c, intra_q, 0, flags);
                ret = bink2g_decode_mv(c, gb, x, y, &mv);
                if (ret < 0)
                    goto fail;
                bink2g_predict_mv(c, x, y, flags, mv);
                dq = bink2g_decode_dq(gb);
                update_inter_q(c, inter_q, dq, flags);
                if (*inter_q < 0 || *inter_q >= 37) {
                    ret = AVERROR_INVALIDDATA;
                    goto fail;
                }
                c->comp = 0;
                ret = bink2g_mcompensate_luma(c, x, y,
                                              dst[0], stride[0],
                                              src[0], sstride[0],
                                              w, h);
                if (ret < 0)
                    goto fail;
                c->comp = 1;
                ret = bink2g_mcompensate_chroma(c, x/2, y/2,
                                                dst[2], stride[2],
                                                src[2], sstride[2],
                                                w/2, h/2);
                if (ret < 0)
                    goto fail;
                c->comp = 2;
                ret = bink2g_mcompensate_chroma(c, x/2, y/2,
                                                dst[1], stride[1],
                                                src[1], sstride[1],
                                                w/2, h/2);
                if (ret < 0)
                    goto fail;
                if (c->has_alpha) {
                    c->comp = 3;
                    ret = bink2g_mcompensate_luma(c, x, y,
                                                  dst[3], stride[3],
                                                  src[3], sstride[3],
                                                  w, h);
                    if (ret < 0)
                        goto fail;
                }
                c->comp = 0;
                ret = bink2g_decode_inter_luma(c, gb, c->iblock, &y_cbp_inter, *inter_q, &c->dsp,
                                               dst[0] + x, stride[0], flags);
                if (ret < 0)
                    goto fail;
                if (get_bits1(gb)) {
                    c->comp = 1;
                    ret = bink2g_decode_inter_chroma(c, gb, c->iblock, &u_cbp_inter, *inter_q, &c->dsp,
                                                     dst[2] + x/2, stride[2], flags);
                    if (ret < 0)
                        goto fail;
                    c->comp = 2;
                    ret = bink2g_decode_inter_chroma(c, gb, c->iblock, &v_cbp_inter, *inter_q, &c->dsp,
                                                     dst[1] + x/2, stride[1], flags);
                    if (ret < 0)
                        goto fail;
                } else {
                    u_cbp_inter = 0;
                    v_cbp_inter = 0;
                }
                if (c->has_alpha) {
                    c->comp = 3;
                    ret = bink2g_decode_inter_luma(c, gb, c->iblock, &a_cbp_inter, *inter_q, &c->dsp,
                                                   dst[3] + x, stride[3], flags);
                    if (ret < 0)
                        goto fail;
                }
                break;
            default:
                return AVERROR_INVALIDDATA;
            }

        }

        memcpy(c->frame_idc + (y >> 5) * mb_width, c->current_idc,
               mb_width * sizeof(*c->frame_idc));
        memcpy(c->frame_mv + (y >> 5) * mb_width, c->current_mv,
               mb_width * sizeof(*c->frame_mv));

        dst[0] += stride[0] * 32;
        dst[1] += stride[1] * 16;
        dst[2] += stride[2] * 16;
        if (c->has_alpha)
            dst[3] += stride[3] * 32;

        FFSWAP(MVPredict *, c->current_mv, c->prev_mv);
        FFSWAP(QuantPredict *, c->current_q, c->prev_q);
        FFSWAP(DCIPredict *, c->current_idc, c->prev_idc);
    }
fail:
    emms_c();

    return ret;
}

#endif /* AVCODEC_BINK2G_H */
