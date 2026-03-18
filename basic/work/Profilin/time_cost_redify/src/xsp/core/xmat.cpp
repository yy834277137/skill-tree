#include "xmat.hpp"

XMat::XMat()
	: type(XSP_8U), data{ LIBXRAY_NULL },
	hei(0), wid(0), tpad(0), bpad(0), size(0)
{}

XRAY_LIB_HRESULT XMat::Init(int32_t hei, int32_t wid, int32_t type, uint8_t *ptr)
{
	XSP_CHECK( ptr, XRAY_LIB_NULLPTR );
	XSP_CHECK( hei > 0 || wid > 0, 
		       XRAY_LIB_XMAT_SIZE_ERR );
	XSP_CHECK( (unsigned)XSP_MAT_DEPTH(type) <= XSP_64F, 
		       XRAY_LIB_XMAT_TYPE_ERR );

	this->data.ptr = ptr;
	this->type = XSP_MAT_TYPE(type);
	this->hei = hei;
	this->wid = wid;
	this->tpad = 0;
	this->bpad = 0;
	this->step = this->wid * XSP_ELEM_SIZE(type);
	this->size = hei * wid * XSP_ELEM_SIZE(this->type);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XMat::Init(int32_t hei, int32_t wid, int32_t pad, int32_t type, uint8_t* ptr)
{
	XSP_CHECK( ptr, XRAY_LIB_NULLPTR );
	XSP_CHECK( hei > 0 || wid > 0 || pad >= 0, 
		       XRAY_LIB_XMAT_SIZE_ERR );
	XSP_CHECK(pad * 2 < hei, XRAY_LIB_XMAT_SIZE_ERR);
	XSP_CHECK( (unsigned)XSP_MAT_DEPTH(type) <= XSP_64F, 
		       XRAY_LIB_XMAT_TYPE_ERR );

	this->data.ptr = ptr;
	this->type = XSP_MAT_TYPE(type);
	this->hei = hei;
	this->wid = wid;
	this->tpad = pad;
	this->bpad = pad;
	this->step = this->wid * XSP_ELEM_SIZE(type);
	this->size = hei * wid * XSP_ELEM_SIZE(this->type);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XMat::Init(int32_t hei, int32_t wid, int32_t tpad, int32_t bpad, int32_t type, uint8_t* ptr)
{
	XSP_CHECK( ptr, XRAY_LIB_NULLPTR );
	XSP_CHECK( hei > 0 || wid > 0 || tpad >= 0 || bpad >= 0, 
		       XRAY_LIB_XMAT_SIZE_ERR );
	XSP_CHECK( (unsigned)XSP_MAT_DEPTH(type) <= XSP_64F, 
		       XRAY_LIB_XMAT_TYPE_ERR );

	this->data.ptr = ptr;
	this->type = XSP_MAT_TYPE(type);
	this->hei = hei;
	this->wid = wid;
	this->tpad = tpad;
	this->bpad = bpad;
	this->step = this->wid * XSP_ELEM_SIZE(type);
	this->size = hei * wid * XSP_ELEM_SIZE(this->type);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XMat::InitNoCheck(int32_t hei, int32_t wid, int32_t type, uint8_t *ptr)
{
	XSP_CHECK(hei > 0 || wid > 0,
		XRAY_LIB_XMAT_SIZE_ERR);
	XSP_CHECK((unsigned)XSP_MAT_DEPTH(type) <= XSP_64F,
		XRAY_LIB_XMAT_TYPE_ERR);

	this->data.ptr = ptr;
	this->type = XSP_MAT_TYPE(type);
	this->hei = hei;
	this->wid = wid;
	this->step = this->wid * XSP_ELEM_SIZE(type);
	this->size = hei * wid * XSP_ELEM_SIZE(this->type);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XMat::Reshape(int32_t hei, int32_t wid, int32_t type)
{
	XSP_ASSERT(data.ptr);
	XSP_ASSERT((unsigned)XSP_MAT_DEPTH(type) <= XSP_64F);
	XSP_CHECK(0 < hei && 0 < wid, XRAY_LIB_XMAT_SIZE_MINUS, "0 < %d && 0 < %d", hei, wid);
	XSP_CHECK(XSP_ELEM_SIZE(type) * hei * wid <= size,
		      XRAY_LIB_XMAT_MEM_OVERFLOW, "%d * %d * %d <= %ld", XSP_ELEM_SIZE(type), hei, wid, size);
	this->hei = hei;
	this->wid = wid;
	this->tpad = 0;
	this->bpad = 0;
	this->type = XSP_MAT_TYPE(type);
	this->step = this->wid * XSP_ELEM_SIZE(type);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XMat::Reshape(int32_t hei, int32_t wid, int32_t tpad, int32_t bpad, int32_t type)
{
	XSP_ASSERT(data.ptr);
	XSP_ASSERT((unsigned)XSP_MAT_DEPTH(type) <= XSP_64F);
	XSP_CHECK(0 < hei && 0 < wid && 0 <= tpad && 0 <= bpad, 
		      XRAY_LIB_XMAT_SIZE_MINUS, "0 < %d && 0 <= %d && 0 <= %d", hei, wid, tpad, bpad);
	XSP_CHECK(XSP_ELEM_SIZE(type) * hei * wid <= size,
		      XRAY_LIB_XMAT_MEM_OVERFLOW, "%d * %d * %d <= %ld", XSP_ELEM_SIZE(type), hei, wid, size);
	this->hei = hei;
	this->wid = wid;
	this->tpad = tpad;
	this->bpad = bpad;
	this->type = XSP_MAT_TYPE(type);
	step = this->wid * XSP_ELEM_SIZE(type);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XMat::Reshape(int32_t hei, int32_t wid, int32_t pad, int32_t type)
{
	XSP_ASSERT(data.ptr);
	XSP_ASSERT((unsigned)XSP_MAT_DEPTH(type) <= XSP_64F);
	XSP_CHECK(pad * 2 < hei, XRAY_LIB_XMAT_SIZE_ERR);
	XSP_CHECK(0 < hei && 0 < wid && 0 <= pad, 
		      XRAY_LIB_XMAT_SIZE_MINUS);
	XSP_CHECK(XSP_ELEM_SIZE(type) * hei * wid <= size,
		      XRAY_LIB_XMAT_MEM_OVERFLOW, "%d * %d * %d <= %ld", XSP_ELEM_SIZE(type), hei, wid, size);
	this->hei = hei;
	this->wid = wid;
	this->tpad = pad;
	this->bpad = pad;
	this->type = XSP_MAT_TYPE(type);
	step = this->wid * XSP_ELEM_SIZE(type);
	return XRAY_LIB_OK;
}

int32_t WriteXMat(const char* filename, XMat& mat)
{
	XSP_CHECK(filename, -1);
	XSP_CHECK(mat.IsValid(), -2);

	FILE* file = nullptr;
	file = fopen(filename, "wb");

	int32_t nHeight = mat.hei;
	int32_t nWidth = mat.wid;
	fwrite(mat.data.ptr, XSP_ELEM_SIZE(mat.type), nHeight * nWidth, file);
	fclose(file);
	return 1;
}
