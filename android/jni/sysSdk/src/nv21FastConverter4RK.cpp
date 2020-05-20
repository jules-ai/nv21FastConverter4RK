#include "nv21FastConverter4RK.h"
#include <rockchip_rga.h>
#include <memory>
#include "LogUtil.h"

nv21FastCvt4RK::nv21FastCvt4RK(int width_color, int height_color, int angle_color, int mirror_color, int width_ir, int height_ir, int angle_ir, int mirror_ir)
{
	handle0 = NULL;
	handle1 = NULL;
	handle2 = NULL;
	handle3 = NULL;
	width_dest = 480;
	height_dest = 640;
	handle4 = new char[width_dest*height_dest*3];
	unsigned char* inter_buffer = (unsigned char*)handle4;

	handle0= RgaCreate();
	if (!handle0)
	{
		LOGI("Create rga0 for fast converter failed !\n");
		return;
	}
	//最快的方法就是第一步尽可能做更多的步骤，比如转码+缩放+旋转，镜像放在第二步
	((RockchipRga *)handle0)->ops->initCtx((RockchipRga *)handle0);
	((RockchipRga *)handle0)->ops->setSrcFormat(((RockchipRga *)handle0), V4L2_PIX_FMT_NV12, width_color, height_color);
	((RockchipRga *)handle0)->ops->setDstFormat(((RockchipRga *)handle0), V4L2_PIX_FMT_BGR24, width_dest, height_dest);
	if(angle_color!=0)
	{
		((RockchipRga *)handle0)->ops->setRotate(((RockchipRga *)handle0), RgaRotate(angle_color));
	}
	else
	{
		if (mirror_color==0)
		{
			((RockchipRga *)handle0)->ops->setRotate(((RockchipRga *)handle0), RGA_ROTATE_NONE);
		}
		else
		{
			((RockchipRga *)handle0)->ops->setRotate(((RockchipRga *)handle0), RgaRotate(mirror_color+3));
		}
	}
	((RockchipRga *)handle0)->ops->setSrcBufferPtr(((RockchipRga *)handle0), inter_buffer);
	((RockchipRga *)handle0)->ops->setDstBufferPtr(((RockchipRga *)handle0), inter_buffer);


	if ((angle_color!=0) && (mirror_color!=0))
	{
		handle1= RgaCreate();
		if (!handle1)
		{
			LOGI("Create rga1 for fast converter failed !\n");
			return;
		}
		((RockchipRga *)handle1)->ops->initCtx((RockchipRga *)handle1);
		((RockchipRga *)handle1)->ops->setSrcFormat(((RockchipRga *)handle1), V4L2_PIX_FMT_BGR24, width_dest, height_dest);
		((RockchipRga *)handle1)->ops->setDstFormat(((RockchipRga *)handle1), V4L2_PIX_FMT_BGR24, width_dest, height_dest);
		((RockchipRga *)handle1)->ops->setRotate(((RockchipRga *)handle1), RgaRotate(mirror_color+3));
		((RockchipRga *)handle1)->ops->setSrcBufferPtr(((RockchipRga *)handle1), inter_buffer);
		((RockchipRga *)handle1)->ops->setDstBufferPtr(((RockchipRga *)handle1), inter_buffer);
	}

	handle2= RgaCreate();
	if (!handle2)
	{
		LOGI("Create rga2 for fast converter failed !\n");
		return;
	}
	((RockchipRga *)handle2)->ops->initCtx((RockchipRga *)handle2);
	((RockchipRga *)handle2)->ops->setSrcFormat(((RockchipRga *)handle2), V4L2_PIX_FMT_NV12, width_ir, height_ir);
	((RockchipRga *)handle2)->ops->setDstFormat(((RockchipRga *)handle2), V4L2_PIX_FMT_NV12, width_dest, height_dest);
	if(angle_ir!=0)
	{
		((RockchipRga *)handle2)->ops->setRotate(((RockchipRga *)handle2), RgaRotate(angle_ir));
	}
	else
	{
		if (mirror_ir==0)
		{
			((RockchipRga *)handle2)->ops->setRotate(((RockchipRga *)handle2), RGA_ROTATE_NONE);
		}
		else
		{
			((RockchipRga *)handle2)->ops->setRotate(((RockchipRga *)handle2), RgaRotate(mirror_ir+3));
		}
	}
	((RockchipRga *)handle2)->ops->setSrcBufferPtr(((RockchipRga *)handle2), inter_buffer);
	((RockchipRga *)handle2)->ops->setDstBufferPtr(((RockchipRga *)handle2), inter_buffer);


	if ((angle_ir!=0) && (mirror_ir!=0))
	{
		handle3= RgaCreate();
		if (!handle3)
		{
			LOGI("Create rga3 for fast converter failed !\n");
			return;
		}
		((RockchipRga *)handle3)->ops->initCtx((RockchipRga *)handle3);
		((RockchipRga *)handle3)->ops->setSrcFormat(((RockchipRga *)handle3), V4L2_PIX_FMT_NV12, width_dest, height_dest);
		((RockchipRga *)handle3)->ops->setDstFormat(((RockchipRga *)handle3), V4L2_PIX_FMT_NV12, width_dest, height_dest);
		((RockchipRga *)handle3)->ops->setRotate(((RockchipRga *)handle3), RgaRotate(mirror_ir+3));
		((RockchipRga *)handle3)->ops->setSrcBufferPtr(((RockchipRga *)handle3), inter_buffer);
		((RockchipRga *)handle3)->ops->setDstBufferPtr(((RockchipRga *)handle3), inter_buffer);
	}

}

