/**
 * Image SoLution Matrix Class
 */

#ifndef __ISL_FILTER_H__
#define __ISL_FILTER_H__

#include "isl_mat.hpp"
#include <omp.h>

#ifdef X86_PLATFORM
#include <immintrin.h>
#endif
#ifdef RK3588_PLATFORM
#include <arm_neon.h>
#endif
#include <chrono>
#include <cmath>
#include <vector>
#include <type_traits>
#include <new>

namespace isl
{

class Filter
{

public:

    // template <typename _T>
    // static int32_t Gaussian(Imat<_T> &imgout, Imat<_T> &imgin, Imat<_T> &imgtmp, const int32_t radius=2, const float64_t sigma=1.0);
    template <typename _T, typename SpecialHandler = std::function<bool(const _T*, int32_t)>>
    static int32_t Gaussian(Imat<_T> &imgout, Imat<_T> &imgin, Imat<_T> &imgtmp, const int32_t radius=2, const float64_t sigma=1.0, SpecialHandler specialHandler = nullptr);

    // template <typename _T>
    // static int32_t GaussianTest(Imat<_T> &imgout, Imat<_T> &imgin, Imat<_T> &imgtmp, const int32_t radius=2, const float64_t sigma=1.0, const uint8_t fgVal = 255);

    template <typename _T>
    static int32_t Mean(Imat<_T> &imgout, Imat<_T> &imgin, Imat<_T> &imgtmp, const int32_t radius=2);

    template <typename _T>
    static int32_t MeanX(Imat<_T> &imgout, Imat<_T> &imgin, const int32_t radius=1);

    template <typename _T>
    static int32_t MeanY(Imat<_T> &imgout, Imat<_T> &imgin, const int32_t radius=1);

    template <typename _T, typename _P>
    static int32_t Dilate(Imat<_T> &imgout, Imat<_P> &imgin, const int32_t radius, std::function<bool(const _P)> isDilating, std::pair<_T, _T> bgfg);

    template <typename _T, typename _P>
    static int32_t Erode(Imat<_T> &imgout, Imat<_P> &imgin, const int32_t radius, std::function<bool(const _P)> isEroding, std::pair<_T, _T> bgfg);

};

/**
 * @class   Gauss
 * @brief   计算一维高斯滤波核，若_T为整型，其精度为12Bit
 */
template <typename _T>
class Gauss
{
public:
    std::vector<_T> kernel;
    const size_t iaccuracy = 12; // 整型高斯滤波核的精度，12bit
    const int32_t iround = 1 << (iaccuracy - 1); // 整型高斯滤波核四舍五入初始值

    Gauss(int32_t _radius, float64_t _sigma)
    {
        std::vector<float64_t> ktmp;
        for (int32_t i = -_radius; i <= _radius; i++)
        {
            int32_t j = (_sigma > 0) ? i : ((i > 0) ? (_radius - i) : (_radius + i));
            ktmp.push_back(std::exp(-0.5 * (j * j) / (_sigma * _sigma)));
        }

        float64_t ksum = std::accumulate(ktmp.begin(), ktmp.end(), 0.0);
        for (auto it = ktmp.begin(); it != ktmp.end(); ++it)
        {
            if (std::distance(ktmp.begin(), it) != _radius)
            {
                if constexpr (std::is_integral_v<_T>)
                {
                    kernel.push_back(static_cast<uint32_t>(std::round(*it * (1 << iaccuracy) / ksum)));
                }
                else // if constexpr (std::is_floating_point_v<_T>)
                {
                    kernel.push_back(std::round(*it / ksum));
                }
            }
            else
            {
                kernel.push_back(0); // 中心点下面会重新赋值，使该卷积核的和正好是1.0或(1 << iaccuracy)
            }
        }
        if constexpr (std::is_integral_v<_T>)
        {
            kernel.at(static_cast<size_t>(_radius)) = (1 << iaccuracy) - std::accumulate(kernel.begin(), kernel.end(), 0);
        }
        else // if constexpr (std::is_floating_point_v<_T>)
        {
            kernel.at(static_cast<size_t>(_radius)) = 1.0 - std::accumulate(kernel.begin(), kernel.end(), 0);
        }
    }
};


#ifdef X86_PLATFORM
template <typename _T, typename SpecialHandler = std::function<bool(const _T*, int32_t)>>
int32_t Filter::Gaussian(Imat<_T> &imgout, Imat<_T> &imgin, Imat<_T> &imgtmp, const int32_t radius, const float64_t sigma, SpecialHandler specialHandler)
{
    if (radius <= 0 || radius >= std::min(imgin.width(), imgin.height()) / 2) {
        return -1;
    }
    if (!(imgout.isvalid() && imgin.isvalid() && imgtmp.isvalid())) {
        return -1;
    }
    if (!(imgin.isize() == imgtmp.isize() && imgin.pwin() == imgtmp.pwin() && 
          imgout.pwin().size() == imgin.pwin().size() &&
          imgin.itype() == imgtmp.itype() && imgin.itype() == imgout.itype())) {
        return -1;
    }
    if (imgin.channels() > 1) {
        return -1;
    }

    using ktype = std::conditional_t<is_u8_v<_T> || is_u16_v<_T>, uint32_t, 
                    std::conditional_t<is_u32_v<_T>, uint64_t, 
                    std::conditional_t<is_s8_v<_T> || is_s16_v<_T>, int32_t, 
                    std::conditional_t<is_s32_v<_T>, int64_t, float64_t>>>>;

    Gauss<ktype> gauss(radius, sigma);
    const int32_t diameter = 2 * radius + 1;
    
    // 获取一级Cache缓存行大小
#if 0
    constexpr size_t sCacheLineSize = std::hardware_destructive_interference_size;
#endif
    constexpr int sCacheLineSize = 64;
    const int32_t ROWSMAX = std::min(imgin.pwin().y + imgin.pwin().height + radius, imgin.height());
    const int32_t ROWSTART = std::max(0, imgin.pwin().y - radius);

    // X方向
    #pragma omp parallel num_threads(8)
    {
        #pragma omp for schedule(static)
        // 按一级缓存行拆分成不同的Block
        for (int32_t sRowBlock = ROWSTART; sRowBlock < ROWSMAX; sRowBlock += sCacheLineSize) 
        {
            // 单块的最大高度为一级Cache缓存行大小
            const int32_t sActualBlockHei = std::min(sCacheLineSize, ROWSMAX - sRowBlock);
            for (int32_t sRowPerBlock = 0; sRowPerBlock < sActualBlockHei; ++sRowPerBlock) 
            {
                const int32_t sRow = sRowBlock + sRowPerBlock;
                _T* dst = imgtmp.ptr(sRow, imgtmp.pwin().x);
                typename Imat<_T>::Rptr&& src = imgin.rptr(sRow);
                const auto& gauss_kernel = gauss.kernel;
                
                // 处理uint8_t类型的AVX2优化
                if constexpr (std::is_same_v<_T, uint8_t>) 
                {
                    const int PIXEL_PER_VECTOR = sizeof(__m256i) / sizeof(int8_t);
                    const int WIDTH_ALIGNED = (imgin.pwin().width / PIXEL_PER_VECTOR) * PIXEL_PER_VECTOR;
                    const int PIXELPROC_PERNUM = 8;
                    
                    for (int32_t j = 0; j < WIDTH_ALIGNED; j += PIXEL_PER_VECTOR) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        // 检查是否需要特殊处理
                        bool needSpecialHandling = false;
                        if (specialHandler) {
                            needSpecialHandling = specialHandler(&src[col], PIXEL_PER_VECTOR);
                        }
                        
                        if (needSpecialHandling) 
                        {
                            // 标量处理（便于特殊处理）
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                if (specialHandler(&src[col + sProcTime], 1)) 
                                {
                                    dst[j + sProcTime] = src[col + sProcTime]; // 保持原值
                                    continue;
                                }
                                
                                ktype convsum = 0;
                                for (int32_t k = -radius; k <= radius; k++) 
                                {
                                    convsum += gauss_kernel[radius + k] * src[col + sProcTime + k];
                                }
                                uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                dst[j + sProcTime] = static_cast<uint8_t>(std::min(255u, result));
                            }
                        } 
                        else 
                        {
                            // AVX2向量化处理
                            alignas(PIXEL_PER_VECTOR) uint32_t u32ArrayLocalSums[PIXEL_PER_VECTOR] = {0};
                            
                            for (int32_t k = -radius; k <= radius; k++) 
                            {
                                for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                                {
                                    // 加载8个uint8_t并零扩展为uint32_t
                                    __m128i u8vec_DataLoad = _mm_loadl_epi64(
                                        reinterpret_cast<const __m128i*>(&src[col + sProcTime + k]));
                                    __m256i u32vec_DataLoad = _mm256_cvtepu8_epi32(u8vec_DataLoad);
                                    
                                    uint32_t u32KernelVal = static_cast<uint32_t>(gauss_kernel[radius + k]);
                                    __m256i u32vec_KernelVal = _mm256_set1_epi32(u32KernelVal);
                                    
                                    // 乘加运算
                                    __m256i u32vec_ProdVal = _mm256_mullo_epi32(u32vec_DataLoad, u32vec_KernelVal);
                                    __m256i u32vec_PrevSum = _mm256_loadu_si256(
                                        reinterpret_cast<const __m256i*>(&u32ArrayLocalSums[sProcTime]));
                                    __m256i u32vec_NewSum = _mm256_add_epi32(u32vec_PrevSum, u32vec_ProdVal);
                                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&u32ArrayLocalSums[sProcTime]), u32vec_NewSum);
                                }
                            }
                            
                            // 结果处理
                            alignas(PIXEL_PER_VECTOR) uint8_t u8ArrayResults[PIXEL_PER_VECTOR];
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                            {
                                __m256i u32vec_Sum = _mm256_loadu_si256(
                                    reinterpret_cast<const __m256i*>(&u32ArrayLocalSums[sProcTime]));
                                __m256i u32vec_AfterRounded = _mm256_add_epi32(u32vec_Sum, _mm256_set1_epi32(gauss.iround));
                                __m256i u32vec_AfterShifted = _mm256_srli_epi32(u32vec_AfterRounded, gauss.iaccuracy);
                                
                                __m128i u8vec_low = _mm256_castsi256_si128(u32vec_AfterShifted);
                                __m128i u8vec_high = _mm256_extracti128_si256(u32vec_AfterShifted, 1);
                                __m128i u16vec_PackedVal = _mm_packus_epi32(u8vec_low, u8vec_high);
                                __m128i u8vec_PackedVal = _mm_packus_epi16(u16vec_PackedVal, u16vec_PackedVal);
                                
                                _mm_storel_epi64(reinterpret_cast<__m128i*>(&u8ArrayResults[sProcTime]), u8vec_PackedVal);
                            }
                            
                            // 存储结果
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                dst[j + sProcTime] = u8ArrayResults[sProcTime];
                            }
                        }
                    }
                    
