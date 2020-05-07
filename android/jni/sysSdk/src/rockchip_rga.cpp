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

#include <linux/stddef.h>
#include <linux/videodev2.h>
#include <errno.h>
#include <time.h>

#include "rockchip_rga.h"
//#include "log.h"
#include "rga_priv.h"

#define SYS_PATH		"/sys/class/video4linux/"
#define DEV_PATH		"/dev/"
//#define V4L2_RGA_NAME		"rockchip-rga"
#define ANDROID_RGA_DEV		"/dev/rga"
//#define DRM_DEV			"/dev/dri/card0"

enum drm_rockchip_gem_mem_type {
	ROCKCHIP_BO_CONTIG	= 1 << 0,
	ROCKCHIP_BO_CACHABLE	= 1 << 1,
	ROCKCHIP_BO_WC		= 1 << 2,
	ROCKCHIP_BO_SECURE	= 1 << 3,
	ROCKCHIP_BO_MASK	= ROCKCHIP_BO_CONTIG | ROCKCHIP_BO_CACHABLE | ROCKCHIP_BO_WC | ROCKCHIP_BO_SECURE,
};

static size_t
RockchipRgaGetSize(__u32 v4l2Format, __u32 width, __u32 height)
{
    switch(v4l2Format) {
    case V4L2_PIX_FMT_NV12: // YUV420SP
    case V4L2_PIX_FMT_YUV420:
        return RGA_ALIGN(width, 16) * RGA_ALIGN(height, 16) * 3 / 2;
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_NV16:
        return RGA_ALIGN(width, 16) * RGA_ALIGN(height, 16) * 2;
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
        return RGA_ALIGN(width, 16) * RGA_ALIGN(height, 16) * 3;
    case V4L2_PIX_FMT_ABGR32: // BGRA8888
    case V4L2_PIX_FMT_ARGB32: // ARGB8888
    case V4L2_PIX_FMT_XRGB32: // XRGB8888
    case V4L2_PIX_FMT_XBGR32: // XBGR8888
        return RGA_ALIGN(width, 16) * RGA_ALIGN(height, 16) * 4;
    default:
	assert(0);
	break;
    }

    return 0;
}

static int
RgaOpenByName(const char *name) {
    DIR *dir;
    struct dirent *ent;
    int ret = -1;

    if((dir = opendir(SYS_PATH)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            FILE *fp;
            char path[512];
            char dev_name[512];
            
            snprintf(path, 512, SYS_PATH"%s/name",ent->d_name);
            fp = fopen(path, "r");
            if(!fp)
                continue;
            if(!fgets(dev_name, 32, fp))
                dev_name[0] = '\0';
            fclose(fp);

            if (!strstr(dev_name, name))
                continue;

            snprintf(path, sizeof(path), DEV_PATH"%s", ent->d_name);
            ret = open(path, O_RDWR);
            break;
        }
        closedir(dir);
    }

    return ret;
}

static void
RockchipRgaInitCtx(RockchipRga *rga)
{
    memset(&rga->ctx, 0, sizeof(rga->ctx));
}

static void
RockchipRgaSetRotate(RockchipRga *rga, RgaRotate rotate)
{
    rga->ctx.rotate = rotate;
    rga_debug(rga, "SetRotate: rotate %d\n", rotate);
}

static void
RockchipRgaSetFillColor(RockchipRga *rga, int color)
{
    rga->ctx.color = color;
    rga_debug(rga, "SetFillColor: color %d\n", color);
}

static void
RockchipRgaSetSrcFormat(RockchipRga *rga, __u32 v4l2Format,
		        __u32 width, __u32 height)
{
    assert(RockchipRgaCheckFormat(v4l2Format));

    rga->ctx.srcFormat = v4l2Format;
    rga->ctx.srcWidth = width;
    rga->ctx.srcHeight = height;
    rga->srcBuf.size = RockchipRgaGetSize(v4l2Format, width, height);

    rga_debug(rga, "SetSrcFormat: format %u, width %u, height %u\n",
             v4l2Format, width, height);
}


static void
RockchipRgaSetDstFormat(RockchipRga *rga, __u32 v4l2Format,
		        __u32 width, __u32 height)
{
    assert(RockchipRgaCheckFormat(v4l2Format));

    rga->ctx.dstFormat = v4l2Format;
    rga->ctx.dstWidth = width;
    rga->ctx.dstHeight = height;
    rga->dstBuf.size = RockchipRgaGetSize(v4l2Format, width, height);

    rga_debug(rga, "SetDstFormat: format %u, width %u, height %u\n",
             v4l2Format, width, height);
}

