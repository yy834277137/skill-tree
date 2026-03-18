#include "xsp_dump.hpp"
#include <string>

#define MAX_IDX 10

static int32_t g_nIdx[MAX_IDX] = {0};

struct XSP_FILE_HEADER   //共40个字节
{
	uint16_t version;
	uint16_t width;
	uint16_t height;
	uint16_t pixelType;
	uint16_t nEnergyMode; //0,双能；2，单高；3，单低
	uint16_t reverse[15];
};

// 	WriteRaw("./TEST", "dump", 0, matAiEdgemask);
int32_t WriteRaw(const char* filepath, const char* name, int32_t idx, 
	XMat& raw)
{
	XSP_CHECK(filepath && name, XSP_NULLPTR, "filepath or name str is null.");

	XSP_CHECK(raw.IsValid(), XSP_MAT_INVALID);

	XSP_CHECK(idx < MAX_IDX && idx >= 0, XSP_IDX_OVERFLOW);

	int32_t nHei = raw.hei;
	int32_t nWid = raw.wid;

	std::string strFilePath(filepath);
	std::string strName(name);
	std::string strSavePath = strFilePath + "/" + strName + "_" + 
						std::to_string(nHei) + "_" + 
						std::to_string(nWid) + "_idx_" + 
						std::to_string(g_nIdx[idx]) + ".raw";

	g_nIdx[idx]++;

	FILE* file = nullptr;
	file = fopen(strSavePath.c_str(), "wb");

	fwrite(raw.Ptr(), XSP_ELEM_SIZE(raw.type), nHei * nWid, file);
	fclose(file);

	return XSP_OK;
}

int32_t WriteRaw(const char* filepath, const char* name, int32_t idx, 
	         XMat& low, XMat& high, XMat& ZValue)
{
	XSP_CHECK(filepath && name, XSP_NULLPTR, "filepath or name str is null.");

	XSP_CHECK(low.IsValid() && high.IsValid() && ZValue.IsValid(), XSP_MAT_INVALID);

	XSP_CHECK(MatSizeEq(low, high) && MatSizeEq(low, ZValue), XSP_MAT_SIZE_ERR);

	XSP_CHECK(idx < MAX_IDX && idx >= 0, XSP_IDX_OVERFLOW);

	int32_t nHei = low.hei;
	int32_t nWid = low.wid;

	std::string strFilePath(filepath);
	std::string strName(name);
	std::string strSavePath = strFilePath + "/" + strName + "_" + 
		                      std::to_string(nHei) + "_" + 
		                      std::to_string(nWid) + "_idx_" + 
		                      std::to_string(g_nIdx[idx]) + ".raw";

	g_nIdx[idx]++;

	FILE* file = nullptr;
	file = fopen(strSavePath.c_str(), "wb");

	fwrite(low.Ptr(), XSP_ELEM_SIZE(low.type), nHei * nWid, file);
	fwrite(high.Ptr(), XSP_ELEM_SIZE(high.type), nHei * nWid, file);
	fwrite(ZValue.Ptr(), XSP_ELEM_SIZE(ZValue.type), nHei * nWid, file);

	fclose(file);

	return XSP_OK;
}

int32_t WriteDat(const char* filepath, const char* name, int32_t idx,
	         XMat& low, XMat& high)
{
	XSP_CHECK(filepath && name, XSP_NULLPTR, "filepath or name str is null.");

	XSP_CHECK(low.IsValid() && high.IsValid(), XSP_MAT_INVALID);

	XSP_CHECK(MatSizeEq(low, high), XSP_MAT_SIZE_ERR);

	XSP_CHECK(idx < MAX_IDX && idx >= 0, XSP_IDX_OVERFLOW);

	int32_t nHei = low.hei;
	int32_t nWid = low.wid;

	XSP_FILE_HEADER header;
	memset(&header, 0, sizeof(XSP_FILE_HEADER));
	header.height = nHei;
	header.width = nWid;

	std::string strFilePath(filepath);
	std::string strName(name);
	std::string strSavePath = strFilePath + "/" + strName + "_" +
		                      std::to_string(nHei) + "_" +
		                      std::to_string(nWid) + "_idx_" +
		                      std::to_string(g_nIdx[idx]) + ".dat";

	g_nIdx[idx]++;

	FILE* file = nullptr;
	file = fopen(strSavePath.c_str(), "wb");
	fwrite(&header, sizeof(XSP_FILE_HEADER), 1, file);
	fwrite(high.Ptr(), XSP_ELEM_SIZE(high.type), nHei * nWid, file);
	fwrite(low.Ptr(), XSP_ELEM_SIZE(low.type), nHei * nWid, file);

	fclose(file);

	return XSP_OK;
}