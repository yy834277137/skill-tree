/**
 * Noise Reduction and Edge Enhance
 */

#include "isl_nree.hpp"
#include "spdlog/spdlog.h"
#include "isl_log.hpp"

namespace isl
{

Nree::Nree()
{
    SetEeParam();

    return;
}


/**
 * @fn      EdgeDetectUsm
 * @brief   UnSharp Mask边缘检测
 * 
 * @param   [OUT] edge 边缘图
 * @param   [IN] imgori 原始图
 * @param   [IN] imglp 低通图
 * @param   [IN] dif_range 边缘范围，低于first的认为噪声不计入边缘图，高于second的截止到second 
 *                         first值需要根据输入图像的实际噪声确定，原图噪声小该值则小，满足噪声基本抑制边缘大部分保留 
 * 
 * @return  int32_t 0：处理正常，其他：异常
 */
int32_t Nree::EdgeDetectUsm(Imat<int16_t> &edge, Imat<uint16_t> &imgori, Imat<uint16_t> &imglp, 
                            std::pair<int32_t, int32_t> dif_range)
{
    if (!(edge.isvalid() && imgori.isvalid() && imglp.isvalid()))
    {
        ISL_LOGE("edge: {}, imgori: {}, imglp: {}", edge, imgori, imglp);
        return -1;
    }

    if (!(imgori.pwin().size() == imglp.pwin().size() && edge.pwin().size() == imgori.pwin().size()))
    {
        ISL_LOGE("pwin size are different, edge: {}, imgori: {}, imglp: {}", 
                 edge.pwin().size(), imgori.pwin().size(), imglp.pwin().size());
        return -1;
    }

    for (int32_t i = 0; i < imgori.pwin().height; ++i)
    {
        uint16_t* ori = imgori.ptr(imgori.pwin().y+i, imgori.pwin().x);
        uint16_t* lp = imglp.ptr(imglp.pwin().y+i, imglp.pwin().x);
        int16_t* dst = edge.ptr(edge.pwin().y+i, edge.pwin().x);

        for (int32_t j = 0; j < imgori.pwin().width; ++j, ++ori, ++lp, ++dst)
        {
            if (*ori >= *lp)
            {
                int32_t dif = std::min(*ori - *lp, dif_range.second);
                *dst = (dif >= dif_range.first) ? dif : 0;
            }
            else
            {
                int32_t dif = std::min(*lp - *ori, dif_range.second);
                *dst = (dif >= dif_range.first) ? -dif : 0;
            }
        }
    }

    return 0;
}


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
void Nree::SetEeParam(const float64_t intensity, const float64_t lum_gain_middle, const float64_t lum_gain_suppress, std::map<int32_t, float64_t> weak_strong_adj)
{
    // 根据亮度重定义增强强度：低亮和高亮，降低边缘增益（复合了入参intensity）
    if (intensity != eeprm.intensity || lum_gain_middle != eeprm.lum_gain_middle || lum_gain_suppress != eeprm.lum_gain_suppress)
    {
        const int32_t lut_imax = lut_bins - 1;
        for (size_t i = 0; i < lut_bins; i++)
        {
            float64_t fidx = static_cast<float64_t>(i) / lut_imax - lum_gain_middle;
            eeprm.lum_gain.at(i) = std::round(std::exp(-lum_gain_suppress * fidx * fidx) * intensity * (1 << lut_bitq));
        }

        eeprm.intensity = intensity;
        eeprm.lum_gain_middle = lum_gain_middle;
        eeprm.lum_gain_suppress = lum_gain_suppress;
    }

    // 强边缘抑制，对各强度的边缘重映射，小于strong_edge_range.first的维持原样，大于strong_edge_range.second的截断，中间的递减
    if (weak_strong_adj != eeprm.weak_strong_adj)
    {
        const int32_t lut_len = static_cast<int32_t>(eeprm.edge_gain.size());
        const int32_t weak_edge_th = std::clamp(weak_strong_adj.begin()->first, 500, lut_len << eg_shift); // 弱边缘不低于500
        const float64_t weak_enhance_coeff = subcf0(1.0, weak_strong_adj.begin()->second); // weak_enhance_coeff值越小，增强越明显，范围[0, 1]
        for (int32_t i = 0; i <= (weak_edge_th >> eg_shift); ++i)
        {
            // 在（i = weak_edge_th）处（y = x），在（i = weak_edge_th * weak_enhance_coeff ^ (1 / (1 - weak_enhance_coeff))）处导数为1
            eeprm.edge_gain.at(i) = std::round(std::pow(static_cast<float64_t>(i << eg_shift) / weak_edge_th, weak_enhance_coeff) * weak_edge_th);
        }

        const int32_t strong_edge_th = std::clamp(weak_strong_adj.rbegin()->first, 2000, lut_len << eg_shift); // 强边缘不低于2000
        const float64_t strong_suppress_coeff = std::max(weak_strong_adj.rbegin()->second, 0.0); // strong_suppress_coeff值越大，抑制越明显，不小于0，0即不抑制
        for (int32_t i = (weak_edge_th >> eg_shift) + 1; i < lut_len; ++i)
        {
            // 在（i = weak_edge_th）处（y = x），在（i = strong_edge_th/strong_suppress_coeff）处有极大值
            //eeprm.edge_gain.at(i) = std::round(std::exp(-strong_suppress_coeff * (i - weak_edge_th) / strong_edge_th) * i); // 这条曲线中间段斜率很小，强边缘抑制不明显
            float64_t fidx = static_cast<float64_t>((i << eg_shift) - weak_edge_th) / strong_edge_th;
            eeprm.edge_gain.at(i) = std::round(std::exp(-strong_suppress_coeff * fidx * fidx) * (i << eg_shift)); // strong_edge_th并非极值点处
        }
        eeprm.weak_strong_adj = weak_strong_adj;
    }

    return;
}


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
int32_t Nree::EdgeEnhance(Imat<uint16_t> &imgout, Imat<uint16_t> &imgin, Imat<int16_t> &edge, float64_t neg_suppress)
{
    if (!(imgout.isvalid() && imgin.isvalid() && edge.isvalid()))
    {
        ISL_LOGE("imgout: {}, imgin: {}, edge: {}", imgout, imgin, edge);
        return -1;
    }

    if (!(imgout.pwin().size() == imgin.pwin().size() && edge.pwin().size() == imgin.pwin().size()))
    {
        ISL_LOGE("pwin size are different, imgout: {}, imgin: {}, edge: {}", 
                 imgout.pwin().size(), imgin.pwin().size(), edge.pwin().size());
        return -1;
    }

    if (eeprm.intensity <= FLT_EPSILON) // 边缘增强等级为0时直接拷贝
    {
        if (imgout.ptr(imgout.pwin().postl().y, imgout.pwin().postl().x) != imgin.ptr(imgin.pwin().postl().y, imgin.pwin().postl().x))
        {
            imgin.copyto(imgout, imgin.pwin(), imgout.pwin());
        }

        return 0;
    }

    // Undershoot and OverShoot Suppression
    // TODO: 可对edge图中ABS大于TH的生成mask，对mask做高斯滤波，然后imgori + egde * mask-gauss，以消除shoot的不自然感
    const int32_t lut_imax = lut_bins - 1;
    neg_suppress = std::min(subcf0(1.0, neg_suppress), 1.0);
    for (int32_t i = 0; i < imgin.pwin().height; ++i)
    {
        int16_t *dif = edge.ptr(edge.pwin().y+i, edge.pwin().x);
        uint16_t *src = imgin.ptr(imgin.pwin().y+i, imgin.pwin().x);
        uint16_t *dst = imgout.ptr(imgout.pwin().y+i, imgout.pwin().x);

        for (int32_t j = 0; j < imgin.pwin().width; ++j, ++dif, ++src, ++dst)
        {
            int32_t imval = *src;
            int32_t edval = *dif * eeprm.lum_gain.at(imval >> ll_shift) >> lut_bitq; // 根据边缘亮度调节，抑制暗区与亮区边缘
            if (edval > 0)
            {
                edval = eeprm.edge_gain.at(std::min(edval >> eg_shift, lut_imax)); // 自适应边缘增益，弱边增强，强边抑制
                *dst = std::min(imval + edval, 65535);
            }
            else if (edval < 0)
            {
                edval = eeprm.edge_gain.at(std::min(-edval >> eg_shift, lut_imax));
                edval = std::round(neg_suppress * edval); // 负边缘抑制，减少负边缘引起的黑边问题
                *dst = std::max(imval - edval, 0);
            }
            else
            {
                *dst = imval;
            }
        }
    }

    return 0;
}

} // namespace

