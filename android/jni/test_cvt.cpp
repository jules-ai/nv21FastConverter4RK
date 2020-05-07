#include "nv21FastConverter4RK.h"
#include <cstring>
#include <fstream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <thread>

#define BUFFER_WIDTH_SRC 480
#define BUFFER_HEIGHT_SRC 640
#define BUFFER_SIZE_SRC BUFFER_WIDTH_SRC*BUFFER_HEIGHT_SRC*3/2
#define BUFFER_WIDTH_DEST 480
#define BUFFER_HEIGHT_DEST 640
#define BUFFER_SIZE_DEST BUFFER_WIDTH_DEST*BUFFER_HEIGHT_DEST*3

unsigned char *srcBuffer = NULL;
unsigned char *dstBuffer = NULL;
unsigned char *interBuffer = NULL;
unsigned char *dstBufferIR = NULL;

int64_t timestamp_ms_fdg()
{
#ifndef _WIN32
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
		std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());  

	auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());  

	return tmp.count();  
#else
	return cvGetTickCount()/(cvGetTickFrequency()*1000);
#endif
}


void rga_copy() {


	long long b1,e1,b2,e2,b3,e3,b4,e4;


	b1 = timestamp_ms_fdg();

	nv21FastCvt4RK* cvt = new nv21FastCvt4RK(BUFFER_WIDTH_SRC, BUFFER_HEIGHT_SRC, 2, 2, BUFFER_WIDTH_SRC,BUFFER_HEIGHT_SRC, 2, 1);
	cvt->Convert(srcBuffer,dstBuffer, srcBuffer,dstBufferIR);
	delete cvt;

	e1= timestamp_ms_fdg();


	printf("%lld\n",(e1-b1));

}

int main() {
	srcBuffer = new unsigned char[BUFFER_SIZE_SRC];
	dstBuffer = new unsigned char[BUFFER_SIZE_DEST];
	dstBufferIR = new unsigned char[BUFFER_SIZE_DEST/3];
	//printf("dstBuffer@%p",dstBuffer);

	//randomData();
	std::ifstream yuv_reader("../data/yckj.nv21", std::ios::binary);
	if (!yuv_reader.good())
	{
		printf("yuv_reader init failed!\n");
		return -1;
	}
	yuv_reader.read((char*)srcBuffer, BUFFER_SIZE_SRC);
	yuv_reader.close();

	long long begintime = timestamp_ms_fdg();

	for (int i = 0; i < 1; i++)
	{
		rga_copy();
	}
	long long endtime = timestamp_ms_fdg();
	printf("RGA copy time : %lld ms\n", endtime-begintime);

	cv::Mat img(BUFFER_HEIGHT_DEST,BUFFER_WIDTH_DEST,CV_8UC3,dstBuffer);
	cv::imwrite("../data/tmp_test.jpg",img);

	cv::Mat imgIR(BUFFER_HEIGHT_DEST,BUFFER_WIDTH_DEST,CV_8UC1,dstBufferIR);
	cv::imwrite("../data/tmp_test_IR.jpg",imgIR);

	delete srcBuffer;
	delete dstBuffer;
	delete dstBufferIR;
	return 0;
}