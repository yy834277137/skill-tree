/**
 * XRay Image Process Pipe
 */
#include <functional>
#include <iomanip>
#include <filesystem>
#include "spdlog/spdlog.h"
#include "isl_log.hpp"
#include "isl_filter.hpp"
#include "isl_pipe.hpp"
// #include <linux/perf_event.h>
// #include <sys/syscall.h>
// #include <unistd.h>
// #include <stdio.h>
// #include <string.h>
// #include <sys/ioctl.h>
// #include <errno.h>

// static int fd_cycles, fd_instructions, fd_cache_misses;
// static int fd_stalled_frontend, fd_stalled_backend;
// static struct perf_event_attr attr;

// static int perf_event_open(struct perf_event_attr *attr, pid_t pid, 
//                           int cpu, int group_fd, unsigned long flags) {
//     return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
// }

// static void perf_init(void) {
//     memset(&attr, 0, sizeof(attr));
//     attr.size = sizeof(attr);
//     attr.disabled = 1;
//     attr.exclude_kernel = 1;
//     attr.exclude_hv = 1;
    
//     // 基础计数器
//     attr.type = PERF_TYPE_HARDWARE;
    
//     attr.config = PERF_COUNT_HW_CPU_CYCLES;
//     fd_cycles = perf_event_open(&attr, 0, -1, -1, 0);
    
//     attr.config = PERF_COUNT_HW_INSTRUCTIONS;
//     fd_instructions = perf_event_open(&attr, 0, -1, -1, 0);
    
//     attr.config = PERF_COUNT_HW_CACHE_MISSES;
//     fd_cache_misses = perf_event_open(&attr, 0, -1, -1, 0);
    
//     // 停滞周期计数器 - 关键诊断指标
//     attr.config = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND;
//     fd_stalled_frontend = perf_event_open(&attr, 0, -1, -1, 0);
    
//     attr.config = PERF_COUNT_HW_STALLED_CYCLES_BACKEND;
//     fd_stalled_backend = perf_event_open(&attr, 0, -1, -1, 0);
// }

// static void perf_start(void) {
//     ioctl(fd_cycles, PERF_EVENT_IOC_RESET, 0);
//     ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);
//     ioctl(fd_cache_misses, PERF_EVENT_IOC_RESET, 0);
//     ioctl(fd_stalled_frontend, PERF_EVENT_IOC_RESET, 0);
//     ioctl(fd_stalled_backend, PERF_EVENT_IOC_RESET, 0);
    
//     ioctl(fd_cycles, PERF_EVENT_IOC_ENABLE, 0);
//     ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);
//     ioctl(fd_cache_misses, PERF_EVENT_IOC_ENABLE, 0);
//     ioctl(fd_stalled_frontend, PERF_EVENT_IOC_ENABLE, 0);
//     ioctl(fd_stalled_backend, PERF_EVENT_IOC_ENABLE, 0);
// }

// static void perf_stop(void) {
//     long long count_cycles, count_instructions, count_cache_misses;
//     long long count_stalled_frontend, count_stalled_backend;
    
//     ioctl(fd_cycles, PERF_EVENT_IOC_DISABLE, 0);
//     ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0);
//     ioctl(fd_cache_misses, PERF_EVENT_IOC_DISABLE, 0);
//     ioctl(fd_stalled_frontend, PERF_EVENT_IOC_DISABLE, 0);
//     ioctl(fd_stalled_backend, PERF_EVENT_IOC_DISABLE, 0);
    
//     read(fd_cycles, &count_cycles, sizeof(count_cycles));
//     read(fd_instructions, &count_instructions, sizeof(count_instructions));
//     read(fd_cache_misses, &count_cache_misses, sizeof(count_cache_misses));
//     read(fd_stalled_frontend, &count_stalled_frontend, sizeof(count_stalled_frontend));
//     read(fd_stalled_backend, &count_stalled_backend, sizeof(count_stalled_backend));
    
//     // 计算关键指标
//     double cpi = (double)count_cycles / count_instructions;
//     double stalled_frontend_ratio = (double)count_stalled_frontend / count_cycles * 100;
//     double stalled_backend_ratio = (double)count_stalled_backend / count_cycles * 100;
//     double total_stalled_ratio = stalled_frontend_ratio + stalled_backend_ratio;
    
//     printf("=== Performance Analysis Results ===\n");
//     printf("CPU Cycles: %lld\n", count_cycles);
//     printf("Instructions: %lld\n", count_instructions);
//     printf("Cache Misses: %lld\n", count_cache_misses);
//     printf("CPI: %.2f\n", cpi);
//     printf("Frontend Stalled Cycles: %lld (%.1f%%)\n", count_stalled_frontend, stalled_frontend_ratio);
//     printf("Backend Stalled Cycles: %lld (%.1f%%)\n", count_stalled_backend, stalled_backend_ratio);
//     printf("Total Stalled Ratio: %.1f%%\n", total_stalled_ratio);
    
//     // 瓶颈诊断
//     printf("=== Bottleneck Diagnosis ===\n");
//     if (total_stalled_ratio > 30) {
//         printf("RED Memory Bound: High stalled cycle ratio\n");
//         if (stalled_backend_ratio > 20) {
//             printf("   - Severe backend stalls -> Memory access bottleneck\n");
//         }
//         if (stalled_frontend_ratio > 10) {
//             printf("   - Frontend stalls -> Instruction cache or branch prediction issues\n");
//         }
//     } else if (total_stalled_ratio < 15) {
//         printf("GREEN Compute Bound: Low stalled cycle ratio\n");
//         printf("   - CPU compute units are the bottleneck\n");
//     } else {
//         printf("YELLOW Balanced: Medium stalled cycle ratio\n");
//     }
    
//     // 关闭所有计数器
//     close(fd_cycles);
//     close(fd_instructions);
//     close(fd_cache_misses);
//     close(fd_stalled_frontend);
//     close(fd_stalled_backend);
// }



