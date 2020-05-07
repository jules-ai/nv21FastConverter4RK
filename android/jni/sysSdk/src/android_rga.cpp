/*
 * Copyright 2017 Rockchip Electronics S.LSI Co. LTD
 * Author: Addy Ke <addy.ke@iotwrt.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version
 */

#include <asm/types.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#include <linux/stddef.h>
#include <errno.h>

#include <time.h>

#include <linux/videodev2.h>

#include "rockchip_rga.h"
//#include "log.h"
#include "rga_priv.h"
#include "android_rga.h"

typedef struct _RgaRect {
    __u32 x;  // xpos
    __u32 y;  // ypos
    __u32 w;  // width
    __u32 h;  // height
    __u32 ws; // wstride
    __u32 hs; // hstride
    __u32 f;  // format
    __u32 s;  // size
} RgaRect;

enum yuv2RgbMode{
    RGB_TO_RGB = 0,
    YUV_TO_YUV = 0,
    YUV_TO_RGB = 0x1 << 0,
    RGB_TO_YUV = 0x2 << 4,
};

static RgaSURF_FORMAT
V4l2ToRgaFormat(__u32 v4l2Format, __u32 yuvToRgbMode) {
    switch(v4l2Format) {
    case V4L2_PIX_FMT_ARGB32:
        return RK_FORMAT_RGBA_8888;
    case V4L2_PIX_FMT_RGB24:
        return RK_FORMAT_RGB_888;
    case V4L2_PIX_FMT_BGR24:
        return RK_FORMAT_BGR_888;
    case V4L2_PIX_FMT_ABGR32:
        return RK_FORMAT_BGRA_8888;
    case V4L2_PIX_FMT_RGB565:
        return RK_FORMAT_RGB_565;
    case V4L2_PIX_FMT_NV12:
        return RK_FORMAT_YCrCb_420_SP;  // Todo: why is Ycrcb NOT Ycbcr
    case V4L2_PIX_FMT_YUV420:
    if(yuvToRgbMode == RGB_TO_YUV)
        return RK_FORMAT_YCbCr_420_P;
    else
            return RK_FORMAT_YCrCb_420_P; // Todo: why is RK_FORMAT_YCrCb_420_P, not RK_FORMAT_YCbCr_420_P ?
    case V4L2_PIX_FMT_NV16:
        return RK_FORMAT_YCrCb_422_SP;

    default:
        return RK_FORMAT_UNKNOWN;
    }    
}