                    // 处理剩余未对齐部分
                    for (int32_t j = WIDTH_ALIGNED; j < imgin.pwin().width; ++j) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        if (specialHandler && specialHandler(&src[col], 1)) 
                        {
                            dst[j] = src[col];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = -radius; k <= radius; k++) 
                        {
                            convsum += gauss_kernel[radius + k] * src[col + k];
                        }
                        
                        uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                        dst[j] = static_cast<uint8_t>(std::min(255u, result));
                    }
                    
                }
                // 处理int8_t类型的AVX2优化
                else if constexpr (std::is_same_v<_T, int8_t>) 
                {
                    const int PIXEL_PER_VECTOR = sizeof(__m256i) / sizeof(int8_t);
                    const int WIDTH_ALIGNED = (imgin.pwin().width / PIXEL_PER_VECTOR) * PIXEL_PER_VECTOR;
                    const int PIXELPROC_PERNUM = 8;
                    
                    for (int32_t j = 0; j < WIDTH_ALIGNED; j += PIXEL_PER_VECTOR) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        // 检查是否需要特殊处理
                        bool needSpecialHandling = false;
                        if (specialHandler) {
                            needSpecialHandling = specialHandler(&src[col], PIXEL_PER_VECTOR);
                        }
                        
                        if (needSpecialHandling) 
                        {
                            // 标量处理
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                if (specialHandler(&src[col + sProcTime], 1)) 
                                {
                                    dst[j + sProcTime] = src[col + sProcTime];
                                    continue;
                                }
                                
                                ktype convsum = 0;
                                for (int32_t k = -radius; k <= radius; k++) 
                                {
                                    convsum += gauss_kernel[radius + k] * src[col + sProcTime + k];
                                }
                                int32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                result = std::max(-128, std::min(127, result));
                                dst[j + sProcTime] = static_cast<int8_t>(result);
                            }
                        } 
                        else 
                        {
                            // AVX2向量化处理
                            alignas(PIXEL_PER_VECTOR) int32_t s32ArrayLocalSums[PIXEL_PER_VECTOR] = {0};
                            
                            for (int32_t k = -radius; k <= radius; k++) 
                            {
                                for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                                {
                                    // 加载8个int8_t并符号扩展为int32_t
                                    __m128i s8vec_DataLoad = _mm_loadl_epi64(
                                        reinterpret_cast<const __m128i*>(&src[col + sProcTime + k]));
                                    __m256i s32vec_DataLoad = _mm256_cvtepi8_epi32(s8vec_DataLoad);
                                    
                                    int32_t s32KernelVal = static_cast<int32_t>(gauss_kernel[radius + k]);
                                    __m256i s32vec_KernelVal = _mm256_set1_epi32(s32KernelVal);
                                    
                                    __m256i s32vec_ProdVal = _mm256_mullo_epi32(s32vec_DataLoad, s32vec_KernelVal);
                                    __m256i s32vec_PrevSum = _mm256_loadu_si256(
                                        reinterpret_cast<const __m256i*>(&s32ArrayLocalSums[sProcTime]));
                                    __m256i s32vec_NewSum = _mm256_add_epi32(s32vec_PrevSum, s32vec_ProdVal);
                                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&s32ArrayLocalSums[sProcTime]), s32vec_NewSum);
                                }
                            }
                            
                            // 结果处理
                            alignas(PIXEL_PER_VECTOR) int8_t s8ArrayResults[PIXEL_PER_VECTOR];
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                            {
                                __m256i s32vec_Sum = _mm256_loadu_si256(
                                    reinterpret_cast<const __m256i*>(&s32ArrayLocalSums[sProcTime]));
                                __m256i s32vec_AfterRounded = _mm256_add_epi32(s32vec_Sum, _mm256_set1_epi32(gauss.iround));
                                __m256i s32vec_AfterShifted = _mm256_srai_epi32(s32vec_AfterRounded, gauss.iaccuracy);
                                
                                __m128i s8vec_low = _mm256_castsi256_si128(s32vec_AfterShifted);
                                __m128i s8vec_high = _mm256_extracti128_si256(s32vec_AfterShifted, 1);
                                __m128i s16vec_PackedVal = _mm_packs_epi32(s8vec_low, s8vec_high);
                                __m128i s8vec_PackedVal = _mm_packs_epi16(s16vec_PackedVal, s16vec_PackedVal);
                                
                                _mm_storel_epi64(reinterpret_cast<__m128i*>(&s8ArrayResults[sProcTime]), s8vec_PackedVal);
                            }
                            
                            // 存储结果
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                dst[j + sProcTime] = s8ArrayResults[sProcTime];
                            }
                        }
                    }
                    
                    // 处理剩余部分
                    for (int32_t j = WIDTH_ALIGNED; j < imgin.pwin().width; ++j) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        if (specialHandler && specialHandler(&src[col], 1)) 
                        {
                            dst[j] = src[col];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = -radius; k <= radius; k++) 
                        {
                            convsum += gauss_kernel[radius + k] * src[col + k];
                        }
                        
                        int32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                        result = std::max(-128, std::min(127, result));
                        dst[j] = static_cast<int8_t>(result);
                    }
                    
                } 
                // 其他数据类型的处理
                else 
                {
                    for (int32_t j = 0; j < imgin.pwin().width; ++j) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        if (specialHandler && specialHandler(&src[col], 1)) 
                        {
                            dst[j] = src[col];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = -radius; k <= radius; k++) 
                        {
                            convsum += gauss_kernel[radius + k] * src[col + k];
                        }
                        
                        if constexpr (std::is_integral_v<_T>) 
                        {
                            if constexpr (std::is_signed_v<_T>) 
                            {
                                using wider_type = std::conditional_t<sizeof(_T) == 1, int32_t, 
                                                        std::conditional_t<sizeof(_T) == 2, int64_t, int64_t>>;
                                wider_type result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                constexpr wider_type min_val = std::numeric_limits<_T>::min();
                                constexpr wider_type max_val = std::numeric_limits<_T>::max();
                                result = std::max(min_val, std::min(max_val, result));
                                dst[j] = static_cast<_T>(result);
                            } 
                            else 
                            {
                                uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                if constexpr (std::is_same_v<_T, uint8_t>) 
                                {
                                    dst[j] = static_cast<uint8_t>(std::min(255u, result));
                                } 
                                else 
                                {
                                    dst[j] = static_cast<_T>(result);
                                }
                            }
                        } 
                        else 
                        {
                            dst[j] = convsum;
                        }
                    }
                }
            }
        }
    }

    // Y方向卷积
    #pragma omp parallel num_threads(8)
    {
        struct alignas(sCacheLineSize) AlignedRptr 
        {
            typename Imat<_T>::Rptr ptr;
        };
        std::vector<AlignedRptr> srcnb(diameter);
        
        #pragma omp for schedule(static)
        for (int32_t sRowBlock = 0; sRowBlock < imgtmp.pwin().height; sRowBlock += sCacheLineSize) 
        {
            const int32_t sActualBlockHei = std::min(sCacheLineSize, imgtmp.pwin().height - sRowBlock);
            for (int32_t sRowPerBlock = 0; sRowPerBlock < sActualBlockHei; ++sRowPerBlock) 
            {
                const int32_t sRowOffset = sRowBlock + sRowPerBlock;
                const int32_t sRow = imgtmp.pwin().y + sRowOffset;
                _T* dst = imgout.ptr(imgout.pwin().y + sRowOffset, imgout.pwin().x);
                _T* osrc = specialHandler ? imgin.ptr(sRow, imgin.pwin().x) : nullptr;
                
                // 预加载当前Block所需的所有行
                for (int32_t k = -radius; k <= radius; k++) 
                {
                    int32_t sActualRow = sRow + k;
                    if (sActualRow < 0) sActualRow = 0;
                    if (sActualRow >= imgtmp.height()) sActualRow = imgtmp.height() - 1;
                    srcnb[radius + k].ptr = imgtmp.rptr(sActualRow);
                }

                const auto& gauss_kernel = gauss.kernel;
                
                // 处理uint8_t类型的AVX2优化
                if constexpr (std::is_same_v<_T, uint8_t>) 
                {
                    const int PIXEL_PER_VECTOR = sizeof(__m256i) / sizeof(int8_t);
                    const int WIDTH_ALIGNED = (imgtmp.pwin().width / PIXEL_PER_VECTOR) * PIXEL_PER_VECTOR;
                    const int PIXELPROC_PERNUM = 8;
                    
                    for (int32_t j = 0; j < WIDTH_ALIGNED; j += PIXEL_PER_VECTOR) 
                    {
                        int32_t col = imgtmp.pwin().x + j;
                        
                        // 检查是否需要特殊处理
                        bool needSpecialHandling = false;
                        if (specialHandler && osrc) {
                            needSpecialHandling = specialHandler(&osrc[j], PIXEL_PER_VECTOR);
                        }
                        
                        if (needSpecialHandling) 
                        {
                            // 标量处理
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                if (specialHandler(&osrc[j + sProcTime], 1)) 
                                {
                                    dst[j + sProcTime] = osrc[j + sProcTime];
                                    continue;
                                }
                                
                                ktype convsum = 0;
                                for (int32_t k = 0; k < diameter; k++) 
                                {
                                    typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                                    convsum += gauss_kernel[k] * src_rptr[col + sProcTime];
                                }
                                uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                dst[j + sProcTime] = static_cast<uint8_t>(std::min(255u, result));
                            }
                        } 
                        else 
                        {
                            // AVX2向量化处理
                            alignas(PIXEL_PER_VECTOR) uint32_t u32ArrayLocalSums[PIXEL_PER_VECTOR] = {0};
                            
                            for (int32_t k = 0; k < diameter; k++) 
                            {
                                typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                                
                                for (int32_t s32ProcTime = 0; s32ProcTime < PIXEL_PER_VECTOR; s32ProcTime += PIXELPROC_PERNUM) 
                                {
                                    __m128i u8vec_DataLoad = _mm_loadl_epi64(
                                        reinterpret_cast<const __m128i*>(&src_rptr[col + s32ProcTime]));
                                    __m256i u32vec_DataLoad = _mm256_cvtepu8_epi32(u8vec_DataLoad);
                                    
                                    uint32_t u32KernelVal = static_cast<uint32_t>(gauss_kernel[k]);
                                    __m256i u32vec_KernelVal = _mm256_set1_epi32(u32KernelVal);
                                    
                                    __m256i u32vec_ProdVal = _mm256_mullo_epi32(u32vec_DataLoad, u32vec_KernelVal);
                                    __m256i u32vec_PrevSum = _mm256_loadu_si256(
                                        reinterpret_cast<const __m256i*>(&u32ArrayLocalSums[s32ProcTime]));
                                    __m256i u32vec_NewSum = _mm256_add_epi32(u32vec_PrevSum, u32vec_ProdVal);
                                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&u32ArrayLocalSums[s32ProcTime]), u32vec_NewSum);
                                }
                            }
                            
                            // 结果处理
                            alignas(PIXEL_PER_VECTOR) uint8_t u8ArrayResults[PIXEL_PER_VECTOR];
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                            {
                                __m256i u32vec_Sum = _mm256_loadu_si256(
                                    reinterpret_cast<const __m256i*>(&u32ArrayLocalSums[sProcTime]));
                                __m256i u32vec_AfterRounded = _mm256_add_epi32(u32vec_Sum, _mm256_set1_epi32(gauss.iround));
                                __m256i u32vec_AfterShifted = _mm256_srli_epi32(u32vec_AfterRounded, gauss.iaccuracy);
                                
                                __m128i u8vec_low = _mm256_castsi256_si128(u32vec_AfterShifted);
                                __m128i u8vec_high = _mm256_extracti128_si256(u32vec_AfterShifted, 1);
                                __m128i u16vec_PackedVal = _mm_packus_epi32(u8vec_low, u8vec_high);
                                __m128i u8vec_PackedVal = _mm_packus_epi16(u16vec_PackedVal, u16vec_PackedVal);
                                
                                _mm_storel_epi64(reinterpret_cast<__m128i*>(&u8ArrayResults[sProcTime]), u8vec_PackedVal);
                            }
                            
                            // 存储结果
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                dst[j + sProcTime] = u8ArrayResults[sProcTime];
                            }
                        }
                    }
                    
                    // 处理剩余部分
                    for (int32_t j = WIDTH_ALIGNED; j < imgtmp.pwin().width; ++j) 
                    {
                        int32_t col = imgtmp.pwin().x + j;
                        
                        if (specialHandler && osrc && specialHandler(&osrc[j], 1)) 
                        {
                            dst[j] = osrc[j];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = 0; k < diameter; k++) 
                        {
                            typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                            convsum += gauss_kernel[k] * src_rptr[col];
                        }
                        
                        uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                        dst[j] = static_cast<uint8_t>(std::min(255u, result));
                    }
                    
                }
                // 处理int8_t类型的AVX2优化
                else if constexpr (std::is_same_v<_T, int8_t>) 
                {
                    const int PIXEL_PER_VECTOR = sizeof(__m256i) / sizeof(int8_t);
                    const int WIDTH_ALIGNED = (imgtmp.pwin().width / PIXEL_PER_VECTOR) * PIXEL_PER_VECTOR;
                    const int PIXELPROC_PERNUM = 8;
                    
                    for (int32_t j = 0; j < WIDTH_ALIGNED; j += PIXEL_PER_VECTOR) 
                    {
                        int32_t col = imgtmp.pwin().x + j;
                        
                        // 检查是否需要特殊处理
                        bool needSpecialHandling = false;
                        if (specialHandler && osrc) {
                            needSpecialHandling = specialHandler(&osrc[j], PIXEL_PER_VECTOR);
                        }
                        
                        if (needSpecialHandling) 
                        {
                            // 标量处理
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                if (specialHandler(&osrc[j + sProcTime], 1)) 
                                {
                                    dst[j + sProcTime] = osrc[j + sProcTime];
                                    continue;
                                }
                                
                                ktype convsum = 0;
                                for (int32_t k = 0; k < diameter; k++) 
                                {
                                    typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                                    convsum += gauss_kernel[k] * src_rptr[col + sProcTime];
                                }
                                int32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                result = std::max(-128, std::min(127, result));
                                dst[j + sProcTime] = static_cast<int8_t>(result);
                            }
                        } 
                        else 
                        {
                            // AVX2向量化处理
                            alignas(PIXEL_PER_VECTOR) int32_t s32ArrayLocalSums[PIXEL_PER_VECTOR] = {0};
                            
                            for (int32_t k = 0; k < diameter; k++) 
                            {
                                typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                                for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                                {
                                    __m128i s8vec_DataLoad = _mm_loadl_epi64(
                                        reinterpret_cast<const __m128i*>(&src_rptr[col + sProcTime]));
                                    __m256i s32vec_DataLoad = _mm256_cvtepi8_epi32(s8vec_DataLoad);
                                    
                                    int32_t s32KernelVal = static_cast<int32_t>(gauss_kernel[k]);
                                    __m256i s32vec_KernelVal = _mm256_set1_epi32(s32KernelVal);
                                    
                                    __m256i s32vec_ProdVal = _mm256_mullo_epi32(s32vec_DataLoad, s32vec_KernelVal);
                                    __m256i s32vec_PrevSum = _mm256_loadu_si256(
                                        reinterpret_cast<const __m256i*>(&s32ArrayLocalSums[sProcTime]));
                                    __m256i s32vec_NewSum = _mm256_add_epi32(s32vec_PrevSum, s32vec_ProdVal);
                                    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&s32ArrayLocalSums[sProcTime]), s32vec_NewSum);
                                }
                            }
                            
                            // 结果处理
                            alignas(PIXEL_PER_VECTOR) int8_t s8ArrayResults[PIXEL_PER_VECTOR];
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                            {
                                __m256i s32vec_Sum = _mm256_loadu_si256(
                                    reinterpret_cast<const __m256i*>(&s32ArrayLocalSums[sProcTime]));
                                __m256i s32vec_AfterRounded = _mm256_add_epi32(s32vec_Sum, _mm256_set1_epi32(gauss.iround));
                                __m256i s32vec_AfterShifted = _mm256_srai_epi32(s32vec_AfterRounded, gauss.iaccuracy);
                                
                                __m128i s8vec_low = _mm256_castsi256_si128(s32vec_AfterShifted);
                                __m128i s8vec_high = _mm256_extracti128_si256(s32vec_AfterShifted, 1);
                                __m128i s16vec_PackedVal = _mm_packs_epi32(s8vec_low, s8vec_high);
                                __m128i s8vec_PackedVal = _mm_packs_epi16(s16vec_PackedVal, s16vec_PackedVal);
                                
                                _mm_storel_epi64(reinterpret_cast<__m128i*>(&s8ArrayResults[sProcTime]), s8vec_PackedVal);
                            }
                            
                            // 存储结果
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                dst[j + sProcTime] = s8ArrayResults[sProcTime];
                            }
                        }
                    }
                    
                    // 处理剩余部分
                    for (int32_t j = WIDTH_ALIGNED; j < imgtmp.pwin().width; ++j) 
                    {
                        int32_t col = imgtmp.pwin().x + j;
                        
                        if (specialHandler && osrc && specialHandler(&osrc[j], 1)) 
                        {
                            dst[j] = osrc[j];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = 0; k < diameter; k++) 
                        {
                            typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                            convsum += gauss_kernel[k] * src_rptr[col];
                        }
                        
                        int32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                        result = std::max(-128, std::min(127, result));
                        dst[j] = static_cast<int8_t>(result);
                    }
                    
                } 
                // 其他数据类型的处理
                else 
                {
                    for (int32_t j = 0; j < imgtmp.pwin().width; ++j) 
                    {
                        int32_t col = imgtmp.pwin().x + j;
                        
                        if (specialHandler && osrc && specialHandler(&osrc[j], 1)) 
                        {
                            dst[j] = osrc[j];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = 0; k < diameter; k++) 
                        {
                            typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                            convsum += gauss_kernel[k] * src_rptr[col];
                        }
                        
                        if constexpr (std::is_integral_v<_T>) 
                        {
                            if constexpr (std::is_signed_v<_T>) 
                            {
                                using wider_type = std::conditional_t<sizeof(_T) == 1, int32_t, 
                                                        std::conditional_t<sizeof(_T) == 2, int64_t, int64_t>>;
                                wider_type result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                constexpr wider_type min_val = std::numeric_limits<_T>::min();
                                constexpr wider_type max_val = std::numeric_limits<_T>::max();
                                result = std::max(min_val, std::min(max_val, result));
                                dst[j] = static_cast<_T>(result);
                            } 
                            else 
                            {
                                uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                if constexpr (std::is_same_v<_T, uint8_t>) 
                                {
                                    dst[j] = static_cast<uint8_t>(std::min(255u, result));
                                } 
                                else 
                                {
                                    dst[j] = static_cast<_T>(result);
                                }
                            }
                        } 
                        else 
                        {
                            dst[j] = convsum;
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

#elif defined(RK3588_PLATFORM)
template <typename _T, typename SpecialHandler = std::function<bool(const _T*, int32_t)>>
int32_t Filter::Gaussian(Imat<_T> &imgout, Imat<_T> &imgin, Imat<_T> &imgtmp, const int32_t radius, const float64_t sigma, SpecialHandler specialHandler)
{
    if (radius <= 0 || radius >= std::min(imgin.width(), imgin.height()) / 2) {
        return -1;
    }
    if (!(imgout.isvalid() && imgin.isvalid() && imgtmp.isvalid())) {
        return -1;
    }
    if (!(imgin.isize() == imgtmp.isize() && imgin.pwin() == imgtmp.pwin() && 
          imgout.pwin().size() == imgin.pwin().size() &&
          imgin.itype() == imgtmp.itype() && imgin.itype() == imgout.itype())) {
        return -1;
    }
    if (imgin.channels() > 1) {
        return -1;
    }

    using ktype = std::conditional_t<is_u8_v<_T> || is_u16_v<_T>, uint32_t, 
                    std::conditional_t<is_u32_v<_T>, uint64_t, 
                    std::conditional_t<is_s8_v<_T> || is_s16_v<_T>, int32_t, 
                    std::conditional_t<is_s32_v<_T>, int64_t, float64_t>>>>;

    Gauss<ktype> gauss(radius, sigma);
    const int32_t diameter = 2 * radius + 1;
    
    // 获取一级Cache缓存行大小
    constexpr int sCacheLineSize = 64;
    const int32_t ROWSMAX = std::min(imgin.pwin().y + imgin.pwin().height + radius, imgin.height());
    const int32_t ROWSTART = std::max(0, imgin.pwin().y - radius);

    // X方向
    #pragma omp parallel num_threads(4)
    {
        #pragma omp for schedule(dynamic)
        for (int32_t sRowBlock = ROWSTART; sRowBlock < ROWSMAX; sRowBlock += sCacheLineSize) 
        {
            const int32_t sActualBlockHei = std::min(sCacheLineSize, ROWSMAX - sRowBlock);
            for (int32_t sRowPerBlock = 0; sRowPerBlock < sActualBlockHei; ++sRowPerBlock) 
            {
                const int32_t sRow = sRowBlock + sRowPerBlock;
                _T* dst = imgtmp.ptr(sRow, imgtmp.pwin().x);
                typename Imat<_T>::Rptr&& src = imgin.rptr(sRow);
                const auto& gauss_kernel = gauss.kernel;
                
                // 处理uint8_t类型的NEON优化
                if constexpr (std::is_same_v<_T, uint8_t>) 
                {
                    const int PIXEL_PER_VECTOR = 16; // NEON 128位寄存器处理16个uint8_t
                    const int WIDTH_ALIGNED = (imgin.pwin().width / PIXEL_PER_VECTOR) * PIXEL_PER_VECTOR;
                    const int PIXELPROC_PERNUM = 4; // 每次处理4个像素（32位精度）
                    
                    for (int32_t j = 0; j < WIDTH_ALIGNED; j += PIXEL_PER_VECTOR) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        // 检查是否需要特殊处理
                        bool needSpecialHandling = false;
                        if (specialHandler) {
                            needSpecialHandling = specialHandler(&src[col], PIXEL_PER_VECTOR);
                        }
                        
                        if (needSpecialHandling) 
                        {
                            // 标量处理
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                if (specialHandler(&src[col + sProcTime], 1)) 
                                {
                                    dst[j + sProcTime] = src[col + sProcTime];
                                    continue;
                                }
                                
                                ktype convsum = 0;
                                for (int32_t k = -radius; k <= radius; k++) 
                                {
                                    convsum += gauss_kernel[radius + k] * src[col + sProcTime + k];
                                }
                                uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                dst[j + sProcTime] = static_cast<uint8_t>(std::min(255u, result));
                            }
                        } 
                        else 
                        {
                            // ARM NEON 向量化处理 - 使用32位累加器避免溢出
                            alignas(16) uint32_t u32ArrayLocalSums[PIXEL_PER_VECTOR] = {0};
                            
                            for (int32_t k = -radius; k <= radius; k++) 
                            {
                                for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                                {
                                    // 加载4个uint8_t并零扩展为uint32_t
                                    uint8x8_t u8vec_data = vld1_u8(&src[col + sProcTime + k]);
                                    uint32x4_t u32vec_data = vmovl_u16(vget_low_u16(vmovl_u8(u8vec_data)));
                                    
                                    uint32_t u32KernelVal = static_cast<uint32_t>(gauss_kernel[radius + k]);
                                    uint32x4_t u32vec_kernel = vdupq_n_u32(u32KernelVal);
                                    
                                    // 乘加运算
                                    uint32x4_t u32vec_prod = vmulq_u32(u32vec_data, u32vec_kernel);
                                    uint32x4_t u32vec_prev_sum = vld1q_u32(&u32ArrayLocalSums[sProcTime]);
                                    uint32x4_t u32vec_new_sum = vaddq_u32(u32vec_prev_sum, u32vec_prod);
                                    vst1q_u32(&u32ArrayLocalSums[sProcTime], u32vec_new_sum);
                                }
                            }
                            
                            // 结果处理
                            alignas(16) uint8_t u8ArrayResults[PIXEL_PER_VECTOR];
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                            {
                                uint32x4_t u32vec_sum = vld1q_u32(&u32ArrayLocalSums[sProcTime]);
                                uint32x4_t u32vec_round = vdupq_n_u32(gauss.iround);
                                uint32x4_t u32vec_after_rounded = vaddq_u32(u32vec_sum, u32vec_round);
                                uint32x4_t u32vec_after_shifted = vshrq_n_u32(u32vec_after_rounded, gauss.iaccuracy);
                                
                                // 饱和缩放到8位
                                uint16x4_t u16vec_result = vqmovn_u32(u32vec_after_shifted);
                                uint8x8_t u8vec_result = vqmovn_u16(vcombine_u16(u16vec_result, u16vec_result));
                                
                                // 存储4个结果
                                vst1_lane_u32(reinterpret_cast<uint32_t*>(&u8ArrayResults[sProcTime]), 
                                             vreinterpret_u32_u8(u8vec_result), 0);
                            }
                            
                            // 存储结果
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                dst[j + sProcTime] = u8ArrayResults[sProcTime];
                            }
                        }
                    }
                    
                    // 处理剩余未对齐部分
                    for (int32_t j = WIDTH_ALIGNED; j < imgin.pwin().width; ++j) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        if (specialHandler && specialHandler(&src[col], 1)) 
                        {
                            dst[j] = src[col];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = -radius; k <= radius; k++) 
                        {
                            convsum += gauss_kernel[radius + k] * src[col + k];
                        }
                        
                        uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                        dst[j] = static_cast<uint8_t>(std::min(255u, result));
                    }
                    
                }
                // 处理int8_t类型的NEON优化
                else if constexpr (std::is_same_v<_T, int8_t>) 
                {
                    const int PIXEL_PER_VECTOR = 16;
                    const int WIDTH_ALIGNED = (imgin.pwin().width / PIXEL_PER_VECTOR) * PIXEL_PER_VECTOR;
                    const int PIXELPROC_PERNUM = 4;
                    
                    for (int32_t j = 0; j < WIDTH_ALIGNED; j += PIXEL_PER_VECTOR) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        // 检查是否需要特殊处理
                        bool needSpecialHandling = false;
                        if (specialHandler) {
                            needSpecialHandling = specialHandler(&src[col], PIXEL_PER_VECTOR);
                        }
                        
                        if (needSpecialHandling) 
                        {
                            // 标量处理
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                if (specialHandler(&src[col + sProcTime], 1)) 
                                {
                                    dst[j + sProcTime] = src[col + sProcTime];
                                    continue;
                                }
                                
                                ktype convsum = 0;
                                for (int32_t k = -radius; k <= radius; k++) 
                                {
                                    convsum += gauss_kernel[radius + k] * src[col + sProcTime + k];
                                }
                                int32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                result = std::max(-128, std::min(127, result));
                                dst[j + sProcTime] = static_cast<int8_t>(result);
                            }
                        } 
                        else 
                        {
                            // ARM NEON 向量化处理 - 使用32位累加器
                            alignas(16) int32_t s32ArrayLocalSums[PIXEL_PER_VECTOR] = {0};
                            
                            for (int32_t k = -radius; k <= radius; k++) 
                            {
                                for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                                {
                                    // 加载4个int8_t并符号扩展为int32_t
                                    int8x8_t s8vec_data = vld1_s8(reinterpret_cast<const int8_t*>(&src[col + sProcTime + k]));
                                    int32x4_t s32vec_data = vmovl_s16(vget_low_s16(vmovl_s8(s8vec_data)));
                                    
                                    int32_t s32KernelVal = static_cast<int32_t>(gauss_kernel[radius + k]);
                                    int32x4_t s32vec_kernel = vdupq_n_s32(s32KernelVal);
                                    
                                    // 乘加运算
                                    int32x4_t s32vec_prod = vmulq_s32(s32vec_data, s32vec_kernel);
                                    int32x4_t s32vec_prev_sum = vld1q_s32(&s32ArrayLocalSums[sProcTime]);
                                    int32x4_t s32vec_new_sum = vaddq_s32(s32vec_prev_sum, s32vec_prod);
                                    vst1q_s32(&s32ArrayLocalSums[sProcTime], s32vec_new_sum);
                                }
                            }
                            
                            // 结果处理
                            alignas(16) int8_t s8ArrayResults[PIXEL_PER_VECTOR];
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                            {
                                int32x4_t s32vec_sum = vld1q_s32(&s32ArrayLocalSums[sProcTime]);
                                int32x4_t s32vec_round = vdupq_n_s32(gauss.iround);
                                int32x4_t s32vec_after_rounded = vaddq_s32(s32vec_sum, s32vec_round);
                                int32x4_t s32vec_after_shifted = vshrq_n_s32(s32vec_after_rounded, gauss.iaccuracy);
                                
                                // 饱和缩放到8位
                                int16x4_t s16vec_result = vqmovn_s32(s32vec_after_shifted);
                                int8x8_t s8vec_result = vqmovn_s16(vcombine_s16(s16vec_result, s16vec_result));
                                
                                // 存储4个结果
                                vst1_lane_s32(reinterpret_cast<int32_t*>(&s8ArrayResults[sProcTime]), 
                                             vreinterpret_s32_s8(s8vec_result), 0);
                            }
                            
                            // 存储结果
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                dst[j + sProcTime] = s8ArrayResults[sProcTime];
                            }
                        }
                    }
                    
                    // 处理剩余部分
                    for (int32_t j = WIDTH_ALIGNED; j < imgin.pwin().width; ++j) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        if (specialHandler && specialHandler(&src[col], 1)) 
                        {
                            dst[j] = src[col];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = -radius; k <= radius; k++) 
                        {
                            convsum += gauss_kernel[radius + k] * src[col + k];
                        }
                        
                        int32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                        result = std::max(-128, std::min(127, result));
                        dst[j] = static_cast<int8_t>(result);
                    }
                    
                } 
                // 其他数据类型的处理
                else 
                {
                    for (int32_t j = 0; j < imgin.pwin().width; ++j) 
                    {
                        int32_t col = imgin.pwin().x + j;
                        
                        if (specialHandler && specialHandler(&src[col], 1)) 
                        {
                            dst[j] = src[col];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = -radius; k <= radius; k++) 
                        {
                            convsum += gauss_kernel[radius + k] * src[col + k];
                        }
                        
                        if constexpr (std::is_integral_v<_T>) 
                        {
                            if constexpr (std::is_signed_v<_T>) 
                            {
                                using wider_type = std::conditional_t<sizeof(_T) == 1, int32_t, 
                                                        std::conditional_t<sizeof(_T) == 2, int64_t, int64_t>>;
                                wider_type result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                constexpr wider_type min_val = std::numeric_limits<_T>::min();
                                constexpr wider_type max_val = std::numeric_limits<_T>::max();
                                result = std::max(min_val, std::min(max_val, result));
                                dst[j] = static_cast<_T>(result);
                            } 
                            else 
                            {
                                uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                if constexpr (std::is_same_v<_T, uint8_t>) 
                                {
                                    dst[j] = static_cast<uint8_t>(std::min(255u, result));
                                } 
                                else 
                                {
                                    dst[j] = static_cast<_T>(result);
                                }
                            }
                        } 
                        else 
                        {
                            dst[j] = convsum;
                        }
                    }
                }
            }
        }
    }

    // Y方向卷积 - 采用与X方向相同的优化策略
    #pragma omp parallel num_threads(4)
    {
        struct alignas(sCacheLineSize) AlignedRptr 
        {
            typename Imat<_T>::Rptr ptr;
        };
        std::vector<AlignedRptr> srcnb(diameter);
        
        #pragma omp for schedule(dynamic)
        for (int32_t sRowBlock = 0; sRowBlock < imgtmp.pwin().height; sRowBlock += sCacheLineSize) 
        {
            const int32_t sActualBlockHei = std::min(sCacheLineSize, imgtmp.pwin().height - sRowBlock);
            for (int32_t sRowPerBlock = 0; sRowPerBlock < sActualBlockHei; ++sRowPerBlock) 
            {
                const int32_t sRowOffset = sRowBlock + sRowPerBlock;
                const int32_t sRow = imgtmp.pwin().y + sRowOffset;
                _T* dst = imgout.ptr(imgout.pwin().y + sRowOffset, imgout.pwin().x);
                _T* osrc = specialHandler ? imgin.ptr(sRow, imgin.pwin().x) : nullptr;
                
                // 预加载当前Block所需的所有行
                for (int32_t k = -radius; k <= radius; k++) 
                {
                    int32_t sActualRow = sRow + k;
                    if (sActualRow < 0) sActualRow = 0;
                    if (sActualRow >= imgtmp.height()) sActualRow = imgtmp.height() - 1;
                    srcnb[radius + k].ptr = imgtmp.rptr(sActualRow);
                }

                const auto& gauss_kernel = gauss.kernel;
                
                if constexpr (std::is_same_v<_T, uint8_t>) 
                {
                    const int PIXEL_PER_VECTOR = 16;
                    const int WIDTH_ALIGNED = (imgtmp.pwin().width / PIXEL_PER_VECTOR) * PIXEL_PER_VECTOR;
                    const int PIXELPROC_PERNUM = 4;
                    
                    for (int32_t j = 0; j < WIDTH_ALIGNED; j += PIXEL_PER_VECTOR) 
                    {
                        int32_t col = imgtmp.pwin().x + j;
                        
                        // 检查是否需要特殊处理
                        bool needSpecialHandling = false;
                        if (specialHandler && osrc) {
                            needSpecialHandling = specialHandler(&osrc[j], PIXEL_PER_VECTOR);
                        }
                        
                        if (needSpecialHandling) 
                        {
                            // 标量处理
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                if (specialHandler(&osrc[j + sProcTime], 1)) 
                                {
                                    dst[j + sProcTime] = osrc[j + sProcTime];
                                    continue;
                                }
                                
                                ktype convsum = 0;
                                for (int32_t k = 0; k < diameter; k++) 
                                {
                                    typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                                    convsum += gauss_kernel[k] * src_rptr[col + sProcTime];
                                }
                                uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                dst[j + sProcTime] = static_cast<uint8_t>(std::min(255u, result));
                            }
                        } 
                        else 
                        {
                            // ARM NEON 向量化处理
                            alignas(16) uint32_t u32ArrayLocalSums[PIXEL_PER_VECTOR] = {0};
                            
                            for (int32_t k = 0; k < diameter; k++) 
                            {
                                typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                                
                                for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                                {
                                    // 加载4个uint8_t并零扩展为uint32_t
                                    uint8x8_t u8vec_data = vld1_u8(&src_rptr[col + sProcTime]);
                                    uint32x4_t u32vec_data = vmovl_u16(vget_low_u16(vmovl_u8(u8vec_data)));
                                    
                                    uint32_t u32KernelVal = static_cast<uint32_t>(gauss_kernel[k]);
                                    uint32x4_t u32vec_kernel = vdupq_n_u32(u32KernelVal);
                                    
                                    // 乘加运算
                                    uint32x4_t u32vec_prod = vmulq_u32(u32vec_data, u32vec_kernel);
                                    uint32x4_t u32vec_prev_sum = vld1q_u32(&u32ArrayLocalSums[sProcTime]);
                                    uint32x4_t u32vec_new_sum = vaddq_u32(u32vec_prev_sum, u32vec_prod);
                                    vst1q_u32(&u32ArrayLocalSums[sProcTime], u32vec_new_sum);
                                }
                            }
                            
                            // 结果处理
                            alignas(16) uint8_t u8ArrayResults[PIXEL_PER_VECTOR];
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime += PIXELPROC_PERNUM) 
                            {
                                uint32x4_t u32vec_sum = vld1q_u32(&u32ArrayLocalSums[sProcTime]);
                                uint32x4_t u32vec_round = vdupq_n_u32(gauss.iround);
                                uint32x4_t u32vec_after_rounded = vaddq_u32(u32vec_sum, u32vec_round);
                                uint32x4_t u32vec_after_shifted = vshrq_n_u32(u32vec_after_rounded, gauss.iaccuracy);
                                
                                // 饱和缩放到8位
                                uint16x4_t u16vec_result = vqmovn_u32(u32vec_after_shifted);
                                uint8x8_t u8vec_result = vqmovn_u16(vcombine_u16(u16vec_result, u16vec_result));
                                
                                vst1_lane_u32(reinterpret_cast<uint32_t*>(&u8ArrayResults[sProcTime]), 
                                             vreinterpret_u32_u8(u8vec_result), 0);
                            }
                            
                            // 存储结果
                            for (int32_t sProcTime = 0; sProcTime < PIXEL_PER_VECTOR; sProcTime++) 
                            {
                                dst[j + sProcTime] = u8ArrayResults[sProcTime];
                            }
                        }
                    }
                    
                    // 处理剩余部分
                    for (int32_t j = WIDTH_ALIGNED; j < imgtmp.pwin().width; ++j) 
                    {
                        int32_t col = imgtmp.pwin().x + j;
                        
                        if (specialHandler && osrc && specialHandler(&osrc[j], 1)) 
                        {
                            dst[j] = osrc[j];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = 0; k < diameter; k++) 
                        {
                            typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                            convsum += gauss_kernel[k] * src_rptr[col];
                        }
                        
                        uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                        dst[j] = static_cast<uint8_t>(std::min(255u, result));
                    }
                    
                }
                else 
                {
                    for (int32_t j = 0; j < imgtmp.pwin().width; ++j) 
                    {
                        int32_t col = imgtmp.pwin().x + j;
                        
                        if (specialHandler && osrc && specialHandler(&osrc[j], 1)) 
                        {
                            dst[j] = osrc[j];
                            continue;
                        }
                        
                        ktype convsum = 0;
                        for (int32_t k = 0; k < diameter; k++) 
                        {
                            typename Imat<_T>::Rptr& src_rptr = srcnb[k].ptr;
                            convsum += gauss_kernel[k] * src_rptr[col];
                        }
                        
                        if constexpr (std::is_integral_v<_T>) 
                        {
                            if constexpr (std::is_signed_v<_T>) 
                            {
                                using wider_type = std::conditional_t<sizeof(_T) == 1, int32_t, 
                                                        std::conditional_t<sizeof(_T) == 2, int64_t, int64_t>>;
                                wider_type result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                constexpr wider_type min_val = std::numeric_limits<_T>::min();
                                constexpr wider_type max_val = std::numeric_limits<_T>::max();
                                result = std::max(min_val, std::min(max_val, result));
                                dst[j] = static_cast<_T>(result);
                            } 
                            else 
                            {
                                uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                                if constexpr (std::is_same_v<_T, uint8_t>) 
                                {
                                    dst[j] = static_cast<uint8_t>(std::min(255u, result));
                                } 
                                else 
                                {
                                    dst[j] = static_cast<_T>(result);
                                }
                            }
                        } 
                        else 
                        {
                            dst[j] = convsum;
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

#else
template <typename _T, typename SpecialHandler>
int32_t Filter::Gaussian(Imat<_T> &imgout, Imat<_T> &imgin, Imat<_T> &imgtmp, 
                        const int32_t radius, const float64_t sigma, SpecialHandler specialHandler)
{
    if (radius <= 0 || radius >= std::min(imgin.width(), imgin.height()) / 2) {
        return -1;
    }
    if (!(imgout.isvalid() && imgin.isvalid() && imgtmp.isvalid())) {
        return -1;
    }
    if (!(imgin.isize() == imgtmp.isize() && imgin.pwin() == imgtmp.pwin() && 
          imgout.pwin().size() == imgin.pwin().size() &&
          imgin.itype() == imgtmp.itype() && imgin.itype() == imgout.itype())) {
        return -1;
    }
    if (imgin.channels() > 1) {
        return -1;
    }

    using ktype = std::conditional_t<is_u8_v<_T> || is_u16_v<_T>, uint32_t, 
                    std::conditional_t<is_u32_v<_T>, uint64_t, 
                    std::conditional_t<is_s8_v<_T> || is_s16_v<_T>, int32_t, 
                    std::conditional_t<is_s32_v<_T>, int64_t, float64_t>>>>;

    Gauss<ktype> gauss(radius, sigma);
    const int32_t diameter = 2 * radius + 1;
    
    // 标量实现 - X方向卷积
    for (int32_t row = std::max(0, imgin.pwin().y - radius); 
         row < std::min(imgin.pwin().y + imgin.pwin().height + radius, imgin.height()); 
         ++row)
    {
        _T* dst = imgtmp.ptr(row, imgtmp.pwin().x);
        typename Imat<_T>::Rptr&& src = imgin.rptr(row);

        for (int32_t j = 0; j < imgin.pwin().width; ++j)
        {
            int32_t col = imgin.pwin().x + j;
            
            // 检查是否需要特殊处理
            if (specialHandler && specialHandler(&src[col], 1)) 
            {
                dst[j] = src[col];
                continue;
            }
            
            ktype convsum = 0;
            for (int32_t k = -radius; k <= radius; k++)
            {
                convsum += gauss.kernel[radius + k] * src[col + k];
            }
            
            // 结果处理
            if constexpr (std::is_integral_v<_T>) 
            {
                if constexpr (std::is_signed_v<_T>) 
                {
                    using wider_type = std::conditional_t<sizeof(_T) == 1, int32_t, 
                                            std::conditional_t<sizeof(_T) == 2, int64_t, int64_t>>;
                    wider_type result = (convsum + gauss.iround) >> gauss.iaccuracy;
                    constexpr wider_type min_val = std::numeric_limits<_T>::min();
                    constexpr wider_type max_val = std::numeric_limits<_T>::max();
                    result = std::max(min_val, std::min(max_val, result));
                    dst[j] = static_cast<_T>(result);
                } 
                else 
                {
                    uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                    if constexpr (std::is_same_v<_T, uint8_t>) 
                    {
                        dst[j] = static_cast<uint8_t>(std::min(255u, result));
                    } 
                    else 
                    {
                        dst[j] = static_cast<_T>(result);
                    }
                }
            } 
            else 
            {
                dst[j] = static_cast<_T>(convsum);
            }
        }
    }

    // 标量实现 - Y方向卷积
    for (int32_t i = 0; i < imgtmp.pwin().height; ++i)
    {
        const int32_t row = imgtmp.pwin().y + i;
        _T* dst = imgout.ptr(imgout.pwin().y + i, imgout.pwin().x);
        _T* osrc = specialHandler ? imgin.ptr(row, imgin.pwin().x) : nullptr;
        
        // 预加载当前行所需的所有行
        std::vector<typename Imat<_T>::Rptr> srcnb(diameter);
        for (int32_t k = -radius; k <= radius; k++)
        {
            int32_t actual_row = row + k;
            if (actual_row < 0) actual_row = 0;
            if (actual_row >= imgtmp.height()) actual_row = imgtmp.height() - 1;
            srcnb[radius + k] = imgtmp.rptr(actual_row);
        }

        for (int32_t j = 0; j < imgtmp.pwin().width; ++j)
        {
            int32_t col = imgtmp.pwin().x + j;
            
            // 检查是否需要特殊处理
            if (specialHandler && osrc && specialHandler(&osrc[j], 1)) 
            {
                dst[j] = osrc[j];
                continue;
            }
            
            ktype convsum = 0;
            for (int32_t k = 0; k < diameter; k++)
            {
                convsum += gauss.kernel[k] * srcnb[k][col];
            }
            
            // 结果处理
            if constexpr (std::is_integral_v<_T>) 
            {
                if constexpr (std::is_signed_v<_T>) 
                {
                    using wider_type = std::conditional_t<sizeof(_T) == 1, int32_t, 
                                            std::conditional_t<sizeof(_T) == 2, int64_t, int64_t>>;
                    wider_type result = (convsum + gauss.iround) >> gauss.iaccuracy;
                    constexpr wider_type min_val = std::numeric_limits<_T>::min();
                    constexpr wider_type max_val = std::numeric_limits<_T>::max();
                    result = std::max(min_val, std::min(max_val, result));
                    dst[j] = static_cast<_T>(result);
                } 
                else 
                {
                    uint32_t result = (convsum + gauss.iround) >> gauss.iaccuracy;
                    if constexpr (std::is_same_v<_T, uint8_t>) 
                    {
                        dst[j] = static_cast<uint8_t>(std::min(255u, result));
                    } 
                    else 
                    {
                        dst[j] = static_cast<_T>(result);
                    }
                }
            } 
            else 
            {
                dst[j] = static_cast<_T>(convsum);
            }
        }
    }
    
    return 0;
}

#endif


/**
 * @fn      Mean
 * @brief   均值滤波（无硬件加速）
 * 
 * @param   [OUT] imgout 输出图像，数据类型、处理区域尺寸需和imgin相同
 * @param   [IN] imgin 输入图像，仅支持单通道，仅对pwin()区域做处理
 * @param   [IN] imgtmp 临时图像内存，数据类型、整图尺寸、处理区域尺寸需和imgin相同
 * @param   [IN] radius 滤波核半径，仅支持方形滤波核
 * 
 * @return  int32_t 0：处理正常，其他：异常
 */
template <typename _T>
int32_t Filter::Mean(Imat<_T> &imgout, Imat<_T> &imgin, Imat<_T> &imgtmp, const int32_t radius)
{
    if (radius <= 0 || radius >= std::min(imgin.width(), imgin.height()) / 2)
    {
        return -1;
    }

    if (!(imgout.isvalid() && imgin.isvalid() && imgtmp.isvalid()))
    {
        return -1;
    }

    if (!(imgin.isize() == imgtmp.isize() && imgin.pwin() == imgtmp.pwin() && imgout.pwin().size() == imgin.pwin().size() &&
          imgin.itype() == imgtmp.itype() && imgin.itype() == imgout.itype()))
    {
        return -1;
    }

    if (imgin.channels() > 1) // 暂不支持多通道图像
    {
        return -1;
    }

    const int32_t diameter = 2 * radius + 1;
    const int32_t center = radius + 1;
    using ktype = std::conditional_t<is_u8_v<_T> || is_u16_v<_T>, uint32_t,
                  std::conditional_t<is_u32_v<_T>, uint64_t,
                  std::conditional_t<is_s8_v<_T> || is_s16_v<_T>, int32_t,
                  std::conditional_t<is_s32_v<_T>, int64_t, 
                                     float64_t>>>>;

	// X方向
    const int32_t ROWSMAX = std::min(imgin.pwin().y + imgin.pwin().height + radius, imgin.height());
	for (int32_t sRow = subcf0(imgin.pwin().y, radius); sRow < ROWSMAX; ++sRow)
	{
        typename Imat<_T>::Rptr&& src = imgin.rptr(sRow);
        _T* dst = imgtmp.ptr(sRow, imgtmp.pwin().x);

        // 初始化第0个像素点的均值
        ktype conv_sum = std::is_integral_v<_T> ? radius : 0;
        for (int32_t i = -radius; i <= radius; i++)
        {
            conv_sum += src[imgin.pwin().x + i];
        }
        *dst++ = conv_sum / diameter;

        // 迭代计算后续像素点的均值
        for (int32_t j = 1, colout = imgin.pwin().x - radius, colin = imgin.pwin().x + center; j < imgin.pwin().width; ++j, ++colout, ++colin, ++dst)
		{
            conv_sum = conv_sum - src[colout] + src[colin];
            *dst = conv_sum / diameter;
		}
	}

    // Y方向
    // 初始化第0行像素点的均值
    typename Imat<_T>::Rptr srcnb[diameter]; // 为每行创建一个指针，便于列遍历
    for (int32_t k = -radius; k <= radius; k++)
    {
        srcnb[radius + k] = imgtmp.rptr(imgtmp.pwin().y + k);
    }
    _T* dst = imgout.ptr(imgout.pwin().y, imgout.pwin().x);
    std::vector<ktype> conv_sum(imgtmp.pwin().width, std::is_integral_v<_T> ? radius : 0);
    int32_t col = imgtmp.pwin().x;
    for (auto& elem : conv_sum)
    {
        for (int32_t k = 0; k < diameter; k++)
        {
            elem += srcnb[k][col];
        }
        *dst++ = elem / diameter;
        ++col;
    }

    // 迭代计算后续行像素点的均值
    for (int32_t j = 1, rowout = imgtmp.pwin().y - radius, rowin = imgtmp.pwin().y + center; j < imgtmp.pwin().height; ++j, ++rowout, ++rowin, ++dst)
    {
        typename Imat<_T>::Rptr&& srcout = imgtmp.rptr(rowout);
        typename Imat<_T>::Rptr&& srcin = imgtmp.rptr(rowin);
        _T* dst = imgout.ptr(imgout.pwin().y+j, imgout.pwin().x);
        int32_t col = imgtmp.pwin().x;
        for (auto& elem : conv_sum)
        {
            elem = elem - srcout[col] + srcin[col];
            *dst++ = elem / diameter;
            ++col;
        }
    }

    return 0;
}


/**
 * @fn      MeanX
 * @brief   X方向均值滤波（无硬件加速）
 * 
 * @param   [OUT] imgout 输出图像，数据类型、处理区域尺寸需和imgin相同
 * @param   [IN] imgin 输入图像，仅支持单通道，仅对pwin()区域做处理
 * @param   [IN] radius 滤波核半径，仅支持方形滤波核
 * 
 * @return  int32_t 0：处理正常，其他：异常
 */
template <typename _T>
int32_t Filter::MeanX(Imat<_T> &imgout, Imat<_T> &imgin, const int32_t radius)
{
    if (radius <= 0 || radius >= std::min(imgin.width(), imgin.height()) / 2)
    {
        return -1;
    }

    if (!(imgout.isvalid() && imgin.isvalid()))
    {
        return -1;
    }

    if (!(imgout.pwin().size() == imgin.pwin().size() && imgin.itype() == imgout.itype()))
    {
        return -1;
    }

    if (imgin.channels() > 1) // 暂不支持多通道图像
    {
        return -1;
    }

    const int32_t diameter = 2 * radius + 1;
    const int32_t center = radius + 1;
    using ktype = std::conditional_t<is_u8_v<_T> || is_u16_v<_T>, uint32_t,
                  std::conditional_t<is_u32_v<_T>, uint64_t,
                  std::conditional_t<is_s8_v<_T> || is_s16_v<_T>, int32_t,
                  std::conditional_t<is_s32_v<_T>, int64_t, 
                                     float64_t>>>>;

	for (int32_t i = 0, rowi = imgin.pwin().y, rowo = imgout.pwin().y; i < imgin.pwin().height; ++i, ++rowi, ++rowo)
	{
        typename Imat<_T>::Rptr&& src = imgin.rptr(rowi);
        _T* dst = imgout.ptr(rowo, imgout.pwin().x);

        // 初始化第0个像素点的均值
        ktype conv_sum = std::is_integral_v<_T> ? radius : 0;
        for (int32_t i = -radius; i <= radius; i++)
        {
            conv_sum += src[imgin.pwin().x + i];
        }
        *dst++ = conv_sum / diameter;

        // 迭代计算后续像素点的均值
        for (int32_t j = 1, colout = imgin.pwin().x - radius, colin = imgin.pwin().x + center; j < imgin.pwin().width; ++j, ++colout, ++colin, ++dst)
		{
            conv_sum = conv_sum - src[colout] + src[colin];
            *dst = conv_sum / diameter;
		}
	}

    return 0;
}



/**
 * @fn      MeanY
 * @brief   Y方向均值滤波（无硬件加速）
 * 
 * @param   [OUT] imgout 输出图像，数据类型、处理区域尺寸需和imgin相同
 * @param   [IN] imgin 输入图像，仅支持单通道，仅对pwin()区域做处理
 * @param   [IN] radius 滤波核半径，仅支持方形滤波核
 * 
 * @return  int32_t 0：处理正常，其他：异常
 */
template <typename _T>
int32_t Filter::MeanY(Imat<_T> &imgout, Imat<_T> &imgin, const int32_t radius)
{
    if (radius <= 0 || radius >= std::min(imgin.width(), imgin.height()) / 2)
    {
        return -1;
    }

    if (!(imgout.isvalid() && imgin.isvalid()))
    {
        return -1;
    }

    if (!(imgout.pwin().size() == imgin.pwin().size() && imgin.itype() == imgout.itype()))
    {
        return -1;
    }

    if (imgin.channels() > 1) // 暂不支持多通道图像
    {
        return -1;
    }

    const int32_t diameter = 2 * radius + 1;
    const int32_t center = radius + 1;
    using ktype = std::conditional_t<is_u8_v<_T> || is_u16_v<_T>, uint32_t,
                  std::conditional_t<is_u32_v<_T>, uint64_t,
                  std::conditional_t<is_s8_v<_T> || is_s16_v<_T>, int32_t,
                  std::conditional_t<is_s32_v<_T>, int64_t, 
                                     float64_t>>>>;

    typename Imat<_T>::Rptr srcnb[diameter]; // 为每行创建一个指针，便于列遍历
    for (int32_t k = -radius; k <= radius; k++)
    {
        srcnb[radius + k] = imgin.rptr(imgin.pwin().y + k);
    }
    _T* dst = imgout.ptr(imgout.pwin().y, imgout.pwin().x);

    // 初始化第0行像素点的均值
    std::vector<ktype> conv_sum(imgin.pwin().width, std::is_integral_v<_T> ? radius : 0);
    int32_t col = imgin.pwin().x;
    for (auto& elem : conv_sum)
    {
        for (int32_t k = 0; k < diameter; k++)
        {
            elem += srcnb[k][col];
        }
        *dst++ = elem / diameter;
        ++col;
    }

    // 迭代计算后续行像素点的均值
    for (int32_t j = 1, rowout = imgin.pwin().y - radius, rowin = imgin.pwin().y + center; j < imgin.pwin().height; ++j, ++rowout, ++rowin, ++dst)
    {
        typename Imat<_T>::Rptr&& srcout = imgin.rptr(rowout);
        typename Imat<_T>::Rptr&& srcin = imgin.rptr(rowin);
        _T* dst = imgout.ptr(imgout.pwin().y+j, imgout.pwin().x);
        int32_t col = imgin.pwin().x;
        for (auto& elem : conv_sum)
        {
            elem = elem - srcout[col] + srcin[col];
            *dst++ = elem / diameter;
            ++col;
        }
    }

    return 0;
}


/**
 * @fn      Dilate
 * @brief   对符合isDilating条件的值做膨胀，膨胀后图像背景值为bgfg.first，前景值为bgfg.second
 * 
 * @param   [OUT] imgout 输出图像，数据类型、处理区域尺寸需和imgin相同
 * @param   [IN] imgin 输入图像，仅支持单通道，仅对pwin()区域做处理
 * @param   [IN] radius 滤波核半径，仅支持方形滤波核
 * @param   [IN] isDilating 输入图像中像素值是否需要做膨胀的Lambda判断函数
 * @param   [IN] bgfg 输出图像的背景前景值，first：背景，second：前景，两者不可相等
 *  
 * @return  int32_t 0：处理正常，其他：异常
 */
template <typename _T, typename _P>
int32_t Filter::Dilate(Imat<_T> &imgout, Imat<_P> &imgin, const int32_t radius, std::function<bool(const _P)> isDilating, std::pair<_T, _T> bgfg)
{
    if (radius <= 0)
    {
        return -1;
    }

    if (!(imgout.isvalid() && imgin.isvalid()))
    {
        return -1;
    }

    if (imgout.pwin().size() != imgin.pwin().size())
    {
        return -1;
    }

    if (imgin.channels() > 1) // 暂不支持多通道图像
    {
        return -1;
    }
 
    if (bgfg.first == bgfg.second) // 背景值不能与前景值相同
    {
        return -1;
    }

    const int32_t diameter = 2 * radius + 1;
    const _T bgval = bgfg.first, fgval = bgfg.second; // background && foreground
    const _T mgval = ((std::is_integral_v<_T>) && subabs(bgfg.first, bgfg.second) == 1) ? // middle-ground，运算的中间变量
        (std::max(bgfg.first, bgfg.second) + 1) : (bgfg.first + bgfg.second) / 2;

    if (std::min(imgin.pwin().width, imgin.pwin().height) <= radius * 2) // 小区域直接逐像素处理
    {
        std::vector<typename Imat<_P>::Rptr> rSrc(diameter);
        auto _dilate4pixel = [&radius, &diameter, &isDilating, &rSrc](const int32_t x) -> bool
        {
            for (int32_t m = 0; m < diameter; ++m)
            {
                for (int32_t n = -radius; n <= radius; ++n)
                {
                    if (isDilating(rSrc[m][x+n]))
                    {
                        return true;
                    }
                }
            }
            return false;
        };
        for (int32_t i = 0, y = imgin.pwin().y; i < imgout.pwin().height; ++i, ++y)
        {
            for (int32_t k = 0; k < diameter; ++k)
            {
                rSrc[k] = imgin.rptr(y - radius + k);
            }
            _T* pDst = imgout.ptr(imgout.pwin().y + i, imgout.pwin().x);
            for (int32_t j = 0; j < imgout.pwin().width; ++j, ++pDst)
            {
                *pDst = _dilate4pixel(imgin.pwin().x + j) ? fgval : bgval;
            }
        }

        return 0;
    }

    /// X方向
    const std::pair<int32_t, int32_t> hnb{std::min(radius, imgin.get_hpads().first), std::min(radius, imgin.get_hpads().second)}; // 左右邻域大小
    const std::pair<int32_t, int32_t> vnb{std::min(radius, imgin.get_vpads().first), std::min(radius, imgin.get_vpads().second)}; // 上下邻域大小
    std::unique_ptr<_T[]> vnb_mem{nullptr};
    if (vnb.first + vnb.second > 0)
    {
        vnb_mem = std::make_unique<_T[]>(imgout.pwin().width * (vnb.first + vnb.second));
        if (nullptr == vnb_mem)
        {
            return -1;
        }
    }

    const int32_t tidx = imgin.pwin().y - vnb.first, bidx = imgin.pwin().y + imgin.pwin().height + vnb.second; // top and bottom index
    const int32_t boxb = imgin.pwin().y + imgin.pwin().height; // box bottom
    const int32_t htrav1 = hnb.first + imgin.pwin().width - radius, htrav2 = radius + hnb.second; //  Horizontal TRAVerse 1st and 2nd Times
    const int32_t vtrav1 = vnb.first + imgout.pwin().height - radius, vtrav2 = radius + vnb.second;

    for (int32_t rowi = tidx, rowo = imgout.pwin().y; rowi < bidx; ++rowi)
	{
        _T* dst = nullptr;
        if (rowi < imgin.pwin().y)
        {
            dst = (_T*)vnb_mem.get() + imgout.pwin().width * (vnb.first - (imgin.pwin().y - rowi));
        }
        else if (rowi >= boxb)
        {
            dst = (_T*)vnb_mem.get() + imgout.pwin().width * (vnb.first + (rowi - boxb));
        }
        else
        {
            dst = imgout.ptr(rowo++, imgout.pwin().x);
        }

        /// 重置右侧边界区为背景值
        _T* const dst_rb = dst + (imgout.pwin().width - 1); // Right Border
        for (int32_t k = 0; k < diameter; k++)
        {
            *(dst_rb - k) = bgval;
        }

        int32_t assign_cnt = 1 + radius - hnb.first; // 目的像素点开始赋值为前景值的次数
        _P* src = imgin.ptr(rowi, imgin.pwin().x - hnb.first);
        for (int32_t i = 0; i < htrav1; ++i, ++src)
        {
            // assign_cnt的值在范围[1, diameter]内变化
            if (isDilating(*src))
            {
                for (int32_t k = 0; k < assign_cnt; k++)
                {
                    *dst++ = mgval;
                }
                assign_cnt = 1;
            }
            else
            {
                if (assign_cnt == diameter)
                {
                    *dst++ = bgval;
                }
                else
                {
                    assign_cnt++;
                }
            }
        }

        /// 右侧边界区统一处理
        for (int32_t i = 0; i < htrav2; ++i, ++src)
        {
            if (isDilating(*src))
            {
                for (int32_t k = 0; k < 2 * radius - i; k++)
                {
                    *(dst_rb - k) = mgval;
                }
                break;
            }
        }
    }

    /// Y方向
    std::vector<int32_t> assign_cols(imgout.pwin().width, 1 + radius - vnb.first);
    std::vector<_T*> dst_cols(imgout.pwin().width);
    _T* dst_lb = imgout.ptr(imgout.pwin().y, imgout.pwin().x); // Left Border
    for (int32_t i = 0; i < imgout.pwin().width; ++i)
    {
        dst_cols.at(i) = dst_lb++;
    }
    for (int32_t j = 0; j < vtrav1; ++j)
    {
        _T* src = (j < vnb.first) ? (_T*)vnb_mem.get() + imgout.pwin().width * j : 
                                    imgout.ptr(imgout.pwin().y + (j - vnb.first), imgout.pwin().x);
        for (int32_t i = 0; i < imgout.pwin().width; ++i, ++src)
        {
            int32_t& assign_cnt = assign_cols.at(i);
            _T*& dst = dst_cols.at(i);
            if (*src == mgval)
            {
                int32_t k = 0;
                for (; k < assign_cnt; k++)
                {
                    if (*dst == mgval && (j < vnb.first || dst > src)) // dst所在行不能超过src所在行
                    {
                        break;
                    }
                    else
                    {
                        *dst = fgval;
                        dst = (_T*)((uint8_t*)dst + imgout.get_stride());
                    }
                }
                assign_cnt = (assign_cnt - k) % diameter + 1;
            }
            else
            {
                if (assign_cnt == diameter)
                {
                    dst = (_T*)((uint8_t*)dst + imgout.get_stride());
                }
                else
                {
                    assign_cnt++;
                }
            }
        }
    }

    _T* dst_bb = imgout.ptr(imgout.pwin().y + imgout.pwin().height - 1, imgout.pwin().x); // Bottom Border
    for (int32_t i = 0; i < imgout.pwin().width; ++i)
    {
        dst_cols.at(i) = dst_bb++;
    }
    for (int32_t j = 0; j < vtrav2; ++j)
    {
        _T* src = (j >= radius) ? (_T*)vnb_mem.get() + imgout.pwin().width * (j - radius + vnb.first) : 
                                  imgout.ptr(imgout.pwin().y + (imgout.pwin().height - radius + j), imgout.pwin().x);
        for (int32_t i = 0; i < imgout.pwin().width; ++i, ++src)
        {
            _T*& dst = dst_cols.at(i);
            if (*src == mgval && dst != nullptr)
            {
                for (int32_t k = 0; k < 2 * radius - j; k++)
                {
                    *dst = fgval;
                    dst = (_T*)((uint8_t*)dst - imgout.get_stride());
                }
                dst = nullptr;
            }
        }
    }

    return 0;
}


/**
 * @fn      Erode
 * @brief   对符合isEroding条件的值做腐蚀，腐蚀后图像背景值为bgfg.first，前景值为bgfg.second
 * 
 * @param   [OUT] imgout 输出图像，数据类型、处理区域尺寸需和imgin相同
 * @param   [IN] imgin 输入图像，仅支持单通道，仅对pwin()区域做处理
 * @param   [IN] radius 滤波核半径，仅支持方形滤波核
 * @param   [IN] isEroding 输入图像中像素值是否需要做腐蚀的Lambda判断函数
 * @param   [IN] bgfg 输出图像的背景前景值，first：背景，second：前景，两者不可相等
 *  
 * @return  int32_t 0：处理正常，其他：异常
 */
template <typename _T, typename _P>
int32_t Filter::Erode(Imat<_T> &imgout, Imat<_P> &imgin, const int32_t radius, std::function<bool(const _P)> isEroding, std::pair<_T, _T> bgfg)
{
    std::function<bool(const _P)> isDilating = [&isEroding](const _P src) -> bool {return !isEroding(src);}; // 相当于对补集做膨胀
    return Dilate(imgout, imgin, radius, isDilating, std::pair<_T, _T>(bgfg.second, bgfg.first));
}


#if 0
/**
 * @fn      MedianFilter
 * @brief   
 * 
 * @param   [] imgout 
 * @param   [] imgin 
 * @param   [] radius 
 * @param   [] th_range 
 * @param   [IN] wtin 输入图像与中值滤波输出值加权时，输入图像的权重
 */
template <typename _T>
void MedianFilter(Imat<_T> &imgout, Imat<_T> &imgin, const int32_t radius, 
                  std::pair<_T, _T> th_range, const _T* wtin, const size_t wtsize, const _T wtunif)
{
    const size_t diameter = 2 * radius + 1;
    const uint32_t kernel_size = diameter * diameter;
    const uint32_t img_vmax = static_cast<uint32_t>(std::pow(2.0, 8.0 * XSP_ELEM_SIZE(imgin.type))) - 1;
    const uint32_t wt_vmax = wtin.at(0).size() - 1;
    const int32_t bottom_boundary = imgin.hei - (int32_t)radius * 2;
    const int32_t right_boundary = imgin.wid - (int32_t)diameter;
    std::vector<uint16_t *> srcnb(diameter);
    std::vector<uint32_t> kernel_vals(kernel_size);

    memcpy(imgout.Ptr(), imgin.Ptr(), imgin.hei*imgin.wid*XSP_ELEM_SIZE(imgin.type));
    if (wtin.size() != 1) // 暂仅支持一张权重表
    {
        return;
    }

    if (radius <= 2) // 小尺寸核直接排序
    {
        uint32_t median_pos = kernel_size >> 1;

        for (int32_t nh = 0; nh < bottom_boundary; ++nh)
        {
            for (auto it = srcnb.begin(); it != srcnb.end(); ++it)
            {
                *it = imgin.Ptr<uint16_t>(nh + std::distance(srcnb.begin(), it));
            }

            uint16_t *src = imgin.Ptr<uint16_t>(nh + radius) + radius;
            uint16_t *dst = imgout.Ptr<uint16_t>(nh + radius) + radius;
            for (int32_t nw = 0; nw <= right_boundary; ++nw, ++src, ++dst)
            {
                size_t idx = (clamp17(static_cast<uint32_t>(*src), th_range.first, th_range.second) - th_range.first) * wt_vmax / (th_range.second - th_range.first);
                uint32_t wt = wtin.at(0).at(idx);
                if (wt != wt_vmax)
                {
                    size_t k = 0;
                    for (auto it = srcnb.begin(); it != srcnb.end(); ++it)
                    {
                        for (size_t n = 0; n < diameter; n++)
                        {
                            kernel_vals[k++] = *(*it + (nw + n));
                        }
                    }
                    std::sort(kernel_vals.begin(), kernel_vals.end());
                    *dst = (wt * (*src) + (wt_vmax - wt) * kernel_vals.at(median_pos)) / wt_vmax;
                }
            }
        }
    }
    #if 0
    else // 大尺寸核使用直方图加速
    {
        const size_t bins = 256;
        size_t vshift = std::ceil(std::log2(static_cast<float64_t>(ysegs.second - ysegs.first) / 256.0)); // value shift for down-sample OR up-sample
        uint32_t oor = static_cast<uint32_t>(std::pow(2.0, static_cast<float64_t>(vshift))); // oor: out of range
        auto minus_u = [] (uint32_t a, uint32_t b) -> uint32_t { return (a > b) ? (a - b) : 0; };

        // 这里写的有问题的！！，需要用offset，不能直接使用yrange
        ysegs.first = minus_u(ysegs.first, oor); // 修改下限范围，使得hist[0]为小于输入yrange.first的值
        ysegs.second = std::min(ysegs.second + oor, img_vmax); // 修改下限范围，使得hist[0]为小于输入yrange.second的值
        if (vshift < std::ceil(std::log2(float64_t(ysegs.second - ysegs.first) / 256.0)))
        {
            ysegs.first = minus_u(ysegs.first, oor);
            ysegs.second = std::min(ysegs.second + oor, img_vmax);
            vshift++;
        }

        const uint32_t mth = diameter * diameter / 2 + 1; // 第mth个数为中值，从1开始计，注意是个数，不是索引
        std::array<size_t, bins> hist;
        uint32_t median = 0, left_out = 0, right_in = 0;

        for (int32_t nh = 0; nh < bottom_boundary; ++nh)
        {
            std::fill(hist.begin(), hist.end(), 0); // clear hist
            uint16_t *dst = imgout.Ptr<uint16_t>(nh + radius);
            for (auto it = srcnb.begin(); it != srcnb.end(); ++it)
            {
                *it = imgin.Ptr<uint16_t>(nh + std::distance(srcnb.begin(), it));
                for (size_t k = 0; k < diameter; k++)
                {
                    hist.at((*(*it + k) - ysegs.first) >> vshift)++;
                }
            }

            size_t n = 0;
            for (size_t k = 0; k < bins; k++)
            {
                n += hist.at(k);
                if (n >= mth)
                {
                    median = k;
                    break;
                }
            }
            if (hist.at(0) == 0) // 都在yrange范围内
            {
                *dst = (median << vshift) + ysegs.first;
            }
            ++dst;

            for (int32_t nw = 0; nw < right_boundary; ++nw)
            {
                // 中心向右移动一位，从直方图中将左列的数移除，将右列的数移入
                for (auto it = srcnb.begin(); it != srcnb.end(); ++it)
                {
                    left_out = (*(*it + nw) - ysegs.first) >> vshift;
                    hist[left_out]--;
                    if (left_out <= median)
                    {
                        n--;
                    }

                    right_in = (*(*it + (nw + diameter)) - ysegs.first) >> vshift;
                    hist[right_in]++;
                    if (right_in <= median)
                    {
                        n++;
                    }
                }

                while (n > mth) // 大于mth，新的中值在当前median左边，往左找（像素减一寻找）
                {
                    n -= hist[median--]; // 跳出该循环时，其实中值是过了的，需往右移一个像素，使刚好(n >= mth)，所以继续走下面的while()
                }
                while (n < mth) // 小于mth，新的中值在当前median右边，往右找（像素加一寻找）
                {
                    n += hist[++median];
                }
                if (hist.at(0) == 0) // 都在yrange范围内
                {
                    *dst++ = (median << vshift) + ysegs.first;
                }
                ++dst;
            }
        }
    }
    #endif

    return;
}
#endif

} // namespace

#endif