namespace isl
{

int32_t PipeComm::pipe_count = 0;

/**
 * @fn      GetModuleVpadsNeeded
 * @brief   获取ISL-PIPE Continued处理模式下垂直方向所需的上下Paddings（邻域）
 * 
 * @param   [IN] slice_height 输入给ISL-PIPE需做增强的条带高度（不含邻域）
 * 
 * @return  std::pair<int32_t,int32_t> first：上Padding，second：下Padding
 */
std::pair<int32_t, int32_t> PipeComm::GetModuleVpadsNeeded(int32_t slice_height)
{
    return std::pair<int32_t, int32_t>(10, std::max(PipeGray::GetHistSmoothRadius(slice_height), 10));
}


/**
 * @fn      SetBrightnessParams
 * @brief   设置所有流程中的亮度参数（仅调用PipeGray和PipeChroma中的重载接口，不做实际操作）
 * 
 * @param   [IN] blvl 亮度等级，范围[0, 100]
 * @param   [IN] hltRange 高亮范围，first和second的取值范围均为[0, 65535]，且first不大于second
                          first：高亮起始，该值越小，高亮区（易穿区）动态范围被压缩的越多
                          second：高亮结束，XRay模块输入图像的处理阈值，高于该值则不被统计
 */
int32_t PipeComm::SetBrightnessParams(int32_t blvl, std::pair<uint16_t, uint16_t> hltRange)
{
    blvl = std::clamp(blvl, 0, 100);
    hltRange.second = std::max(hltRange.second, static_cast<uint16_t>(58000));
    hltRange.first = std::clamp(hltRange.first, static_cast<uint16_t>(50000), hltRange.second);

    PipeGray* objGray = nullptr;
    PipeChroma* objChroma = nullptr;
    for (auto& ii : objs)
    {
        if (nullptr == objGray)
        {
            objGray = dynamic_cast<PipeGray *>(ii);
        }
        if (nullptr == objChroma)
        {
            objChroma = dynamic_cast<PipeChroma*>(ii);
        }
    }
    if (nullptr != objGray && nullptr != objChroma)
    {
        int32_t bgGrayDefault = objGray->SetBrightnessParams(blvl, hltRange);
        objChroma->SetBrightnessParams(bgGrayDefault, hltRange);
    }

    return 0;
}


auto _SetDump = [] (uint32_t& dpsOut, std::string& dirOut, uint32_t dpsIn, const std::string& dirIn, int32_t chIn)
{
    dpsOut = dpsIn;
    dirOut = dirIn; // 以目录符'/'结尾
    if (dirOut.back() != '/') // TODO: 这里要区分Linux和Windows
    {
        dirOut.push_back('/');
    }
    dirOut.append("ch").append(std::to_string(chIn));
};


/**
 * @fn      SetDump
 * @brief   设置所有流程中的Dump信息（仅调用PipeGray和PipeChroma中的重载接口，不做实际操作）
 * 
 * @param   [IN] dps Dump Points，dump_point_t中枚举的或值
 * @param   [IN] dir Dump Directory，必须是已存在目录，否则不会写入
 * @param   [IN] dpc Dump Count, dump次数
 */
void PipeComm::SetDump(uint32_t dps, std::string dir, uint32_t dpc)
{
    for (auto& ii : objs)
    {
        if (nullptr != ii)
        {
            ii->SetDump(dps, dir, dpc);
        }
    }

    return;
}


static void CalcAceSmoothWeight(std::vector<PipeGray::wtx4_t>& lut_wtx4, std::vector<PipeGray::wtx2_t>& lut_wtx2h, int32_t tile_width, int32_t smooth_len)
{
    const int32_t tile_half = tile_width / 2;
    if (tile_width % 2 != 0 || tile_half < smooth_len)
    {
        ISL_LOGE("tile_width({}) is invalid, must align to 2 and no less than smooth_len({})", tile_width, smooth_len);
        return;
    }

    /**
     * Tile Lut权重表 
     * 以拼接点为中心向四周矩形（也可为椭圆）扩散，拼接点处的各Lut权重相同，x4为0.25，x2为0.5 
     * 需处理的当前条带，先以x4方式与上个条带加权，平滑高度为smooth_len，后续则全都使用x2方式加权
     * x4的加权方式中，水平方向：从垂直中线向左右平滑(smooth_len / 2)，垂直方向：从水平上沿向下平滑smooth_len 
     * x2的加权方式中，只有水平方向，水平方向与x4计算方法相同
     */
    const int32_t wtx2_sum = smooth_len * 2;
    lut_wtx4.resize(tile_width * smooth_len);
    lut_wtx2h.resize(tile_width);
    for (size_t xidx = 0; xidx < 2; xidx++) // 左右2边关联的Tile Lut是不同的，需分别处理
    {
        int32_t xoffs = xidx * tile_half;
        if (0 == xidx) // Tile的左半部分
        {
            for (int32_t j = 0, w = xoffs; j < tile_half; ++j, ++w)
            {
                lut_wtx2h.at(w).at(0) = subcf0(smooth_len, j);
                lut_wtx2h.at(w).at(1) = wtx2_sum - subcf0(smooth_len, j);
            }
        }
        else // Tile的右半部分
        {
            for (int32_t j = tile_half-1, w = xoffs; j >= 0; --j, ++w)
            {
                lut_wtx2h.at(w).at(0) = wtx2_sum - subcf0(smooth_len, j);
                lut_wtx2h.at(w).at(1) = subcf0(smooth_len, j);
            }
        }

        for (int32_t h = 0; h < smooth_len; ++h)
        {
            auto wtx4 = lut_wtx4.begin() + tile_width * h + xoffs;
            PipeGray::wtx2_t wtx2v{static_cast<uint16_t>(smooth_len - h), static_cast<uint16_t>(h)}; // 垂直方向2分
            for (int32_t j = 0, w = xoffs; j < tile_half; ++j, ++w, ++wtx4)
            {
                PipeGray::wtx2_t wtx2h = lut_wtx2h.at(w); // 水平方向2分
                wtx4->at(0) = wtx2v[0] * wtx2h[0]; // 左上00关联分块的权重，lut00
                wtx4->at(1) = wtx2v[0] * wtx2h[1]; // 右上01关联分块的权重，lut01
                wtx4->at(2) = wtx2v[1] * wtx2h[0]; // 左下10关联分块的权重，lut10
                wtx4->at(3) = wtx2v[1] * wtx2h[1]; // 右下11关联分块的权重，lut11
            }
        }
    }

    return;
}


/**
 * @fn      PipeGray
 * @brief   PipeGray流程的默认构造
 */
PipeGray::PipeGray() : PipeComm()
{
    SetBrightnessParams(); // 初始化亮度参数
    SetLaceParams(); // 初始化LACE模块参数
    SetNreeParams(); // 初始化NREE模块参数

    return;
}


/**
 * @fn      PipeGray
 * @brief   PipeGray流程的带参数构造
 */
PipeGray::PipeGray(uint32_t xray_width) : PipeComm()
{
    if (0 == xray_width % tile_base)
    {
        tile_cols = xray_width / tile_base;
    }
    else
    {
        ISL_LOGW("xray_width({}) is unaligned to {}", xray_width, tile_base);
        tile_cols = xray_width / tile_base + 1;
    }

    SetBrightnessParams(); // 初始化亮度参数
    SetLaceParams(); // 初始化LACE模块参数
    SetNreeParams(); // 初始化NREE模块参数

    return;
}


/**
 * @fn      SetBrightnessParams
 * @brief   设置PipeGray流程中的亮度参数
 * 
 * @param   [IN] blvl 亮度等级，范围[0, 100]，作用于Global Gamma系数
 * @param   [IN] hltRange 高亮范围，first和second的取值范围均为[0, 65535]，且first不大于second
                          first：高亮起始，该值越大，高亮区（易穿区）对比度增强也越多
                          second：高亮结束，XRay模块输入图像的处理阈值，高于该值则不被统计
 */
int32_t PipeGray::SetBrightnessParams(int32_t blvl, std::pair<uint16_t, uint16_t> hltRange)
{
    this->lum_exp = std::clamp(0.005 * blvl + 0.25, 0.25, 0.75); // lum_exp的实际范围是[0.25, 0.75]
    this->blc_coeff = std::clamp(1.0 - 0.005 * blvl, 0.2, 1.0); // blc_coeff的实际范围是[0.2, 1.0]
    for (auto it = lut_base.begin(), ii = lace.get_lut_fidx().begin(); it != lut_base.end(); ++it, ++ii)
    {
        *it = std::pow(*ii, this->blc_coeff);
    }

    this->stat_lum_th = std::max(static_cast<int32_t>(hltRange.first), 55000);
    this->gamma_max = (hltRange.first == hltRange.second) ? 0.0 : // 高亮起点与背景阈值相同时，对易穿区也进行增强
        1.0 + std::log((this->stat_lum_th - 54000) / 1000); // 1.0(55000)~3.14(62500)
    if (this->hist_smooth_rt.size() > 0 && this->hist_smooth_rt.size() % tile_cols == 0)
    {
        lut_cache_rt.clear();
        TileLutImpel(this->lut_cache_rt, nullptr, this->hist_smooth_rt, 0, 1);
    }

    // XRay输入的归一化灰度图使用的背景值，0.97是Cali模块使用的反溢出系数，65000是SharedPara.m_nBackGroundGray的值
    const uint16_t bg_gray_in = std::min(static_cast<int32_t>(0.97 * 65535), 65000);
    int32_t bg_gray_out = lut_base.at(bg_gray_in >> Lace::ll_shift) * 65535;

    return bg_gray_out;
}


/**
 * @fn      SetLaceParams
 * @brief   设置PipeGray流程中的LACE模块参数
 * 
 * @param   [IN] contrast_coeff 对比度系数，范围[0, 1]，值越大对比度越高，为0时不做对比度增强
 * @param   [IN] dark_boost LumAdjust中的暗区亮度增强，值越大增强越多，范围：[0, 1.0]
 */
void PipeGray::SetLaceParams(float64_t contrast_coeff, float64_t dark_boost)
{
    /// 设置ACE参数
    this->contrast_coeff = std::clamp(contrast_coeff, 0.0, 1.0);
    this->dark_boost = std::clamp(dark_boost, 0.0, 1.0);
    if (this->hist_smooth_rt.size() > 0 && this->hist_smooth_rt.size() % tile_cols == 0)
    {
        lut_cache_rt.clear();
        TileLutImpel(this->lut_cache_rt, nullptr, this->hist_smooth_rt, 0, 1);
    }

    return;
}


/**
 * @fn      ResetCache
 * @brief   清空PipeGray流程中的缓存 
 * @note    更新条带高度、切换实时过包/回拉处理模式时使用
 */
void PipeGray::ResetCache(bool bRtProc)
{
    if (bRtProc)
    {
        hist_cache_rt.clear();
        lut_cache_rt.clear();
    }
    else
    {
        hist_cache_pb.clear();
        lut_cache_pb.clear();
    }

    return;
}


/**
 * @fn      SetNreeParams
 * @brief   设置PipeGray流程中的NREE模块参数
 * 
 * @param   [IN] ee_intensity 边缘整体增强强度，取值范围：[0, 16]，0为不做增强
 */
void PipeGray::SetNreeParams(float64_t ee_intensity, int32_t weak_th, float64_t weak_enhance, int32_t strong_th, float64_t strong_suppress)
{
    //egde.SetEeParam(ee_intensity, 0.2, 2.0);
    texture.SetEeParam(ee_intensity, 0.2, 2.0, {{weak_th, weak_enhance}, {strong_th, strong_suppress}});

    return;
}


static void HistSmooth(PipeGray::hist_f64t& histout, const std::vector<std::reference_wrapper<PipeGray::hist_f64t>>& histins)
{
    histout.second = 0;
    for (const auto& histin : histins)
    {
        histout.second += histin.get().second;
    }

    if (histout.second > 0)
    {
        auto wtsum = [&](size_t idx)
        {
            float64_t sum = 0.0;
            for (auto& histin : histins)
            {
                sum += histin.get().first.at(idx) * histin.get().second;
            }
            return (sum / histout.second);
        };
        for (size_t i = 0; i < histout.first.size(); ++i)
        {
            histout.first.at(i) = wtsum(i);
        }
    }
    else
    {
        std::fill(histout.first.begin(), histout.first.end(), 0.0);
    }

    return;
}


static void HistSmooth(PipeGray::tile_hist_t& histout, const std::vector<std::reference_wrapper<PipeGray::tile_hist_t>>& histins)
{
    std::vector<std::reference_wrapper<PipeGray::hist_f64t>> histin_egde;
    std::vector<std::reference_wrapper<PipeGray::hist_f64t>> histin_glb;

    for (PipeGray::tile_hist_t& histin : histins)
    {
        histin_egde.push_back(std::ref(histin.first));
        histin_glb.push_back(std::ref(histin.second));
    }
    HistSmooth(histout.first, histin_egde);
    HistSmooth(histout.second, histin_glb);

    return;
}


void PipeGray::TileHistStatistic(std::deque<tile_hist_t>& hist_cache, Imat<uint16_t>& imgin, Imat<uint16_t>& imgmask, std::vector<Rect<int32_t>>& win_vseq)
{
    const Rect<int32_t> win_in = imgin.pwin();
    for (auto it = win_vseq.begin(); it != win_vseq.end(); ++it)
    {
        it->width = tile_width;
        for (size_t j = 0; j < tile_cols; ++j)
        {
            if (j == tile_cols - 1) // 最后一个Tile的宽不定
            {
                it->width = win_in.x + win_in.width - it->x;
            }

            Imat<uint16_t>& tile_img = imgin(*it);
            Imat<uint16_t>& tile_mask = imgmask(*it);

            tile_hist_t& hist_next = hist_cache.emplace_back();
            lace.HistStatistic(hist_next.first.first, hist_next.first.second, nullptr, tile_img, {0, stat_lum_th}, &tile_mask);
            lace.HistStatistic(hist_next.second.first, hist_next.second.second, nullptr, tile_img, {0, stat_lum_th});

            it->x += it->width; // 下一个Tile的X坐标
        }
    }
    imgin(win_in);

    return;
}


void PipeGray::TileLutImpel(std::deque<tile_lut_t>& lut_cache, std::deque<tile_hist_t>* hist_smooth, 
                            std::deque<tile_hist_t>& hist_cache, size_t smooth_idx, size_t smooth_num)
{
    size_t smooth_max = subcf0(hist_cache.size() / tile_cols, smooth_idx);
    if (smooth_num > smooth_max)
    {
        ISL_LOGD("smooth out of range, hist_cache size {}, tile_cols {}, smooth_idx {}, smooth_num {} / {}", 
                 hist_cache.size(), tile_cols, smooth_idx, smooth_num, smooth_max);
        if (smooth_max > 0)
        {
            smooth_num = smooth_max;
        }
        else
        {
            return;
        }
    }

    std::vector<float64_t> ace_params; // ACE的参数，仅作为调试使用
    pofs_t ace_dump;
    std::stringstream ss(this->dump_dir);
    ss.seekp(0, std::ios_base::end);
    ss << std::setw(6) << std::setfill('0') << this->proc_count;
    ss << "_s" << smooth_idx << "-" << smooth_num << "@" << hist_cache.size() / tile_cols << "_acelut.raw";
    std::string&& fn_ace = ss.str();
    bool b_dump_ace = false;
    if (dump_points & dp_ace)
    {
        if (dump_counts > 0)
        {
            dump_counts--;
            b_dump_ace = true;
        }
    }

    tile_hist_t hist_tmp; // 用于存储前后直方图平滑、边缘/全局直方图合成的结果
    for (size_t j = 0, smooth_offs = smooth_idx * tile_cols; j < tile_cols; ++j, ++smooth_offs)
    {
        tile_hist_t& hist_smo_ref = (nullptr == hist_smooth) ? hist_tmp : hist_smooth->emplace_back();
        if (smooth_num > 1) // 直方图平滑
        {
            std::vector<std::reference_wrapper<tile_hist_t>> hist_vseq; // histogram with top-bottom pads
            for (size_t i = 0; i < smooth_num; ++i)
            {
                hist_vseq.push_back(std::ref(*(hist_cache.begin() + smooth_offs + tile_cols * i)));
            }
            HistSmooth(hist_smo_ref, hist_vseq);
        }
        hist_f64t& hist_edge = (smooth_num > 1) ? hist_smo_ref.first : hist_cache.at(smooth_offs).first; // smooth_num为1时直接使用hist_cache
        hist_f64t& hist_glb = (smooth_num > 1) ? hist_smo_ref.second : hist_cache.at(smooth_offs).second;
        if (b_dump_ace)
        {
            ace_params.clear();
            ace_params.push_back(hist_glb.second);
            ace_params.push_back(hist_edge.second);
            write_file(fn_ace, hist_glb.first.data(), container_memuse(hist_glb.first), (0 == j), false, &ace_dump);
            write_file(fn_ace, hist_edge.first.data(), container_memuse(hist_edge.first), false, false, &ace_dump);
        }

        // 直方图合成
        Lace::lut_f64t& hist_comp = hist_tmp.first.first;
        lace.HistCompsite(hist_comp, hist_edge.first, hist_glb.first, hist_edge.second, hist_glb.second, 0.95);
        if (b_dump_ace)
        {
            write_file(fn_ace, hist_comp.data(), container_memuse(hist_comp), false, false, &ace_dump);
        }

        tile_lut_t& tile_atj = lut_cache.emplace_back(); // lut at index 'j'
        Lace::lut_f64t& lut_atj = tile_atj.first;
        if (std::max(hist_edge.second, hist_glb.second) > (Lace::lut_bins >> 6)) // Tile中不超过16个像素，则不做ACE
        {
            tile_atj.second = true;
            lace.LumAdjust(lut_atj, hist_glb.first, hist_glb.second, nullptr, lum_exp, dark_boost, gamma_max, &ace_params);
            if (b_dump_ace)
            {
                write_file(fn_ace, lut_atj.data(), container_memuse(lut_atj), false, false, &ace_dump);
            }

            float64_t ce_level = (hist_glb.second > 0) ? 
                (std::pow(static_cast<float64_t>(hist_edge.second) / hist_glb.second, 0.5) * contrast_coeff) : 0.0;
            lace.ContrastEnhance(lut_atj, lut_atj, hist_comp, ce_level);
            if (b_dump_ace)
            {
                ace_params.push_back(tile_atj.second);
                ace_params.push_back((hist_glb.second > FLT_EPSILON) ? static_cast<float64_t>(hist_edge.second) / hist_glb.second : 0.0);
                ace_params.push_back(ce_level);
                write_file(fn_ace, lut_atj.data(), container_memuse(lut_atj), false, false, &ace_dump);
            }
        }
        else
        {
            lut_atj = lace.get_lut_fidx(); // 线性LUT：Y = X
            tile_atj.second = false; // 不做ACE
            if (b_dump_ace)
            {
                ace_params.insert(ace_params.end(), 10, 0.0);
                ace_params.at(9) = tile_atj.second;
                write_file(fn_ace, lut_atj.data(), container_memuse(lut_atj), false, false, &ace_dump);
                write_file(fn_ace, lut_atj.data(), container_memuse(lut_atj), false, false, &ace_dump);
            }
        }

        if (b_dump_ace)
        {
            write_file(fn_ace, ace_params.data(), container_memuse(ace_params), false, (j == tile_cols-1), &ace_dump);
            this->dump_counts--;
        }
    }

    return;
}


void PipeGray::TileLutApply(Imat<uint16_t>& imgout, Imat<uint16_t>& imgin, std::deque<tile_lut_t>& lut_cache, size_t prev_idx, size_t curt_idx, bool binv)
{
    const int32_t smooth_width = tile_width / 2;
    prev_idx *= tile_cols;
    curt_idx *= tile_cols;

    for (size_t j = 0; j < tile_cols; ++j)
    {
        for (size_t xidx = 0; xidx < 2; xidx++) // 左右2边关联的Tile Lut是不同的，需分别处理
        {
            const int32_t tile_startx = (0 == xidx) ? (j - 1) : j;
            const int32_t tile_endx = tile_cols - 1;
            const size_t tile_left = std::max(tile_startx, 0); // 左关联分块的列X索引，范围[0, tile_cols-1]
            const size_t tile_right = std::min(tile_startx+1, tile_endx); // 右关联分块的列X索引，范围[0, tile_cols-1]

            // 4个关联分块及其Lut
            tile_lut_t tile00 = lut_cache.at(tile_left + prev_idx);
            tile_lut_t tile01 = lut_cache.at(tile_right + prev_idx);
            tile_lut_t tile10 = lut_cache.at(tile_left + curt_idx);
            tile_lut_t tile11 = lut_cache.at(tile_right + curt_idx);
            Lace::lut_f64t& lut00 = tile00.second ? tile00.first : lace.get_lut_fidx();
            Lace::lut_f64t& lut01 = tile01.second ? tile01.first : lace.get_lut_fidx();
            Lace::lut_f64t& lut10 = tile10.second ? tile10.first : lace.get_lut_fidx();
            Lace::lut_f64t& lut11 = tile11.second ? tile11.first : lace.get_lut_fidx();

            int32_t toffs = xidx * smooth_width; // point(x, ) offset
            int32_t xoffs = tile_width * j + toffs;
            int32_t wleft = imgin.pwin().width - xoffs;
            if (wleft > 0)
            {
                wleft = std::min(wleft, smooth_width);
            }
            else
            {
                break;
            }

            int32_t xin = imgin.pwin().x + xoffs;
            int32_t yin = imgin.pwin().y + (binv ? (imgin.pwin().height - 1) : 0);
            int32_t xout = imgout.pwin().x + xoffs;
            int32_t yout = imgout.pwin().y + (binv ? (imgout.pwin().height - 1) : 0);
            int32_t hx4 = (prev_idx != curt_idx) ? std::min(imgin.pwin().height, wt_smooth_radius) : 0; // 4分块平滑的高度
            int32_t hx2 = imgin.pwin().height - hx4; // 2分块平滑的高度

            if (tile00.second || tile01.second || tile10.second || tile11.second) // 任意一个有效
            {
                for (int32_t i = 0; i < hx4; ++i, (binv ? (--yin, --yout) : (++yin, ++yout))) // 4分块平滑
                {
                    uint16_t* src = imgin.ptr(yin, xin);
                    uint16_t* dst = imgout.ptr(yout, xout);
                    auto wtx4 = lut_wtx4.begin() + tile_width * i + toffs;
                    for (int32_t w = 0; w < wleft; ++w, ++src, ++dst, ++wtx4)
                    {
                        size_t bin_idx = *src >> Lace::ll_shift;
                        float64_t intp_tmp = lut00.at(bin_idx) * wtx4->at(0);
                        intp_tmp += lut01.at(bin_idx) * wtx4->at(1);
                        intp_tmp += lut10.at(bin_idx) * wtx4->at(2);
                        intp_tmp += lut11.at(bin_idx) * wtx4->at(3);
                        *dst = intp_tmp / lut_wtx4.size() * 65535;
                    }
                }
            }
            else
            {
                if (hx4 > 0)
                {
                    Rect<int32_t> winin(xin, binv ? (imgin.pwin().y + hx2) : imgin.pwin().y, wleft, hx4);
                    Rect<int32_t> winout(xout, binv ? (imgout.pwin().y + hx2) : imgout.pwin().y, wleft, hx4);
                    imgin.copyto(imgout, winin, winout);
                }
            }

            if (hx2 > 0)
            {
                if (tile10.second || tile11.second)
                {
                    for (int32_t i = 0; i < hx2; ++i, (binv ? (--yin, --yout) : (++yin, ++yout))) // 2分块平滑
                    {
                        uint16_t* src = imgin.ptr(yin, xin);
                        uint16_t* dst = imgout.ptr(yout, xout);
                        auto wtx2 = lut_wtx2h.begin() + toffs;
                        for (int32_t w = 0; w < wleft; ++w, ++src, ++dst, ++wtx2)
                        {
                            size_t bin_idx = *src >> Lace::ll_shift;
                            *dst = (lut10.at(bin_idx) * wtx2->at(0) + lut11.at(bin_idx) * wtx2->at(1)) / lut_wtx2h.size() * 65535;
                        }
                    }
                }
                else
                {
                    Rect<int32_t> winin(xin, binv ? imgin.pwin().y : (imgin.pwin().y + hx4), wleft, hx2);
                    Rect<int32_t> winout(xout, binv ? imgout.pwin().y : (imgout.pwin().y + hx4), wleft, hx2);
                    imgin.copyto(imgout, winin, winout);
                }
            }
        }
    }

    return;
}


/**
 * @fn      GetHistSmoothRadius
 * @brief   获取实时处理中不同条带高度对应的直方图平滑的半径
 * 
 * @param   [IN] rt_slice_height 实时处理中的条带高度
 * 
 * @return  int32_t 直方图平滑的半径
 */
int32_t PipeGray::GetHistSmoothRadius(int32_t rt_slice_height)
{
    int32_t smooth_radius = 0;

    const std::map<int32_t, int32_t> slice_smr{{16, 16}, {18, 9}, {20, 10}, {22, 11}, {24, 12}, {26, 13}, {28, 7}, {30, 10}, {32, 0}};
    if (rt_slice_height < slice_smr.begin()->first)
    {
        smooth_radius = rt_slice_height; // 直方图平滑半径与条带高一致
    }
    else if (rt_slice_height > slice_smr.rbegin()->first)
    {
        smooth_radius = 0; // 直方图平滑半径为0
    }
    else
    {
        auto it = slice_smr.find(rt_slice_height);
        if (it != slice_smr.end())
        {
            smooth_radius = it->second; // slice_smr[]是非const成员函数，使用会编译错误
        }
    }

    return smooth_radius;
}


/**
 * @fn      AutoContraceEnhance
 * @brief   自动对比增强，分3步：statistic、impel、apply
 *  
 * @param   [OUT] imgout 输出图像，数据类型需和imgin相同，与imgin不可同源 
                         当为实时条带处理时，imgout需作为连续条带输出的缓存，且与imgin尺寸不同时，会重置对象内的缓存
                         当为回拉处理时，imgout即时输出，其处理区域尺寸需和imgin相同
 * @param   [IN] imgin 输入图像，仅对pwin()区域做处理
 * @param   [IN] imgtmp 临时图像内存，需有3块，每块内存的数据类型、整图尺寸需和imgin相同
 * @param   [IN] pmode 处理模式，0-实时条带迭代，1-从上往下顺序，2-从下往上逆序
 * 
 * @return  int32_t 0：处理正常，其他：异常
 */
int32_t PipeGray::AutoContraceEnhance(Imat<uint16_t>& imgout, Imat<uint16_t>& imgin, std::array<Imat<uint16_t>*, 3>& imgtmp, pmode_t pmode)
{
    if (!(imgout.isvalid() && imgin.isvalid() && imgout.itype() == imgin.itype()))
    {
        ISL_LOGE("imgin: {}; imgout: {}", imgin, imgout);
        return -1;
    }
    for (auto& iimg : imgtmp)
    {
        if (iimg == nullptr)
        {
            ISL_LOGE("iimg is nullptr");
            return -1;
        }
        if (!(iimg->isvalid() && iimg->isize() == imgin.isize() && iimg->itype() == imgin.itype()))
        {
            ISL_LOGE("iimg: {}", *iimg);
            return -1;
        }
    }

    if (imgin.pwin().height == 0 || imgin.pwin().height % 2 != 0 || 
        imgin.pwin().x != 0 || imgin.pwin().width != imgin.width())
    {
        ISL_LOGE("the pwin of imgin is invalid: {}", imgin.pwin());
        return -1;
    }

    if (pmode != PM_SLICE_SEQUENCE && pmode != PM_ENTIRE_POSITIVE && pmode != PM_ENTIRE_NEGATIVE)
    {
        ISL_LOGE("pmode({}) is invalid", static_cast<int32_t>(pmode));
        return -1;
    }

    if (PM_SLICE_SEQUENCE == pmode)
    {
        std::pair<int32_t, int32_t>&& rt_vpads_nd = GetModuleVpadsNeeded(imgin.pwin().height);
        if (imgin.get_vpads().first < rt_vpads_nd.first || imgin.get_vpads().second < rt_vpads_nd.second)
        {
            ISL_LOGE("imgin's vpads({}) is too small, need at least {}", imgin.get_vpads(), rt_vpads_nd);
            return -1;
        }
    }
    else // 回拉转存模式
    {
        if (imgout.pwin().size() != imgin.pwin().size()) // 输入输出的处理区域需相同
        {
            ISL_LOGE("pwin size is unmatched, imgin: {}; imgout: {}", imgout.pwin().size(), imgin.pwin().size());
            return -1;
        }
    }

    std::stringstream ss(this->dump_dir);
    ss.seekp(0, std::ios_base::end);
    ss << std::setw(6) << std::setfill('0') << ++this->proc_count;
    std::string&& dump_fn = ss.str();
    std::string fn_imgio = dump_fn + "_laceio";
    std::string fn_hist = dump_fn + "_img2hist";
    std::string fn_ace = dump_fn + "_acelut.raw";
    bool b_dump_imgio = false, b_dump_hist = false;

    ss.str(""); // 清空stringstream
    ss << '_' << imgin.width() << 'x' << imgin.height();
    std::string&& res_pattern_imgio = ss.str();
    if (dump_points & (dp_imgio | dp_hist))
    {
        if (dump_counts > 0)
        {
            dump_counts--;
            if (dump_points & dp_imgio)
            {
                b_dump_imgio = true;
                imgin.dump(fn_imgio, true);
                if (PM_SLICE_SEQUENCE == pmode)
                {
                    imgout.dump(fn_imgio, false);
                }
            }
            if (dump_points & dp_hist)
            {
                b_dump_hist = true;
            }
        }
    }

    std::deque<tile_hist_t>& hist_cache = (pmode == PM_SLICE_SEQUENCE) ? hist_cache_rt : hist_cache_pb; // Tile Histogram缓存
    std::deque<tile_lut_t>& lut_cache = (pmode == PM_SLICE_SEQUENCE) ? lut_cache_rt : lut_cache_pb; // Tile Lut缓存
    const int32_t front_neighb = (pmode == PM_ENTIRE_NEGATIVE) ? imgin.get_vpads().second : imgin.get_vpads().first; // 前邻域
    const int32_t rear_neighb = (pmode == PM_ENTIRE_NEGATIVE) ? imgin.get_vpads().first : imgin.get_vpads().second; // 后邻域
    bool bRtSliceContinuedCopied = false; // 实时处理模式下，前一次输出的是否作为当前上邻域直接使用

    /// 亮度调整，调整全局亮度会优先使用该模块，因为后续的锐化增强会受到图像亮度影响的
    auto blc_module = [&] ()
    {
        if (1.0 - blc_coeff > FLT_EPSILON)
        {
            const Rect<int32_t> win_bak = imgout.pwin(); // 输出的需处理区域
            int32_t fnb = bRtSliceContinuedCopied ? 0 : std::min(front_neighb, lace_neighb);
            int32_t rnb = std::min(rear_neighb, lace_neighb);
            pads_t&& pads_diff = (pmode == PM_ENTIRE_NEGATIVE) ? 
                std::make_pair(-rnb, -fnb) : std::make_pair(-fnb, -rnb); // vpads减小，pwin增加
            imgout.move_vpads(pads_diff);
            imgout.map([&](uint16_t& src) {src = lut_base.at(src >> Lace::ll_shift) * 65535;});
            imgout(win_bak);
        }
        if (b_dump_imgio)
        {
            imgout.dump(fn_imgio, false);
            std::string::size_type res_pos = fn_imgio.rfind(res_pattern_imgio);
            if (res_pos != std::string::npos)
            {
                ss.str(""); // 清空stringstream
                ss << '_' << imgin.width() << 'x' << (std::filesystem::file_size(fn_imgio) / imgin.elemsize() / imgin.width());
                std::string fn_bak(fn_imgio);
                fn_imgio.replace(res_pos, res_pattern_imgio.length(), ss.str());
                std::filesystem::rename(fn_bak, fn_imgio);
            }
        }
    };

    /// 可能为TIP，只做亮度提升，不做对比度增强
    if (pmode != PM_SLICE_SEQUENCE && imgin.width() != m_nPipeResizeWidth && 0 == front_neighb && 0 == rear_neighb)
    {
        imgin.copyto(imgout, imgin.pwin(), imgout.pwin());
        blc_module();
        return 0;
    }

    /// 更新Tile宽和Tile区域的平滑权重表 
    tile_cols = (imgin.pwin().width + tile_base - 1) / tile_base;
    int32_t tile_wtmp = ((imgin.pwin().width + tile_cols - 1) / tile_cols + 1) & (~1); // 向上取整再向上2对齐
    if (this->tile_width != tile_wtmp)
    {
        tile_cols = (imgin.pwin().width + tile_wtmp - 1) / tile_wtmp;
        ISL_LOGI("pipe chan {}, tile width {} -> {}, cols {}, rebulid the smooth weight", 
                 pipe_chan, this->tile_width, tile_wtmp, tile_cols);
        this->tile_width = tile_wtmp;
        CalcAceSmoothWeight(lut_wtx4, lut_wtx2h, this->tile_width, wt_smooth_radius);
    }

    if (PM_SLICE_SEQUENCE == pmode)
    {
        rt_slice_height = imgin.pwin().height; // 实时处理条带高度
        this->hist_smooth_rt.clear();

        if (imgout.isize() == imgin.isize() && imgout.pwin() == imgin.pwin())
        {
            // 将上一个条带的对比度增强结果移到上Padding
            int32_t cp_height = std::min(imgout.get_vpads().first, rt_slice_height);
            Rect<int32_t> slice_top(imgout.pwin().x, imgout.get_vpads().first-cp_height, imgout.pwin().width, cp_height);
            Rect<int32_t> slice_mid(imgout.pwin().x, imgout.pwin().y + (rt_slice_height-cp_height), imgout.pwin().width, cp_height);
            imgout.copyto(imgout, slice_mid, slice_top);
            bRtSliceContinuedCopied = true;
            if (lut_cache.size() != tile_cols || hist_cache.size() % tile_cols != 0)
            {
                ISL_LOGW("lut_cache({}) OR hist_cache({}) is invalid, tile_cols: {}", hist_cache.size(), lut_cache.size(), tile_cols);
                ResetCache(true);
            }
        }
        else // 首次进入、更新条带高度或修改超分系数
        {
            if (imgout.pwin().height != rt_slice_height) // 更新直方图平滑半径
            {
                hist_smooth_radius = GetHistSmoothRadius(rt_slice_height);
            }
            ISL_LOGI("alter size, imgin: {}; imgout: {}, smr: {}", imgin, imgout, hist_smooth_radius);
            imgout.reshape(imgin.isize(), imgin.channels(), imgin.get_vpads(), imgin.get_hpads());
            ResetCache(true);
        }
    }
    else
    {
        //imgout.reshape(imgin.isize(), imgin.channels(), imgin.get_vpads(), imgin.get_hpads());
        ResetCache(false);
    }

    Rect<int32_t> win_stat;
    if (lut_cache.size() > 0) // 先判断是否已有上一Tile行的Lut（只有实时条带处理才会进入此分支）
    {
        if (hist_smooth_radius == 0) // 直方图无需平滑，直接统计当前处理区域
        {
            win_stat = imgin.pwin();
        }
        else // 根据缓存的直方图Tile行数自适应统计区域
        {
            int32_t hist_cache_lnum_cur = hist_cache.size() / tile_cols;
            win_stat = Rect<int32_t>(imgin.pwin().x, imgin.pwin().y + (hist_cache_lnum_cur - 1) * hist_smooth_radius, 
                                     imgin.pwin().width, imgin.pwin().height + (2 - hist_cache_lnum_cur) * hist_smooth_radius);
        }
    }
    else // 实时处理模式切换条带尺寸后首次进入，或回拉转存模式
    {
        if (PM_SLICE_SEQUENCE == pmode)
        {
            win_stat = Rect<int32_t>(imgin.pwin().x, 0, imgin.pwin().width, imgin.pwin().y + imgin.pwin().height + hist_smooth_radius);
        }
        else // 回拉转存模式简化计算，统计全区域
        {
            win_stat = Rect<int32_t>(imgin.pwin().x, 0, imgin.pwin().width, imgin.height());
        }
    }

    Imat<uint16_t>& imgtmp0 = *imgtmp[0];
    Imat<uint16_t>& imgtmp1 = *imgtmp[1];
    const int32_t stat_dilate_nb = 1; // 膨胀滤波的邻域
    const Rect<int32_t> win_shot_in = imgin.pwin(); // 输入的需处理的区域
    const Rect<int32_t> win_shot_out = imgout.pwin(); // 输出的需处理区域

    /// 扩大pwin，使其包含膨胀滤波的邻域
    imgin(win_stat);
    imgin.move_vpads(std::make_pair(-stat_dilate_nb, -stat_dilate_nb)); // vpads减小，pwin增加
    for (auto& iimg : imgtmp) // imgtmp邻域与imgin同步
    {
        (*iimg)(imgin.pwin());
    }

    /// 计算边缘信息
    Imat<uint16_t>& imgedge = imgtmp1;
    Nree::EdgeDetectSobel(imgedge, imgin, imgtmp0, std::make_pair<uint16_t, uint16_t>(1600, 32767), 
                          std::pair{static_cast<uint16_t>(0), static_cast<uint16_t>(stat_lum_th)});
    std::string&& res_pattern_hist = b_dump_hist ? 
        (ss.str(""), ss << '_' << imgin.pwin().width << 'x' << imgin.pwin().height, ss.str()) : "";
    std::string&& img2hist_seg_height = b_dump_hist ? 
        (ss.str(""), ss << imgin.pwin().height << '+' << imgedge.pwin().height << '+' << win_stat.height, ss.str()) : "";
    if (b_dump_hist)
    {
        imgin.dump(fn_hist, true, imgin.pwin());
        imgedge.dump(fn_hist, false, imgedge.pwin());
    }

    /// 还原pwin到直方图统计区域
    imgin(win_stat);
    for (auto& iimg : imgtmp) // imgtmp邻域与imgin同步
    {
        (*iimg)(win_stat);
    }

    /// 计算边缘信息的mask，用于直方图统计
    Imat<uint16_t>& edge_mask = imgtmp0;
    std::function<bool(const uint16_t)> isDilating = [](const uint16_t edge) -> bool { return edge != 0; };
    Filter::Dilate(edge_mask, imgedge, stat_dilate_nb, isDilating, {0, 0xFFFF});
    if (b_dump_hist)
    {
        edge_mask.dump(fn_hist, false, edge_mask.pwin());
        std::string::size_type res_pos = fn_hist.rfind(res_pattern_hist);
        if (res_pos != std::string::npos)
        {
            ss.str(""); // 清空stringstream
            ss << '_' << img2hist_seg_height;
            ss << '_' << edge_mask.pwin().width << 'x' << (std::filesystem::file_size(fn_hist) / edge_mask.elemsize() / edge_mask.pwin().width);
            std::string fn_bak(fn_hist);
            fn_hist.replace(res_pos, res_pattern_hist.length(), ss.str());
            std::filesystem::rename(fn_bak, fn_hist);
        }
    }

    imgin(win_shot_in); // 恢复至输入的处理区域
    std::vector<Rect<int32_t>> win_vseq;
    auto append_vseq = [&] (int32_t win_y, int32_t win_h)
    {
        win_vseq.emplace_back(imgin.pwin().x, (pmode == PM_ENTIRE_NEGATIVE) ? (imgin.height() - win_y - win_h) : win_y, 
                              imgin.pwin().width, win_h);
    };

    if (lut_cache.size() == 0) // 无上一Tile行的Lut，先生成，满足lut_cache.size()为1，hist_cache.size()为0或2
    {
        const int32_t cache_tile_h = rt_slice_height + hist_smooth_radius;
        if (front_neighb < cache_tile_h)
        {
            ISL_LOGD("front neighborhood({}) is not enough, at least: {}, pmode: {}", front_neighb, cache_tile_h, static_cast<int32_t>(pmode));
        }

        if (0 == hist_smooth_radius)
        {
            if (front_neighb > 0)
            {
                append_vseq(subcf0(front_neighb, rt_slice_height), std::min(front_neighb - hist_smooth_radius, rt_slice_height));
                TileHistStatistic(hist_cache, imgin, edge_mask, win_vseq);
                TileLutImpel(lut_cache, nullptr, hist_cache, 0, 1);
                hist_cache.clear();
            }
            else
            {
                lut_cache.resize(tile_cols, {{}, false}); // 使用默认的Lut
            }
        }
        else
        {
            if (front_neighb > hist_smooth_radius)
            {
                append_vseq(subcf0(front_neighb, cache_tile_h), std::min(front_neighb - hist_smooth_radius, rt_slice_height));
                append_vseq(front_neighb - hist_smooth_radius, hist_smooth_radius);
                append_vseq(front_neighb, hist_smooth_radius);
                TileHistStatistic(hist_cache, imgin, edge_mask, win_vseq);
                TileLutImpel(lut_cache, nullptr, hist_cache, 0, 3);
                for (size_t j = 0; j < tile_cols; ++j)
                {
                    hist_cache.pop_front();
                }
            }
            else if (front_neighb > 0)
            {
                append_vseq(0, front_neighb);
                append_vseq(front_neighb, hist_smooth_radius);
                TileHistStatistic(hist_cache, imgin, edge_mask, win_vseq);
                TileLutImpel(lut_cache, nullptr, hist_cache, 0, 2);
            }
            else
            {
                lut_cache.resize(tile_cols, {{}, false}); // 使用默认的Lut
                hist_cache.resize(tile_cols, {{{}, 0}, {{}, 0}});
                append_vseq(0, hist_smooth_radius);
                TileHistStatistic(hist_cache, imgin, edge_mask, win_vseq);
            }
        }
    }

    /// 对输入的处理区域做直方图统计
    int32_t stat_unit = (0 == hist_smooth_radius) ? rt_slice_height : hist_smooth_radius; // 每次统计的高度
    int32_t stat_cnt = (imgin.pwin().height - hist_smooth_radius) / stat_unit; // 去掉最上面一Tile行（上面的流程已统计了）
    int32_t stat_yoffs = front_neighb + hist_smooth_radius; // 统计的垂直偏移
    win_vseq.clear();
    for (int32_t i = 0; i < stat_cnt; ++i)
    {
        append_vseq(stat_yoffs, stat_unit);
        stat_yoffs += stat_unit;
    }
    int32_t stat_left = imgin.height() - stat_yoffs; // 可统计的剩余高度
    if (imgin.pwin().height - hist_smooth_radius > stat_unit * stat_cnt) // 输入的处理区域还有剩余未统计，尽量补齐到stat_unit
    {
        int32_t htmp = std::min(stat_unit, stat_left);
        append_vseq(stat_yoffs, htmp);
        stat_yoffs += htmp;
        stat_left -= htmp;
    }
    if (hist_smooth_radius > 0 && stat_left > 0) // 直方图平滑的下邻域
    {
        int32_t htmp = std::min(hist_smooth_radius, stat_left);
        append_vseq(stat_yoffs, htmp);
        stat_yoffs += htmp;
        stat_left -= htmp;
    }
    TileHistStatistic(hist_cache, imgin, edge_mask, win_vseq); // 至此，hist_cache.size()应不小于(impel_cnt+1) * tile_cols

    /// 对统计的直方图进行Lut推理
    int32_t impel_cnt = imgin.pwin().height / rt_slice_height;
    int32_t hist_in1slice = (hist_smooth_radius > 0) ? (rt_slice_height / hist_smooth_radius) : 1;
    int32_t smooth_num = (hist_smooth_radius > 0) ? (hist_in1slice + 2) : 1;
    int32_t lut_idx = 0;
    for (lut_idx = 0; lut_idx < impel_cnt; ++lut_idx)
    {
        std::deque<tile_hist_t>* hist_smooth = (pmode == PM_SLICE_SEQUENCE) ? &this->hist_smooth_rt : nullptr;
        TileLutImpel(lut_cache, hist_smooth, hist_cache, lut_idx * hist_in1slice, smooth_num); // 注意：最后一次处理，实际的平滑数可能会少于smooth_num，该函数内部有保护
        pads_t&& pads_diff = (pmode == PM_ENTIRE_NEGATIVE) ? // 反向，上邻域减小，下邻域增多；正向，上邻域增加，下邻域减小
            ((0 == lut_idx) ? std::make_pair(imgin.pwin().height - rt_slice_height, 0) : std::make_pair(-rt_slice_height, rt_slice_height)) :
            ((0 == lut_idx) ? std::make_pair(0, imgin.pwin().height - rt_slice_height) : std::make_pair(rt_slice_height, -rt_slice_height));
        imgin.move_vpads(pads_diff);
        imgout.move_vpads(pads_diff);
        TileLutApply(imgout, imgin, lut_cache, lut_idx, lut_idx + 1, (pmode == PM_ENTIRE_NEGATIVE));
    }
    if (win_shot_in.height > impel_cnt * rt_slice_height) // 仍剩余部分Lut未推理，使用剩余所有的直方图
    {
        int32_t hleft = win_shot_in.height - impel_cnt * rt_slice_height;
        smooth_num = hist_cache.size() / tile_cols - impel_cnt * hist_in1slice;
        TileLutImpel(lut_cache, nullptr, hist_cache, impel_cnt * hist_in1slice, smooth_num);
        if (impel_cnt > 0) // 上面已做过pads的移动，再次移动；否则直接处理原图区域
        {
            pads_t&& pads_diff = (pmode == PM_ENTIRE_NEGATIVE) ? 
                std::make_pair(-hleft, rt_slice_height) : std::make_pair(rt_slice_height, -hleft);
            imgin.move_vpads(pads_diff);
            imgout.move_vpads(pads_diff);
        }
        TileLutApply(imgout, imgin, lut_cache, lut_idx, lut_idx+1, (pmode == PM_ENTIRE_NEGATIVE));
        ++lut_idx;
    }

    if (rear_neighb > 0)
    {
        int32_t hnb = std::min(rear_neighb, lace_neighb);
        pads_t&& pads_diff = (pmode == PM_ENTIRE_NEGATIVE) ? 
            std::make_pair(-hnb, imgin.pwin().height) : std::make_pair(imgin.pwin().height, -hnb);
        imgin.move_vpads(pads_diff);
        imgout.move_vpads(pads_diff);
        TileLutApply(imgout, imgin, lut_cache, lut_idx, lut_idx, (pmode == PM_ENTIRE_NEGATIVE));
    }

    imgin(win_shot_in);
    imgout(win_shot_out);
    if (!bRtSliceContinuedCopied && front_neighb > 0) // 实时处理模式下，若前一次输出的作为当前上邻域直接使用，这里跳过不可重复处理
    {
        int32_t hnb = std::min(front_neighb, lace_neighb);
        pads_t&& pads_diff = (pmode == PM_ENTIRE_NEGATIVE) ? 
            std::make_pair(imgin.pwin().height, -hnb) : std::make_pair(-hnb, imgin.pwin().height);
        imgin.move_vpads(pads_diff);
        imgout.move_vpads(pads_diff);
        TileLutApply(imgout, imgin, lut_cache, 0, 0, (pmode != PM_ENTIRE_NEGATIVE));    // 反序从下至上, 无prev_idx信息, 保持和rear_neighbor处理方式一致
        imgin(win_shot_in);
        imgout(win_shot_out);
    }
    if (b_dump_imgio)
    {
        imgout.dump(fn_imgio, false);
    }

    blc_module();

    if (pmode == PM_SLICE_SEQUENCE)
    {
        /// 清除cache中无效数据
        if (hist_smooth_radius > 0)
        {
            while (hist_cache.size() / tile_cols > 2)
            {
                for (size_t j = 0; j < tile_cols; ++j)
                {
                    hist_cache.pop_front();
                }
            }
        }
        else
        {
            hist_cache.clear();
        }
        if (lut_cache.size() / tile_cols > 1) // lut_cache只需要一组
        {
            for (size_t j = 0; j < tile_cols; ++j)
            {
                lut_cache.pop_front();
            }
        }
    }

    return 0;
}


/**
 * @fn      AutoSharpnessEnhance
 * @brief   自动锐化增强，分3步：高亮降噪、边缘增强、纹理增强
 * 
 * @param   [OUT] imgout 输出图像，数据类型、处理区域尺寸需和imgin相同，与imgin不可同源 
 * @param   [IN] imgin 输入图像，仅对pwin()区域做处理
 * @param   [IN] imgtmp 临时图像内存，需有3块，每块内存的数据类型、整图尺寸需和imgin相同
 * 
 * @return  int32_t 0：处理正常，其他：异常
 */
int32_t PipeGray::AutoSharpnessEnhance(Imat<uint16_t>& imgout, Imat<uint16_t>& imgin, std::array<Imat<uint16_t>*, 3>& imgtmp)
{
    if (!(imgout.isvalid() && imgin.isvalid() && imgout.itype() == imgin.itype() && imgout.pwin().size() == imgin.pwin().size()))
    {
        ISL_LOGE("imgin: {}; imgout: {}", imgin, imgout);
        return -1;
    }

    for (auto& iimg : imgtmp)
    {
        if (iimg == nullptr)
        {
            ISL_LOGE("iimg is nullptr");
            return -1;
        }
        if (!(iimg->isvalid() && iimg->itype() == imgin.itype()))
        {
            ISL_LOGE("iimg: {}", *iimg);
            return -1;
        }
        if (!(iimg->isize() == imgin.isize()))
        {
            iimg->reshape(imgin.isize(), imgin.channels(), imgin.get_vpads(), imgin.get_hpads());
        }
    }

    bool b_dump_imgio = false, b_dump_edge = false;
    std::stringstream ss(this->dump_dir);
    ss.seekp(0, std::ios_base::end);
    ss << std::setw(6) << std::setfill('0') << this->proc_count;
    std::string&& dump_fn = ss.str();
    std::string fn_imgio = dump_fn + "_nreeio";
    std::string fn_ee = dump_fn + "_edge";
    ss.str(""); // 清空stringstream
    ss << '_' << imgin.width() << 'x' << imgin.height();
    std::string&& res_pattern = ss.str();
    if ((dump_points & dp_imgio) && (this->dump_counts))
    {
        b_dump_imgio = true;
        imgin.dump(fn_imgio, true);
        this->dump_counts--;
    }

    Imat<uint16_t>& imgtmp0 = *imgtmp[0];
    Imat<uint16_t>& imgtmp1 = *imgtmp[1];
    Imat<uint16_t>& imgtmp2 = *imgtmp[2];
    Rect<int32_t> win_shot_in = imgin.pwin(), win_shot_out = imgout.pwin();
    // 上色模块cbcr平滑计算权重需要保证灰度数据pwin区域的上下两个像素点邻域数据正确, 因Y方向均值滤波会消耗一个像素点邻域, 因此在此处扩充一个像素点邻域
    imgin.move_vpads({-edge_neighb, -edge_neighb});
    imgout.move_vpads({-edge_neighb, -edge_neighb});
    const Rect<int32_t>& win_pads_in = imgin.pwin();
    for (auto& iimg : imgtmp) // imgtmp邻域与imgin同步
    {
        (*iimg)(win_pads_in);
    }

    #if 0 // Edge的效果不佳，暂时屏蔽，放开后做高斯滤波的区域需同步调整
    /// Edge
    Imat<int16_t> edge_gaus(imgtmp1);
    egde.EdgeDetect(edge_gaus, imgout, img_gaus, {200, 8000});
    egde.EdgeEnhance(imgout, imgout, edge_gaus, 0.6);
    if (b_dump_edge)
    {
        img_gaus.dump(fn_ee, true);
        edge_gaus.dump(fn_ee, false);
        imgout.dump(fn_ee, false);
    }
    #endif

    /// Texture
    Imat<uint16_t>& img_mean = imgtmp0;
    #if 1
    Imat<int16_t> texture_ver(imgtmp1), texture_hor(imgtmp2);
    /**
     * TODO: EdgeDetect()的入参dif_range.first，需要根据输入到NREE模块的图像噪声调整， 
     *       和探测板信噪比、降噪等级、对比度增强等均有一定关系，
     *       若输入图像噪声较小，可以降低该值以显示出更多小细节 
     */
    Filter::MeanX(img_mean, imgin, 1);
    texture.EdgeDetectUsm(texture_ver, imgin, img_mean, {200, 10000});
    Filter::MeanY(img_mean, imgin, 1);
    texture.EdgeDetectUsm(texture_hor, imgin, img_mean, {200, 10000});
    if ((dump_points & dp_edge) && (this->dump_counts))
    {
        b_dump_edge = true;
        this->dump_counts--;
        imgin.dump(fn_ee, true);
        texture_ver.dump(fn_ee, false);
        texture_hor.dump(fn_ee, false);
    }

    Imat<int16_t>& texture_around = texture_hor;
    for (int32_t i = 0, row = win_pads_in.y; i < win_pads_in.height; ++i, ++row)
    {
        int16_t* src = texture_ver.ptr(row, win_pads_in.x);
        int16_t* dst = texture_around.ptr(row, win_pads_in.x);
        for (int32_t j = 0; j < win_pads_in.width; ++j, ++src, ++dst)
        {
            *dst += (*src * 1.3); // 垂直方向（时间方向）的清晰度较水平方向要弱一些，所以垂直方向的增益大一些
        }
    }
    texture.EdgeEnhance(imgout, imgin, texture_hor, 0.3);
    #else // 无方向均值滤波
    Imat<int16_t> texture_around(imgtmp1);
    Filter::Mean(img_mean, imgout, imgtmp2, 1);
    texture.EdgeDetect(texture_around, imgout, img_mean, {600, 10000});
    if (b_dump_edge)
    {
        texture_around.dump(fn_ee, false);
    }
    texture.EdgeEnhance(imgout, imgout, texture_around, 0.2);
    #endif
    imgin(win_shot_in);
    imgout(win_shot_out);

    if (b_dump_edge)
    {
        texture_around.dump(fn_ee, false);
        imgout.dump(fn_ee, false);
        std::string::size_type res_pos = fn_ee.rfind(res_pattern);
        if (res_pos != std::string::npos)
        {
            ss.str(""); // 清空stringstream
            ss << '_' << imgin.width() << 'x' << (std::filesystem::file_size(fn_ee) / imgin.elemsize() / imgin.width());
            std::string fn_bak(fn_ee);
            fn_ee.replace(res_pos, res_pattern.length(), ss.str());
            std::filesystem::rename(fn_bak, fn_ee);
        }
    }
    if (b_dump_imgio)
    {
        imgout.dump(fn_imgio, false);
        std::string::size_type res_pos = fn_imgio.rfind(res_pattern);
        if (res_pos != std::string::npos)
        {
            ss.str(""); // 清空stringstream
            ss << '_' << imgin.width() << 'x' << (std::filesystem::file_size(fn_imgio) / imgin.elemsize() / imgin.width());
            std::string fn_bak(fn_imgio);
            fn_imgio.replace(res_pos, res_pattern.length(), ss.str());
            std::filesystem::rename(fn_bak, fn_imgio);
        }
    }

    return 0;
}


void PipeGray::SetDump(uint32_t dps, std::string dir, uint32_t dpc)
{
    _SetDump(this->dump_points, this->dump_dir, dps, dir, this->pipe_chan);
    this->dump_dir.append("-gray_");
    this->dump_counts = dpc;

    return;
}


/**
 * RGB与转YUV的互转，使用BT.709 Full Range标准
 * R、G、B、Y为uint8_t类型，取值范围：[0, 255]
 * Cb、Cr为int8_t类型，取值范围为：[-128, 127] 
 *  
 * |Y |   |  0.2126  0.7152  0.0722 |   |R|
 * |Cb| = | -0.1146 -0.3854  0.5    | * |G|
 * |Cr|   |  0.5    -0.4542 -0.0458 |   |B|
 */
auto _RGB2Y = [] (uint8_t r, uint8_t g, uint8_t b) -> uint8_t { return (218 * r + 732 * g + 74 * b + 512) >> 10; };
auto _RGB2Cb = [] (uint8_t r, uint8_t g, uint8_t b) -> int8_t { return (-117 * r - 395 * g + 512 * b + 512) >> 10; };
auto _RGB2Cr = [] (uint8_t r, uint8_t g, uint8_t b) -> int8_t { return (512 * r - 465 * g - 47 * b + 512) >> 10; };

auto _YCbCr2R = [] (uint8_t y, [[maybe_unused]]int8_t cb, int8_t cr) -> uint8_t 
                    { return static_cast<uint8_t>((std::clamp(1612 * cr + (y << 10), 0, 261120) + 512) >> 10); };
auto _YCbCr2G = [] (uint8_t y, int8_t cb, int8_t cr) -> uint8_t 
                    { return static_cast<uint8_t>((std::clamp(-192 * cb - 479 * cr + (y << 10), 0, 261120) + 512) >> 10); };
auto _YCbCr2B = [] (uint8_t y, int8_t cb, [[maybe_unused]]int8_t cr) -> uint8_t 
                    { return static_cast<uint8_t>((std::clamp(1900 * cb + (y << 10), 0, 261120) + 512) >> 10); };


/**
 * @fn      PipeChroma
 * @brief   PipeChroma流程的默认构造
 */
PipeChroma::PipeChroma() : PipeComm(), zctab(zctab_cols, zctab_rows, zctab_ptr.get()), zctab_inter(zctab_cols, zctab_rows, zctabinter_ptr.get())
// PipeChroma::PipeChroma() : PipeComm(), zctab(zctab_cols, zctab_rows, zctab_ptr.get())
{
    SetBrightnessParams();
}


/**
 * @fn      SetBrightnessParams
 * @brief   设置PipeChroma流程中的亮度参数
 * 
 * @param   [IN] bgGrayDefault 前级Pipe输出的背景灰度值，范围[50000, 65535]
 * @param   [IN] hltRange 高亮范围，first和second的取值范围均为[0, 65535]，且first不大于second
                          first：高亮起始，该值越小，高亮区（易穿区）动态范围被压缩的越多
                          second：高亮结束，该值在PipeChroma流程中不使用
 */
int32_t PipeChroma::SetBrightnessParams(int32_t bgGrayDefault, std::pair<uint16_t, uint16_t> hltRange)
{
    this->bg_gray_default = std::clamp(bgGrayDefault, 50000, 65535);
    this->hlt_vi_begin = hltRange.first;

    SetHlpenYout(this->hlpen_ratio, this->gray_offs, this->bg_color >> 16); // 重置高低穿与高亮区映射曲线

    return 0;
}


/**
 * @fn      SetHlpenYout
 * @brief   设置高低穿系数与灰度偏移值，并重新生成hlpen_lut
 * 
 * @param   [IN] hlpenRatio 高低穿系数，范围[0, 1]，小于0.5为低穿系数，大于0.5为高穿系数
 * @param   [IN] yOffs 灰度偏移值，正值为+，负值为-，取值范围：[-1023, 1023] 
 * @param   [IN] voBgl VO的背景亮度
 */
void PipeChroma::SetHlpenYout(float64_t hlpenRatio, int32_t yOffs, uint8_t voBgl)
{
    this->hlpen_ratio = std::clamp(hlpenRatio, 0.0, 1.0);
    this->gray_offs = std::clamp(yOffs, -300, 300);
    this->hlt_vo.second = static_cast<float64_t>(std::max(static_cast<int32_t>(voBgl), 229)) / 255.0;
    if (this->gray_offs < 0)
    {
        this->hlt_vo.second += static_cast<float64_t>(this->gray_offs) / Lace::lut_imax;
    }
    this->hlt_vo.first = 240.0 / 255.0 * this->hlt_vo.second;

    /// 生成高低穿亮度映射曲线
    if (this->hlpen_ratio > 0.5) // 高穿，关注低灰度区
    {
        float64_t hpenRatio = (this->hlpen_ratio - 0.5) * 12; // (0.5, 1.0] -> (0, 6]
        for (size_t i = 0; i < Lace::lut_bins; i++)
        {
            hlpen_lut[i] = std::log(1.0 + hpenRatio * i / Lace::lut_imax) / std::log(1.0 + hpenRatio) * Lace::lut_imax;
        }
    }
    else if (this->hlpen_ratio < 0.5) // 低穿曲线是高穿映射曲线关于y=x对称计算得到的
    {
        float64_t lpenRatio = (0.5 - this->hlpen_ratio) * 24; // [0, 0.5) -> [12, 0)
        for (size_t i = 0; i < Lace::lut_bins; i++)
        {
            hlpen_lut[i] = (std::pow(1.0 + lpenRatio, static_cast<float64_t>(i) / Lace::lut_imax) - 1.0) / lpenRatio * Lace::lut_imax;
        }
    }
    else // 线性
    {
        std::iota(hlpen_lut.begin(), hlpen_lut.end(), 0);
    }

    /// 起点拉伸到“增亮值”，终点拉伸到“背景亮度-减暗值”
    std::map<float64_t, float64_t> iomap{
        {0.0, this->gray_offs > 0 ? static_cast<float64_t>(this->gray_offs) / Lace::lut_imax : 0.0}, 
        {1.0, this->hlt_vo.second}
    };
    if (this->bg_gray_default < 65535)
    {
        iomap.insert({static_cast<float64_t>(this->bg_gray_default) / 65535.0, iomap.at(1.0)});
    }
    if (DynRangeRedistribute(hlpen_lut, hlpen_lut, iomap))
    {
        ISL_LOGE("dynamic range redistribute failed #1, map: {}", iomap);
    }

    /// 重新映射高亮区曲线
    if (this->hlpen_ratio >= 0.5 && this->hlt_vi_begin < this->bg_gray_default && // 非低穿，且hlt_vi_begin小于背景灰度值
        hlpen_lut.at(Lace::lut_imax*this->hlt_vi_begin/65535) < static_cast<uint16_t>(this->hlt_vo.first * Lace::lut_imax)) // 仍比高亮区间起始值要小
    {
        iomap.insert({static_cast<float64_t>(this->hlt_vi_begin) / 65535.0, this->hlt_vo.first});
    }
    if (DynRangeRedistribute(hlpen_lut, hlpen_lut, iomap))
    {
        ISL_LOGE("dynamic range redistribute failed #2, map: {}", iomap);
    }

    return;
}


/**
 * @fn      SetBgColor
 * @brief   设置PipeChroma流程中的背景颜色和背景亮度
 * 
 * @param   [IN] bgc 背景颜色，BGR格式，[0, 7]-B，[8, 15]-G，[16, 23]-R，内部转换为UV值使用
 * @param   [IN] bgl 背景亮度，取值范围[0, 255]
 */
void PipeChroma::SetBgColor(uint32_t bgc, uint8_t bgl)
{
    const uint8_t r = (bgc >> 16) & 0xFF, g = (bgc >> 8) & 0xFF, b = bgc & 0xFF;
    this->bg_color = (bgl << 16) | ((128 + _RGB2Cb(r, g, b)) << 8) | (128 + _RGB2Cr(r, g, b));

    SetHlpenYout(this->hlpen_ratio, this->gray_offs, bgl); // 重置高低穿与高亮区映射曲线

    return;
}


/**
 * @fn      GetBgColor
 * @brief   获取当前的背景色
 * 
 * @param   [IN] bInverse 是否反色
 * 
 * @return  uint32_t 背景RGB值（BGR24格式，B在低8位，R在高8位）
 */
uint32_t PipeChroma::GetBgColor(bool bInverse)
{
    uint8_t y = (this->bg_color >> 16) & 0xFF;
    int8_t cb = -128 + ((this->bg_color >> 8) & 0xFF);
    int8_t cr = -128 + (this->bg_color & 0xFF);

    if (bInverse)
    {
        y = 0xFF - y;
    }

    return (_YCbCr2R(y, cb, cr) << 16) | (_YCbCr2G(y, cb, cr) << 8) | _YCbCr2B(y, cb, cr);
}


/**
 * @fn      SetZColorTable
 * @brief   设置PipeChroma流程中的“灰度值-原子序数”颜色表
 * 
 * @param   [IN] rgb XRay模块中RGB格式的颜色表
 * @param   [IN] ylvl 灰度等级，暂仅支持1024
 * @param   [IN] znum 原子序数分类数，暂仅支持44
 * 
 * @return  int32_t 0：设置成功，其他：失败
 */
int32_t PipeChroma::SetZColorTable(uint8_t* rgb, int32_t ylvl, int32_t znum)
{
    uint8_t* pcin = rgb;
    std::vector<float64_t> yIn(ylvl, 0.0);

    if (nullptr == rgb || (ylvl != this->zctab_cols) || znum > this->zctab_rows)
    {
        ISL_LOGE("rgb({}) OR ylvl({}) OR znum({}) is invalid", (void*)rgb, ylvl, znum);
        return -1;
    }

    /// 颜色表RGB域转YUV域 
    this->znum_max = znum;
    for (int32_t i = 0; i < znum; i++)
    {
        uint8_t* pYout = zctab.y.ptr(i);
        int8_t* pUout = zctab.cb.ptr(i);
        int8_t* pVout = zctab.cr.ptr(i);
        uint8_t* p1rgb = pcin; // 每个Z对应的RGB表

        for (auto& ii : yIn)
        {
            uint8_t r = *pcin++, g = *pcin++, b = *pcin++;
            ii = 1023.0 * _RGB2Y(r, g, b) / 255.0;
        }

        for (int32_t j = 0, prev = 0, next = 0; j < zctab_cols; j++)
        {
            while (yIn.at(next) <= static_cast<float64_t>(j))
            {
                prev = next;
                if (next+1 < ylvl)
                {
                    next++;
                }
                else
                {
                    break;
                }
            }

            uint8_t* rgb_prev = p1rgb + prev * 3;
            uint8_t* rgb_next = p1rgb + next * 3;
            uint8_t r = *rgb_prev, g = *(rgb_prev+1), b = *(rgb_prev+2);
            if (prev != next)
            {
                float64_t pwt = (yIn.at(next) - static_cast<float64_t>(j)) / (yIn.at(next) - yIn.at(prev));
                r = static_cast<uint8_t>(std::round(pwt * r + (1.0 - pwt) * (*rgb_next)));
                g = static_cast<uint8_t>(std::round(pwt * g + (1.0 - pwt) * (*(rgb_next+1))));
                b = static_cast<uint8_t>(std::round(pwt * b + (1.0 - pwt) * (*(rgb_next+2))));
            }

            *pYout++ = _RGB2Y(r, g, b);
            *pUout++ = _RGB2Cb(r, g, b);
            *pVout++ = _RGB2Cr(r, g, b);
        }
    }

    zctab_inter.fromYCbCr(zctab);

    if ((dump_points & dp_ctbe) && (this->dump_counts))
    {
        std::string&& dump_fn = this->dump_dir + "ctbe0-yuv";
        zctab.y.dump(dump_fn, true);
        zctab.cb.dump(dump_fn, false);
        zctab.cr.dump(dump_fn, false);
        this->dump_counts--;
    }

     return 0;
}

#ifdef X86_PLATFORM
static void CalcChromaSmoothWeight(Imat<uint8_t>& cWt, Imat<int8_t>& cbOri, Imat<int8_t>& crOri, Imat<int8_t>& crSmo, float64_t wtCoeff)
{
    auto cabs = [&](auto ori, auto smo) -> auto { 
        return (ori >= smo) ? ((ori >= 0) ? (ori - smo) : 0) : (smo - ori); 
    };

    // 预计算 pow 查找表
    alignas(32) uint8_t pow_lut[256];
    for (int i = 0; i < 256; ++i) {
        float64_t fdiff = std::min(static_cast<float64_t>(i) / 128.0, 1.0);
        pow_lut[i] = static_cast<uint8_t>(std::pow(fdiff, wtCoeff) * 255.0);
    }

    const __m256i zero = _mm256_setzero_si256();
    const __m256i const_255 = _mm256_set1_epi8(255);
    const __m256i sign_mask = _mm256_set1_epi8(0x80); // 符号位掩码

    #pragma omp parallel num_threads(8)
    {
        #pragma omp for schedule(static)
        for (int32_t i = 0; i < cWt.pwin().height; ++i)
        {
            uint8_t* pWt = cWt.ptr(cWt.pwin().y + i, cWt.pwin().x);
            int8_t* pCrSmo = crSmo.ptr(crSmo.pwin().y + i, crSmo.pwin().x);
            int8_t* pCrOri = crOri.ptr(crOri.pwin().y + i, crOri.pwin().x);
            int8_t* pCbOri = cbOri.ptr(cbOri.pwin().y + i, cbOri.pwin().x);
            
            int32_t width = cWt.pwin().width;
            
            // 初始化整行为 0
            memset(pWt, 0, width * sizeof(uint8_t));
            
            int32_t j = 0;
            
            // AVX 处理（每次处理 32 个像素）
            for (; j <= width - 32; j += 32)
            {
                // 数据预取
                _mm_prefetch(reinterpret_cast<const char*>(pCbOri + j + 64), _MM_HINT_T0);
                _mm_prefetch(reinterpret_cast<const char*>(pCrOri + j + 64), _MM_HINT_T0);
                _mm_prefetch(reinterpret_cast<const char*>(pCrSmo + j + 64), _MM_HINT_T0);
                
                // 加载数据
                __m256i cb_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pCbOri + j));
                __m256i cr_ori_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pCrOri + j));
                __m256i cr_smo_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pCrSmo + j));
                
                // 创建 Cb < 0 的掩码（使用符号位比较）
                __m256i cb_mask = _mm256_cmpgt_epi8(zero, cb_vec);
                
                // 如果没有需要处理的像素，直接跳过
                if (_mm256_testz_si256(cb_mask, cb_mask)) {
                    continue;
                }
                
                // 检测符号变化：Cr_ori 和 Cr_smo 符号位不同
                __m256i ori_sign = _mm256_and_si256(cr_ori_vec, sign_mask);
                __m256i smo_sign = _mm256_and_si256(cr_smo_vec, sign_mask);
                __m256i sign_change_mask = _mm256_cmpeq_epi8(ori_sign, smo_sign);
                sign_change_mask = _mm256_xor_si256(sign_change_mask, _mm256_set1_epi8(-1));
                
                // 计算绝对值差
                __m256i abs_diff = _mm256_abs_epi8(_mm256_sub_epi8(cr_ori_vec, cr_smo_vec));
                
                // 处理符号变化的情况：idiff * idiff
                // 将 int8_t 扩展为 int16_t 进行乘法
                __m256i abs_diff_low = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(abs_diff));
                __m256i abs_diff_high = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(abs_diff, 1));
                
                __m256i squared_low = _mm256_mullo_epi16(abs_diff_low, abs_diff_low);
                __m256i squared_high = _mm256_mullo_epi16(abs_diff_high, abs_diff_high);
                
                // 打包回 uint8_t 并限制在 255 以内
                __m256i squared_packed = _mm256_packus_epi16(
                    _mm256_min_epu16(squared_low, _mm256_set1_epi16(255)),
                    _mm256_min_epu16(squared_high, _mm256_set1_epi16(255))
                );
                __m256i result_sign_change = _mm256_min_epu8(squared_packed, const_255);
                
                // 处理符号不变且 Cr > 0 的情况（橙色区域）
                __m256i cr_pos_mask = _mm256_cmpgt_epi8(cr_ori_vec, zero);
                __m256i valid_mask = _mm256_and_si256(cb_mask, cr_pos_mask);
                valid_mask = _mm256_andnot_si256(sign_change_mask, valid_mask);
                
                // 使用标量方式处理查找表（AVX2 没有直接的查找表指令）
                alignas(32) uint8_t result_orange_arr[32] = {0};
                alignas(32) int8_t abs_diff_arr[32];
                alignas(32) uint8_t valid_mask_arr[32];
                
                _mm256_store_si256(reinterpret_cast<__m256i*>(abs_diff_arr), abs_diff);
                _mm256_store_si256(reinterpret_cast<__m256i*>(valid_mask_arr), valid_mask);
                
                for (int k = 0; k < 32; ++k) {
                    if (valid_mask_arr[k]) {
                        int diff = abs_diff_arr[k] & 0xFF;
                        result_orange_arr[k] = pow_lut[std::min(diff, 255)];
                    }
                }
                
                __m256i result_orange = _mm256_load_si256(reinterpret_cast<const __m256i*>(result_orange_arr));
                
                // 合并结果
                __m256i final_sign_change = _mm256_and_si256(result_sign_change, sign_change_mask);
                __m256i final_orange = _mm256_and_si256(result_orange, valid_mask);
                __m256i final_result = _mm256_or_si256(final_sign_change, final_orange);
                
                // 只更新需要处理的像素
                __m256i current_wt = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pWt + j));
                __m256i updated_wt = _mm256_blendv_epi8(current_wt, final_result, cb_mask);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pWt + j), updated_wt);
            }
            
            // 处理剩余像素（标量处理）
            for (; j < width; ++j)
            {
                int8_t cb_ori = pCbOri[j];
                if (cb_ori >= 0) continue;
                
                int8_t cr_ori = pCrOri[j];
                int8_t cr_smo = pCrSmo[j];
                
                if (((cr_ori ^ cr_smo) & 0x80) != 0) { // 符号变化检测
                    int32_t idiff = cabs(cr_ori, cr_smo);
                    pWt[j] = std::min(idiff * idiff, 255);
                } else if (cr_ori > 0) {
                    int32_t diff = cabs(cr_ori, cr_smo);
                    pWt[j] = pow_lut[std::min(diff, 255)];
                }
            }
        }
    }
}
#elif defined(RK3588_PLATFORM)
static void CalcChromaSmoothWeight(Imat<uint8_t>& cWt, Imat<int8_t>& cbOri, Imat<int8_t>& crOri, Imat<int8_t>& crSmo, float64_t wtCoeff)
{
    auto cabs = [&](auto ori, auto smo) -> auto { 
        return (ori >= smo) ? ((ori >= 0) ? (ori - smo) : 0) : (smo - ori); 
    };

    // 预计算 pow 查找表
    alignas(32) uint8_t pow_lut[256];
    for (int i = 0; i < 256; ++i) {
        float64_t fdiff = std::min(static_cast<float64_t>(i) / 128.0, 1.0);
        pow_lut[i] = static_cast<uint8_t>(std::pow(fdiff, wtCoeff) * 255.0);
    }

    const int8x16_t zero_i8 = vdupq_n_s8(0);
    const uint8x16_t const_255 = vdupq_n_u8(255);
    const int8x16_t sign_mask = vdupq_n_s8(0x80); // 符号位掩码

    #pragma omp parallel num_threads(8)
    {
        #pragma omp for schedule(static)
        for (int32_t i = 0; i < cWt.pwin().height; ++i)
        {
            uint8_t* pWt = cWt.ptr(cWt.pwin().y + i, cWt.pwin().x);
            int8_t* pCrSmo = crSmo.ptr(crSmo.pwin().y + i, crSmo.pwin().x);
            int8_t* pCrOri = crOri.ptr(crOri.pwin().y + i, crOri.pwin().x);
            int8_t* pCbOri = cbOri.ptr(cbOri.pwin().y + i, cbOri.pwin().x);
            
            int32_t width = cWt.pwin().width;
            
            // 初始化整行为 0
            memset(pWt, 0, width * sizeof(uint8_t));
            
            int32_t j = 0;
            
            // NEON 处理（每次处理 16 个像素）
            for (; j <= width - 16; j += 16)
            {
                // 加载数据
                int8x16_t cb_vec = vld1q_s8(pCbOri + j);
                int8x16_t cr_ori_vec = vld1q_s8(pCrOri + j);
                int8x16_t cr_smo_vec = vld1q_s8(pCrSmo + j);
                
                // 创建 Cb < 0 的掩码
                uint8x16_t cb_mask = vcltq_s8(cb_vec, zero_i8);
                
                // 如果没有需要处理的像素，直接跳过
                if (vmaxvq_u8(cb_mask) == 0) {
                    continue;
                }
                
                // 检测符号变化：Cr_ori 和 Cr_smo 符号位不同
                int8x16_t ori_sign = vandq_s8(cr_ori_vec, sign_mask);
                int8x16_t smo_sign = vandq_s8(cr_smo_vec, sign_mask);
                uint8x16_t sign_change_mask = vceqq_s8(ori_sign, smo_sign);
                sign_change_mask = vmvnq_u8(sign_change_mask); // 取反
                
                // 计算绝对值差
                int8x16_t diff = vsubq_s8(cr_ori_vec, cr_smo_vec);
                uint8x16_t abs_diff = vreinterpretq_u8_s8(vabsq_s8(diff));
                
                // 处理符号变化的情况：idiff * idiff
                // 将 uint8_t 扩展为 uint16_t 进行乘法
                uint16x8_t abs_diff_low = vmovl_u8(vget_low_u8(abs_diff));
                uint16x8_t abs_diff_high = vmovl_high_u8(abs_diff);
                
                uint16x8_t squared_low = vmulq_u16(abs_diff_low, abs_diff_low);
                uint16x8_t squared_high = vmulq_u16(abs_diff_high, abs_diff_high);
                
                // 打包回 uint8_t 并限制在 255 以内
                uint16x8_t squared_min_low = vminq_u16(squared_low, vdupq_n_u16(255));
                uint16x8_t squared_min_high = vminq_u16(squared_high, vdupq_n_u16(255));
                
                uint8x8_t squared_packed_low = vqmovn_u16(squared_min_low);
                uint8x8_t squared_packed_high = vqmovn_u16(squared_min_high);
                uint8x16_t result_sign_change = vcombine_u8(squared_packed_low, squared_packed_high);
                result_sign_change = vminq_u8(result_sign_change, const_255);
                
                // 处理符号不变且 Cr > 0 的情况（橙色区域）
                uint8x16_t cr_pos_mask = vcgtq_s8(cr_ori_vec, zero_i8);
                uint8x16_t valid_mask = vandq_u8(cb_mask, cr_pos_mask);
                valid_mask = vbicq_u8(valid_mask, sign_change_mask); // 排除符号变化的情况
                
                // 使用标量方式处理查找表（NEON 没有直接的查找表指令）
                alignas(16) uint8_t result_orange_arr[16] = {0};
                alignas(16) uint8_t abs_diff_arr[16];
                alignas(16) uint8_t valid_mask_arr[16];
                
                vst1q_u8(abs_diff_arr, abs_diff);
                vst1q_u8(valid_mask_arr, valid_mask);
                
                for (int k = 0; k < 16; ++k) {
                    if (valid_mask_arr[k]) {
                        int diff = abs_diff_arr[k];
                        result_orange_arr[k] = pow_lut[std::min(diff, 255)];
                    }
                }
                
                uint8x16_t result_orange = vld1q_u8(result_orange_arr);
                
                // 合并结果
                uint8x16_t final_sign_change = vandq_u8(result_sign_change, sign_change_mask);
                uint8x16_t final_orange = vandq_u8(result_orange, valid_mask);
                uint8x16_t final_result = vorrq_u8(final_sign_change, final_orange);
                
                // 只更新需要处理的像素
                uint8x16_t current_wt = vld1q_u8(pWt + j);
                uint8x16_t updated_wt = vbslq_u8(cb_mask, final_result, current_wt);
                vst1q_u8(pWt + j, updated_wt);
            }
            
            // 处理剩余像素（标量处理）
            for (; j < width; ++j)
            {
                int8_t cb_ori = pCbOri[j];
                if (cb_ori >= 0) continue;
                
                int8_t cr_ori = pCrOri[j];
                int8_t cr_smo = pCrSmo[j];
                
                if (((cr_ori ^ cr_smo) & 0x80) != 0) { // 符号变化检测
                    int32_t idiff = cabs(cr_ori, cr_smo);
                    pWt[j] = std::min(idiff * idiff, 255);
                } else if (cr_ori > 0) {
                    int32_t diff = cabs(cr_ori, cr_smo);
                    pWt[j] = pow_lut[std::min(diff, 255)];
                }
            }
        }
    }
    
    
}
#else
static void CalcChromaSmoothWeight(Imat<uint8_t>& cWt, Imat<int8_t>& cbOri, Imat<int8_t>& crOri, Imat<int8_t>& crSmo, float64_t wtCoeff)
{
    auto cabs = [&] (auto ori, auto smo) -> auto { return (ori >= smo) ? ((ori >= 0) ? (ori - smo) : 0) : (smo - ori); };


    // cb > 0 蓝色; cb < 0 黄色
    // cr > 0 红色; cr < 0 绿色
    for (int32_t i = 0; i < cWt.pwin().height; ++i)
	{
        uint8_t* pWt = cWt.ptr(cWt.pwin().y + i, cWt.pwin().x);
        int8_t* pCrSmo = crSmo.ptr(crSmo.pwin().y + i, crSmo.pwin().x);
        int8_t* pCrOri = crOri.ptr(crOri.pwin().y + i, crOri.pwin().x);
        int8_t* pCbOri = cbOri.ptr(cbOri.pwin().y + i, cbOri.pwin().x);
        for (int32_t j = 0; j < cWt.pwin().width; ++j, ++pWt, ++pCrSmo, ++pCrOri, ++pCbOri)
        {
            if (*pCbOri < 0) // 橙色与绿色才做处理
            {
                if (*pCrOri * *pCrSmo <= 0) // Cr符号变化，橙色与绿色交界
                {
                    int32_t idiff = cabs(*pCrOri, *pCrSmo);
                    *pWt = std::min(idiff * idiff, 255);
                }
                else // Cr符号不变，橙色与绿色分别插值
                {
                    if (*pCrOri > 0) // 橙色
                    {
                        float64_t fdiff = std::min(static_cast<float64_t>(cabs(*pCrOri, *pCrSmo)) / 128, 1.0);
                        *pWt = std::pow(fdiff, wtCoeff) * 255.0;
                    }
                    else
                    {
                        *pWt = 0;
                    }
                }
            }
            else
            {
                *pWt = 0;
            }
        }
    }

    return;
}
#endif

