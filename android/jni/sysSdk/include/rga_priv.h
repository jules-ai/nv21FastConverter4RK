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

#ifndef __RGA_PRIV_H__
#define __RGA_PRIV_H__

#include "rockchip_rga.h"

#define MAX_YUV_SIZE		(16 * 1024 * 1024)    // 16M

extern "C" {
    int AndroidRgaProcess(RockchipRga *rga, RgaBuffer *srcBuf, RgaBuffer *dstBuf);
}
//static int V4l2RgaProcess(RockchipRga *rga, RgaBuffer *srcBuf, RgaBuffer *dstBuf);

#endif
