LOCAL_PATH := $(call my-dir)

#清除变量
include $(CLEAR_VARS)

#调试语句，调试脚本用
$(info "current path :"$(LOCAL_PATH)) 

#opencv库
OPENCVROOT :=$(LOCAL_PATH)/../opencv-3100
OPENCV_CAMERA_MODULES := on
OPENCV_INSTALL_MODULES := on
#OPENCV_LIB_TYPE:=SHARED
OPENCV_LIB_TYPE:=STATIC
include $(OPENCVROOT)/sdk/native/jni/OpenCV.mk  #源码列表

#头文件路径
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sysSdk/include


#生成的目标文件名
LOCAL_MODULE := nv21FastConverter4RK
#源码列表
SRC_SUFFIX := *.cpp *.c 
SRC_ROOT := $(LOCAL_PATH)/sysSdk
#通配符递归遍历
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
SRC_FILES := $(call rwildcard,$(SRC_ROOT)/,$(SRC_SUFFIX))
LOCAL_SRC_FILES := $(SRC_FILES:$(LOCAL_PATH)/%=%)
$(info "src files :" $(LOCAL_SRC_FILES))
				
LOCAL_LDFLAGS:=#传递给链接器的选项
LOCAL_LDLIBS += -lm -llog -landroid -lz -ldl


#编译动态库
ifeq ($(PRODUCT_TYPE),SO)
	include $(BUILD_SHARED_LIBRARY)
endif

#编译静态库
ifeq ($(PRODUCT_TYPE),A)
	include $(BUILD_STATIC_LIBRARY)
endif

#编译可执行文件
ifeq ($(PRODUCT_TYPE),EXE)
	include $(BUILD_EXECUTABLE)
endif
