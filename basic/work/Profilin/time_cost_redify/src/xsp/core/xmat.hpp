#ifndef _XMAT_HPP_
#define _XMAT_HPP_

#include "libXRay_def.h"
#include "xsp_check.hpp"
#include "xsp_def.hpp"

/***************************************************************************************************
*                                        矩阵结构体定义
***************************************************************************************************/
/*
* @brief XMat
* @note XSP内部自定义矩阵结构体, 内存按wid方向连续存储
* @note 支持像素类型 见xsp_def.hpp
*       单通道 XSP_8U ~ XSP_64F 
*       多通道 XSP_8UC1 ~ XSP_64FC4
*/
struct XMat
{
	int32_t type;    /* 像素类型 XSP_8UC1 ~ XSP_64FC4 */
	int32_t step;    /* 矩阵行间距 字节单位 */

	union
	{
		uint8_t* ptr;
		uint16_t* us;
		uint32_t* ui;
		float32_t* fl;
		float64_t* db;
	} data;      /* 数据指针 */

	int32_t hei;     /* 高度 包括两端扩边 */
	int32_t wid;     /* 宽度 连续内存方向 */
	int32_t tpad;    /* 高度方向上部top的扩边高度 */
	int32_t bpad;    /* 高度方向下部bottom的扩边高度 */
	size_t size; 	 /* 矩阵最大内存 字节单位 */

	XMat();

	/**@fn       Init
	* @brief     通过数据指针初始化
	* @param1    hei              [i]     - 高度
	* @param2    wid              [i]     - 宽度
	* @param3    type             [i]     - 数据类型 XSP_8UC1 ~ XSP_64FC4
	* @param4    ptr              [i]     - 数据指针
	* @return    错误码
	*/
	XRAY_LIB_HRESULT Init(int32_t hei, int32_t wid, int32_t type, uint8_t* ptr);

	/// 上下扩边高度相等，均为pad
	XRAY_LIB_HRESULT Init(int32_t hei, int32_t wid, int32_t pad, int32_t type, uint8_t* ptr);

	/// 上扩边高度为tpad，下扩边高度为bpad
	XRAY_LIB_HRESULT Init(int32_t hei, int32_t wid, int32_t tpad, int32_t bpad, int32_t type, uint8_t* ptr);

	/**@fn       InitNoCheck
	* @brief     通过数据指针初始化
	* @param1    hei              [i]     - 高度
	* @param2    wid              [i]     - 宽度
	* @param3    type             [i]     - 数据类型 XSP_8UC1 ~ XSP_64FC4
	* @param4    ptr              [i]     - 数据指针
	* @return    错误码
	* @note      该接口不判断指针的有效性，便于外部赋值
	*/
	XRAY_LIB_HRESULT InitNoCheck(int32_t hei, int32_t wid, int32_t type, uint8_t* ptr);

	/**@fn       SetMem
	* @brief     设置矩阵最大内存
	* @param1    size             [i]     - 内存像素数
	* @return    void
	*
	* @note      XMat内部不做内存申请，该函数只设置矩阵支持的最大内存，
	*            一般配合XSP_ELEM_SIZE宏使用；
	* @note      该函数只能在XMat内部指针无效时设置,内部指针初始化后，
	*            debug模式下二次设置会assert报错, 
	*/
	void SetMem(size_t size);

	/**@fn       SetValue
	* @brief     设置矩阵每个字节的数值，默认为0XFF
	*
	* @note     将Mat的值全部置为最大值
	*/
	void SetValue(uint8_t value);

	/**@fn       Size
	* @brief     返回内存字节大小
	* @return    size_t
	*
	* @note      XMat的内存大于等于 hei * wid * XSP_ELEM_SIZE(type)，一般在
	*            初始化阶段获取所需的最大内存，处理阶段不再进行内存的额外申请
	*/
	size_t Size();

	/**@fn       Depth
	* @brief     返回XMat数据类型depth（XSP_8U~XSP_64F）
	* @return    int32_t
	*
	* @note      
	*/
	int32_t Depth();

	/**@fn       Channel
	* @brief     返回XMat数据类型通道数
	* @return    int32_t
	*
	* @note
	*/
	int32_t Channel();