static const int sinaTable[360] = {
    0,   1144,   2287,   3430,   4572,   5712,   6850,   7987,   9121,  10252,
    11380,  12505,  13626,  14742,  15855,  16962,  18064,  19161,  20252,  21336,
    22415,  23486,  24550,  25607,  26656,  27697,  28729,  29753,  30767,  31772,
    32768,  33754,  34729,  35693,  36647,  37590,  38521,  39441,  40348,  41243,
    42126,  42995,  43852,  44695,  45525,  46341,  47143,  47930,  48703,  49461,
    50203,  50931,  51643,  52339,  53020,  53684,  54332,  54963,  55578,  56175,
    56756,  57319,  57865,  58393,  58903,  59396,  59870,  60326,  60764,  61183,
    61584,  61966,  62328,  62672,  62997,  63303,  63589,  63856,  64104,  64332,
    64540,  64729,  64898,  65048,  65177,  65287,  65376,  65446,  65496,  65526,
    65536,  65526,  65496,  65446,  65376,  65287,  65177,  65048,  64898,  64729,
    64540,  64332,  64104,  63856,  63589,  63303,  62997,  62672,  62328,  61966,
    61584,  61183,  60764,  60326,  59870,  59396,  58903,  58393,  57865,  57319,
    56756,  56175,  55578,  54963,  54332,  53684,  53020,  52339,  51643,  50931,
    50203,  49461,  48703,  47930,  47143,  46341,  45525,  44695,  43852,  42995,
    42126,  41243,  40348,  39441,  38521,  37590,  36647,  35693,  34729,  33754,
    32768,  31772,  30767,  29753,  28729,  27697,  26656,  25607,  24550,  23486,
    22415,  21336,  20252,  19161,  18064,  16962,  15855,  14742,  13626,  12505,
    11380,  10252,   9121,   7987,   6850,   5712,   4572,   3430,   2287,   1144,
    0,  -1144,  -2287,  -3430,  -4572,  -5712,  -6850,  -7987,  -9121, -10252,
    -11380, -12505, -13626, -14742, -15855, -16962, -18064, -19161, -20252, -21336,
    -22415, -23486, -24550, -25607, -26656, -27697, -28729, -29753, -30767, -31772,
    -32768, -33754, -34729, -35693, -36647, -37590, -38521, -39441, -40348, -41243,
    -42126, -42995, -43852, -44695, -45525, -46341, -47143, -47930, -48703, -49461,
    -50203, -50931, -51643, -52339, -53020, -53684, -54332, -54963, -55578, -56175,
    -56756, -57319, -57865, -58393, -58903, -59396, -59870, -60326, -60764, -61183,
    -61584, -61966, -62328, -62672, -62997, -63303, -63589, -63856, -64104, -64332,
    -64540, -64729, -64898, -65048, -65177, -65287, -65376, -65446, -65496, -65526,
    -65536, -65526, -65496, -65446, -65376, -65287, -65177, -65048, -64898, -64729,
    -64540, -64332, -64104, -63856, -63589, -63303, -62997, -62672, -62328, -61966,
    -61584, -61183, -60764, -60326, -59870, -59396, -58903, -58393, -57865, -57319,
    -56756, -56175, -55578, -54963, -54332, -53684, -53020, -52339, -51643, -50931,
    -50203, -49461, -48703, -47930, -47143, -46341, -45525, -44695, -43852, -42995,
    -42126, -41243, -40348, -39441, -38521, -37590, -36647, -35693, -34729, -33754,
    -32768, -31772, -30767, -29753, -28729, -27697, -26656, -25607, -24550, -23486,
    -22415, -21336, -20252, -19161, -18064, -16962, -15855, -14742, -13626, -12505,
    -11380, -10252, -9121,   -7987,  -6850,  -5712,  -4572,  -3430,  -2287,  -1144
};

static const int cosaTable[360] = {
    65536,  65526,  65496,  65446,  65376,  65287,  65177,  65048,  64898,  64729,
    64540,  64332,  64104,  63856,  63589,  63303,  62997,  62672,  62328,  61966,
    61584,  61183,  60764,  60326,  59870,  59396,  58903,  58393,  57865,  57319,
    56756,  56175,  55578,  54963,  54332,  53684,  53020,  52339,  51643,  50931,
    50203,  49461,  48703,  47930,  47143,  46341,  45525,  44695,  43852,  42995,
    42126,  41243,  40348,  39441,  38521,  37590,  36647,  35693,  34729,  33754,
    32768,  31772,  30767,  29753,  28729,  27697,  26656,  25607,  24550,  23486,
    22415,  21336,  20252,  19161,  18064,  16962,  15855,  14742,  13626,  12505,
    11380,  10252,   9121,   7987,   6850,   5712,   4572,   3430,   2287,   1144,
    0,  -1144,  -2287,  -3430,  -4572,  -5712,  -6850,  -7987,  -9121, -10252,
    -11380, -12505, -13626, -14742, -15855, -16962, -18064, -19161, -20252, -21336,
    -22415, -23486, -24550, -25607, -26656, -27697, -28729, -29753, -30767, -31772,
    -32768, -33754, -34729, -35693, -36647, -37590, -38521, -39441, -40348, -41243,
    -42126, -42995, -43852, -44695, -45525, -46341, -47143, -47930, -48703, -49461,
    -50203, -50931, -51643, -52339, -53020, -53684, -54332, -54963, -55578, -56175,
    -56756, -57319, -57865, -58393, -58903, -59396, -59870, -60326, -60764, -61183,
    -61584, -61966, -62328, -62672, -62997, -63303, -63589, -63856, -64104, -64332,
    -64540, -64729, -64898, -65048, -65177, -65287, -65376, -65446, -65496, -65526,
    -65536, -65526, -65496, -65446, -65376, -65287, -65177, -65048, -64898, -64729,
    -64540, -64332, -64104, -63856, -63589, -63303, -62997, -62672, -62328, -61966,
    -61584, -61183, -60764, -60326, -59870, -59396, -58903, -58393, -57865, -57319,
    -56756, -56175, -55578, -54963, -54332, -53684, -53020, -52339, -51643, -50931,
    -50203, -49461, -48703, -47930, -47143, -46341, -45525, -44695, -43852, -42995,
    -42126, -41243, -40348, -39441, -38521, -37590, -36647, -35693, -34729, -33754,
    -32768, -31772, -30767, -29753, -28729, -27697, -26656, -25607, -24550, -23486,
    -22415, -21336, -20252, -19161, -18064, -16962, -15855, -14742, -13626, -12505,
    -11380, -10252,  -9121,  -7987,  -6850,  -5712,  -4572,  -3430,  -2287,  -1144,
    0,   1144,   2287,   3430,   4572,   5712,   6850,   7987,   9121,  10252,
    11380,  12505,  13626,  14742,  15855,  16962,  18064,  19161,  20252,  21336,
    22415,  23486,  24550,  25607,  26656,  27697,  28729,  29753,  30767,  31772,
    32768,  33754,  34729,  35693,  36647,  37590,  38521,  39441,  40348,  41243,
    42126,  42995,  43852,  44695,  45525,  46341,  47143,  47930,  48703,  49461,
    50203,  50931,  51643,  52339,  53020,  53684,  54332,  54963,  55578,  56175,
    56756,  57319,  57865,  58393,  58903,  59396,  59870,  60326,  60764,  61183,
    61584,  61966,  62328,  62672,  62997,  63303,  63589,  63856,  64104,  64332,
    64540,  64729,  64898,  65048,  65177,  65287,  65376,  65446,  65496,  65526
};