// int pCount = 0;
/**
 * @fn      PaintZColorImage
 * @brief   根据“灰度值-原子序数”颜色表伪彩化XRay图像
 * 
 * @param   [OUT] rgbOut RGB图像，交叉格式，内存从低位到高位按B-G-R三通道依次排布，其处理区域尺寸需和grayIn相同
 * @param   [OUT] yuvOut YUV图像，平面格式，其处理区域尺寸需和rgbOut相同，因外部流程不需要，该输出暂不支持
 * @param   [IN] yuvTmp YUV图像临时内存，用于存放中间计算结果，其处理区域需和grayIn完全相同
 * @param   [IN] grayIn XRay灰度图
 * @param   [IN] zIn XRay原子序数图，其处理区域需和grayIn完全相同 
 * @param   [IN] wtIn XRay处理权重图，0为纯背景，255为纯前景，中间值为过渡区，其处理区域需和grayIn相同 
 * @param   [IN] wtArea 非背景区的外接矩形框
 * @param   [IN] dispRange 输入的有效区间，即需显示的灰度图范围（可变吸收率），当first大于second时为反色处理
 * 
 * @return  int32_t 0：处理正常，其他：异常
 */
// int32_t PipeChroma::PaintZColorImage(rgb8c_t& rgbOut, yuv4p_t& yuvOut, yuv4p_t& yuvTmp, Imat<uint16_t>& grayIn, Imat<uint8_t>& zIn, 
//                                      Imat<uint8_t>& wtIn, std::vector<Rect<int32_t>>& wtArea, std::array<int32_t, 44> &zMap, std::pair<uint16_t, uint16_t> dispRange)
// {
//     if (!(grayIn.isvalid() && zIn.isvalid() && wtIn.isvalid() && grayIn.pwin() == wtIn.pwin()))
//     {
//         ISL_LOGE("grayIn: {}, zIn: {}, wtIn: {}", grayIn, zIn, wtIn);
//         return -1;
//     }
//     if (grayIn.pwin() != zIn.pwin() || grayIn.get_vpads() != zIn.get_vpads())
//     {
//         ISL_LOGE("size is unequal, grayIn: {}, zIn: {}", grayIn, zIn);
//         return -1;
//     }
//     if (grayIn.pwin() != wtIn.pwin() || grayIn.get_vpads() != wtIn.get_vpads())
//     {
//         ISL_LOGE("size is unequal, grayIn: {}, wtIn: {}", grayIn, wtIn);
//         return -1;
//     }
//     if (!(rgbOut.isvalid() && rgbOut.channels() == 3 && rgbOut.pwin().size() == grayIn.pwin().size()))
//     {
//         ISL_LOGE("rgbOut is invalid: {}, grayIn.pwin: {}", rgbOut, grayIn.pwin());
//         return -1;
//     }
//     if (!(yuvTmp.isvalid() && yuvTmp.pwin() == grayIn.pwin())) // yuvTmp的处理区域与grayIn相同
//     {
//         ISL_LOGE("yuvTmp is invalid, Y: {}, Cb: {}, Cr: {}", yuvTmp.y, yuvTmp.cb, yuvTmp.cr);
//         return -1;
//     }
//     if (!(yuvOut.isvalid() && yuvOut.pwin().size() == rgbOut.pwin().size())) // yuvOut的处理区域与rgbOut相同
//     {
//         ISL_LOGE("yuvOut is invalid, Y: {}, Cb: {}, Cr: {}", yuvOut.y, yuvOut.cb, yuvOut.cr);
//         return -1;
//     }

