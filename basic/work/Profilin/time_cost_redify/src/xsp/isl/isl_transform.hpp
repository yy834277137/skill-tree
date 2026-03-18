/**
 * Image SoLution Morphological Transform
 */

#ifndef __ISL_TRANSFORM_H__
#define __ISL_TRANSFORM_H__

#include <map>
#include "isl_hal.hpp"
#include "isl_mat.hpp"
#include "spdlog/spdlog.h"
#include "isl_log.hpp"

namespace isl
{

enum class intp_method_t
{
    na,             // 无
    nearest,        // 最邻近
    linear,         // 线性
    cubic,          // 三次
    lanczos,        // lanczos
    mnum            // 插值方法总数
};
using px_intp_t = std::map<int32_t, float64_t>;     // pixel index and interpolate weight

/**
 * @fn      buildLinearScalePos
 * @brief   生成线性缩放各目标像素点对应原图像的位置
 * 
 * @param   [OUT] pxPos 一维图像中各像素对应原图像的位置
 * @param   [IN] lenIn 输入一维图像的长度
 * @param   [IN] lenOut 输出一维图像的长度
 * 
 * @return  int32_t 0：生成成功，其他：失败
 */
int32_t buildLinearScalePos(std::vector<float64_t>& pxPos, const int32_t lenIn, const int32_t lenOut);

/**
 * @fn      transPos2IntpLut
 * @brief   将一维图像的像素位置索引转换为复杂插值的查找表
 *
 * @param   [OUT] intpLut 输出一维图像中各像素的插值查找表，每个插值查找表长度不定，但不超过lutSize
 * @param   [IN] lutSize 单个插值查找表的最大长度，≥3：Lanczos插值表，=2：Linear插值表，=1：Nearest插值表
 * @param   [IN] pxPos 输出插值后的一维图像各像素在输入图像中的相对位置，pxPos.size()即为输出一维图像的长度
 * @param   [IN] lenIn 输入一维图像的长度
 *
 * @return  int32_t 0：转换成功，其他：转换失败
 */
int32_t transPos2IntpLut(std::vector<px_intp_t>& intpLut, size_t lutSize, std::vector<float64_t>& pxPos, const int32_t lenIn, const float64_t lanczos_blur = 1.3);

/**
 * @fn      buildResizingLut
 * @brief   生成等比例缩放的查找表，即串联函数buildLinearScalePos()和transPos2IntpLut
 * 
 * @param   [OUT] intpLut 输出一维图像中各像素的插值查找表，每个插值查找表长度不定，但不超过lutSize
 * @param   [IN] lutSize 单个插值查找表的最大长度，≥3：Lanczos插值表，=2：Linear插值表，=1：Nearest插值表
 * @param   [IN] lenIn 输入一维图像的长度
 * @param   [IN] lenOut 输出一维图像的长度
 * 
 * @return  int32_t 0：生成成功，其他：失败
 */
int32_t buildResizingLut(std::vector<px_intp_t>& intpLut, size_t lutSize, const int32_t lenIn, const int32_t lenOut);

/**
 * @fn      buildAverageLut
 * @brief   生成经过平均化处理的插值表
 *
 * @param   [IN/OUT] intpLut 输入常规插值表
 * @param   [IN] lenIn 输入表长度
 * @param   [IN] lenOut 输出表长度
 * @param   [IN] interpolationMethod 插值方式，1：Nearest，2：Lanczos
 *
 * @return  int32_t 0：转换成功，其他：转换失败
 */
int32_t buildAverageLut(std::vector<px_intp_t>& intpLut, const int32_t lenIn, const int32_t lenOut, const int32_t interpolationMethod);

/**
 * @fn      buildDownSizingAvgLut
 * @brief   生成缩小时使用均值方法的查找表
 *
 * @param   [OUT] dsAvgLut 缩小时一维图像中各像素的权重表
 * @param   [IN] lenIn 输入一维图像的长度
 * @param   [IN] lenOut 输出一维图像的长度
 *
 * @return  int32_t 0：生成成功，其他：失败
 */
int32_t buildDownSizingAvgLut(std::vector<px_intp_t>& dsAvgLut, const int32_t lenIn, const int32_t lenOut);

/**
 * @fn      imgAnyScale
 * @brief   图像任意（按相同比例或不同比例、输入输出不同数据类型等）缩放
 * 
 * @param   [OUT] imgout 输出缩放后的图像
 * @param   [IN] imgin 输入需缩放的图像
 * @param   [IN] imgtmp 临时图像内存，在横向、纵向均需缩放时需要，图像尺寸不小于输入输出尺寸的较大值，单方向缩放时可省略（置nullptr）
 * @param   [IN] intpLutHor 横向缩放查找表，置nullptr时函数内部会根据intpMethod方法临时生成
 * @param   [IN] intpLutVer 纵向缩放查找表，置nullptr时函数内部会根据intpMethod方法临时生成
 * @param   [IN] fmap 缩放后图像到最终输出图像的映射关系，相当于imgscaled.mapto(imgout)
 * 
 * @return  int32_t 0：缩放成功，其他：失败
 */
template <typename _P, typename _T>
int32_t imgAnyScale(Imat<_P>& imgout, Imat<_T>& imgin, Imat<_T>* imgtmp, std::vector<px_intp_t>* intpLutHor, std::vector<px_intp_t>* intpLutVer, 
                    std::function<_P(const _T)> fmap=nullptr)
{
    if (!(imgout.isvalid() && imgin.isvalid()))
    {
        ISL_LOGE("imgin: {}, imgout: {}", imgin, imgout);
        return -1;
    }
    if (nullptr != intpLutHor && imgout.pwin().width != static_cast<int32_t>(intpLutHor->size())) // 需水平缩放，但输出宽度不匹配
    {
        return -1;
    }
    if (nullptr != intpLutVer && imgout.pwin().height != static_cast<int32_t>(intpLutVer->size())) // 需垂直缩放，但输出高度不匹配
    {
        return -1;
    }

    constexpr float64_t vMin = static_cast<float64_t>(std::numeric_limits<_T>::min());
    constexpr float64_t vMax = static_cast<float64_t>(std::numeric_limits<_T>::max());
    bool bScaleHor = (nullptr != intpLutHor); // 是否水平变换
    bool bScaleVer = (nullptr != intpLutVer); // 是否垂直变换

    Rect<int32_t> winTmp;
    if (nullptr != imgtmp)
    {
        if (!imgtmp->isvalid() ||
            imgtmp->pwin().width < std::max(imgin.pwin().width, imgout.pwin().width) ||
            imgtmp->pwin().height < std::max(imgin.pwin().height, imgout.pwin().height))
        {
            ISL_LOGE("imgin: {}, imgout: {}, imgtmp: {}", imgin, imgout, *imgtmp);
            return -1;
        }
        winTmp = imgtmp->pwin();
    }
    else
    {
        if (bScaleHor && bScaleVer)
        {
            return -1; // 单方向变换不需要临时内存，否则需要临时内存来存储单方向变换的中间结果
        }
    }

    if (!fmap && !std::is_same_v<_P, _T>) // fmap函数为空时，_P和_T必须类型相同
    {
        return -1;
    }

    /// 无变换直接拷贝
    if (!bScaleHor && !bScaleVer)
    {
        if (nullptr == fmap)
        {
            Imat<_T> imgoutT(imgout);
            return imgin.copyto(imgoutT, imgin.pwin(), imgout.pwin());
        }
        else
        {
            return imgin.mapto(imgout, fmap, &imgin.pwin(), &imgout.pwin());
        }
    }

    auto fcomp = (nullptr == fmap) ? 
        std::function<_P(float64_t)>{[&vMin, &vMax](const float64_t src) -> _P {return static_cast<_P>(std::clamp(std::round(src), vMin, vMax));}} :
        std::function<_P(float64_t)>{[&vMin, &vMax, &fmap](const float64_t src) -> _P {return fmap(static_cast<_T>(std::clamp(std::round(src), vMin, vMax)));}};

    /// 水平变换
    if (nullptr != intpLutHor)
    {
        if (nullptr != intpLutVer)
        {
            Imat<_T>& imgdst = (*imgtmp)(Rect<int32_t>(winTmp.x, winTmp.y, imgout.pwin().width, imgin.pwin().height));
            for (int32_t i = 0, rowi = imgin.pwin().y, rowo = imgdst.pwin().y; i < imgin.pwin().height; ++i, ++rowi, ++rowo)
            {
                _T* srcl = imgin.ptr(rowi, imgin.pwin().x);
                _T* dst = imgdst.ptr(rowo, imgdst.pwin().x);
                for (int32_t j = 0; j < imgdst.pwin().width; ++j, ++dst)
                {
                    px_intp_t& intpLut = intpLutHor->at(j);
                    float64_t wtsum = 0.0;
                    for (auto it = intpLut.begin(); it != intpLut.end(); ++it)
                    {
                        wtsum += it->second * srcl[it->first];

                    }
                    *dst = std::clamp(std::round(wtsum), vMin, vMax); 
                }
            }
        }
        else
        {
            Imat<_P>& imgdst = imgout;
            for (int32_t i = 0, rowi = imgin.pwin().y, rowo = imgdst.pwin().y; i < imgin.pwin().height; ++i, ++rowi, ++rowo)
            {
                _T* srcl = imgin.ptr(rowi, imgin.pwin().x);
                _P* dst = imgdst.ptr(rowo, imgdst.pwin().x);
                for (int32_t j = 0; j < imgdst.pwin().width; ++j, ++dst)
                {
                    px_intp_t& intpLut = intpLutHor->at(j);
                    float64_t wtsum = 0.0;
                    for (auto it = intpLut.begin(); it != intpLut.end(); ++it)
                    {
                        wtsum += it->second * srcl[it->first];
                    }
                    *dst = fcomp(wtsum);
                }
            }

            return 0;
        }
    }

    /// 垂直变换
    if (nullptr != intpLutVer)
    {
        Imat<_T>& imgsrc = (nullptr != intpLutHor) ? (*imgtmp) : imgin;
        for (int32_t i = 0, rowi = imgsrc.pwin().y, rowo = imgout.pwin().y; i < imgout.pwin().height; ++i, ++rowo)
        {
            /// 将map转换为vector，方便遍历
            px_intp_t& intpLut = intpLutVer->at(i);
            std::vector<_T*> srcl;
            std::vector<float64_t> wt;
            int32_t intpNum = 0;
            for (auto it = intpLut.begin(); it != intpLut.end(); ++it)
            {
                srcl.push_back(imgsrc.ptr(rowi + it->first, imgsrc.pwin().x));
                wt.push_back(it->second);
                ++intpNum;
            }
            _P *dst = imgout.ptr(rowo, imgout.pwin().x);
            for (int32_t j = 0; j < imgout.pwin().width; ++j, ++dst)
            {
                float64_t wtsum = 0.0;
                for (int32_t k = 0; k < intpNum; ++k)
                {
                    wtsum += wt[k] * srcl[k][j];

                }
                *dst = fcomp(wtsum);
            }
        }
        if (nullptr != imgtmp)
        {
            (*imgtmp)(winTmp); // 恢复imgtmp的处理区域
        }
    }

    return 0;
}

/**
 * @fn      imgAnyScale
 * @brief   图像任意（按相同比例或不同比例、输入输出不同数据类型等）缩放
 * 
 * @param   [OUT] imgout 输出缩放后的图像
 * @param   [IN] imgin 输入需缩放的图像
 * @param   [IN] imgtmp 临时图像内存，在横向、纵向均需缩放时需要，图像尺寸不小于输入输出尺寸的较大值，单方向缩放时可省略（置nullptr）
 * @param   [IN] intpMethod 插值方法，默认为intp_method_t::linear
 * @param   [IN] fmap 缩放后图像到最终输出图像的映射关系，相当于imgscaled.mapto(imgout)
 * 
 * @return  int32_t 0：缩放成功，其他：失败
 */
template <typename _P, typename _T>
int32_t imgAnyScale(Imat<_P>& imgout, Imat<_T>& imgin, Imat<_T>* imgtmp, intp_method_t intpMethod=intp_method_t::linear, 
                    std::function<_P(const _T)> fmap=nullptr)
{
    if (!(imgout.isvalid() && imgin.isvalid()))
    {
        ISL_LOGE("imgin: {}, imgout: {}", imgin, imgout);
        return -1;
    }

    bool bScaleHor = (imgin.pwin().width != imgout.pwin().width); // 是否水平变换
    bool bScaleVer = (imgin.pwin().height != imgout.pwin().height); // 是否垂直变换

    /// 构建水平垂直方向的缩放表
    std::vector<px_intp_t> intpLutHor, intpLutVer;
    size_t lutSize = (intpMethod == intp_method_t::lanczos) ? 6 : ((intpMethod == intp_method_t::nearest) ? 1 : 2);

    if (bScaleHor) // 水平方向（列宽）的缩放表
    {
        std::vector<float64_t> pxPos;
        if (0 != buildLinearScalePos(pxPos, imgin.pwin().width, imgout.pwin().width))
        {
            return -1;
        }
        if (0 != transPos2IntpLut(intpLutHor, lutSize, pxPos, imgin.pwin().width))
        {
            return -1;
        }
    }

    if (bScaleVer) // 垂直方向（行高）的缩放表
    {
        std::vector<float64_t> pxPos;
        if (0 != buildLinearScalePos(pxPos, imgin.pwin().height, imgout.pwin().height))
        {
            return -1;
        }
        if (0 != transPos2IntpLut(intpLutVer, lutSize, pxPos, imgin.pwin().height))
        {
            return -1;
        }
    }

    return imgAnyScale(imgout, imgin, imgtmp, bScaleHor ? &intpLutHor : nullptr, bScaleVer ? &intpLutVer : nullptr, fmap);
}


} // namespace

#endif

