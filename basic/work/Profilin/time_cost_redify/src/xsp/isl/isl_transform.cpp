/**
 * Image SoLution Morphological Transform
 */

#include <vector>
#include "isl_util.hpp"
#include "spdlog/spdlog.h"
#include "isl_log.hpp"
#include "isl_transform.hpp"

namespace isl
{

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
int32_t buildLinearScalePos(std::vector<float64_t>& pxPos, const int32_t lenIn, const int32_t lenOut)
{
    if (lenOut <= 1 || lenIn <= 1)
    {
        ISL_LOGE("lenOut({}) or lenIn({}) is invalid", lenOut, lenIn);
        return -1;
    }

    const float64_t scaleRatio = static_cast<float64_t>(lenIn - 1) / (lenOut - 1);

    pxPos.clear();
    for (int32_t i = 0; i < lenOut; i++)
    {
        pxPos.push_back(scaleRatio * i);
    }

    return 0;
}


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
int32_t transPos2IntpLut(std::vector<px_intp_t>& intpLut, size_t lutSize, std::vector<float64_t>& pxPos, const int32_t lenIn, const float64_t lanczos_blur)
{
    const int32_t lenOut = pxPos.size(), idxMax = lenIn - 1;

    // 参数校验
    if (lenOut <= 1 || lenIn <= 1)
    {
        ISL_LOGE("pxPos.size({}) or lenIn({}) is invalid", lenOut, lenIn);
        return -1;
    }

    intpLut.clear();
    if (lutSize >= 3) // Lanczos插值
    {
        // x是相对坐标，x*PI是相位，r是sinc窗口半径
        auto lanczos = [](float64_t x, uint32_t r) -> float64_t
        {
            if (std::abs(x) <= 1e-15 || r == 0)
            {
                return 1.0;
            }
            else if (std::abs(x) < static_cast<float64_t>(r))
            {
                float64_t xpi = x * M_PI; // xpi是相位
                return sin(xpi) * sin(xpi / r) * r / (xpi * xpi);
            }
            else
            {
                return 0.0;
            }
        };

        const uint32_t lanczos_win = (lutSize - 1) / 2;
        
        // 根据缩放或者缩小的尺寸，选择合适的lanczos_blur，目前放大表选用1.3，等比缩小表选用1.0
        for (int32_t i = 0; i < lenOut; i++)
        {
            float64_t center = std::clamp(pxPos[i], 0.0, static_cast<float64_t>(idxMax));
            int32_t idxStart = std::max(std::ceil(center - lanczos_win), 0.0);
            int32_t idxEnd = std::min(std::floor(center + lanczos_win), static_cast<float64_t>(idxMax));
            int32_t wtnum = idxEnd - idxStart + 1;
            float64_t wtsum = 0.0;
            float64_t phase = idxStart - center;
            px_intp_t& intpElem = intpLut.emplace_back();
            for (int32_t j = 0; j < wtnum; j++)
            {
                float64_t wt = lanczos((j + phase) / lanczos_blur, lanczos_win);
                intpElem[idxStart + j] = wt;
                wtsum += wt;
            }
            for (int32_t j = 0; j < wtnum; j++)
            {
                intpElem[idxStart + j] /= wtsum;
            }
        }
    }
    else if (lutSize == 2) // 线性插值
    {
        for (int32_t i = 0; i < lenOut; i++)
        {
            float64_t center = std::clamp(pxPos[i], 0.0, static_cast<float64_t>(idxMax));
            int32_t idxStart = std::floor(center);
            int32_t idxEnd = std::min(idxStart + 1, idxMax);
            intpLut.emplace_back(px_intp_t{{idxStart, -center + idxEnd}, {idxEnd, center - idxStart}});
        }
    }
    else // 最近邻近插值
    {
        for (int32_t i = 0; i < lenOut; i++)
        {
            intpLut.emplace_back(px_intp_t{{std::clamp(static_cast<int32_t>(std::round(pxPos[i])), 0, idxMax), 1.0}});
        }
    }

    return 0;
}

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
int32_t buildAverageLut(std::vector<px_intp_t>& intpLut, const int32_t lenIn, const int32_t lenOut, const int32_t interpolationMethod)
{
    // 参数校验
    if (lenOut <= 1 || lenIn <= 1)
    {
        ISL_LOGE("pxPos.size({}) or lenIn({}) is invalid", lenOut, lenIn);
        return -1;
    }

    intpLut.clear();

    const uint32_t lanczos_win = 2 ;
    const float64_t Scale = static_cast<float64_t>(lenIn) / lenOut;
    
    auto lanczos =  [lanczos_win](double x) -> float
    {
        if (std::abs(x) < lanczos_win) 
        {
            if (x == 0) 
            {
                return 1.0f;
            } 
            else 
            {
                return (lanczos_win * std::sin(M_PI * x) * std::sin(M_PI * x / lanczos_win)) / (M_PI * M_PI * x * x);
            }
        } 
        else 
        {
            return 0.0f;
        }
    };

    auto dstIndex = [Scale](double x) -> double { return (x + 0.5) / Scale - 0.5; };

    auto srcIndex = [Scale](double x) -> double { return (x + 0.5) * Scale - 0.5; };

    const int unUsed = -1; // 假设unUsed是一个常量，表示未使用的索引
    const int kernelSize = 2; // 默认平均化像素数量为2，可根据实际需要修改

    int srcStartUncorr = static_cast<int>(std::ceil(1e-8 + srcIndex(-0.5 * kernelSize))); // 可能小于0
    int srcStart = srcStartUncorr < 0 ? 0 : srcStartUncorr; // 纠正后的值，避免指向数组外
    int srcEndUncorr = static_cast<int>(std::floor(1e-8 + srcIndex(lenOut - 1 + 0.5 * kernelSize)));
    int srcEnd = srcEndUncorr >= lenIn ? lenIn - 1 : srcEndUncorr;
    int arraySize = (srcEnd - srcStart + 1) * kernelSize;
    std::vector<int> indices = std::vector<int>(arraySize, unUsed);
    std::vector<float> weights = std::vector<float>(arraySize, 0.0f);

    for (int dst = 0; dst < lenOut; dst++) 
    {
        double sum = 0;
        int lowestS = static_cast<int>(std::ceil(1e-8 + srcIndex(dst - 0.5 * kernelSize)));
        int highestS = static_cast<int>(std::floor(-1e-8 + srcIndex(dst + 0.5 * kernelSize)));
        for (int src = lowestS; src <= highestS; src++) 
        {
            int s = src < 0 ? 0 : (src >= lenIn ? lenIn - 1 : src);
            int p = (s - srcStart) * kernelSize; // 指向'indices'和'weights'数组中为此源像素保留的第一个值
            while (indices[p] != unUsed && indices[p] != dst)
            {
                p++; // 用于其他目标像素的位置，尝试下一个
            } 
            indices[p] = dst;
            float weight = (interpolationMethod == 2) ? lanczos(dst - dstIndex(src)) : 1.0;
            sum += weight;
            weights[p] += weight;
        }
        // 规范化：使得目标权重之和应为1
        int iStart = (lowestS - srcStart) * kernelSize;
        if (iStart < 0) iStart = 0;
        int iStop = (highestS - srcStart) * kernelSize + (kernelSize - 1);
        if (iStop >= static_cast<int32_t>(indices.size())) iStop = indices.size() - 1;
        for (int i = iStart; i <= iStop; i++)
        {
            if (indices[i] == dst) // 如果像素可用点为目标像素位置的话
            {
                weights[i] = static_cast<float>(weights[i] / sum);
            }          
        }
    }

    for (size_t i = 0; i < indices.size(); i++)
    {
        if (indices[i] == unUsed)
        {
            indices[i] = 0;
        }
    }

    // 将weights的权重表和indices的索引表转换为intpLut，intpLut表共有lenOut个vector，每个vector包含当前Out像素所对应的原像素的索引和权重
    intpLut.resize(lenOut);

    for(int dst = 0; dst < lenOut; dst++)
    {
        px_intp_t& intpElem = intpLut[dst];
        for(int k = 0; k < kernelSize * lenIn; k++)
        {
            if(indices[k] == dst)
            {
                int src = k / kernelSize;
                intpElem[src] = weights[k];
            }
        }
    }
    
    return 0;
}


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
int32_t buildResizingLut(std::vector<px_intp_t>& intpLut, size_t lutSize, const int32_t lenIn, const int32_t lenOut)
{
    std::vector<float64_t> pxPos;
    if (0 == buildLinearScalePos(pxPos, lenIn, lenOut))
    {
        return transPos2IntpLut(intpLut, lutSize, pxPos, lenIn);
    }
    else
    {
        return -1;
    }
}


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
int32_t buildDownSizingAvgLut(std::vector<px_intp_t>& dsAvgLut, const int32_t lenIn, const int32_t lenOut)
{
    if (lenOut < 1 || lenIn < lenOut) // 参数校验
    {
        ISL_LOGE("lenOut({}) or lenIn({}) is invalid", lenOut, lenIn);
        return -1;
    }

    dsAvgLut.clear();
    const float64_t dscale = static_cast<float64_t>(lenIn) / lenOut;

    int32_t idx = 0;
    float64_t wt = 0.0;
    for (int32_t k = 0; k < lenOut; ++k)
    {
        px_intp_t& dsAvgElem = dsAvgLut.emplace_back();
        dsAvgElem[idx] = 1.0 - wt;
        float64_t wtLeft = dscale - dsAvgElem[idx];
        while (wtLeft > FLT_EPSILON)
        {
            dsAvgElem[++idx] = std::min(wtLeft, 1.0);
            wtLeft -= dsAvgElem[idx];
        }

        wt = dsAvgElem[idx];
        if (1.0 - wt <= FLT_EPSILON) // 使下一次计算自动跳过权重是0的像素
        {
            ++idx;
            wt = 0.0;
        }

        for (auto& i : dsAvgElem)
        {
            i.second /= dscale;
        }
    }

    return 0;
}

} // namespace
