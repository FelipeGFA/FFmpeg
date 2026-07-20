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

#include "libavutil/avassert.h"
#include "libavutil/attributes.h"
#include "libavutil/emms.h"
#include "libavutil/imgutils.h"
#include "libavutil/internal.h"
#include "libavutil/mem.h"
#include "avcodec.h"
#include "blockdsp.h"
#include "codec_internal.h"
#include "copy_block.h"
#include "decode.h"
#include "idctdsp.h"
#include "internal.h"
#include "mathops.h"

#define BITSTREAM_READER_LE
#include "get_bits.h"
#include "unary.h"
#include "bink2.h"

#include "bink2f.h"
#include "bink2g.h"

static void bink2_get_block_flags(GetBitContext *gb, int offset, int size, uint8_t *dst)
{
    int j, v = 0, flags_left, mode = 0, nv;
    unsigned cache, flag = 0;

    if (get_bits1(gb) == 0) {
        for (j = 0; j < size >> 3; j++)
            dst[j] = get_bits(gb, 8);
        dst[j] = get_bitsz(gb, size & 7);

        return;
    }

    flags_left = size;
    while (flags_left > 0) {
        cache = offset;
        if (get_bits1(gb) == 0) {
            if (mode == 3) {
                flag ^= 1;
            } else {
                flag = get_bits1(gb);
            }
            mode = 2;
            if (flags_left < 5) {
                nv = get_bitsz(gb, flags_left - 1);
                nv <<= (offset + 1) & 0x1f;
                offset += flags_left;
                flags_left = 0;
            } else {
                nv = get_bits(gb, 4) << ((offset + 1) & 0x1f);
                offset += 5;
                flags_left -= 5;
            }
            v |= flag << (cache & 0x1f) | nv;
            if (offset >= 8) {
                *dst++ = v & 0xff;
                v >>= 8;
                offset -= 8;
            }
        } else {
            int temp, bits, nb_coded;

            bits = flags_left < 4 ? 2 : flags_left < 16 ? 4 : 5;
            nb_coded = bits + 1;
            if (mode == 3) {
                flag ^= 1;
            } else {
                nb_coded++;
                flag = get_bits1(gb);
            }
            nb_coded = FFMIN(nb_coded, flags_left);
            flags_left -= nb_coded;
            if (flags_left > 0) {
                temp = get_bits(gb, bits);
                flags_left -= temp;
                nb_coded += temp;
                mode = temp == (1 << bits) - 1U ? 1 : 3;
            }

            temp = (flag << 0x1f) >> 0x1f & 0xff;
            while (nb_coded > 8) {
                v |= temp << (cache & 0x1f);
                *dst++ = v & 0xff;
                v >>= 8;
                nb_coded -= 8;
            }
            if (nb_coded > 0) {
                offset += nb_coded;
                v |= ((1 << (nb_coded & 0x1f)) - 1U & temp) << (cache & 0x1f);
                if (offset >= 8) {
                    *dst++ = v & 0xff;
                    v >>= 8;
                    offset -= 8;
                }
            }
        }
    }

    if (offset != 0)
        *dst = v;
}

static av_always_inline int bink2_round_q16(int value)
{
    return value < 0 ? -((-value + 32768) >> 16) : (value + 32768) >> 16;
}

static void bink2_convert_new_ycrcb(AVFrame *frame)
{
    const int chroma_width  = (frame->width  + 1) >> 1;
    const int chroma_height = (frame->height + 1) >> 1;

    for (int y = 0; y < chroma_height; y++) {
        uint8_t *cb = frame->data[1] + y * frame->linesize[1];
        uint8_t *cr = frame->data[2] + y * frame->linesize[2];

        for (int x = 0; x < chroma_width; x++) {
            const int raw_cb = cb[x] - 128;
            const int raw_cr = cr[x] - 128;
            const int out_cb = bink2_round_q16(32768 * raw_cb - 22554 * raw_cr);
            const int out_cr = bink2_round_q16(-46802 * raw_cb + 32768 * raw_cr);

            cb[x] = av_clip_uint8(128 + out_cb);
            cr[x] = av_clip_uint8(128 + out_cr);
        }
    }
}

