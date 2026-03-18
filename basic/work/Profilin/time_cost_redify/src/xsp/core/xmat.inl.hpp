#ifndef _XMAT_OPERATIONS_HPP_
#define _XMAT_OPERATIONS_HPP_

inline
void XMat::SetMem(size_t size)
{
	XSP_ASSERT(size > 0);
	XSP_ASSERT(LIBXRAY_NULL == data.ptr);
	this->size = size;
}

inline
void XMat::SetValue(uint8_t value)
{
	memset(this->data.ptr, value, this->size);
	return;
}

inline
size_t XMat::Size()
{ 
	return size; 
}

inline
int32_t XMat::Depth()
{
	return XSP_MAT_DEPTH(type);
}

inline
int32_t XMat::Channel()
{
	return XSP_MAT_CN(type);
}

inline 
uint8_t* XMat::PadPtr() 
{ 
	XSP_ASSERT(data.ptr);
	XSP_ASSERT(tpad + bpad < hei);
	return data.ptr + tpad * step; 
}

inline 
uint8_t* XMat::Ptr(int32_t hei)
{ 
	XSP_ASSERT(data.ptr);
	XSP_ASSERT((unsigned)hei < (unsigned)this->hei);
	return data.ptr + hei * step; 
}

inline
uint8_t* XMat::Ptr(int32_t hei, int32_t wid)
{
	XSP_ASSERT(data.ptr);
	XSP_ASSERT((unsigned)hei < (unsigned)this->hei &&
		(unsigned)wid < (unsigned)this->wid);
	return data.ptr + hei * step + wid * XSP_ELEM_SIZE(this->type);
}

template<typename Dtype> inline
Dtype* XMat::PadPtr()
{
	XSP_ASSERT(data.ptr);
	XSP_ASSERT(tpad + bpad < hei);
	return (Dtype*)(data.ptr + tpad * step);
}

template<typename Dtype> inline
Dtype* XMat::Ptr(int32_t hei)
{
	XSP_ASSERT(data.ptr);
	XSP_ASSERT((unsigned)hei < (unsigned)this->hei);
	return (Dtype*)(data.ptr + hei * step);
}

template<typename Dtype> inline
Dtype* XMat::Ptr(int32_t hei, int32_t wid)
{
	XSP_ASSERT(data.ptr);
	XSP_ASSERT((unsigned)hei < (unsigned)this->hei &&
		(unsigned)wid < (unsigned)this->wid);
	return (Dtype*)(data.ptr + hei * step + wid * XSP_ELEM_SIZE(this->type));
}

template<typename Dtype> inline
Dtype* XMat::NeighborPtr(int32_t hei, int32_t wid)
{
    XSP_ASSERT(data.ptr);
    
    auto clamp_reflect = [](int32_t coord, int32_t max) -> uint32_t {
		if (coord < 0) return (-coord >= max) ? max - 1 : (-coord - 1);
		if (coord >= max) return (coord >= 2 * max) ? 0 : (2 * max - coord - 1);
        return coord;
    };
    
    uint32_t u32HeiLoc = clamp_reflect(hei, this->hei);
    uint32_t u32WidLoc = clamp_reflect(wid, this->wid);
    
    return (Dtype*)(data.ptr + u32HeiLoc * step + u32WidLoc * XSP_ELEM_SIZE(this->type));
}

// 计算当前XMat 在指定矩形框中的均值
template<typename Dtype>
Dtype XMat::Mean(int32_t x, int32_t y, int32_t w, int32_t h)
{
    float64_t sum = 0;
    int32_t count = 0;
    for (int32_t hei = y; hei < y + h; hei++) 
	{
        for (int32_t wid = x; wid < x + w; wid++) 
		{
            sum += Ptr<Dtype>(hei, wid)[0];
            count++;
        }
    }
    return Dtype(sum / count);
}

inline
bool XMat::IsValid()
{
	return (hei > 0 && wid > 0 && data.ptr);
}

inline 
bool MatTypeEq(XMat &mat1, XMat &mat2)
{
	return (((mat1.type ^ mat2.type) & XSP_MAT_TYPE_MASK) == 0);
}

inline 
bool MatSizeEq(XMat &mat1, XMat &mat2)
{
	return (mat1.hei == mat2.hei && 
		    mat1.wid == mat2.wid && 
		    mat1.tpad == mat2.tpad &&
			mat1.bpad == mat2.bpad);
}

inline
void SwitchMat(XMat& mat1, XMat& mat2)
{
	XMat temp = mat1;
	mat1 = mat2;
	mat2 = temp;
	return;
}


#endif // _XMAT_OPERATIONS_HPP_