static __u32
BytesPerPixel(__u32 v4l2Format)
{
    switch(v4l2Format) {
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_ABGR32:
        return 4;
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
    return 3;
    case V4L2_PIX_FMT_RGB565:
        return 2;
    default:
        return 0;
    }
}

static int
IsYuvFormat(__u32 v4l2Format)
{
    switch(v4l2Format) {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YUYV:
        return 1;
    default:
        return 0;
    }
}

static void changeValue(__u32 *v1, __u32 *v2)
{
    __u32 tmp = *v1;

    *v1 = *v2;
    *v2 = tmp;
}

static int
AndroidRgaSetReq(RockchipRga *rga, RgaBuffer *srcBuf, RgaBuffer *dstBuf, struct rga_req *req)
{
    RgaRect srcRect, dstRect;
    __u32 scaleMode,  orientation, rotateMode, ditherEn;
    __u32 srcMmuFlag, dstMmuFlag;
    __u32 stretch = 0;
    __u32 yuvToRgbMode;
    float hScale, vScale;

    srcRect.x = rga->ctx.srcCropX;
    srcRect.y = rga->ctx.srcCropY;
    srcRect.w = (rga->ctx.srcCropW != 0)? rga->ctx.srcCropW : rga->ctx.srcWidth;
    srcRect.h = (rga->ctx.srcCropH != 0)? rga->ctx.srcCropH : rga->ctx.srcHeight;
    srcRect.ws = RGA_ALIGN(rga->ctx.srcWidth, 16);
    srcRect.hs = RGA_ALIGN(rga->ctx.srcHeight, 16);
    srcRect.f = rga->ctx.srcFormat;
    srcRect.s = srcBuf->size;

    dstRect.x = rga->ctx.dstCropX;
    dstRect.y = rga->ctx.dstCropY;
    dstRect.w = (rga->ctx.dstCropW != 0)? rga->ctx.dstCropW : rga->ctx.dstWidth;
    dstRect.h = (rga->ctx.dstCropH != 0)? rga->ctx.dstCropH : rga->ctx.dstHeight;
    dstRect.ws = RGA_ALIGN(rga->ctx.dstWidth, 8);
    dstRect.hs = RGA_ALIGN(rga->ctx.dstHeight, 8);
    dstRect.f = rga->ctx.dstFormat;
    dstRect.s = dstBuf->size;

    hScale = (float)srcRect.w / dstRect.w;
    vScale = (float)srcRect.h / dstRect.h;
    if (rga->ctx.rotate == RGA_ROTATE_90 || rga->ctx.rotate == RGA_ROTATE_270) {
        hScale = (float)srcRect.w / dstRect.h;
        vScale = (float)srcRect.h / dstRect.w;
    }

    if (hScale < 1/16 || hScale > 16 || vScale < 1/16 || vScale > 16) {
        rga_err(rga, "Error scale[%f,%f]\n", hScale, vScale);
        return -EINVAL;
    }

    scaleMode = 0;
    stretch = (hScale != 1.0f) || (vScale != 1.0f);
    if (hScale < 1 || vScale < 1)
    {
        scaleMode = 2;
        if(srcRect.f == V4L2_PIX_FMT_ABGR32 ||
            srcRect.f == V4L2_PIX_FMT_ARGB32) {
            rga_warn(rga, "Format %u: scale is not support, set scaleMode zero\n");
            scaleMode = 0;
        }
    }
    
    switch(rga->ctx.rotate) {
    case RGA_ROTATE_90:
        rotateMode = BB_ROTATE;
        orientation = 90;
        dstRect.x = dstRect.w - 1;
        dstRect.y = 0;
        changeValue(&dstRect.w, &dstRect.h);
        break;
    case RGA_ROTATE_180:
        rotateMode = BB_ROTATE;
        orientation = 180;
        dstRect.x = dstRect.w - 1;
        dstRect.y = dstRect.h - 1;
        break;
    case RGA_ROTATE_270:
        rotateMode = BB_ROTATE;
        orientation = 270;
        dstRect.x = 0;
        dstRect.y = dstRect.h - 1;
        changeValue(&dstRect.w, &dstRect.h);
        break;
    case RGA_ROTATE_HFLIP:
        rotateMode = BB_X_MIRROR;
    orientation = 0;
        break;
    case RGA_ROTATE_VFLIP:
        rotateMode = BB_Y_MIRROR;
    orientation = 0;
        break;
    default:
        rotateMode = BB_BYPASS;
        orientation = stretch;
    break;
    }

    ditherEn = (BytesPerPixel(srcRect.f) != BytesPerPixel(dstRect.f)) ? 1 : 0;

    if (srcBuf->fd < 0) {
        srcMmuFlag = 1;
    }
    if (dstBuf->fd < 0) {
        dstMmuFlag = 1;
    }

    if(IsYuvFormat(srcRect.f) && !IsYuvFormat(dstRect.f))
        yuvToRgbMode = YUV_TO_RGB;
    else if(!IsYuvFormat(srcRect.f) && IsYuvFormat(dstRect.f))
        yuvToRgbMode = RGB_TO_YUV;
    else if(IsYuvFormat(srcRect.f) && IsYuvFormat(dstRect.f))
        yuvToRgbMode = YUV_TO_YUV;
    else
        yuvToRgbMode = RGB_TO_RGB;

    memset(req, 0, sizeof(*req));

    /* Set source image information */
    if(srcBuf->fd >= 0)
        req->src.yrgb_addr = srcBuf->fd;
    if(srcBuf->ptr != NULL) {
        //req->src.yrgb_addr = (unsigned long)srcBuf->ptr;
        req->src.uv_addr  = (unsigned long)srcBuf->ptr;
        //req->src.v_addr   = (unsigned long)srcBuf->ptr + srcRect.ws * srcRect.hs;
    }
    req->src.vir_w = srcRect.ws;
    req->src.vir_h = srcRect.hs;
    req->src.format = V4l2ToRgaFormat(srcRect.f, yuvToRgbMode);
    req->src.alpha_swap = 0;

    req->src.act_w = srcRect.w;
    req->src.act_h = srcRect.h;
    req->src.x_offset = srcRect.x;
    req->src.y_offset = srcRect.y;

    /* Set destination image information */
    if(dstBuf->fd >= 0)
        req->dst.yrgb_addr = dstBuf->fd;
    if(dstBuf->ptr != NULL) {
        //req->dst.yrgb_addr = (unsigned long)dstBuf->ptr;
        req->dst.uv_addr  = (unsigned long)dstBuf->ptr;
        //req->dst.v_addr   = (unsigned long)dstBuf->ptr + dstRect.ws * dstRect.hs;
    }
    req->dst.vir_w = dstRect.ws;
    req->dst.vir_h = dstRect.hs;
    req->dst.format = V4l2ToRgaFormat(dstRect.f, yuvToRgbMode);
    req->dst.alpha_swap = 0;

    req->dst.act_w = dstRect.w;
    req->dst.act_h = dstRect.h;
    req->dst.x_offset = dstRect.x;
    req->dst.y_offset = dstRect.y;

    req->clip.xmin = 0;
    req->clip.xmax = dstRect.ws - 1;
    req->clip.ymin = 0;
    req->clip.ymax = dstRect.hs - 1;

    /* Set Bitblt Mode */
    req->render_mode = bitblt_mode;
    req->scale_mode = scaleMode;
    req->rotate_mode = rotateMode;
    req->sina = sinaTable[orientation];
    req->cosa = cosaTable[orientation];
    req->yuv2rgb_mode = yuvToRgbMode;
    req->alpha_rop_flag = ditherEn << 5;

    /* Set mmu information */
    if (srcMmuFlag || dstMmuFlag) {
        req->mmu_info.mmu_en    = 1;
        req->mmu_info.base_addr = 0;
        req->mmu_info.mmu_flag  = ((2 & 0x3) << 4) |
                ((0 & 0x1) << 3) |
                ((0 & 0x1) << 2) |
                ((0 & 0x1) << 1) | 1;
        req->mmu_info.mmu_flag |= (0x1 << 31) | ((srcMmuFlag) << 8) | ((dstMmuFlag) << 10);
    }

    //req->fg_color = rga->ctx.color;

    return 0;
}