typedef struct Bink2ThreadData {
    AVFrame *frame;
    int is_kf;
    int slice_end[BINK2_MAX_SLICES];
} Bink2ThreadData;

static int bink2_decode_slice(AVCodecContext *avctx, void *arg,
                              int jobnr, int threadnr)
{
    Bink2Context *parent = avctx->priv_data;
    Bink2ThreadData *td = arg;
    Bink2Context *c = parent->slice_ctx[jobnr];
    GetBitContext *gb = &c->gb;
    AVFrame *frame = td->frame;
    uint8_t *dst[4];
    uint8_t *src[4];
    int stride[4];
    int sstride[4];
    const int start = jobnr ? parent->slice_height[jobnr - 1] : 0;
    const int end = parent->slice_height[jobnr];
    int bits_left, ret;

    (void)threadnr;

    for (int i = 0; i < 4; i++) {
        src[i]     = parent->last->data[i];
        dst[i]     = frame->data[i];
        stride[i]  = frame->linesize[i];
        sstride[i] = parent->last->linesize[i];
    }

    dst[0] += start * stride[0];
    dst[1] += start / 2 * stride[1];
    dst[2] += start / 2 * stride[2];
    if (c->has_alpha)
        dst[3] += start * stride[3];

    if (c->version <= 'f')
        ret = bink2f_decode_slice(c, dst, stride, src, sstride,
                                  td->is_kf, start, end);
    else
        ret = bink2g_decode_slice(c, dst, stride, src, sstride,
                                  td->is_kf, start, end);
    if (ret < 0)
        return ret;

    align_get_bits(gb);
    bits_left = 8 * td->slice_end[jobnr] - get_bits_count(gb);
    if (bits_left < 0)
        av_log(avctx, AV_LOG_WARNING, "slice %d: overread\n", jobnr);
    else if (bits_left > 24)
        av_log(avctx, AV_LOG_WARNING, "slice %d: underread %d\n",
               jobnr, bits_left);

    return 0;
}

