/**
 * Luminance Adjust and Contrace Enhance
 */
#include "isl_lace.hpp"
#include "spdlog/spdlog.h"
#include "isl_log.hpp"

namespace isl
{

Lace::Lace()
{
    for (size_t i = 0; i < lut_bins; ++i)
    {
        lut_fidx.at(i) = static_cast<float64_t>(i) / lut_imax;
    }

    std::map<size_t, float64_t> slope_limit_segi;
    for (auto it = slope_limit_segf.begin(); it != slope_limit_segf.end(); ++it)
    {
        slope_limit_segi.insert({it->first*lut_imax, it->second});
    }
    for (auto iprev = slope_limit_segi.begin(), inext = std::next(slope_limit_segi.begin()); inext != slope_limit_segi.end(); ++iprev, ++inext)
    {
        float64_t linear_k = (inext->second - iprev->second) / (inext->first - iprev->first); // k
        float64_t linear_b = iprev->second - linear_k * iprev->first; // b
        for (size_t j = iprev->first; j <= inext->first; j++)
        {
            slope_limit.at(j) = linear_b + linear_k * j;
        }
    }

    return;
}


#if 0
int32_t Lace::HistStatistic(std::vector<lut_f64t> &hist, std::vector<size_t> &pxsum, std::vector<float64_t> &avg, 
                               Imat<uint16_t> &imgin, std::pair<uint16_t, uint16_t> hs_lum_range, 
                               Imat<uint16_t> *mask, const bool bUniform)
{
    if (hist.empty() || hist.size() != pxsum.size() || pxsum.size() != avg.size())
    {
        return -1;
    }
    if (!(imgin.isvalid()))
    {
        return -1;
    }
    if (NULL != mask)
    {
        if (!(mask->isvalid() && imgin.pwin().size() == mask->pwin().size()))
        {
            return -1;
        }
    }

    // 清空数据
    for (auto it = hist.begin(); it != hist.end(); ++it)
    {
        std::fill((*it).begin(), (*it).end(), 0.0);
    }
    for (auto it = pxsum.begin(); it != pxsum.end(); ++it)
    {
        *it = 0;
    }

    size_t hist_idx = 0;
    if (NULL != mask)
    {
        uint16_t lum_tmp = 0;
        uint64_t lum_sum = 0;
        lut_f64t &hist_mask = hist.at(hist_idx);
        size_t &pxsum_mask = pxsum.at(hist_idx);
        float64_t &avg_mask = avg.at(hist_idx);
        hist_idx++;

        for (int32_t i = 0, rowy = imgin.pwin().postl().y, rowm = mask->pwin().postl().y; i < imgin.pwin().height; ++i, ++rowy, ++rowm)
        {
            uint16_t *py = imgin.ptr(rowy, imgin.pwin().postl().x);
            uint16_t *pm = mask->ptr(rowm, mask->pwin().postl().x);
            for (int32_t j = 0; j < imgin.pwin().width; ++j, ++py, ++pm)
            {
                if (*py >= hs_lum_range.first && *py <= hs_lum_range.second && *pm)
                {
                    lum_tmp = *py >> this->ll_shift;
                    hist_mask.at(lum_tmp)++;
                    lum_sum += lum_tmp;
                    pxsum_mask++;
                }
            }
        }
        if (pxsum_mask > 0)
        {
            lum_sum = (lum_sum + pxsum_mask / 2) / pxsum_mask;
        }
        avg_mask = static_cast<float64_t>(lum_sum) / lut_imax;
    }

    if (NULL == mask || hist.size() >= 2)
    {
        uint16_t lum_tmp = 0;
        uint64_t lum_sum = 0;
        lut_f64t &hist_glb = hist.at(hist_idx);
        size_t &pxsum_glb = pxsum.at(hist_idx);
        float64_t &avg_glb = avg.at(hist_idx);
        hist_idx++;

        for (int32_t row = imgin.pwin().postl().y; row <= imgin.pwin().posbr().y; row++)
        {
            uint16_t *py = imgin.ptr(row, imgin.pwin().postl().x);
            for (int32_t col = 0; col < imgin.pwin().width; ++col, ++py)
            {
                if (*py >= hs_lum_range.first && *py <= hs_lum_range.second)
                {
                    lum_tmp = *py >> this->ll_shift;
                    hist_glb.at(lum_tmp)++;
                    lum_sum += lum_tmp;
                    pxsum_glb++;
                }
            }
        }
        if (pxsum_glb > 0)
        {
            lum_sum = (lum_sum + pxsum_glb / 2) / pxsum_glb;
        }
        avg_glb = static_cast<float64_t>(lum_sum) / lut_imax;
    }

    if (bUniform)
    {
        for (auto it = pxsum.begin(); it != pxsum.end(); ++it)
        {
            if (*it > 0)
            {
                lut_f64t &htmp = hist.at(it - pxsum.begin());
                for (auto ih = htmp.begin(); ih != htmp.end(); ++ih)
                {
                    *ih /= *it;
                }
            }
        }
    }

    return 0;
}
#endif


int32_t Lace::HistStatistic(lut_f64t& hist, size_t& pxsum, float64_t* avg, Imat<uint16_t>& imgin, 
                            std::pair<uint16_t, uint16_t> hs_lum_range, Imat<uint16_t>* mask, const bool bUniform)
{
    if (!(imgin.isvalid()))
    {
        ISL_LOGE("imgin is invalid: {}", imgin);
        return -1;
    }
    if (NULL != mask)
    {
        if (!(mask->isvalid() && imgin.pwin().size() == mask->pwin().size()))
        {
            ISL_LOGE("mask is invalid: {}", *mask);
            return -1;
        }
    }

    // 清空数据
    std::fill(hist.begin(), hist.end(), 0.0);
    pxsum = 0;

    uint16_t lum_tmp = 0;
    uint64_t lum_sum = 0;
    std::vector<uint16_t> vmask(imgin.pwin().width, 0xFFFF);

    for (int32_t i = 0, rowy = imgin.pwin().postl().y, rowm = (nullptr != mask) ? mask->pwin().postl().y : 0; i < imgin.pwin().height; ++i, ++rowy)
    {
        uint16_t *py = imgin.ptr(rowy, imgin.pwin().postl().x);
        uint16_t *pm = (nullptr != mask) ? mask->ptr(rowm++, mask->pwin().postl().x) : vmask.data();
        for (int32_t j = 0; j < imgin.pwin().width; ++j, ++py, ++pm)
        {
            if (*py >= hs_lum_range.first && *py <= hs_lum_range.second && *pm)
            {
                lum_tmp = *py >> this->ll_shift;
                hist.at(lum_tmp)++;
                lum_sum += lum_tmp;
                pxsum++;
            }
        }
    }
    if (pxsum > 0)
    {
        lum_sum = (lum_sum + pxsum / 2) / pxsum;
        if (bUniform)
        {
            for (auto& it : hist)
            {
                it /= pxsum;
            }
        }
    }
    if (nullptr != avg)
    {
        *avg = static_cast<float64_t>(lum_sum) / (bUniform ? lut_imax : 1);
    }

    return 0;
}


void Lace::HistCompsite(lut_f64t &histout, lut_f64t &histina, lut_f64t &histinb, 
                        const size_t pxsuma, const size_t pxsumb, const float64_t awt)
{
    if (pxsuma == 0 && pxsumb == 0)
    {
        return;
    }
    else if (pxsuma == 0 || awt <= FLT_EPSILON)
    {
        if (histout.data() != histinb.data())
        {
            histout = histinb;
        }
        return;
    }
    else if (pxsumb == 0 || 1.0 - awt <= FLT_EPSILON)
    {
        if (histout.data() != histina.data())
        {
            histout = histina;
        }
        return;
    }

    float64_t ratioa = static_cast<float64_t>(pxsuma) / (pxsuma + pxsumb); // 直方图a的融合比例
    ratioa = 1.0 / (1.0 + (1.0 - ratioa) * (1.0 - awt) / (ratioa * awt));
    float64_t ratiob = 1.0 - ratioa;
    for (auto ia = histina.begin(), ib = histinb.begin(), ho = histout.begin(); ia != histina.end(); ++ia, ++ib, ++ho)
    {
        *ho = *ia * ratioa + *ib * ratiob;
    }

    return;
}


template <typename _T>
void Lace::HistResample(lut_f64t &histout, float64_t &histsum, _T &histin, 
                        std::pair<size_t, size_t> &intercept, bool bUniform, bool bResize)
{
    histsum = 0.0;
    if (intercept.first >= intercept.second || intercept.second > histin.size())
    {
        return;
    }

    if (!bResize) // 不做缩放，只做截取
    {
        if (histin.size() >= this->lut_bins) // 仅截取时，输入直方图的Bins不小于输出的
        {
            for (size_t i = 0; i < lut_bins; i++)
            {
                histout.at(i) = (intercept.first <= i && i < intercept.second) ? histin.at(i) : 0.0;
                histsum += histout.at(i);
            }
        }
    }
    else
    {
        std::vector<float64_t> hist_itcpt(hist_itcpt.begin(), hist_itcpt.end() - (hist_itcpt.size() - intercept.second));
        if (hist_itcpt.size() != this->lut_bins)
        {
            int32_t interpolation_ratio = (hist_itcpt.size() << 16) / (this->lut_bins); // Q16格式
            int32_t wtnext = 0, wtsum = 0;
            size_t idxhist = 0;
            if (hist_itcpt.size() < this->lut_bins)
            {
                for (size_t i = 0; i < this->lut_bins; i++)
                {
                    wtnext = (interpolation_ratio + wtsum) - (((wtsum >> 16) + 1) << 16);
                    if (wtnext > 0) // 累加进1
                    {
                        idxhist++;
                        histout.at(i) = (hist_itcpt.at(idxhist-1) * (interpolation_ratio - wtnext) + hist_itcpt.at(idxhist) * wtnext) / 65536.0;
                        histsum += histout.at(i);
                    }
                    else
                    {
                        histout.at(i) = hist_itcpt.at(idxhist) * interpolation_ratio / 65536.0;
                        histsum += histout.at(i);
                        if (wtnext == 0)
                        {
                            idxhist++;
                        }
                    }
                    wtsum += interpolation_ratio;
                }
            }
            else //if (hist_itcpt.size() > this->lut_bins)
            {
                // unsupport now!!
            }
        }
        else
        {
            for (size_t i = 0; i < this->lut_bins; i++)
            {
                histout.at(i) = hist_itcpt.at(i);
                histsum += histout.at(i);
            }
        }
    }

    //histsum = std::accumulate(histout.begin(), histout.end(), 0.0);
    if (bUniform && histsum > FLT_EPSILON)
    {
        for (auto it = histout.begin(); it != histout.end(); ++it)
        {
            *it /= histsum;
        }
    }

    return;
}


float64_t Lace::LumAdjust(lut_f64t &blut, lut_f64t &hist, const size_t pxsum, const float64_t *avg, float64_t lum_exp,
                          float64_t dark_boost, float64_t gamma_max, std::vector<float64_t> *dbg_params)
{
    /// 极黑区→黑区→暗区→中间亮度区，亮度阈值逐渐升高
    float64_t miu = (NULL != avg) ? *avg : 0.0; // 输入图平均亮度
    float64_t shade_hyper = 0.0; // 暗区的复合比例，亮度越低，权重（见如下shade_wt定义）越大，该值可能会超过1
    const std::pair<float64_t, float64_t> shade_wt_range = {6.0, 0.6}; // (0, lum_mid.first) -> {max, min}
    const float64_t shade_wt_coeff = (shade_wt_range.first - shade_wt_range.second) / (lum_mid.first * lum_mid.first * lum_mid.first);
    const float64_t dark_lum_th = 0.4 * lum_mid.first; // 黑区亮度阈值，小于该值定义为黑区
    const float64_t dark_ratio_th = -0.058 * std::log(std::clamp(dark_boost, DBL_EPSILON, 1.0)); // 黑区像素阈值
    float64_t gamma = 0.0; // Gamma系数
    float64_t tau = 0.0; // 暗区拉伸强度，[0, 1]
    float64_t ksi = 0.0; // LUT起始值，[0, 1]

    for (auto it = hist.begin(), ii = lut_fidx.begin(); it != hist.end(); ++it, ++ii)
    {
        // 计算亮度均值与暗区比例
        if (*it > 0.0)
        {
            if (NULL == avg) // avg非空时使用外部输入的平均亮度
            {
                miu += *it * *ii;
            }
            if (*ii <= dark_lum_th)
            {
                shade_hyper += *it * (shade_wt_range.first - shade_wt_coeff * *ii * *ii * *ii); // 三次曲线，shade_wt = max - coeff * lum^3
                if (ksi <= FLT_EPSILON && shade_hyper > dark_ratio_th) // 黑区的像素数超过dark_th
                {
                    ksi = dark_lum_th - *ii;
                }
            }
        }
    }

    tau = 1.0 + ksi * 5; // 极黑区整体向上偏移后，使用凹曲线补偿一些
    miu -= std::clamp(lum_exp, 0.1, 1.0);
    gamma = 0.6 + 0.4 * miu + 1.2 * miu * miu + 1.6 * miu * miu * miu; // 越远离lum_exp，gamma值越偏离1.0
    gamma = std::max(gamma, this->gamma_min);
    if (gamma_max > FLT_EPSILON)
    {
        gamma = std::min(gamma, gamma_max);
    }

    float64_t beta = std::pow(tau, gamma); // 当gamma一定时，幂底（tau）决定了起始斜率，值越大（趋近于1.0），则起始斜率越小
    for (auto it = blut.begin(), ii = lut_fidx.begin(); it != blut.end(); ++it, ++ii)
    {
        float64_t alpha = std::pow(*ii, gamma);
        *it = alpha / (alpha + (1.0 - alpha) * beta); // beta趋向于1时，该式退化为alpha
    }

    const float64_t exdark_lum_th = std::min(0.016, dark_lum_th); // 极黑区亮度阈值，该部分只做线性亮度映射
    DynRangeRedistribute(blut, blut, {{0.0, ksi}, {exdark_lum_th, exdark_lum_th+ksi}, {1.0, 1.0}});

    if (NULL != dbg_params)
    {
        (*dbg_params).push_back(miu+lum_exp);
        (*dbg_params).push_back(gamma);
        (*dbg_params).push_back(pxsum);
        (*dbg_params).push_back(dark_ratio_th);
        (*dbg_params).push_back(shade_hyper);
        (*dbg_params).push_back(tau);
        (*dbg_params).push_back(ksi);
    }

    return shade_hyper;
}


void Lace::ContrastEnhance(lut_f64t &clut, lut_f64t &lutin, lut_f64t &hist, const float64_t ce_level)
{
    lut_f64t blut = {0.0}; // 基础曲线
    lut_f64t pdf = hist;
    float64_t pdf_min = DBL_MAX, pdf_max = DBL_MIN, pdf_sum = 0.0;

    for (auto it = pdf.begin(); it != pdf.end(); ++it)
    {
        // 计算直方图的最大最小值
        if (*it < pdf_min)
        {
            pdf_min = *it;
        }
        if (*it > pdf_max)
        {
            pdf_max = *it;
        }
    }

    if (pdf_max > pdf_min && ce_level > FLT_EPSILON)
    {
        for (auto it = pdf.begin(); it != pdf.end(); ++it)
        {
            *it = pdf_max * std::pow((*it - pdf_min) / (pdf_max - pdf_min), this->pdf_eq_ratio); // 这里计算出来的是期望pdf的变化比例
            pdf_sum += *it;
        }
        for (auto it = pdf.begin(); it != pdf.end(); ++it) // 重归一化
        {
            if (*it > 0.0)
            {
                *it /= pdf_sum;
            }
        }

        // 计算输入曲线的斜率
        for (auto pr = lutin.begin(), nt = lutin.begin()+1, it = blut.begin(); nt != lutin.end(); ++pr, ++nt, ++it)
        {
            *it = *nt - *pr;
        }

        float64_t slope_surplus = 1.0 - lutin.at(0), slope_acc = lutin.at(0), slope_sum = 0.0;
        auto il = blut.begin();
        for (auto ip = pdf.begin() + 1; ip != pdf.end(); ++il, ++ip)
        {
            size_t idx = std::distance(blut.begin(), il);
            float64_t slope_max = slope_limit.at(idx);
            //*il = std::min(*il * (1.0 - celut.at(idx)) + *ip * celut.at(idx), slope_max);
            *il = std::min(*il * (1.0 - ce_level) + *ip * ce_level, slope_max);
            if (slope_acc + *il > 1.0)
            {
                *il++ = 1.0 - slope_acc;
                slope_acc = 1.0;
                break;
            }

            slope_acc += *il;
            if (*il < slope_max)
            {
                slope_sum += *il;
            }
        }

        if (il+1 != blut.end())
        {
            std::fill(il, blut.end(), 0.0);
        }
        else
        {
            slope_surplus = 1.0 - slope_acc;
            while (slope_surplus > 0.001)
            {
                float64_t amp_ratio = slope_surplus / slope_sum + 1.0;
                slope_sum = 0.0;
                for (auto ib = blut.begin(); ib != blut.end()-1; ++ib)
                {
                    float64_t slope_max = slope_limit.at(std::distance(blut.begin(), ib));
                    if (*ib < slope_max)
                    {
                        slope_surplus += *ib;
                        *ib = std::min(*ib * amp_ratio, slope_max);
                        slope_surplus -= *ib;
                        if (*ib < slope_max)
                        {
                            slope_sum += *ib;
                        }
                    }
                }
            }
        }

        clut.at(0) = lutin.at(0);
        for (auto it = clut.begin()+1, pv = clut.begin(), ib = blut.begin(); it != clut.end(); ++it, ++pv, ++ib)
        {
            *it = std::min(*pv + *ib, 1.0);
        }
    }
    else // 像素偏少时，不做对比度增强
    {
        clut = lutin;
    }

    return;
}

} // namespace