//     /// 显示亮度范围映射
//     dispRange.first >>= Lace::ll_shift;
//     dispRange.second >>= Lace::ll_shift;
//     const int32_t ymax = zctab_cols - 1, ydiff = static_cast<int32_t>(dispRange.second) - dispRange.first;
//     auto ymap = [&] (uint16_t val) -> int32_t // 高低穿、反色、可变吸收率的灰度值映射，输入范围：[0, 65535]，输出范围：[0, 1023]
//     {
//         val = this->hlpen_lut.at(val >> Lace::ll_shift);
//         if (0 == ydiff || Lace::lut_imax == ydiff)
//         {
//             return val;
//         }
//         else if (ydiff > 0)
//         {
//             return ymax * (std::clamp(val, dispRange.first, dispRange.second) - dispRange.first) / ydiff;
//         }
//         else //if (ydiff < 0)
//         {
//             return ymax * (dispRange.first - std::clamp(val, dispRange.second, dispRange.first)) / (-ydiff);
//         }
//     };

//     /// 背景的YUV和RGB值，已考虑反色，可变吸收率和高低穿背景亮度不变
//     uint8_t bgY = (this->bg_color >> 16) & 0xFF; if (ydiff < 0) bgY = 255 - bgY; // 反色的背景值
//     const int8_t bgCb = -128 + ((this->bg_color >> 8) & 0xFF), bgCr = -128 + (this->bg_color & 0xFF);
//     const std::array<uint8_t, 3> bgRGB = {_YCbCr2B(bgY, bgCb, bgCr), _YCbCr2G(bgY, bgCb, bgCr), _YCbCr2R(bgY, bgCb, bgCr)}; // B - G - R

