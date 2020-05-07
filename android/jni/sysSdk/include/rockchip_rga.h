/*
 * Copyright 2017 Rockchip Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef __ROCKCHIP_RGA_H__
#define __ROCKCHIP_RGA_H__

#include <stdint.h>
#include <cstddef>
#include <linux/videodev2.h>
//#include "log.h"

#define RGA_ALIGN(x, a)         (((x) + (a) - 1) / (a) * (a))
#define rga_debug(_RGA_, fmt, args...) //LOGD(fmt, ##args)
#define rga_err(_RGA_, fmt, args...) printf(fmt, ##args)
#define rga_warn(_RGA_, fmt, args...) printf(fmt, ##args)

typedef enum _RgaRotate {
    RGA_ROTATE_NONE = 0,
    RGA_ROTATE_90,
    RGA_ROTATE_180,
    RGA_ROTATE_270,
    RGA_ROTATE_VFLIP,       // Vertical Mirror
    RGA_ROTATE_HFLIP,       // Horizontal Mirror
} RgaRotate;

typedef struct _RgaBuffer {
    int isExtBuf;
    int fd;
    unsigned handle;
    unsigned char *ptr;
    size_t size;
} RgaBuffer;

static inline int
RockchipRgaCheckFormat(__u32 v4l2Foramt)
{
    switch(v4l2Foramt) {
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_YUYV:
        return 1;
    default:
        return 0;
    }
}

typedef struct _RgaContext {
    RgaRotate rotate;
    __u32 color;

    __u32 srcFormat;
    __u32 srcWidth;
    __u32 srcHeight;
    __u32 srcCropX;
    __u32 srcCropY;
    __u32 srcCropW;
    __u32 srcCropH;

    __u32 dstFormat;
    __u32 dstWidth;
    __u32 dstHeight;
    __u32 dstCropX;
    __u32 dstCropY;
    __u32 dstCropW;
    __u32 dstCropH;
} RgaContext;

struct _RockchipRga;
typedef struct _RgaOps {
    /* Init rga contex */
    void (*initCtx)(struct _RockchipRga *rga);

    void (*setRotate)(struct _RockchipRga *rga, RgaRotate rotate);
    /* Set solid fill color, color:
     * Blue:  0xffff0000
     * Green: 0xff00ff00
     * Red:   0xff0000ff
     */
    void (*setFillColor)(struct _RockchipRga *rga, int color);

    /* Set source format */
    void (*setSrcFormat)(struct _RockchipRga *rga, __u32 v4l2Format,
		        __u32 width, __u32 height);

    /* Set destination format */
    void (*setDstFormat)(struct _RockchipRga *rga, __u32 v4l2Format,
		        __u32 width, __u32 height);

    /* Set source crop */
    void (*setSrcCrop)(struct _RockchipRga *rga, __u32 cropX, __u32 cropY,
                   __u32 cropW, __u32 cropH);

    /* Set destination crop */
    void (*setDstCrop)(struct _RockchipRga *rga, __u32 cropX, __u32 cropY,
                   __u32 cropW, __u32 cropH);

    /* Set source dma buffer file description */
    void (*setSrcBufferFd)(struct _RockchipRga *rga, int dmaFd);

    /* Set destination dma buffer file description */
    void (*setDstBufferFd)(struct _RockchipRga *rga, int dmaFd);

    /* Set source userspace data pointer */
    void (*setSrcBufferPtr)(struct _RockchipRga *rga, unsigned char *ptr);

    /* Set destination userspace data pointer */
    void (*setDstBufferPtr)(struct _RockchipRga *rga, unsigned char *ptr);

//    /* Allocate dma buffer with drm interface */
//    RgaBuffer *(*allocDmaBuffer)(struct _RockchipRga *rga,
//		              __u32 v4l2Format, __u32 width, __u32 height);
//    /* Free dma buffer */
//    void (*freeDmaBuffer)(struct _RockchipRga *rga, RgaBuffer *buf);

    int (*go)(struct _RockchipRga *rga);
} RgaOps;

typedef enum _RgaDeviceType {
    RGA_TYPE_NONE,
    RGA_TYPE_ANDROID,
    //RGA_TYPE_V4L2,
} RgaDeviceType;

typedef struct _RockchipRga {
    int rgaFd;
    //int drmFd;
    RgaDeviceType type;
    RgaContext ctx;
    RgaOps *ops;
    RgaBuffer srcBuf, dstBuf, buf;

    void *priv;
} RockchipRga;

extern "C" {
RockchipRga *RgaCreate(void);
void RgaDestroy(RockchipRga *rga);
}

#endif