static void AndroidRgaPrintReq(RockchipRga *rga, struct rga_req *req)
{
    rga_debug(rga, "src: yrgb_addr %d, uv_addr 0x%lx, v_addr 0x%lx, vir_w %u, vir_h %u, format %u\n",
          req->src.yrgb_addr, req->src.uv_addr, req->src.v_addr, req->src.vir_w, req->src.vir_h, req->src.format);
    rga_debug(rga, "src: act_w %u, act_h %u, x_offset %u, y_offset %u\n",
          req->src.act_w, req->src.act_h, req->src.x_offset, req->src.y_offset);
    rga_debug(rga, "dst: yrgb_addr %d, uv_addr 0x%lx, v_addr 0x%lx, vir_w %u, vir_h %u, format %u\n",
          req->dst.yrgb_addr, req->dst.uv_addr, req->dst.v_addr, req->dst.vir_w, req->dst.vir_h, req->dst.format);
    rga_debug(rga, "dst: act_w %u, act_h %u, x_offset %u, y_offset %u\n",
          req->dst.act_w, req->dst.act_h, req->dst.x_offset, req->dst.y_offset);
    rga_debug(rga, "clip: xmin %u, xmax %u, ymin %u, ymax %u\n",
          req->clip.xmin, req->clip.xmax, req->clip.ymin, req->clip.ymax);
    rga_debug(rga, "render_mode %u, scale_mode %u, rotate_mode %u, yuv2rgb_mode %u, sina %u, cosa %u, alpha_rop_flag %u\n",
          req->render_mode, req->scale_mode, req->rotate_mode, req->yuv2rgb_mode, req->sina, req->cosa, req->alpha_rop_flag);
    rga_debug(rga, "mmu info: mmu_en %u, base_addr %u, mmu_flag 0x%x\n",
          req->mmu_info.mmu_en, req->mmu_info.base_addr, req->mmu_info.mmu_flag);
}