//     if (!wtArea.empty())
//     {
//         for (auto it = wtArea.begin(); it != wtArea.end();)
//         {
//             if (!(*it <= wtIn.pwin()))
//             {
//                 *it = it->intersect(wtIn.pwin());
//                 if (it->size().area() <= 0)
//                 {
//                     it = wtArea.erase(it);
//                     continue;
//                 }
//             }
//             ++it;
//         }

//         if (!wtArea.empty())
//         {
//             /********************* 填充背景区域 *********************/
//             std::vector<Rect<int32_t>> bgArea;
//             auto iCurt = wtArea.begin();
//             if (iCurt->y > wtIn.pwin().y)
//             {
//                 bgArea.push_back(Rect<int32_t>(wtIn.pwin().x, wtIn.pwin().y, wtIn.pwin().width, iCurt->y - wtIn.pwin().y));
//             }
//             for (; iCurt != wtArea.end(); ++iCurt)
//             {
//                 auto iNext = std::next(iCurt);
//                 Rect<int32_t> boundary(wtIn.pwin().x, iCurt->y, wtIn.pwin().width, 
//                                        (iNext != wtArea.end() ? iNext->y : wtIn.pwin().posbr().y+1) - iCurt->y);
//                 std::vector<Rect<int32_t>>&& comp = iCurt->complement(boundary);
//                 bgArea.insert(bgArea.end(), comp.begin(), comp.end());
//             }
//             for (auto& iWin : bgArea)
//             {
//                 rgbOut.fill(bgRGB, iWin);
//             }