	/**@fn       Reshape
	* @brief     设置矩阵形状
	* @param1    hei              [i]     - 矩阵高
	* @param2    wid              [i]     - 矩阵宽
	* @param3    type             [i]     - 数据类型 XSP_8UC1 ~ XSP_64FC4
	* @return    bool
	*/
	XRAY_LIB_HRESULT Reshape(int32_t hei, int32_t wid, int32_t type);

	/**@fn       Reshape
	* @brief     设置矩阵形状
	* @param1    hei              [i]     - 矩阵高
	* @param2    wid              [i]     - 矩阵宽
	* @param3    tpad             [i]     - 矩阵上部高度扩边
	* @param4    bpad             [i]     - 矩阵下部高度扩边
	* @param5    type             [i]     - 数据类型 XSP_8UC1 ~ XSP_64FC4
	* @return    bool
	*/
	XRAY_LIB_HRESULT Reshape(int32_t hei, int32_t wid, int32_t tpad, int32_t bpad, int32_t type);

	/**@fn       Reshape
	* @brief     设置矩阵形状
	* @param1    hei              [i]     - 矩阵高
	* @param2    wid              [i]     - 矩阵宽
	* @param3    pad              [i]     - 矩阵高度扩边,tpad与bpad相同
	* @param4    type             [i]     - 数据类型 XSP_8UC1 ~ XSP_64FC4
	* @return    bool
	*/
	XRAY_LIB_HRESULT Reshape(int32_t hei, int32_t wid, int32_t pad, int32_t type);


	/**@fn       PadPtr
	* @brief     返回跳过扩边内存的内存首地址
	* @return    uint8_t*
	*/
	uint8_t* PadPtr();

	/**@fn       Ptr
	* @brief     返回hei行头指针
	* @param1    hei              [i]     - 矩阵hei
	* @return    uint8_t*
	*/
	uint8_t* Ptr(int32_t hei = 0);

	/**@fn       Ptr
	* @brief     返回hei/wid坐标下的数据地址
	* @param1    hei              [i]     - 矩阵hei
	* @param2    wid              [i]     - 矩阵wid
	* @return    uint8_t*
	*/
	uint8_t* Ptr(int32_t hei, int32_t wid);

	/**@fn       PadPtr
	* @brief     返回跳过扩边内存的内存首地址
	* @return    Dtype*
	*/
	template<typename Dtype> Dtype* PadPtr();

	/**@fn       Ptr
	* @brief     返回hei行头指针
	* @param1    hei              [i]     - 矩阵hei
	* @return    Dtype*
	*/
	template<typename Dtype> Dtype* Ptr(int32_t hei = 0);

	/**@fn       Ptr
	* @brief     返回hei/wid坐标下的数据地址
	* @param1    hei              [i]     - 矩阵hei
	* @param2    wid              [i]     - 矩阵wid
	* @return    Dtype*
	*/
	template<typename Dtype> Dtype* Ptr(int32_t hei, int32_t wid);

	/**@fn       NeighborPtr
	* @brief     返回hei/wid坐标下的数据地址, hei和wid可为负或超过宽高, 输出镜像邻域数据
	* @param1    hei              [i]     - 矩阵hei
	* @param2    wid              [i]     - 矩阵wid
	* @return    Dtype*
	*/
	template<typename Dtype> Dtype* NeighborPtr(int32_t hei, int32_t wid);

	// @brief 判断XMat是否有效(wid > 0 && hei > 0 && data.ptr)
	bool IsValid();

	// 计算指定框中的像素均值
	template<typename Dtype> Dtype Mean(int32_t x, int32_t y, int32_t w, int32_t h);

}; // struct XMat


// @brief 判断XMat类型type是否相等
bool MatTypeEq(XMat &mat1, XMat &mat2);

// @brief 判断XMat尺寸是否相等
bool MatSizeEq(XMat &mat1, XMat &mat2);

// @brief 存储XMat数据
int32_t WriteXMat(const char* filename, XMat& mat);

// @brief 交换XMat数据
void SwitchMat(XMat& mat1, XMat& mat2);

#include "xmat.inl.hpp"


#endif // _XMAT_HPP_