static int bink2_decode_frame(AVCodecContext *avctx, AVFrame *frame,
                              int *got_frame, AVPacket *pkt)
{
    Bink2Context * const c = avctx->priv_data;
    GetBitContext *gb = &c->gb;
    Bink2ThreadData td = { .frame = frame };
    int thread_ret[BINK2_MAX_SLICES] = { 0 };
    int is_kf = !!(pkt->flags & AV_PKT_FLAG_KEY);
    int first_slice_bit, ret, w, h;
    int height_a;

    if (pkt->size < 4)
        return AVERROR_INVALIDDATA;

    w = avctx->width;
    h = avctx->height;
    ret = ff_set_dimensions(avctx, FFALIGN(w, 32), FFALIGN(h, 32));
    if (ret < 0)
        return ret;
    avctx->width  = w;
    avctx->height = h;

    if ((ret = ff_get_buffer(avctx, frame, AV_GET_BUFFER_FLAG_REF)) < 0)
        return ret;

    if (!is_kf && (!c->last->data[0] ||
                   !c->last->data[1] ||
                   !c->last->data[2]))
        return AVERROR_INVALIDDATA;

    c->frame_flags = AV_RL32(pkt->data);
    ff_dlog(avctx, "frame flags %X\n", c->frame_flags);

    if ((ret = init_get_bits8(gb, pkt->data, pkt->size)) < 0)
        return ret;

    height_a = (avctx->height + 31) & 0xFFFFFFE0;
    if (c->version <= 'f') {
        c->num_slices = 2;
        c->slice_height[0] = (avctx->height / 2 + 16) & 0xFFFFFFE0;
    } else if (c->version == 'g') {
        if (height_a < 128) {
            c->num_slices = 1;
        } else {
            c->num_slices = 2;
            c->slice_height[0] = (avctx->height / 2 + 16) & 0xFFFFFFE0;
        }
    } else {
        int start, end;

        c->num_slices = kb2h_num_slices[c->flags & 3];
        start = 0;
        end = height_a + 32 * c->num_slices - 1;
        for (int i = 0; i < c->num_slices - 1; i++) {
            start += ((end - start) / (c->num_slices - i)) & 0xFFFFFFE0;
            end -= 32;
            c->slice_height[i] = start;
        }
    }
    c->slice_height[c->num_slices - 1] = height_a;

    if (pkt->size < 4 + 4 * (c->num_slices - 1))
        return AVERROR_INVALIDDATA;

    skip_bits_long(gb, 32 + 32 * (c->num_slices - 1));

    if (c->frame_flags & 0x10000) {
        if (!(c->frame_flags & 0x8000))
            bink2_get_block_flags(gb, 1, (((avctx->height + 15) & ~15) >> 3) - 1, c->row_cbp);
        if (!(c->frame_flags & 0x4000))
            bink2_get_block_flags(gb, 1, (((avctx->width + 31) & ~31) >> 3) - 1, c->col_cbp);
    }

    if (get_bits_left(gb) < 0)
        return AVERROR_INVALIDDATA;

    td.is_kf = is_kf;
    first_slice_bit = get_bits_count(gb);
    for (int i = 0; i < c->num_slices; i++) {
        Bink2Context *sc = c->slice_ctx[i];
        const int start = i ? td.slice_end[i - 1] : 0;
        const int start_bit = i ? 8 * start : first_slice_bit;

        td.slice_end[i] = i == c->num_slices - 1 ?
                          pkt->size : AV_RL32(pkt->data + 4 + 4 * i);
        if (td.slice_end[i] < start ||
            td.slice_end[i] > pkt->size ||
            8 * td.slice_end[i] < start_bit)
            return AVERROR_INVALIDDATA;

        sc->frame_flags = c->frame_flags;
        if (i) {
            if ((ret = init_get_bits8(&sc->gb, pkt->data, pkt->size)) < 0)
                return ret;
            skip_bits_long(&sc->gb, start_bit);
        }
    }

    if ((ret = avctx->execute2(avctx, bink2_decode_slice, &td,
                               thread_ret, c->num_slices)) < 0)
        return ret;
    for (int i = 0; i < c->num_slices; i++)
        if (thread_ret[i] < 0)
            return thread_ret[i];

    if (is_kf)
        frame->flags |= AV_FRAME_FLAG_KEY;
    else
        frame->flags &= ~AV_FRAME_FLAG_KEY;
    frame->pict_type = is_kf ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_P;

    av_frame_unref(c->last);
    if ((ret = av_frame_ref(c->last, frame)) < 0)
        return ret;

    if (c->flags & BINK_FLAG_YCRCB_NEW) {
        if ((ret = av_frame_make_writable(frame)) < 0)
            return ret;
        bink2_convert_new_ycrcb(frame);
    }

    frame->colorspace  = avctx->colorspace;
    frame->color_range = avctx->color_range;

    *got_frame = 1;

    /* always report that the buffer was completely consumed */
    return pkt->size;
}

#define INIT_VLC_STATIC_LE(vlc, nb_bits, nb_codes,                 \
                           bits, bits_wrap, bits_size,             \
                           codes, codes_wrap, codes_size,          \
                           symbols, symbols_wrap, symbols_size,    \
                           static_size)                            \
    do {                                                           \
        static VLCElem table[static_size];                         \
        (vlc)->table           = table;                            \
        (vlc)->table_allocated = static_size;                      \
        ff_vlc_init_sparse(vlc, nb_bits, nb_codes,                 \
                           bits, bits_wrap, bits_size,             \
                           codes, codes_wrap, codes_size,          \
                           symbols, symbols_wrap, symbols_size,    \
                           VLC_INIT_LE | VLC_INIT_USE_STATIC);     \
    } while (0)

