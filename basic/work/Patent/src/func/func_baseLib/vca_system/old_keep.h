
#ifndef _OLD_KEEP_H_
#define _OLD_KEEP_H_

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////
// 为了兼容老版本，使得能够编译通过

#define VCA_SYS_MAX_PATH_LEN          10       //最多10个轨迹点
//归一化点结构体
typedef struct _VCA_SYS_POINT_NORMALIZE_
{
	float x;
	float y;
}VCA_SYS_POINT_NORMALIZE;


//目标速度信息
typedef struct  _VCA_SYS_VECTOR_INFO_
{
	float vx; //X轴速度
	float vy; //Y轴速度
	float ax; //X轴加速度
	float ay; //Y轴加速度
} VCA_SYS_VECTOR_INFO;
//目标轨迹
typedef struct _VCA_SYS_PATH_
{
	unsigned int     point_num; //轨迹中轨迹点的数量 
	VCA_SYS_POINT_NORMALIZE  point[VCA_SYS_MAX_PATH_LEN];
}VCA_SYS_PATH;

typedef struct _VCA_SYS_COUNTER_
{
	unsigned int  counter1; //计数值1
	unsigned int  counter2; //计数值2
}VCA_SYS_COUNTER;

#ifdef __cplusplus
}
#endif 

#endif