//             const Rect<int32_t> win_wt = wtIn.pwin(), win_yuv = yuvOut.pwin(), win_rgb = rgbOut.pwin(); // 记录原处理区域
//             for (auto& iWin : wtArea)
//             {
//                 const int32_t bgTopH = iWin.y - win_wt.y, bgLeftW = iWin.x - win_wt.x;

//                 // 重塑yuvOut与rgbOut的前景区域
//                 yuvOut(Rect<int32_t>(win_yuv.x + bgLeftW, win_yuv.y + bgTopH, iWin.width, iWin.height));
//                 rgbOut(Rect<int32_t>(win_rgb.x + bgLeftW, win_rgb.y + bgTopH, iWin.width, iWin.height));

//                 /********************* 处理前景区域 *********************/
//                 Imat<uint8_t>& yOut = yuvOut.y;
//                 Imat<int8_t>& cbSmo = yuvOut.cb;
//                 Imat<int8_t>& crSmo = yuvOut.cr;
//                 Imat<uint8_t>& cSmoWt = yuvTmp.y;       // Ori与Smo融合时，Smo的权重
//                 Imat<int8_t>& cbOri = yuvTmp.cb;
//                 Imat<int8_t>& crOri = yuvTmp.cr;

//                 grayIn(iWin), wtIn(iWin);
//                 auto toffs = grayIn.move_vpads({-this->csmo_neighb, -this->csmo_neighb}); // 垂直方向扩展邻域
//                 auto hoffs = grayIn.move_hpads({-this->csmo_neighb, -this->csmo_neighb}); // 水平方向扩展邻域
//                 Rect<int32_t> win_ext = grayIn.pwin(); // 带邻域的处理区域
//                 zIn(win_ext), cbOri(win_ext), crOri(win_ext);

//                 /// 颜色表映射
//                 toffs.first = -toffs.first; // 遍历yOut的上边界
//                 toffs.second += win_ext.height; // 遍历yOut的下边界
//                 hoffs.first = -hoffs.first; // 遍历yOut的左边界
//                 hoffs.second += win_ext.width; // 遍历yOut的右边界
//                 const uint8_t zidx_max = znum_max - 1;
//                 for (int32_t i = 0; i < win_ext.height; ++i)
//                 {
//                     uint16_t* pGray = grayIn.ptr(grayIn.pwin().y + i, grayIn.pwin().x);
//                     uint8_t* pZ = zIn.ptr(zIn.pwin().y + i, zIn.pwin().x);
//                     uint8_t* pY = (toffs.first <= i && i < toffs.second) ? // 因Y后续无其他处理，可直接写进输出内存，无需写入Tmp内存
//                         yOut.ptr(yOut.pwin().y + i - toffs.first, yOut.pwin().x) : nullptr;
//                     int8_t* pCb = cbOri.ptr(cbOri.pwin().y + i, cbOri.pwin().x);
//                     int8_t* pCr = crOri.ptr(crOri.pwin().y + i, crOri.pwin().x);
//                     for (int32_t j = 0; j < win_ext.width; ++j, ++pGray, ++pZ)
//                     {
//                         int32_t row = std::min(*pZ, zidx_max);
//                         row = zMap.at(row);
//                         int32_t col = ymap(*pGray);
//                         if (pY != nullptr && hoffs.first <= j && j < hoffs.second)
//                         {
//                             *pY++ = *zctab.y.ptr(row, col);
//                         }
//                         *pCb++ = *zctab.cb.ptr(row, col);
//                         *pCr++ = *zctab.cr.ptr(row, col);
//                     }
//                 }

//                 /// 色域的平滑
//                 yuvTmp(iWin);
//                 Imat<int8_t> fTmp(yuvTmp.y); // 滤波函数的临时内存
//                 auto gauss_smooth_0 = std::chrono::high_resolution_clock::now();
//                 Filter::Gaussian(cbSmo, cbOri, fTmp, csmo_neighb, -1.0);
//                 auto gauss_smooth_1 = std::chrono::high_resolution_clock::now();
//                 Filter::Gaussian(crSmo, crOri, fTmp, csmo_neighb, -1.0);

//                 auto gauss_smooth_2 = std::chrono::high_resolution_clock::now();
//                 CalcChromaSmoothWeight(cSmoWt, cbOri, crOri, crSmo, 0.5);
//                 auto gauss_smooth_3 = std::chrono::high_resolution_clock::now();

//                 auto duration_smooth0 = std::chrono::duration_cast<std::chrono::microseconds>(gauss_smooth_1 - gauss_smooth_0);
//                 printf("Step %d: gauss cb cost: %ld us (%.3f ms)\n", pCount, duration_smooth0.count(), duration_smooth0.count() / 1000.0);

//                 auto duration_smooth1 = std::chrono::duration_cast<std::chrono::microseconds>(gauss_smooth_2 - gauss_smooth_1);
//                 printf("Step %d: gauss cr cost: %ld us (%.3f ms)\n", pCount, duration_smooth1.count(), duration_smooth1.count() / 1000.0);

//                 auto duration_smooth2 = std::chrono::duration_cast<std::chrono::microseconds>(gauss_smooth_3 - gauss_smooth_2);
//                 printf("Step %d: cal wt cost: %ld us (%.3f ms)\n", pCount, duration_smooth2.count(), duration_smooth2.count() / 1000.0);

//                 auto duration_smooth3 = std::chrono::duration_cast<std::chrono::microseconds>(gauss_smooth_3 - gauss_smooth_0);
//                 printf("Step %d: total cost: %ld us (%.3f ms)\n", pCount++, duration_smooth3.count(), duration_smooth3.count() / 1000.0);

//                 /// 只计算前景区
//                 for (int32_t i = 0; i < rgbOut.pwin().height; i++)
//                 {
//                     uint8_t* pBgr = rgbOut.ptr(rgbOut.pwin().y + i, rgbOut.pwin().x);
//                     uint8_t* pY = yOut.ptr(yOut.pwin().y + i, yOut.pwin().x);
//                     int8_t* pCbOri = cbOri.ptr(cbOri.pwin().y + i, cbOri.pwin().x);
//                     int8_t* pCrOri = crOri.ptr(crOri.pwin().y + i, crOri.pwin().x);
//                     int8_t* pCbSmo = cbSmo.ptr(cbSmo.pwin().y + i, cbSmo.pwin().x);
//                     int8_t* pCrSmo = crSmo.ptr(crSmo.pwin().y + i, crSmo.pwin().x);
//                     uint8_t* pChrSmoWt = cSmoWt.ptr(cSmoWt.pwin().y + i, cSmoWt.pwin().x); // 色域平滑权重
//                     uint8_t* pFbgSmoWt = wtIn.ptr(wtIn.pwin().y + i, wtIn.pwin().x); // 前景平滑权重
//                     for (int32_t j = 0; j < rgbOut.pwin().width; ++j, ++pY, ++pCbOri, ++pCrOri, ++pCbSmo, ++pCrSmo, ++pChrSmoWt, ++pFbgSmoWt)
//                     {
//                         int32_t fgCb = (static_cast<int32_t>(*pChrSmoWt) * (*pCbSmo) + (255 - *pChrSmoWt) * (*pCbOri) + 127) / 255;
//                         int32_t fgCr = (static_cast<int32_t>(*pChrSmoWt) * (*pCrSmo) + (255 - *pChrSmoWt) * (*pCrOri) + 127) / 255;