static int bink2_alloc_slice_state(Bink2Context *c, int width)
{
    const int mb_width = (width + 31) / 32;

    c->current_q = av_malloc_array(mb_width, sizeof(*c->current_q));
    c->prev_q = av_malloc_array(mb_width, sizeof(*c->prev_q));
    c->current_dc = av_malloc_array(mb_width, sizeof(*c->current_dc));
    c->prev_dc = av_malloc_array(mb_width, sizeof(*c->prev_dc));
    c->current_idc = av_malloc_array(mb_width, sizeof(*c->current_idc));
    c->prev_idc = av_malloc_array(mb_width, sizeof(*c->prev_idc));
    c->current_mv = av_malloc_array(mb_width, sizeof(*c->current_mv));
    c->prev_mv = av_malloc_array(mb_width, sizeof(*c->prev_mv));

    if (!c->current_q || !c->prev_q ||
        !c->current_dc || !c->prev_dc ||
        !c->current_idc || !c->prev_idc ||
        !c->current_mv || !c->prev_mv)
        return AVERROR(ENOMEM);

    return 0;
}

static void bink2_free_slice_state(Bink2Context *c)
{
    av_freep(&c->current_q);
    av_freep(&c->prev_q);
    av_freep(&c->current_dc);
    av_freep(&c->prev_dc);
    av_freep(&c->current_idc);
    av_freep(&c->prev_idc);
    av_freep(&c->current_mv);
    av_freep(&c->prev_mv);
}

