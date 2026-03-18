/**
 * Noise Reduction and Edge Enhance
 */

#ifndef __ISL_NREE_H__
#define __ISL_NREE_H__

#include <map>
#include "isl_mat.hpp"

namespace isl
{

class Nree
{
private:
    static const size_t lut_bins = 1024; // 查找表的阶数
    static const size_t lut_bitq = 10; // 查找表的精度，Q10格式，1024为1.0
    const uint32_t ll_shift = 6; // Lum and Lut shift bits, 16Bit（灰度图） -> 10Bit（LUT）
    const uint32_t eg_shift = 4; // Edge gain shift bits, 11Bit（边缘图） -> 15Bit
    struct
    {
        float64_t nr_ratio; // 越大降噪越强，不小于1.0
        uint16_t hlt_th;
        size_t vshift;
        std::vector<uint32_t> lpwt;
    } nrhlt; /**@waring 这里参数不要赋初值，否则构造函数会不起效，默认值在SetNrhltParam()函数申明中指定 */

    struct
    {
        /// EE强度，与图像本身亮度相关
        float64_t intensity; // 边缘整体增强强度，范围：[0, 16]，0表示不增强
        float64_t lum_gain_middle; // 中间亮度区EE最强，小于0.5时，低亮边缘增益强于高亮，大于0.5时，高亮边缘增益强于低亮，范围：[0, 1]
        float64_t lum_gain_suppress; // 两端亮度区EE较弱，值越小，低亮与高亮的边缘增强抑制越弱，范围：不小于0，为0则不抑制
        std::array<int32_t, lut_bins> lum_gain; // 基于亮度的EE增益表，根据上面三个参数计算得到，Q10格式

        /// Undershoot & OverShoot 抑制
        /**
         * weak_strong_adj 
         * begin()：弱边缘阈值-弱边缘增强系数，小于该阈值的边缘做幂函数增强，范围：[0, 1]，值越大增强越明显，为0则不增强
         * rbegin()：强边缘阈值-强边缘抑制系数，从弱边缘阈值开始抑制，该阈值后抑制程度显著增大，范围：不小于0，为0则不抑制 
         *   其中，边缘阈值是已经过intensity放大的，范围是[0, 2^15] 
         */
        std::map<int32_t, float64_t> weak_strong_adj;
        std::array<int32_t, lut_bins*2> edge_gain; // 边缘增益，注：这里已经转换为了映射表，非系数，精度为Q4，即映射关系为：edge_gain[X >> 4]
    } eeprm; /**@waring 这里参数不要赋初值，否则构造函数会不起效，默认值在SetEeParam()函数申明中指定 */

public:
    Nree();

    /**
     * @fn      EdgeDetectUsm
     * @brief   UnSharp Mask边缘检测
     * 
     * @param   [OUT] edge_amp 边缘幅值图
     * @param   [IN] imgori 原始图
     * @param   [IN] imglp 低通图
     * @param   [IN] dif_range 边缘范围，低于first的认为噪声不计入边缘图，高于second的截止到second 
     *                         first值需要根据输入图像的实际噪声确定，原图噪声小该值则小，满足噪声基本抑制边缘大部分保留 
     * 
     * @return  int32_t 0：处理正常，其他：异常
     */
    static int32_t EdgeDetectUsm(Imat<int16_t> &edge_amp, Imat<uint16_t> &imgori, Imat<uint16_t> &imglp, 
                                 std::pair<int32_t, int32_t> dif_range);

    /**
     * @fn      EdgeDetectSobel
     * @brief   Sobel边缘检测
     * 
     * @param   [OUT] edge_amp 边缘幅值图，其值均大于0，边缘方向图暂未输出
     * @param   [IN] imgori 原始图
     * @param   [IN] imgtmp 临时缓存
     * @param   [IN] dif_range 边缘范围，低于first的认为噪声不计入边缘图，高于second的截止到second 
     *                         first值需要根据输入图像的实际噪声确定，原图噪声小该值则小，满足噪声基本抑制边缘大部分保留 
     * @param   [IN] lum_range 亮度范围，imgori在[first, second]范围内才会计入边缘图 
     *  
     * @return  int32_t 0：处理正常，其他：异常
     */
    template <typename _T>
    static int32_t EdgeDetectSobel(Imat<_T> &edge_amp, Imat<_T> &imgori, Imat<_T> &imgtmp, 
                                   std::pair<_T, _T> dif_range, std::pair<_T, _T> lum_range);