//                         *pY = (static_cast<int32_t>(*pFbgSmoWt) * *pY + (255 - *pFbgSmoWt) * bgY + 127) / 255;
//                         *pCbSmo = (static_cast<int32_t>(*pFbgSmoWt) * fgCb + (255 - *pFbgSmoWt) * bgCb + 127) / 255;
//                         *pCrSmo = (static_cast<int32_t>(*pFbgSmoWt) * fgCr + (255 - *pFbgSmoWt) * bgCr + 127) / 255;

//                         *pBgr++ = _YCbCr2B(*pY, *pCbSmo, *pCrSmo);
//                         *pBgr++ = _YCbCr2G(*pY, *pCbSmo, *pCrSmo);
//                         *pBgr++ = _YCbCr2R(*pY, *pCbSmo, *pCrSmo);
//                     }
//                 }
//             }
//         }
//         else
//         {
//             rgbOut.fill(bgRGB, rgbOut.pwin());
//         }
//     }
//     else
//     {
//         rgbOut.fill(bgRGB, rgbOut.pwin());
//     }

//     if ((dump_points & dp_imgio) && (this->dump_counts))
//     {
//         std::stringstream ss(this->dump_dir);
//         ss.seekp(0, std::ios_base::end);
//         ss << std::setw(6) << std::setfill('0') << ++this->proc_count;
//         size_t offs = 0;
//         ss << "_gray" << offs;
//         offs += grayIn.isize().area() * grayIn.elemsize();
//         ss << "_z" << offs;
//         offs += zIn.isize().area() * zIn.elemsize();
//         ss << "_wt" << offs;
//         offs += wtIn.isize().area() * wtIn.elemsize();
//         //ss << "_y" << offs;
//         //offs += yuvOut.y.isize().area() * yuvOut.y.elemsize();
//         //ss << "_cb" << offs;
//         //offs += yuvOut.cb.isize().area() * yuvOut.cb.elemsize();
//         //ss << "_cr" << offs;
//         //offs += yuvOut.cr.isize().area() * yuvOut.cr.elemsize();
//         ss << "_rgb" << offs << "-" << rgbOut.width() << "x" << rgbOut.height();
//         std::string&& dump_fn = ss.str();
//         grayIn.dump(dump_fn, true);
//         zIn.dump(dump_fn, false);
//         wtIn.dump(dump_fn, false);
//         //yuvOut.y.dump(dump_fn, false);
//         //yuvOut.cb.dump(dump_fn, false);
//         //yuvOut.cr.dump(dump_fn, false);
//         rgbOut.dump(dump_fn, false);
//         this->dump_counts--;
//     }

//     return 0;
// }


int gCount = 0;
int32_t PipeChroma::PaintZColorImage(rgb8c_t& rgbOut, yuv4p_t& yuvOut, yuv4p_t& yuvTmp, Imat<uint16_t>& grayIn, Imat<uint8_t>& zIn, 
                                     Imat<uint8_t>& wtIn, [[maybe_unused]]std::vector<Rect<int32_t>>& wtArea, 
                                     std::array<int32_t, 44> &zMap, std::pair<uint16_t, uint16_t> dispRange)
{
    if (!(grayIn.isvalid() && zIn.isvalid() && wtIn.isvalid() && grayIn.pwin() == wtIn.pwin()))
    {
        ISL_LOGE("grayIn: {}, zIn: {}, wtIn: {}", grayIn, zIn, wtIn);
        return -1;
    }
    if (grayIn.pwin() != zIn.pwin() || grayIn.get_vpads() != zIn.get_vpads())
    {
        ISL_LOGE("size is unequal, grayIn: {}, zIn: {}", grayIn, zIn);
        return -1;
    }
    if (grayIn.pwin() != wtIn.pwin() || grayIn.get_vpads() != wtIn.get_vpads())
    {
        ISL_LOGE("size is unequal, grayIn: {}, wtIn: {}", grayIn, wtIn);
        return -1;
    }
    if (!(rgbOut.isvalid() && rgbOut.channels() == 3 && rgbOut.pwin().size() == grayIn.pwin().size()))
    {
        ISL_LOGE("rgbOut is invalid: {}, grayIn.pwin: {}", rgbOut, grayIn.pwin());
        return -1;
    }
    if (!(yuvTmp.isvalid() && yuvTmp.pwin() == grayIn.pwin())) // yuvTmp的处理区域与grayIn相同
    {
        ISL_LOGE("yuvTmp is invalid, Y: {}, Cb: {}, Cr: {}", yuvTmp.y, yuvTmp.cb, yuvTmp.cr);
        return -1;
    }
    if (!(yuvOut.isvalid() && yuvOut.pwin().size() == rgbOut.pwin().size())) // yuvOut的处理区域与rgbOut相同
    {
        ISL_LOGE("yuvOut is invalid, Y: {}, Cb: {}, Cr: {}", yuvOut.y, yuvOut.cb, yuvOut.cr);
        return -1;
    }

    

    Imat<uint8_t>& yOut = yuvOut.y;
    Imat<int8_t>& cbSmo = yuvOut.cb;
    Imat<int8_t>& crSmo = yuvOut.cr;
    Imat<int8_t> fTmp(yuvTmp.y);        // 滤波函数的临时内存
    Imat<uint8_t>& cWt = yuvTmp.y;      // Ori与Smo融合时，Smo的权重
    Imat<int8_t>& cbOri = yuvTmp.cb;
    Imat<int8_t>& crOri = yuvTmp.cr;
    Imat<uint8_t>& yTab = zctab.y;      // 颜色表分量：Y
    Imat<int8_t>& cbTab = zctab.cb;     // 颜色表分量：Cb
    Imat<int8_t>& crTab = zctab.cr;     // 颜色表分量：Cr
    YCbCrInter<uint8_t, int8_t>& ycbcrTbeInter = zctab_inter;

    /// 显示亮度范围映射
    // perf_init();
    dispRange.first >>= Lace::ll_shift;
    dispRange.second >>= Lace::ll_shift;
    // const int32_t ymax = zctab_cols - 1, ydiff = static_cast<int32_t>(dispRange.second) - dispRange.first;

    Rect<int32_t> win_curt = grayIn.pwin();
    Rect<int32_t> win_pads(win_curt.x, subcf0(win_curt.y, csmo_neighb), win_curt.width, win_curt.height+std::min(win_curt.y, csmo_neighb));
    win_pads.height = std::min(win_pads.height + csmo_neighb, grayIn.height() - win_pads.y);
    const int32_t toffs = win_curt.y - win_pads.y;
    grayIn(win_pads), zIn(win_pads), cbOri(win_pads), crOri(win_pads);
    memset(s32VecColPreComputed->data(), 0, s32VecColPreComputed->size() * sizeof(int32_t));
    memset(s32VecRowPreComputed->data(), 0, s32VecRowPreComputed->size() * sizeof(int32_t));
#ifdef X86_PLATFORM
    #pragma omp parallel num_threads(8)
    {
        // 线程局部拷贝LUT，确保缓存对齐
        alignas(32) std::array<int32_t, 44> local_zMap = zMap;
        alignas(32) std::array<uint16_t, 1024> local_hlpen_lut = this->hlpen_lut;
        
        // 预计算常量
        const int32_t ymax = zctab_cols - 1;
        const int32_t ydiff = static_cast<int32_t>(dispRange.second) - dispRange.first;
        const int32_t disp_first = dispRange.first;
        const int32_t disp_second = dispRange.second;
        const int shift = Lace::ll_shift;
        const int lut_imax_val = Lace::lut_imax;
        
        // 预计算完整的颜色映射表
        alignas(32) std::array<int32_t, 1024> col_map;
        
        // AVX2向量化生成映射表
        #pragma omp simd
        for (int gray_val = 0; gray_val < col_map.size(); ++gray_val) {
            if (ydiff == 0 || ydiff == lut_imax_val) {
                col_map[gray_val] = gray_val;
            } else {
                float scale = (ydiff > 0) ? (1.0f / static_cast<float>(ydiff) * (std::clamp(gray_val, disp_first, disp_second) - disp_first)) : 
                                        (1.0f / static_cast<float>(-ydiff) * (disp_first - std::clamp(gray_val, disp_second, disp_first)));
                col_map[gray_val] = static_cast<int32_t>(ymax * scale);
            }
        }

        #pragma omp for schedule(static)
        for (int i = 0; i < win_pads.height; ++i)
        {
            // 获取当前行指针
            uint16_t* pGray = grayIn.ptr(grayIn.pwin().y + i, grayIn.pwin().x);
            uint8_t* pZ = zIn.ptr(zIn.pwin().y + i, zIn.pwin().x);
            int32_t* pCol = &(*s32VecColPreComputed)[i * win_pads.width];
            int32_t* pRow = &(*s32VecRowPreComputed)[i * win_pads.width];
            
            int j = 0;
            const int width = win_pads.width;
            
            // === 主循环：每次处理32像素（AVX2 256位 = 32字节） ===
            for (; j <= width - 32; j += 32)
            {
                // X86架构预取优化
                if (j + 64 < width) {
                    _mm_prefetch(reinterpret_cast<const char*>(pGray + j + 64), _MM_HINT_T0);
                    _mm_prefetch(reinterpret_cast<const char*>(pZ + j + 64), _MM_HINT_T0);
                    _mm_prefetch(reinterpret_cast<const char*>(pCol + j + 64), _MM_HINT_T0);
                    _mm_prefetch(reinterpret_cast<const char*>(pRow + j + 64), _MM_HINT_T0);
                }
                
                // === 加载阶段：一次加载32个像素 ===
                __m256i gray_vec1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pGray + j));
                __m256i gray_vec2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pGray + j + 16));
                __m256i z_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pZ + j));
                
                // === 预处理：灰度值右移 ===
                __m256i gray_shifted1 = _mm256_srli_epi16(gray_vec1, shift);
                __m256i gray_shifted2 = _mm256_srli_epi16(gray_vec2, shift);
                
                // === 第一级LUT查表：hlpen_lut映射 ===
                alignas(32) uint16_t gray_shifted_arr1[16];
                alignas(32) uint16_t gray_shifted_arr2[16];
                alignas(32) uint8_t z_arr[32];
                
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(gray_shifted_arr1), gray_shifted1);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(gray_shifted_arr2), gray_shifted2);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(z_arr), z_vec);
                
                // 第一级查表结果
                alignas(32) uint16_t lut_result_arr[32];
                
                #pragma omp simd
                for (int k = 0; k < 32; k++) {
                    uint16_t shifted_val = (k < 16) ? gray_shifted_arr1[k] : gray_shifted_arr2[k - 16];
                    lut_result_arr[k] = local_hlpen_lut[shifted_val];
                }
                
                // === 第二级LUT查表：col_map映射 ===
                alignas(32) int32_t col_results[32];
                alignas(32) int32_t row_results[32];
                
                #pragma omp simd
                for (int k = 0; k < 32; k++) {
                    col_results[k] = col_map[lut_result_arr[k]];
                    row_results[k] = local_zMap[z_arr[k]];
                }
                
                __m256i col1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(col_results));
                __m256i col2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(col_results + 8));
                __m256i col3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(col_results + 16));
                __m256i col4 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(col_results + 24));
                
                __m256i row1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row_results));
                __m256i row2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row_results + 8));
                __m256i row3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row_results + 16));
                __m256i row4 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row_results + 24));
                
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pCol + j), col1);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pCol + j + 8), col2);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pCol + j + 16), col3);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pCol + j + 24), col4);
                
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pRow + j), row1);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pRow + j + 8), row2);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pRow + j + 16), row3);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pRow + j + 24), row4);
            }
            
            // === 处理16像素块 ===
            for (; j <= width - 16; j += 16)
            {
                __m256i gray_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pGray + j));
                __m256i z_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pZ + j));
                
                __m256i gray_shifted = _mm256_srli_epi16(gray_vec, shift);
                
                alignas(32) uint16_t gray_shifted_arr[16];
                alignas(32) uint8_t z_arr[16];
                
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(gray_shifted_arr), gray_shifted);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(z_arr), z_vec);
                
                alignas(32) uint16_t lut_result_arr[16];
                alignas(32) int32_t col_results[16];
                alignas(32) int32_t row_results[16];
                
                #pragma omp simd
                for (int k = 0; k < 16; k++) {
                    lut_result_arr[k] = local_hlpen_lut[gray_shifted_arr[k]];
                    col_results[k] = col_map[lut_result_arr[k]];
                    row_results[k] = local_zMap[z_arr[k]];
                }
                
                __m256i col1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(col_results));
                __m256i col2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(col_results + 8));
                __m256i row1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row_results));
                __m256i row2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row_results + 8));
                
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pCol + j), col1);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pCol + j + 8), col2);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pRow + j), row1);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pRow + j + 8), row2);
            }
            
            // === 处理8像素块 ===
            for (; j <= width - 8; j += 8)
            {
                __m128i gray_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pGray + j));
                __m128i z_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pZ + j));
                
                __m128i gray_shifted = _mm_srli_epi16(gray_vec, shift);
                
                alignas(16) uint16_t gray_shifted_arr[8];
                alignas(8) uint8_t z_arr[8];
                
                _mm_store_si128(reinterpret_cast<__m128i*>(gray_shifted_arr), gray_shifted);
                _mm_storel_epi64(reinterpret_cast<__m128i*>(z_arr), z_vec);
                
                alignas(32) uint16_t lut_result_arr[8];
                alignas(32) int32_t col_results[8];
                alignas(32) int32_t row_results[8];
                
                #pragma omp simd
                for (int k = 0; k < 8; k++) {
                    lut_result_arr[k] = local_hlpen_lut[gray_shifted_arr[k]];
                    col_results[k] = col_map[lut_result_arr[k]];
                    row_results[k] = local_zMap[z_arr[k]];
                }
                
                __m256i col_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(col_results));
                __m256i row_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row_results));
                
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pCol + j), col_vec);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(pRow + j), row_vec);
            }
            
            // === 处理剩余像素（4像素块）===
            for (; j <= width - 4; j += 4)
            {
                __m128i gray_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pGray + j));
                __m128i z_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pZ + j));
                
                __m128i gray_shifted = _mm_srli_epi16(gray_vec, shift);
                
                alignas(16) uint16_t gray_shifted_arr[4];
                alignas(4) uint8_t z_arr[4];
                
                _mm_store_si128(reinterpret_cast<__m128i*>(gray_shifted_arr), gray_shifted);
                _mm_storel_epi64(reinterpret_cast<__m128i*>(z_arr), z_vec);
                
                alignas(32) int32_t col_results[4];
                alignas(32) int32_t row_results[4];
                
                #pragma omp simd
                for (int k = 0; k < 4; k++) {
                    uint16_t lut_val = local_hlpen_lut[gray_shifted_arr[k]];
                    col_results[k] = col_map[lut_val];
                    row_results[k] = local_zMap[z_arr[k]];
                }
                
                __m128i col_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(col_results));
                __m128i row_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row_results));
                
                _mm_store_si128(reinterpret_cast<__m128i*>(pCol + j), col_vec);
                _mm_store_si128(reinterpret_cast<__m128i*>(pRow + j), row_vec);
            }
            
            // === 处理最后1-3个剩余像素 ===
            for (; j < width; ++j) {
                uint16_t gray_val = local_hlpen_lut[pGray[j] >> shift];
                pCol[j] = col_map[gray_val];
                pRow[j] = local_zMap[pZ[j]];
            }
        }
    }

    