static void
RockchipRgaSetSrcCrop(struct _RockchipRga *rga, __u32 cropX, __u32 cropY,
                   __u32 cropW, __u32 cropH)
{
    assert(cropW > 0 && cropH > 0);

    rga->ctx.srcCropX = cropX;
    rga->ctx.srcCropY = cropY;
    rga->ctx.srcCropW = cropW;
    rga->ctx.srcCropH = cropH;

    rga_debug(rga, "SetSrcCrop: cropX %u, cropY %u, cropW %u, cropH %u\n",
		    cropX, cropY, cropW, cropH);
}

static void
RockchipRgaSetDstCrop(struct _RockchipRga *rga, __u32 cropX, __u32 cropY,
                   __u32 cropW, __u32 cropH)
{
    assert(cropW > 0 && cropH > 0);

    rga->ctx.dstCropX = cropX;
    rga->ctx.dstCropY = cropY;
    rga->ctx.dstCropW = cropW;
    rga->ctx.dstCropH = cropH;

    rga_debug(rga, "SetDstCrop: cropX %u, cropY %u, cropW %u, cropH %u\n",
		    cropX, cropY, cropW, cropH);
}

static void
RockchipRgaSetSrcBufferFd(struct _RockchipRga *rga, int dmaFd)
{
    rga->srcBuf.fd = dmaFd;
}
static void
RockchipRgaSetDstBufferFd(struct _RockchipRga *rga, int dmaFd)
{
    rga->dstBuf.fd = dmaFd;
}
static void
RockchipRgaSetSrcBufferPtr(struct _RockchipRga *rga, unsigned char *ptr)
{
    rga->srcBuf.fd = -1;
    rga->srcBuf.ptr = ptr;
}
static void
RockchipRgaSetDstBufferPtr(struct _RockchipRga *rga, unsigned char *ptr)
{
    rga->dstBuf.fd = -1;
    rga->dstBuf.ptr = ptr;
}

//static RgaBuffer *
//RockchipRgaAllocDmaBuffer(RockchipRga *rga, __u32 v4l2Format,
//		      __u32 width, __u32 height)
//{
//    struct drm_mode_create_dumb cd;
//    struct drm_mode_map_dumb md;
//    struct drm_mode_destroy_dumb dd;
//    __u32 bpp;
//    __u32 vHeight;
//    RgaBuffer *buf;
//    int ret;
//
//    if(!RockchipRgaCheckFormat(v4l2Format)) {
//	rga_err(rga, "initBuffer: Format %u is not support\n", v4l2Format);
//	return NULL;
//    }
//
//    buf = calloc(1, sizeof(*buf));
//    if(!buf) {
//        rga_err(rga, "No memory for rga buffer\n");
//	return NULL;
//    }
//
//    switch(v4l2Format) {
//    case V4L2_PIX_FMT_NV12: // YUV420SP
//    case V4L2_PIX_FMT_YUV420:
//	bpp = 8;
//	vHeight = RGA_ALIGN(height, 16) * 3 / 2;
//        break;
//    case V4L2_PIX_FMT_NV16:
//    case V4L2_PIX_FMT_YUYV:
//	bpp = 8;
//	vHeight = RGA_ALIGN(height, 16) * 2;
//        break;
//    case V4L2_PIX_FMT_RGB565:
//	bpp = 16;
//	vHeight = RGA_ALIGN(height, 16);
//        break;
//    case V4L2_PIX_FMT_RGB24:
//    case V4L2_PIX_FMT_BGR24:
//	bpp = 24;
//	vHeight = RGA_ALIGN(height, 16);
//        break;
//    case V4L2_PIX_FMT_ABGR32: // BGRA8888
//    case V4L2_PIX_FMT_ARGB32: // ARGB8888
//    case V4L2_PIX_FMT_XRGB32: // XRGB8888
//	bpp = 32;
//	vHeight = RGA_ALIGN(height, 16);
//        break;
//    default:
//	assert(0);
//	break;
//    }
//
//    memset(&cd, 0, sizeof(cd));
//    cd.bpp = bpp;
//    cd.width = RGA_ALIGN(width, 16);
//    cd.height = vHeight;
//    //cd.flags = ROCKCHIP_BO_CACHABLE;
//    ret = ioctl(rga->drmFd, DRM_IOCTL_MODE_CREATE_DUMB, &cd);
//    if(ret < 0) {
//        rga_err(rga, "Allocate drm buffer failed, return %d\n", ret);
//	goto free_buffer;
//    }
//
//    buf->handle = cd.handle;
//    buf->size = cd.size;
//
//    memset(&md, 0, sizeof(md));
//    md.handle = buf->handle;
//    ret = ioctl(rga->drmFd, DRM_IOCTL_MODE_MAP_DUMB, &md);
//    if(ret < 0) {
//        rga_err(rga, "Map drm buffer failed, return %d\n", ret);
//	goto destory_drm;
//    }
//
//    ret = drmPrimeHandleToFD(rga->drmFd, buf->handle, 0, &buf->fd);
//    if(ret < 0) {
//        rga_err(rga, "Get drm bufferfd failed, return %d\n", ret);
//	goto destory_drm;
//    }
//
//    buf->ptr = mmap(NULL, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
//                     rga->drmFd, md.offset);
//
//    if(buf->ptr == MAP_FAILED) {
//        rga_err(rga, "Mmap drm buffer failed\n");
//        ret = -errno;
//	goto close_fd;
//    }
//
//    buf->isExtBuf = 0;
//
//    return buf;
//
//close_fd:
//    close(buf->fd);
//destory_drm:
//    memset(&dd, 0, sizeof(dd));
//    dd.handle = buf->handle;
//    ioctl(rga->drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dd);
//free_buffer:
//    free(buf);
//
//    return NULL;
//}
//
//static void
//RockchipRgaFreeDmaBuffer(RockchipRga *rga, RgaBuffer *buf)
//{
//    struct drm_mode_destroy_dumb dd;
//
//    close(buf->fd);
//    memset(&dd, 0, sizeof(dd));
//    dd.handle = buf->handle;
//    ioctl(rga->drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dd);
//    free(buf);
//}