    /**
     * @fn      SetEeParam
     * @brief   设置边缘增强参数
     * 
     * @param   [IN] intensity 边缘整体增强强度，范围：[0, 16]，0表示不增强
     * @param   [IN] lum_gain_middle 小于0.5时，低亮边缘增益强于高亮，大于0.5时，高亮边缘增益强于低亮，范围：[0, 1]
     * @param   [IN] lum_gain_suppress 根据亮度的增益抑制，值越小，低亮与高亮的边缘增强抑制越弱，范围：不小于0，为0则不抑制
     * @param   [IN] weak_strong_adj begin()：弱边缘阈值-弱边缘增强系数，小于该阈值的边缘做幂函数增强，范围：[0, 1]，值越大增强越明显，为0则不增强
     *                               rbegin()：强边缘阈值-强边缘抑制系数，从弱边缘阈值开始抑制，该阈值后抑制程度显著增大，范围：不小于0，为0则不抑制
     */
    void SetEeParam(const float64_t intensity=1.0, const float64_t lum_gain_mid=0.3, const float64_t lum_gain_coeff=1.0, 
                    std::map<int32_t, float64_t> weak_strong_adj={{4000, 0.4}, {8000, 0.8}});

    /**
     * @fn      EdgeEnhance
     * @brief   边缘增强处理
     * 
     * @param   [OUT] imgout 边缘增强后的图像
     * @param   [IN] imgin 原始图像
     * @param   [IN] edge 边缘图像
     * @param   [IN] neg_suppress 负边缘抑制，减少负边缘引起的黑化问题，范围[0, 1]，越大负边增益越小，0为不抑制
     * 
     * @return  int32_t 0：处理正常，其他：异常
     */
    int32_t EdgeEnhance(Imat<uint16_t> &imgout, Imat<uint16_t> &imgin, Imat<int16_t> &edge, float64_t neg_suppress=0.0);

};

/**
 * @fn      EdgeDetectSobel
 * @brief   Sobel边缘检测
 * 
 * @param   [OUT] edge_amp 边缘幅值图，边缘方向图暂未输出 
 * @note          受限于imgtmp的数据类型，可输出的边缘幅值最大值为std::numeric_limits<_S>::max() * √2
 * @param   [IN] imgori 原始图
 * @param   [IN] imgtmp 临时缓存
 * @param   [IN] dif_range 边缘范围，低于first的认为噪声不计入边缘图，高于second的截止到second 
 *                         first值需要根据输入图像的实际噪声确定，原图噪声小该值则小，满足噪声基本抑制边缘大部分保留 
 * @param   [IN] lum_range 亮度范围，imgori在[first, second]范围内才会计入边缘图 
 *  
 * @return  int32_t 0：处理正常，其他：异常
 */
template <typename _T>
int32_t Nree::EdgeDetectSobel(Imat<_T> &edge_amp, Imat<_T> &imgori, Imat<_T> &imgtmp, 
                              std::pair<_T, _T> dif_range, std::pair<_T, _T> lum_range)
{
    if (!(edge_amp.isvalid() && imgori.isvalid() && imgtmp.isvalid()))
    {
        ISL_LOGE("edge_amp: {}, imgori: {}, imgtmp: {}", edge_amp, imgori, imgtmp);
        return -1;
    }

    if (!(imgori.pwin().size() == imgtmp.pwin().size() && edge_amp.pwin().size() == imgori.pwin().size()))
    {
        ISL_LOGE("pwin size are different, edge_amp: {}, imgori: {}, imgtmp: {}", 
                 edge_amp.pwin().size(), imgori.pwin().size(), imgtmp.pwin().size());
        return -1;
    }

    using _S = typename std::make_signed<_T>::type; // 同等位宽的有符号型
    using _P = std::conditional_t<is_s8_v<_S> || is_s16_v<_S>, int32_t, std::conditional_t<is_s32_v<_S>, int64_t, float64_t>>; // 更高位宽的有符号型
    constexpr _P vMin = std::numeric_limits<_S>::min();
    constexpr _P vMax = std::numeric_limits<_S>::max();

    Imat<_S> imgrad(imgtmp);
    std::pair<_P, _P> dif_limit{dif_range.first, dif_range.second};

    // 0°水平边缘，先计算纵向梯度[1 0 -1]T，再做水平平滑[1 2 1]
    for (int32_t i = 0, rowi = imgori.pwin().y, rowo = imgrad.pwin().y; i < imgori.pwin().height; ++i, ++rowi, ++rowo)
    {
        typename Imat<_T>::Rptr oriPrev = imgori.rptr(rowi-1);
        _T* pPrev = (_T*)&oriPrev[imgori.pwin().x];
        typename Imat<_T>::Rptr oriNext = imgori.rptr(rowi+1);
        _T* pNext = (_T*)&oriNext[imgori.pwin().x];
        _S* pGrad = imgrad.ptr(rowo, imgrad.pwin().x);
        for (int32_t j = 0; j < imgori.pwin().width; ++j, ++pPrev, ++pNext, ++pGrad)
        {
            *pGrad = std::clamp(static_cast<_P>(*pPrev) - static_cast<_P>(*pNext), vMin, vMax);
        }
    }
    for (int32_t i = 0, rowi = imgrad.pwin().y, rowo = edge_amp.pwin().y; i < edge_amp.pwin().height; ++i, ++rowi, ++rowo)
    {
        typename Imat<_S>::Rptr grad = imgrad.rptr(rowi);
        _S* pEdgeHor = (_S*)edge_amp.ptr(rowo, edge_amp.pwin().x);
        for (int32_t j = 0, col = imgrad.pwin().x; j < edge_amp.pwin().width; ++j, ++col, ++pEdgeHor)
        {
            *pEdgeHor = (2 * grad[col] + grad[col-1] + grad[col+1]) >> 2;
        }
    }

    // 90°垂直边缘，先计算纵向平滑[1 2 1]T，再做水平梯度[1 0 -1]
    for (int32_t i = 0, rowi = imgori.pwin().y, rowo = imgtmp.pwin().y; i < imgori.pwin().height; ++i, ++rowi, ++rowo)
    {
        typename Imat<_T>::Rptr oriPrev = imgori.rptr(rowi-1);
        _T* pPrev = (_T*)&oriPrev[imgori.pwin().x];
        typename Imat<_T>::Rptr oriCurt = imgori.rptr(rowi);
        _T* pCurt = (_T*)&oriCurt[imgori.pwin().x];
        typename Imat<_T>::Rptr oriNext = imgori.rptr(rowi+1);
        _T* pNext = (_T*)&oriNext[imgori.pwin().x];
        _T* pSmooth = imgtmp.ptr(rowo, imgtmp.pwin().x);
        for (int32_t j = 0; j < imgori.pwin().width; ++j, ++pSmooth, ++pPrev, ++pCurt, ++pNext)
        {
            *pSmooth = (2 * *pCurt + *pPrev + *pNext) >> 2;
        }
    }
    for (int32_t i = 0, rowi = imgori.pwin().y, rowt = imgtmp.pwin().y, rowo = edge_amp.pwin().y; i < edge_amp.pwin().height; ++i, ++rowi, ++rowt, ++rowo)
    {
        typename Imat<_T>::Rptr smooth = imgtmp.rptr(rowt);
        _T* src = imgori.ptr(rowi, imgori.pwin().x);
        _T* dst = edge_amp.ptr(rowo, edge_amp.pwin().x); // edge_amp中已经记录了edgeHor
        for (int32_t j = 0, col = imgtmp.pwin().x; j < edge_amp.pwin().width; ++j, ++col, ++src, ++dst)
        {
            if (lum_range.first <= *src && *src <= lum_range.second)
            {
                _P edgeHor = static_cast<_S>(*dst);
                _P edgeVer = std::clamp(static_cast<_P>(smooth[col-1]) - static_cast<_P>(smooth[col+1]), vMin, vMax);
                _P edgeComp = std::sqrt(edgeHor * edgeHor + edgeVer * edgeVer);
                *dst = (edgeComp >= dif_limit.first) ? std::min(edgeComp, dif_limit.second) : 0;
            }
            else
            {
                *dst = 0;
            }
        }
    }

    return 0;
}

} // namespace

#endif