static inline void Soft_YUYVToNV16(unsigned char *src, unsigned char *dst, __u32 width, __u32 height)
{
    int i;
    __u32 max;

    /* 1) YUYV format:
     *    y0 = yuv + 0;
     *    u0 = yuv + 1;
     *    y1 = yuv + 2;
     *    v0 = yuv + 3;
     * 2) NV16 format:
     *    y0 = yuv + 0;
     *    u0 = yuv + width * height + 0;
     *    v0 = yuv + width * height + 1;
    */ 

    //LOGD("~~~~~~~~~~ src:%p dst:%p, width:%u, height:%u", src, dst, width, height);
    max = width * height / 2;
    for(i = 0; i < max; i++) {
        // Copy Y */
        *(dst + 2 * i + 0)           = *(src + 4 * i + 0);
        *(dst + 2 * i + 1)           = *(src + 4 * i + 2);
        // Copy U */
        *(dst + max * 2 + 2 * i + 0) = *(src + 4 * i + 1);
        // Copy V */
        *(dst + max * 2 + 2 * i + 1) = *(src + 4 * i + 3);
    }
}

static inline void Soft_NV16ToYUYV(unsigned char *src, unsigned char *dst, __u32 width, __u32 height)
{
    int i;
    __u32 max;

    /* 1) YUYV format:
     *    y0 = yuv + 0;
     *    u0 = yuv + 1;
     *    y1 = yuv + 2;
     *    v0 = yuv + 3;
     * 2) NV16 format:
     *    y0 = yuv + 0;
     *    u0 = yuv + width * height + 0;
     *    v0 = yuv + width * height + 1;
    */ 

    max = width * height / 2;
    for(i = 0; i < max; i++) {
        // Copy Y */
        *(dst + 4 * i + 0) = *(src + 2 * i + 0);
        *(dst + 4 * i + 2) = *(src + 2 * i + 1);

        // Copy U */
        *(dst + 4 * i + 1) = *(src + max * 2 + 2 * i + 0);
        // Copy V */
        *(dst + 4 * i + 3) = *(src + max * 2 + 2 * i + 1);
    }
}

