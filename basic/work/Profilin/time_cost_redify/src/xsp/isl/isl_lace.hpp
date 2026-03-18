/**
 * Luminance Adjust and Contrace Enhance
 */

#ifndef __ISL_LACE_H__
#define __ISL_LACE_H__

#include <map>
#include "isl_mat.hpp"

namespace isl
{

/// 亮度调节与对比度增强，暂仅支持uint16_t数据类型
class Lace
{
public:
    static const size_t lut_bins = 1024; // 查找表与直方图的阶数
    static const uint32_t ll_shift = 6; // Lum and Lut shift bits, 16Bit（灰度图） -> 10Bit（LUT）
    static const uint32_t lut_imax = static_cast<uint32_t>(lut_bins - 1); // LUT Index Max

    using lut_f64t = std::array<float64_t, lut_bins>;
    using lut_u16t = std::array<uint16_t, lut_bins>;

    Lace();

    lut_f64t& get_lut_fidx() { return this->lut_fidx; }

    int32_t HistStatistic(lut_f64t& hist, size_t& pxsum, float64_t* avg, Imat<uint16_t>& imgin, 
                          std::pair<uint16_t, uint16_t> hs_lum_range={0, 65535}, Imat<uint16_t>* mask=nullptr, const bool bUniform=true);

    void HistCompsite(lut_f64t &histout, lut_f64t &histina, lut_f64t &histinb, 
                      const size_t pxsuma, const size_t pxsumb, const float64_t awt);

    // gamma_restrict, 是否有HE补偿，有HE补偿时，限制Gamma参数最小值
    float64_t LumAdjust(lut_f64t &blut, lut_f64t &hist, const size_t pxsum, const float64_t *avg=NULL, float64_t lum_exp=0.5,
                        float64_t dark_boost=0.0, float64_t gamma_max=0.0, std::vector<float64_t> *dbg_params=NULL);

    //float64_t ce_level; // 外部参数，对比度增强系数，范围[0, 1.0]，值越大对比度越强
    void ContrastEnhance(lut_f64t &clut, lut_f64t &lutin, lut_f64t &hist, const float64_t ce_level=0.5);

    template <typename _T>
    void HistResample(lut_f64t &histout, float64_t &histsum, _T &histin, 
                      std::pair<size_t, size_t> &intercept, bool bUniform, bool bResize);

private:
    const std::pair<float64_t, float64_t> lum_mid = {0.3, 0.8}; // 中间亮度区，低于fisrt为难穿暗区，高于second为易穿亮区
    const float64_t gamma_min = 0.1; // 最小Gamma系数
    const float64_t pdf_eq_ratio = 0.5; // 直方图均衡度，范围[0, 1.0]，值越小，期望pdf与原始pdf越接近，对比度变化越小
    std::map<float64_t, float64_t> slope_limit_segf = { // 对比度增强曲线斜率控制
        {0.0, 0.025}, {lum_mid.first, 0.015}, {0.5, 0.01}, {lum_mid.second, 0.005}, {1.0, 0.001}};