static int
RockchipRgaGo(RockchipRga *rga)
{
    int ret = -EINVAL;
    switch(rga->type) {
    case RGA_TYPE_ANDROID:
        ret = AndroidRgaProcess(rga, &rga->srcBuf, &rga->dstBuf);
        break;
//    case RGA_TYPE_V4L2:
//        ret = V4l2RgaProcess(rga, &rga->srcBuf, &rga->dstBuf);
//        break;
    default:
	assert(0);
        break;
    }

    return ret;
}

static RgaOps rgaOps = {
    .initCtx =         RockchipRgaInitCtx,
    .setRotate =       RockchipRgaSetRotate,
    .setFillColor =    RockchipRgaSetFillColor,
    .setSrcFormat =    RockchipRgaSetSrcFormat,
    .setDstFormat =    RockchipRgaSetDstFormat,
    .setSrcCrop =      RockchipRgaSetSrcCrop,
    .setDstCrop =      RockchipRgaSetDstCrop,
    .setSrcBufferFd =  RockchipRgaSetSrcBufferFd,
    .setDstBufferFd =  RockchipRgaSetDstBufferFd,
    .setSrcBufferPtr = RockchipRgaSetSrcBufferPtr,
    .setDstBufferPtr = RockchipRgaSetDstBufferPtr,
    //.allocDmaBuffer =  RockchipRgaAllocDmaBuffer,
    //.freeDmaBuffer =   RockchipRgaFreeDmaBuffer,

    .go = RockchipRgaGo,
};

RockchipRga *RgaCreate(void)
{
    RockchipRga *rga;
 
    rga = (RockchipRga *)calloc(1, sizeof(*rga));
    if(!rga) {
        rga_err(rga, "No memory for rockchip rga\n");
        return NULL;
    }

    rga->buf.fd = -1;
    rga->buf.ptr = (unsigned char*)malloc(MAX_YUV_SIZE);
    if(!rga->buf.ptr) {
        rga_err(rga, "No memory for rockchip rga buffer\n");
        free(rga);
        return NULL;
    }

    rga->ops = &rgaOps;

//    rga->drmFd = open(DRM_DEV, O_RDWR, 0);
//    if(rga->drmFd < 0) {
//        rga_err(rga, "Open drm device failed\n");
//	goto free_rga;
//    }
    
    rga->rgaFd = open(ANDROID_RGA_DEV, O_RDWR, 0);
    if(rga->rgaFd >= 0) {
	    rga->type = RGA_TYPE_ANDROID;
        return rga;
    }
    rga_err(rga, "open rga failed ! errno=%d", errno);
    abort();

close_drm:
    //close(rga->drmFd);
free_rga:
    free(rga->buf.ptr);
    free(rga);
    return NULL;
}

void RgaDestroy(RockchipRga *rga)
{
    //close(rga->drmFd);
    close(rga->rgaFd);
    free(rga->buf.ptr);
    free(rga);
}