int AndroidRgaProcess(RockchipRga *rga, RgaBuffer *srcBuf, RgaBuffer *dstBuf) {
    struct rga_req req;
    int ret;
    //unsigned long long t1, t2, t3;

    if (rga->ctx.srcFormat == V4L2_PIX_FMT_YUYV) {
        assert(rga->ctx.srcWidth * rga->ctx.srcHeight * 2 <= MAX_YUV_SIZE);
        rga->buf.size = rga->ctx.srcWidth * rga->ctx.srcHeight * 2;
        //t1 = nanoTime();
        Soft_YUYVToNV16(srcBuf->ptr, rga->buf.ptr, rga->ctx.srcWidth, rga->ctx.srcHeight);
        //t2 = nanoTime();
        rga_debug(rga, "Software yuyv to nv16 time: %llums\n", (t2 - t1) / 1000000);

        rga->ctx.srcFormat = V4L2_PIX_FMT_NV16;
        ret = AndroidRgaSetReq(rga, &rga->buf, dstBuf, &req);
        if (ret < 0)
            return ret;
    } else if (rga->ctx.dstFormat == V4L2_PIX_FMT_YUYV) {
        assert(rga->ctx.dstWidth * rga->ctx.dstHeight <= MAX_YUV_SIZE);
        rga->buf.size = rga->ctx.srcWidth * rga->ctx.srcHeight;

        rga->ctx.dstFormat = V4L2_PIX_FMT_NV16;
        ret = AndroidRgaSetReq(rga, srcBuf, &rga->buf, &req);
        if (ret < 0)
            return ret;
        //t1 = nanoTime();
        Soft_NV16ToYUYV(rga->buf.ptr, dstBuf->ptr, rga->ctx.dstWidth, rga->ctx.dstHeight);
        //t2 = nanoTime();
        rga_debug(rga, "Software nv16 to yuyv time: %llums\n", (t2 - t1) / 1000000);

    } else {
        ret = AndroidRgaSetReq(rga, srcBuf, dstBuf, &req);
        if (ret < 0)
            return ret;
    }

    AndroidRgaPrintReq(rga, &req);
    ret = ioctl(rga->rgaFd, RGA_BLIT_SYNC, &req);
    if (ret < 0) {
        rga_err(rga, "ioctl: RGA_BLIT_SYNC failed, return %d, errno %d\n", ret, errno);
    }

    return ret;
}
}