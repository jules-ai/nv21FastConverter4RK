#ifndef _NV21_FAST_CONVERTER_FOR_RK_HEADER_
#define _NV21_FAST_CONVERTER_FOR_RK_HEADER_


class nv21FastCvt4RK{
public:
	nv21FastCvt4RK(int width_color, int height_color, int angle_color, int mirror_color, int width_ir, int height_ir, int angle_ir, int mirror_ir);
	~nv21FastCvt4RK();

	int Convert(unsigned char * src_color, unsigned char * dest_color, unsigned char * src_ir, unsigned char * dest_ir);
private:
	void *handle0;
	void *handle1;
	void *handle2;
	void *handle3;
	void *handle4;
	int width_dest;
	int height_dest;
};
#endif