#elif defined(RK3588_PLATFORM)
    #pragma omp parallel num_threads(4)
    {
        // 线程局部拷贝LUT，确保缓存对齐
        alignas(64) std::array<int32_t, 44> local_zMap = zMap;
        alignas(64) std::array<uint16_t, 1024> local_hlpen_lut = this->hlpen_lut;
        
        // 预计算常量
        const int32_t ymax = zctab_cols - 1;
        const int32_t ydiff = static_cast<int32_t>(dispRange.second) - dispRange.first;
        const int32_t disp_first = dispRange.first;
        const int32_t disp_second = dispRange.second;
        const int shift = Lace::ll_shift;
        const int lut_imax_val = Lace::lut_imax;
        
        // 预计算完整的颜色映射表
        alignas(64) std::array<int32_t, 1024> col_map;
        
        // 向量化生成映射表
        #pragma omp simd
        for (int gray_val = 0; gray_val < col_map.size(); ++gray_val) {
            if (ydiff == 0 || ydiff == lut_imax_val) {
                col_map[gray_val] = gray_val;
            } else {
                float scale = (ydiff > 0) ? (1.0f / static_cast<float>(ydiff) * (std::clamp(gray_val, disp_first, disp_second) - disp_first)) : 
                                        (1.0f / static_cast<float>(-ydiff) * (disp_first - std::clamp(gray_val, disp_second, disp_first)));
                col_map[gray_val] = static_cast<int32_t>(ymax * scale);
            }
        }

        #pragma omp for schedule(static)
        for (int i = 0; i < win_pads.height; ++i)
        {
            // 获取当前行指针，假设已经对齐
            uint16_t* pGray = grayIn.ptr(grayIn.pwin().y + i, grayIn.pwin().x);
            uint8_t* pZ = zIn.ptr(zIn.pwin().y + i, zIn.pwin().x);
            int32_t* pCol = &(*s32VecColPreComputed)[i * win_pads.width];
            int32_t* pRow = &(*s32VecRowPreComputed)[i * win_pads.width];
            
            int j = 0;
            const int width = win_pads.width;
            
            // === 主循环：每次处理16像素（最大程度利用NEON） ===
            for (; j <= width - 16; j += 16)
            {
                // ARM架构预取优化
                if (j + 64 < width) {
                    __builtin_prefetch(pGray + j + 64, 0, 3);  // 读取，高时间局部性
                    __builtin_prefetch(pZ + j + 64, 0, 3);
                    __builtin_prefetch(pCol + j + 32, 1, 3);   // 写入，高时间局部性
                    __builtin_prefetch(pRow + j + 32, 1, 3);
                }
                
                // === 加载阶段：一次加载16个像素 ===
                uint16x8_t gray_vec1 = vld1q_u16(pGray + j);      // 前8个灰度值
                uint16x8_t gray_vec2 = vld1q_u16(pGray + j + 8);  // 后8个灰度值
                uint8x16_t z_vec = vld1q_u8(pZ + j);              // 16个Z值
                
                // === 预处理：灰度值右移 ===
                uint16x8_t gray_shifted1 = vshrq_n_u16(gray_vec1, shift);
                uint16x8_t gray_shifted2 = vshrq_n_u16(gray_vec2, shift);
                
                // === 第一级LUT查表：hlpen_lut映射 ===
                // 由于查表操作难以向量化，转换为标量处理但保持SIMD友好
                uint16_t gray_shifted_arr1[8] __attribute__((aligned(16)));
                uint16_t gray_shifted_arr2[8] __attribute__((aligned(16)));
                uint8_t z_arr[16] __attribute__((aligned(16)));
                
                vst1q_u16(gray_shifted_arr1, gray_shifted1);
                vst1q_u16(gray_shifted_arr2, gray_shifted2);
                vst1q_u8(z_arr, z_vec);
                
                // 第一级查表结果
                uint16_t lut_result_arr[16] __attribute__((aligned(16)));
                
                #pragma omp simd
                for (int k = 0; k < 16; k++) {
                    uint16_t shifted_val = (k < 8) ? gray_shifted_arr1[k] : gray_shifted_arr2[k - 8];
                    lut_result_arr[k] = local_hlpen_lut[shifted_val];
                }
                
                // === 第二级LUT查表：col_map映射 ===
                int32_t col_results[16] __attribute__((aligned(64)));
                int32_t row_results[16] __attribute__((aligned(64)));
                
                #pragma omp simd
                for (int k = 0; k < 16; k++) {
                    col_results[k] = col_map[lut_result_arr[k]];
                    row_results[k] = local_zMap[z_arr[k]];
                }
                
                // === 向量化存储结果 ===
                // 分4批存储，每批4个int32_t
                int32x4_t col1 = vld1q_s32(col_results);
                int32x4_t col2 = vld1q_s32(col_results + 4);
                int32x4_t col3 = vld1q_s32(col_results + 8);
                int32x4_t col4 = vld1q_s32(col_results + 12);
                
                int32x4_t row1 = vld1q_s32(row_results);
                int32x4_t row2 = vld1q_s32(row_results + 4);
                int32x4_t row3 = vld1q_s32(row_results + 8);
                int32x4_t row4 = vld1q_s32(row_results + 12);
                
                vst1q_s32(pCol + j, col1);
                vst1q_s32(pCol + j + 4, col2);
                vst1q_s32(pCol + j + 8, col3);
                vst1q_s32(pCol + j + 12, col4);
                
                vst1q_s32(pRow + j, row1);
                vst1q_s32(pRow + j + 4, row2);
                vst1q_s32(pRow + j + 8, row3);
                vst1q_s32(pRow + j + 12, row4);
            }
            
            // === 处理8像素块 ===
            for (; j <= width - 8; j += 8)
            {
                uint16x8_t gray_vec = vld1q_u16(pGray + j);
                uint8x8_t z_vec = vld1_u8(pZ + j);
                
                uint16x8_t gray_shifted = vshrq_n_u16(gray_vec, shift);
                
                uint16_t gray_shifted_arr[8] __attribute__((aligned(16)));
                uint8_t z_arr[8] __attribute__((aligned(8)));
                
                vst1q_u16(gray_shifted_arr, gray_shifted);
                vst1_u8(z_arr, z_vec);
                
                uint16_t lut_result_arr[8] __attribute__((aligned(16)));
                int32_t col_results[8] __attribute__((aligned(32)));
                int32_t row_results[8] __attribute__((aligned(32)));
                
                #pragma omp simd
                for (int k = 0; k < 8; k++) {
                    lut_result_arr[k] = local_hlpen_lut[gray_shifted_arr[k]];
                    col_results[k] = col_map[lut_result_arr[k]];
                    row_results[k] = local_zMap[z_arr[k]];
                }
                
                int32x4_t col_low = vld1q_s32(col_results);
                int32x4_t col_high = vld1q_s32(col_results + 4);
                int32x4_t row_low = vld1q_s32(row_results);
                int32x4_t row_high = vld1q_s32(row_results + 4);
                
                vst1q_s32(pCol + j, col_low);
                vst1q_s32(pCol + j + 4, col_high);
                vst1q_s32(pRow + j, row_low);
                vst1q_s32(pRow + j + 4, row_high);
            }
            
            // === 处理剩余像素（4像素块）===
            for (; j <= width - 4; j += 4)
            {
                // 加载4个像素
                uint16x4_t gray_vec = vld1_u16(pGray + j);
                uint8x8_t z_vec = vld1_u8(pZ + j);  // 加载8个，但只用前4个
                
                uint16x4_t gray_shifted = vshr_n_u16(gray_vec, shift);
                
                uint16_t gray_shifted_arr[4] __attribute__((aligned(8)));
                uint8_t z_arr[4] __attribute__((aligned(4)));
                
                vst1_u16(gray_shifted_arr, gray_shifted);
                vst1_u8(z_arr, z_vec);  // 只存储前4个
                
                int32_t col_results[4] __attribute__((aligned(16)));
                int32_t row_results[4] __attribute__((aligned(16)));
                
                #pragma omp simd
                for (int k = 0; k < 4; k++) {
                    uint16_t lut_val = local_hlpen_lut[gray_shifted_arr[k]];
                    col_results[k] = col_map[lut_val];
                    row_results[k] = local_zMap[z_arr[k]];
                }
                
                int32x4_t col_vec = vld1q_s32(col_results);
                int32x4_t row_vec = vld1q_s32(row_results);
                
                vst1q_s32(pCol + j, col_vec);
                vst1q_s32(pRow + j, row_vec);
            }
            
            // === 处理最后1-3个剩余像素 ===
            for (; j < width; ++j) {
                uint16_t gray_val = local_hlpen_lut[pGray[j] >> shift];
                pCol[j] = col_map[gray_val];
                pRow[j] = local_zMap[pZ[j]];
            }
        }
    }
#else
    #pragma omp parallel for num_threads(4) schedule(static)
    for (int i = 0; i < win_pads.height; ++i) 
    {
        uint16_t* pGray = grayIn.ptr(grayIn.pwin().y + i, grayIn.pwin().x);
        uint8_t* pZ = zIn.ptr(zIn.pwin().y + i, zIn.pwin().x);
        int32_t* pCol = &(*s32VecColPreComputed)[i * win_pads.width];
        int32_t* pRow = &(*s32VecRowPreComputed)[i * win_pads.width];

        const int shift = Lace::ll_shift;
        const int32_t disp_first = dispRange.first;
        const int32_t disp_second = dispRange.second;
        const int32_t ydiff = static_cast<int32_t>(dispRange.second) - dispRange.first;
        const int32_t ymax = zctab_cols - 1;
        const int lut_imax_val = Lace::lut_imax;

        // 构建 col_map 表
        alignas(64) std::array<int32_t, 1024> col_map;
        for (int gray_val = 0; gray_val < 1024; ++gray_val) {
            int32_t clamped = (gray_val < disp_first) ? disp_first :
                             ((gray_val > disp_second) ? disp_second : gray_val);

            if (ydiff == 0 || ydiff == lut_imax_val) {
                col_map[gray_val] = gray_val;
            } else {
                float scale = (ydiff > 0) ? (1.0f / static_cast<float>(ydiff)) :
                                            (1.0f / static_cast<float>(-ydiff));
                col_map[gray_val] = static_cast<int32_t>(ymax * (clamped - disp_first) * scale);
            }
        }

        const int width = win_pads.width;
        for (int j = 0; j < width; ++j) {
            uint16_t gray_val = pGray[j] >> shift;
            uint16_t lut_val = this->hlpen_lut[gray_val];
            pCol[j] = col_map[lut_val];
            pRow[j] = zMap[pZ[j]];
        }
    }
#endif
    /// 颜色表映射
    const uint8_t zidx_max = znum_max - 1;
    const uint8_t y_val_max = *yTab.ptr(0, 1023);
    #pragma omp parallel num_threads(8)
    {
        const int32_t local_y_val_max = y_val_max;
        const auto& local_ycbcrTbeInter = ycbcrTbeInter;
        #pragma omp for schedule(static)
        for (int32_t i = 0; i < win_pads.height; ++i)
        {
            uint16_t* pGray = grayIn.ptr(grayIn.pwin().y + i, grayIn.pwin().x);
            uint8_t* pZ = zIn.ptr(zIn.pwin().y + i, zIn.pwin().x);
            int32_t hoffs = subcf0(i, toffs);
            uint8_t* pY = (i >= toffs && hoffs < yOut.pwin().height) ? yOut.ptr(yOut.pwin().y + hoffs, yOut.pwin().x) : nullptr;
            int8_t* pCb = cbOri.ptr(cbOri.pwin().y + i, cbOri.pwin().x);
            int8_t* pCr = crOri.ptr(crOri.pwin().y + i, crOri.pwin().x);
            
            int32_t* pRow = &(*s32VecRowPreComputed)[i * win_pads.width];
            int32_t* pCol = &(*s32VecColPreComputed)[i * win_pads.width];
            
            int32_t j = 0;
            const int32_t UNROLL_FACTOR = 8; // 8倍循环展开
            
            // 主循环：4倍循环展开 + 数据预取
            for (; j <= win_pads.width - UNROLL_FACTOR; j += UNROLL_FACTOR, 
                pGray += UNROLL_FACTOR, pZ += UNROLL_FACTOR,
                pRow += UNROLL_FACTOR, pCol += UNROLL_FACTOR)
            {
                // 预取下一组数据（提前预取下一缓存行）
                if (j + UNROLL_FACTOR * 2 <= win_pads.width) {
                    __builtin_prefetch(pRow + UNROLL_FACTOR, 0, 3); // 读预取，高局部性
                    __builtin_prefetch(pCol + UNROLL_FACTOR, 0, 3);
                }
                
                // 循环展开处理4个像素
                for (int k = 0; k < UNROLL_FACTOR; ++k) {
                    int32_t row_val = pRow[k];
                    int32_t col_val = pCol[k];
                    
                    if (1023 == col_val) {
                        if (pY != nullptr) pY[k] = local_y_val_max;
                        pCb[k] = 0;
                        pCr[k] = 0;
                    } else {
                        auto pixel = local_ycbcrTbeInter.data.ptr(row_val, col_val);
                        if (pY != nullptr) pY[k] = pixel->y;
                        pCb[k] = pixel->cb;
                        pCr[k] = pixel->cr;
                    }
                }
                
                // 更新指针位置
                if (pY != nullptr) pY += UNROLL_FACTOR;
                pCb += UNROLL_FACTOR;
                pCr += UNROLL_FACTOR;
            }
            
            // 处理剩余像素
            for (; j < win_pads.width; ++j, ++pGray, ++pZ, ++pRow, ++pCol) {
                int32_t row_val = *pRow;
                int32_t col_val = *pCol;
                
                if (1023 == col_val) {
                    if (pY != nullptr) *pY++ = y_val_max;
                    *pCb++ = 0;
                    *pCr++ = 0;
                } else {
                    auto pixel = local_ycbcrTbeInter.data.ptr(row_val, col_val);
                    if (pY != nullptr) *pY++ = pixel->y;
                    *pCb++ = pixel->cb;
                    *pCr++ = pixel->cr;
                }
            }
        }
    }

    grayIn(win_curt), zIn(win_curt), cbOri(win_curt), crOri(win_curt);
    Filter::Gaussian(cbSmo, cbOri, fTmp, csmo_neighb, -1.0);
    Filter::Gaussian(crSmo, crOri, fTmp, csmo_neighb, -1.0);
    CalcChromaSmoothWeight(cWt, cbOri, crOri, crSmo, 0.5);

    #pragma omp parallel num_threads(8)
    {
        #pragma omp for schedule (static)
        for (int32_t i = 0; i < rgbOut.pwin().height; i++)
        {
            uint8_t* pBgr = rgbOut.ptr(rgbOut.pwin().y + i, rgbOut.pwin().x);
            uint8_t* pY = yOut.ptr(yOut.pwin().y + i, yOut.pwin().x);
            int8_t* pCbOri = cbOri.ptr(cbOri.pwin().y + i, cbOri.pwin().x);
            int8_t* pCrOri = crOri.ptr(crOri.pwin().y + i, crOri.pwin().x);
            int8_t* pCbSmo = cbSmo.ptr(cbSmo.pwin().y + i, cbSmo.pwin().x);
            int8_t* pCrSmo = crSmo.ptr(crSmo.pwin().y + i, crSmo.pwin().x);
            uint8_t* pWt = cWt.ptr(cWt.pwin().y + i, cWt.pwin().x);
            for (int32_t j = 0; j < rgbOut.pwin().width; ++j, ++pY, ++pCbOri, ++pCrOri, ++pCbSmo, ++pCrSmo, ++pWt)
            {
                int32_t cb = (static_cast<int>(*pCbSmo) * *pWt + *pCbOri * (255 - *pWt) + 127) / 255;
                int32_t cr = (static_cast<int>(*pCrSmo) * *pWt + *pCrOri * (255 - *pWt) + 127) / 255;
                // int32_t cb = (static_cast<int>(*pCbOri));
                // int32_t cr = (static_cast<int>(*pCrOri));

                *pBgr++ = (std::clamp(1900 * cb + (*pY << 10), 0, 261120) + 512) >> 10; // B, 261120: 255 << 10
                *pBgr++ = (std::clamp(-192 * cb - 479 * cr + (*pY << 10), 0, 261120) + 512) >> 10; // G
                *pBgr++ = (std::clamp(1612 * cr + (*pY << 10), 0, 261120) + 512) >> 10; // R
            }
        }
    }
    
    if ((dump_points & dp_imgio) && (this->dump_counts))
    {
        std::stringstream ss(this->dump_dir);
        ss.seekp(0, std::ios_base::end);
        ss << std::setw(6) << std::setfill('0') << ++this->proc_count;
        size_t offs = 0;
        ss << "_gray" << offs;
        offs += grayIn.isize().area() * grayIn.elemsize();
        ss << "_z" << offs;
        offs += zIn.isize().area() * zIn.elemsize();
        ss << "_rgb" << offs << "-" << rgbOut.width() << "x" << rgbOut.height();
        std::string&& dump_fn = ss.str();
        grayIn.dump(dump_fn, true);
        zIn.dump(dump_fn, false);
        rgbOut.dump(dump_fn, false);
        this->dump_counts--;
    }

    return 0;
}


void PipeChroma::SetDump(uint32_t dps, std::string dir, uint32_t dpc)
{
    _SetDump(this->dump_points, this->dump_dir, dps, dir, this->pipe_chan);
    this->dump_dir.append("-chroma_");
    this->dump_counts = dpc;

    return;
}

} // namespace