    lut_f64t lut_fidx;
    lut_f64t slope_limit;

};


/**
 * @fn      DynRangeRedistribute
 * @brief   曲线动态范围重分配
 * 
 * @param   [OUT] lutout 输出曲线
 * @param   [IN] lutin 输入曲线，数组大小需与lutout相等，数据类型可以不同
 * @param   [IN] iomap 重映射关系，每个元素的key(.first)为需映射的x坐标，val(.second)为需映射的y坐标 
 *                     比如，iomap中有一元素{0.5, 0.7}，则输出的曲线中，lutout索引[_N*0.5]处的值（lutout[_N*0.5]）：
 *                     当_T为整型时，值为(_N - 1)*0.7；当_T为浮点型时，值为0.7
 * 
 * @return  int32_t 0：转换成功，其他：失败
 */
template <typename _T, typename _P, size_t _N>
int32_t DynRangeRedistribute(std::array<_T, _N>& lutout, const std::array<_P, _N>& lutin, const std::map<float64_t, float64_t>& iomap)
{
    if (!(_N >= 64 && _N <= 4096 && (_N & (_N - 1)) == 0))
    {
        return -1;
    }

    uint32_t idx_max = _N - 1;
    std::array<float64_t, _N> lutftmp;
    if constexpr (is_f64_v<_P>)
    {
        lutftmp = lutin;
    }
    else
    {
        std::transform(lutin.begin(), lutin.end(), lutftmp.begin(), [](_P val) { return static_cast<float64_t>(val); });
    }

    if (iomap.size() >= 2 &&
        static_cast<uint32_t>(iomap.begin()->first * idx_max) == 0 && 
        static_cast<uint32_t>(iomap.rbegin()->first * idx_max) == idx_max)
    {
        if (iomap.size() == 2 && 
            static_cast<uint32_t>(iomap.begin()->second * idx_max) == 0 && 
            static_cast<uint32_t>(iomap.rbegin()->second * idx_max) == idx_max)
        {
            // (0.0, 0.0) : (1.0, 1.0), do nothing
            if (std::is_same_v<_T, _P> && // 数据类型相同
                (void*)lutout.data() == (void*)lutin.data()) // 地址也相同
            {
                return 0; // 直接返回，否则统一走下面的校验拷贝流程
            }
        }
        else
        {
            for (auto ia = iomap.begin(), ib = std::next(iomap.begin()); ib != iomap.end(); ++ia, ++ib)
            {
                size_t idxa = std::round(ia->first * idx_max);
                size_t idxb = std::round(ib->first * idx_max);
                /**
                 * 每次循环计算的索引范围是[idxa, idxb)，注意不计算lutftmp.at(idxb)， 
                 * 因为下次循环中的lutftmp.at(idxa)为上次的lutftmp.at(idxb)，会影响迭代的计算结果 
                 */
                if (idxb > idxa + 1)
                {
                    float64_t scale_ratio = 0.0;
                    float64_t diff_in = lutftmp.at(idxb) - lutftmp.at(idxa);
                    if (std::abs(diff_in) > FLT_EPSILON)
                    {
                        /// 计算斜率的缩放系数
                        scale_ratio = (ib->second - ia->second) / diff_in;
                        std::vector<float64_t> slope;
                        for (size_t i = idxa, j = idxa+1; j < idxb; ++i, ++j) // 计算[idxa, idxb-1]中相邻数值斜率
                        {
                            slope.push_back(scale_ratio * (lutftmp.at(j) - lutftmp.at(i)));
                        }

                        auto iprev = lutftmp.begin() + idxa, inext = iprev + 1;
                        *iprev = ia->second; // 起始索引[idxa]处直接赋值
                        for (auto it = slope.begin(); it != slope.end(); ++it, ++iprev, ++inext)
                        {
                            *inext = *iprev + *it;
                        }
                    }
                    else // 当原斜率为0时，直接拉成线性，相邻数值间斜率相等
                    {
                        auto it = lutftmp.begin() + idxa;
                        scale_ratio = (ib->second - ia->second) / (idxb - idxa);
                        for (size_t i = idxa, k = 0; i < idxb; ++i, ++it, ++k)
                        {
                            *it = ia->second + scale_ratio * k;
                        }
                    }
                }
                else if (idxb == idxa + 1)
                {
                    lutftmp.at(idxa) = ia->second;
                }
                else
                {
                    return -1;
                }

                /// the last one
                if (idxb == idx_max)
                {
                    lutftmp.at(idxb) = ib->second;
                }
            }
        }
    }

    auto it = lutftmp.begin();
    if constexpr (std::is_integral_v<_T>)
    {
        for (auto ir = lutout.begin(); ir != lutout.end(); ++ir, ++it)
        {
            *ir = std::min(static_cast<uint32_t>(std::round(std::max(*it, 0.0) * idx_max)), idx_max);
        }
    }
    else // if constexpr (std::is_floating_point_v<_T>)
    {
        for (auto ir = lutout.begin(); ir != lutout.end(); ++ir, ++it)
        {
            *ir = std::clamp(*it, 0.0, 1.0);
        }
    }

    return 0;
}

} // namespace

#endif