nv21FastCvt4RK::~nv21FastCvt4RK()
{
	if (handle0)
	{
		RgaDestroy((RockchipRga *)handle0);
	}

	if (handle1)
	{
		RgaDestroy((RockchipRga *)handle1);
	}

	if (handle2)
	{
		RgaDestroy((RockchipRga *)handle2);
	}

	if (handle3)
	{
		RgaDestroy((RockchipRga *)handle3);
	}

	if (handle4)
	{
		delete ((char*)handle4);
	}
}

int nv21FastCvt4RK::Convert(unsigned char * src_color, unsigned char * dest_color, unsigned char * src_ir, unsigned char * dest_ir)
{
	if ((src_color!=NULL) && (dest_color!=NULL))
	{
		if (handle1)
		{
			((RockchipRga *)handle0)->ops->setSrcBufferPtr((RockchipRga *)handle0, src_color);
			((RockchipRga *)handle1)->ops->setDstBufferPtr((RockchipRga *)handle1, dest_color);
			((RockchipRga *)handle0)->ops->go(((RockchipRga *)handle0));
			((RockchipRga *)handle1)->ops->go(((RockchipRga *)handle1));
			memset(handle4,0,width_dest*height_dest*3);
		}
		else
		{
			((RockchipRga *)handle0)->ops->setSrcBufferPtr((RockchipRga *)handle0, src_color);
			((RockchipRga *)handle0)->ops->setDstBufferPtr((RockchipRga *)handle0, dest_color);
			((RockchipRga *)handle0)->ops->go(((RockchipRga *)handle0));
		}
	}


	if ((src_ir!=NULL) && (dest_ir!=NULL))
	{
		if (handle3)
		{
			((RockchipRga *)handle2)->ops->setSrcBufferPtr((RockchipRga *)handle2, src_ir);
			((RockchipRga *)handle3)->ops->setDstBufferPtr((RockchipRga *)handle3, dest_ir);
			((RockchipRga *)handle2)->ops->go(((RockchipRga *)handle2));
			((RockchipRga *)handle3)->ops->go(((RockchipRga *)handle3));
			memset(handle4,0,width_dest*height_dest*3);
		}
		else
		{
			((RockchipRga *)handle2)->ops->setSrcBufferPtr((RockchipRga *)handle2, src_ir);
			((RockchipRga *)handle2)->ops->setDstBufferPtr((RockchipRga *)handle2, dest_ir);
			((RockchipRga *)handle2)->ops->go(((RockchipRga *)handle2));
		}
	}
}