static av_cold int bink2_decode_init(AVCodecContext *avctx)
{
    Bink2Context * const c = avctx->priv_data;
    int ret;

    c->version = avctx->codec_tag >> 24;
    if (avctx->extradata_size < 4) {
        av_log(avctx, AV_LOG_ERROR, "Extradata missing or too short\n");
        return AVERROR_INVALIDDATA;
    }
    c->flags = AV_RL32(avctx->extradata);
    av_log(avctx, AV_LOG_DEBUG, "flags: 0x%X\n", c->flags);
    c->has_alpha = c->flags & BINK_FLAG_ALPHA;
    c->avctx = avctx;
    c->slice_ctx[0] = c;

    c->last = av_frame_alloc();
    if (!c->last)
        return AVERROR(ENOMEM);

    if ((ret = av_image_check_size(avctx->width, avctx->height, 0, avctx)) < 0)
        return ret;

    avctx->pix_fmt = c->has_alpha ? AV_PIX_FMT_YUVA420P : AV_PIX_FMT_YUV420P;
    avctx->colorspace = AVCOL_SPC_SMPTE170M;
    avctx->color_range = c->flags & BINK_FLAG_YCRCB_NEW ?
                         AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;

    ff_blockdsp_init(&c->dsp);

    INIT_VLC_STATIC_LE(&bink2f_quant_vlc, 9, FF_ARRAY_ELEMS(bink2f_quant_codes),
                       bink2f_quant_bits, 1, 1, bink2f_quant_codes, 1, 1, NULL, 0, 0, 512);
    INIT_VLC_STATIC_LE(&bink2f_ac_val0_vlc, 9, FF_ARRAY_ELEMS(bink2f_ac_val_bits[0]),
                       bink2f_ac_val_bits[0], 1, 1, bink2f_ac_val_codes[0], 2, 2, NULL, 0, 0, 512);
    INIT_VLC_STATIC_LE(&bink2f_ac_val1_vlc, 9, FF_ARRAY_ELEMS(bink2f_ac_val_bits[1]),
                       bink2f_ac_val_bits[1], 1, 1, bink2f_ac_val_codes[1], 2, 2, NULL, 0, 0, 512);
    INIT_VLC_STATIC_LE(&bink2f_ac_skip0_vlc, 9, FF_ARRAY_ELEMS(bink2f_ac_skip_bits[0]),
                       bink2f_ac_skip_bits[0], 1, 1, bink2f_ac_skip_codes[0], 2, 2, NULL, 0, 0, 512);
    INIT_VLC_STATIC_LE(&bink2f_ac_skip1_vlc, 9, FF_ARRAY_ELEMS(bink2f_ac_skip_bits[1]),
                       bink2f_ac_skip_bits[1], 1, 1, bink2f_ac_skip_codes[1], 2, 2, NULL, 0, 0, 512);

    INIT_VLC_STATIC_LE(&bink2g_ac_skip0_vlc, 9, FF_ARRAY_ELEMS(bink2g_ac_skip_bits[0]),
                       bink2g_ac_skip_bits[0], 1, 1, bink2g_ac_skip_codes[0], 2, 2, NULL, 0, 0, 512);
    INIT_VLC_STATIC_LE(&bink2g_ac_skip1_vlc, 9, FF_ARRAY_ELEMS(bink2g_ac_skip_bits[1]),
                       bink2g_ac_skip_bits[1], 1, 1, bink2g_ac_skip_codes[1], 2, 2, NULL, 0, 0, 512);
    INIT_VLC_STATIC_LE(&bink2g_mv_vlc, 9, FF_ARRAY_ELEMS(bink2g_mv_bits),
                       bink2g_mv_bits, 1, 1, bink2g_mv_codes, 1, 1, NULL, 0, 0, 512);

    if ((ret = bink2_alloc_slice_state(c, avctx->width)) < 0)
        return ret;

    c->col_cbp = av_calloc((((avctx->width + 31) >> 3) + 7) >> 3, sizeof(*c->col_cbp));
    if (!c->col_cbp)
        return AVERROR(ENOMEM);

    c->row_cbp = av_calloc((((avctx->height + 31) >> 3) + 7) >> 3, sizeof(*c->row_cbp));
    if (!c->row_cbp)
        return AVERROR(ENOMEM);

    for (int i = 1; i < BINK2_MAX_SLICES; i++) {
        Bink2Context *sc = av_mallocz(sizeof(*sc));

        if (!sc)
            return AVERROR(ENOMEM);
        c->slice_ctx[i] = sc;
        sc->avctx = avctx;
        sc->version = c->version;
        sc->has_alpha = c->has_alpha;
        sc->flags = c->flags;
        sc->dsp = c->dsp;
        if ((ret = bink2_alloc_slice_state(sc, avctx->width)) < 0)
            return ret;
    }

    return 0;
}

static void bink2_flush(AVCodecContext *avctx)
{
    Bink2Context *c = avctx->priv_data;

    av_frame_unref(c->last);
}

static av_cold int bink2_decode_end(AVCodecContext *avctx)
{
    Bink2Context * const c = avctx->priv_data;

    av_frame_free(&c->last);
    for (int i = 1; i < BINK2_MAX_SLICES; i++) {
        if (c->slice_ctx[i]) {
            bink2_free_slice_state(c->slice_ctx[i]);
            av_freep(&c->slice_ctx[i]);
        }
    }
    bink2_free_slice_state(c);
    av_freep(&c->col_cbp);
    av_freep(&c->row_cbp);

    return 0;
}

const FFCodec ff_bink2_decoder = {
    .p.name         = "binkvideo2",
    .p.long_name    = NULL_IF_CONFIG_SMALL("Bink video 2"),
    .p.type         = AVMEDIA_TYPE_VIDEO,
    .p.id           = AV_CODEC_ID_BINKVIDEO2,
    .priv_data_size = sizeof(Bink2Context),
    .init           = bink2_decode_init,
    .close          = bink2_decode_end,
    FF_CODEC_DECODE_CB(bink2_decode_frame),
    .flush          = bink2_flush,
    .p.capabilities = AV_CODEC_CAP_DR1 | AV_CODEC_CAP_SLICE_THREADS,
    .caps_internal  = FF_CODEC_CAP_INIT_CLEANUP,
};
