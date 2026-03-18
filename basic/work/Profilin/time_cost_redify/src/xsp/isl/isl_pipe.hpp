/**
 * XRay Image Process Pipe
 */

#ifndef __ISL_PIPE_H__
#define __ISL_PIPE_H__

#include <deque>
#include <vector>
#include "isl_lace.hpp"
#include "isl_nree.hpp"

namespace isl
{

class PipeComm
{
private:
    static int32_t pipe_count;
    std::vector<PipeComm*> objs; // 当多个子Pipe合成一个完整的Pipe时，用于记录并管理子Pipe

protected:
    /// 以下是调试参数
    int32_t pipe_chan;      // Pipe通道号，该值每个PipeComm对象唯一
    size_t proc_count;      // Pipe中各子Pipe处理次数，PipeGray与PipeChroma对象分别维护
    uint32_t dump_points;   // Dump节点，可通过PipeComm对象统一设置，也可各子Pipe独立设置
    std::string dump_dir;   // Dump目录，可通过PipeComm对象统一设置，也可各子Pipe独立设置
    uint32_t dump_counts;   // Dump次数

public:
    typedef enum
    {
        dp_imgio    = (1 << 0), // Pipe中所有接口的输入输出
        dp_hist     = (1 << 1), // Lace中直方图统计输入
        dp_ace      = (1 << 2), // Lace中Lut的计算过程
        dp_edge     = (1 << 3), // Nree中边缘的计算过程
        dp_ctbe     = (1 << 4), // 当前使用的颜色表
    } dump_point_t;

    PipeComm() : pipe_chan(pipe_count), proc_count(0), dump_points(0){}

    PipeComm(std::vector<PipeComm*> _objs) : objs(_objs), pipe_chan(pipe_count), proc_count(0), dump_points(0)
    {
        ++pipe_count;
        for (auto it = objs.begin(); it != objs.end(); )
        {
            if (nullptr == *it)
            {
                it = objs.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    /**
     * @fn      GetModuleVpadsNeeded
     * @brief   获取ISL-PIPE Continued处理模式下垂直方向所需的上下Paddings（邻域）
     * 
     * @param   [IN] slice_height 输入给ISL-PIPE需做增强的条带高度（不含邻域）
     * 
     * @return  std::pair<int32_t,int32_t> first：上Padding，second：下Padding
     */
    static std::pair<int32_t, int32_t> GetModuleVpadsNeeded(int32_t slice_height);

    /**
     * @fn      SetBrightnessParams
     * @brief   设置所有流程中的亮度参数（仅调用PipeGray和PipeChroma中的重载接口，不做实际操作）
     * 
     * @param   [IN] blvl 亮度等级，范围[0, 100]
     * @param   [IN] hltRange 高亮范围，first和second的取值范围均为[0, 65535]，且first不大于second
                              first：高亮起始，该值越小，高亮区（易穿区）动态范围被压缩的越多
                              second：高亮结束，XRay模块输入图像的处理阈值，高于该值则不被统计
     */
    virtual int32_t SetBrightnessParams(int32_t blvl=50, std::pair<uint16_t, uint16_t> hltRange={58000, 62500});

    /**
     * @fn      SetDump
     * @brief   设置所有流程中的Dump信息（仅调用PipeGray和PipeChroma中的重载接口，不做实际操作）
     * 
     * @param   [IN] dps Dump Points，dump_point_t中枚举的或值
     * @param   [IN] dir Dump Directory，必须是已存在目录，否则不会写入
     * @param   [IN] dpc Dump Count, Dump的次数设置 
     */
    virtual void SetDump(uint32_t dps=0, std::string dir="/tmp", uint32_t dpc=0);
};


class PipeGray : public PipeComm
{
public:
    using hist_f64t = std::pair<Lace::lut_f64t, size_t>; // 归一化直方图数据类型，first：直方图，second：统计的像素数
    using tile_hist_t = std::pair<hist_f64t, hist_f64t>; // Tile直方图，first：边缘直方图，second：全局直方图
    using tile_lut_t = std::pair<Lace::lut_f64t, bool>; // Tile查找表，first：查找表，second：是否有效
    using wtx4_t = std::array<uint16_t, 4>; // 实际数据存储同ISL_U16C4格式，4：左上、右上、左下、右下各Tile Lut的权重
    using wtx2_t = std::array<uint16_t, 2>; // 实际数据存储同ISL_U16C2格式：2：上/下或左/右各Tile Lut的权重
    int32_t m_nPipeResizeWidth;

    enum _pmode// 流程的处理模式，Pipe Process Mode
    {
        PM_SLICE_SEQUENCE,      // 实时处理的条带序列，上次处理的图像区域作为下次处理的邻域，上次输出的图像数据原封不动
        PM_ENTIRE_POSITIVE,     // 回拉与转存的整图，从上往下正序处理
        PM_ENTIRE_NEGATIVE      // 回拉与转存的整图，从下往上倒序处理
    };
    using pmode_t = _pmode;

private:
    Lace lace;
    Nree egde;
    Nree texture;

    Lace::lut_f64t lut_base;
    const int32_t lace_neighb = 6; // 经LACE处理后保留的有效邻域
    const int32_t edge_neighb = 3; // 经Edge处理后保留的有效邻域
    const int32_t texture_neighb = 2; // 经Texture处理后保留的有效邻域

    /// 下面这些是ACE参数
    float64_t lum_exp = 0.5; // LumAdjust中的期望亮度，值越大期望亮度越高，范围：[0, 1.0]
    float64_t dark_boost = 0.5; // LumAdjust中的暗区亮度增强，值越大增强越多，范围：[0, 1.0]
    float64_t gamma_max = 0.0; // LumAdjust中的Gamma Ratio最大值，0为无限制
    float64_t contrast_coeff = 0.5; // ContrastEnhance中的对比度等级，值越大对比度越高，范围：[0, 1.0]
    uint16_t stat_lum_th; // 统计亮度阈值，高于该值不被统计（不是不处理），注意：此处不要初始化，在构造中会自动初始化
    int32_t rt_slice_height = 16; // 实时处理条带高度（不得低于16，因为低于16未测试过）
    const int32_t tile_base = 32; // ACE的Tile尺寸基，真实宽高会在其上下浮动
    size_t tile_cols = 0; // ACE中横向分的Tile数
    int32_t tile_width = 0; // ACE的Tile宽
    int32_t hist_smooth_radius = 16; // Tile Histogram的平滑最小半径，与rt_slice_height的初始值需对应
    const int32_t wt_smooth_radius = 16; // Tile Lut的平滑最小半径，Tile Size超过该值时有效，小于16会不自然
    std::deque<tile_hist_t> hist_cache_rt; // 实时处理Tile直方图缓存
    std::deque<tile_hist_t> hist_smooth_rt; // 实时处理合成后的Tile直方图，用于修改参数时同步更新lut_cache
    std::deque<tile_lut_t> lut_cache_rt; // 实时处理Tile Lut缓存
    std::deque<tile_hist_t> hist_cache_pb; // 回拉转存Tile直方图缓存
    std::deque<tile_lut_t> lut_cache_pb; // 回拉转存Tile Lut缓存
    std::vector<wtx4_t> lut_wtx4; // Tile中各点使用的4个Lut的权重，Tile左半边和右半边对应的4个Lut是不同的
    std::vector<wtx2_t> lut_wtx2h; // 横向边缘Tile无邻域时，各点使用的左右2个Tile Lut的权重，Tile左半边和右半边对应的2个Lut是不同的
    float64_t blc_coeff = 0.6; // 低亮补偿中的Gamma Ratio，值越小低亮区提升越明显，范围：[0, 1.0]

    void TileHistStatistic(std::deque<tile_hist_t>& hist_cache, Imat<uint16_t>& imgin, Imat<uint16_t>& imgmask, std::vector<Rect<int32_t>>& win_vseq);

    void TileLutImpel(std::deque<tile_lut_t>& lut_cache, std::deque<tile_hist_t>* hist_smooth, std::deque<tile_hist_t>& hist_cache, size_t smooth_idx, size_t smooth_num);

    void TileLutApply(Imat<uint16_t>& imgout, Imat<uint16_t>& imgin, std::deque<tile_lut_t>& lut_cache, size_t prev_idx, size_t curt_idx, bool binv);

public:
    PipeGray();

    PipeGray(uint32_t xray_width);

    /**
     * @fn      ResetCache
     * @brief   清空PipeGray流程中的缓存 
     * @note    更新条带高度、切换实时过包/回拉处理模式时使用
     */
    void ResetCache(bool bRtProc);

    /**
     * @fn      SetBrightnessParams
     * @brief   设置PipeGray流程中的亮度参数
     * 
     * @param   [IN] blvl 亮度等级，范围[0, 100]，作用于Global Gamma系数
     * @param   [IN] hltRange 高亮范围，first和second的取值范围均为[0, 65535]，且first不大于second
                              first：高亮起始，该值越大，高亮区（易穿区）对比度增强也越多
                              second：高亮结束，XRay模块输入图像的处理阈值，高于该值则不被统计
     */
    int32_t SetBrightnessParams(int32_t blvl=50, std::pair<uint16_t, uint16_t> hltRange={58000, 62500}) override;

    /**
     * @fn      SetLaceParams
     * @brief   设置PipeGray流程中的LACE模块参数
     * 
     * @param   [IN] contrast_coeff 对比度系数，范围[0, 1]，值越大对比度越高，为0时不做对比度增强
     * @param   [IN] dark_boost LumAdjust中的暗区亮度增强，值越大增强越多，范围：[0, 1.0]
     */
    void SetLaceParams(float64_t contrast_coeff=0.25, float64_t dark_boost=0.3);

    /**
     * @fn      SetNreeParams
     * @brief   设置PipeGray流程中的NREE模块参数
     * 
     * @param   [IN] ee_intensity 边缘整体增强强度，取值范围：[0, 16]，0为不做增强
     */
    void SetNreeParams(float64_t ee_intensity=5.0, int32_t weak_th=4000, float64_t weak_enhance=0.4, int32_t strong_th=15000, float64_t strong_suppress=0.3);

    /**
     * @fn      GetHistSmoothRadius
     * @brief   获取实时处理中不同条带高度对应的直方图平滑的半径
     * 
     * @param   [IN] rt_slice_height 实时处理中的条带高度
     * 
     * @return  int32_t 直方图平滑的半径
     */
    static int32_t GetHistSmoothRadius(int32_t rt_slice_height);

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
    int32_t AutoContraceEnhance(Imat<uint16_t>& imgout, Imat<uint16_t>& imgin, std::array<Imat<uint16_t>*, 3>& imgtmp, pmode_t pmode);

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
    int32_t AutoSharpnessEnhance(Imat<uint16_t>& imgout, Imat<uint16_t>& imgin, std::array<Imat<uint16_t>*, 3>& imgtmp);

    void SetDump(uint32_t dps=0, std::string dir="/tmp", uint32_t dpc=0) override;
};


class PipeChroma : public PipeComm
{
public:
    using rgb8c_t = Imat<uint8_t>; // RGB888交叉格式，U8C3：[0:2]-B G R
    using yuv4p_t = YCbCr<uint8_t, int8_t>; // YUV444平面格式，P[0]-Y，P[1]-U，P[2]-V，每个平面的内存结构、宽高、有效区域均相同
    using yuv4i_t = YCbCrInter<uint8_t, int8_t>;

private:
    const int32_t zctab_rows = 128;
    const int32_t zctab_cols = Lace::lut_bins;
    int32_t znum_max = 44; // 颜色表中最多支持的原子序数-色调映射组数
    const int32_t csmo_neighb = 2; // 色域平滑使用的邻域
    const std::unique_ptr<uint8_t[]> zctab_ptr = std::make_unique<uint8_t[]>(yuv4p_t::min_memsize(zctab_cols, zctab_rows));
    const std::unique_ptr<uint8_t[]> zctabinter_ptr = std::make_unique<uint8_t[]>(yuv4p_t::min_memsize(zctab_cols, zctab_rows));    // 交叉格式YCbCr颜色表
    const std::unique_ptr<std::vector<int32_t>> s32VecRowPreComputed = std::make_unique<std::vector<int32_t>>(2500 * 1000);
    const std::unique_ptr<std::vector<int32_t>> s32VecColPreComputed = std::make_unique<std::vector<int32_t>>(2500 * 1000);
    yuv4p_t zctab; // 原子序数Z-颜色Color映射表
    yuv4i_t zctab_inter;

    uint16_t bg_gray_default = 63568; // 背景灰度缺省值，65535 * 0.97（Xray模块中的反溢出系数）
    uint16_t hlt_vi_begin = 58000; // 输入图像的高亮灰度起始值
    float64_t hlpen_ratio = 0.5; // 高低穿系数，范围[0, 1]，小于0.5为低穿系数，大于0.5为高穿系数
    Lace::lut_u16t hlpen_lut; // 高低穿亮度映射曲线
    int32_t gray_offs = 0; // 灰度值偏移，基于值域[0, 1023]

    uint32_t bg_color = 0xFF8080; // 背景色，[16:23]-Y，[8:15]-Cb，[0:7]-Cr，Cb、Cr的取值范围是[0, 255]

    /// 显示高亮范围，该参数与后级输出（包括显示器）相关，first是高亮区间的起始点，second是后级能显示的最高亮度
    std::pair<float64_t, float64_t> hlt_vo{240.0 / 255.0, 255.0 / 255.0};

public:
    PipeChroma();

    /**
     * @fn      SetBrightnessParams
     * @brief   设置PipeChroma流程中的亮度参数
     * 
     * @param   [IN] bgGrayDefault 前级Pipe输出的背景灰度值，范围[50000, 65535]
     * @param   [IN] hltRange 高亮范围，first和second的取值范围均为[0, 65535]，且first不大于second
                              first：高亮起始，该值越大，高亮区（易穿区）动态范围被压缩的越多
                              second：高亮结束，该值在PipeChroma流程中不使用
     */
    int32_t SetBrightnessParams(int32_t bgGrayDefault=63568, std::pair<uint16_t, uint16_t> hltRange={58000, 62500}) override;

    /**
     * @fn      SetBgColor
     * @brief   设置PipeChroma流程中的背景颜色和背景亮度
     * 
     * @param   [IN] bgc 背景颜色，BGR格式，[0, 7]-B，[8, 15]-G，[16, 23]-R，内部转换为UV值使用
     * @param   [IN] bgl 背景亮度，取值范围[0, 255]
     */
    void SetBgColor(uint32_t bgc=0xFFFFFF, uint8_t bgl=255);

    /**
     * @fn      GetBgColor
     * @brief   获取当前的背景色
     * 
     * @param   [IN] bInverse 是否反色
     * 
     * @return  uint32_t 背景RGB值（BGR24格式，B在低8位，R在高8位）
     */
    uint32_t GetBgColor(bool bInverse);

    /**
     * @fn      SetHlpenYout
     * @brief   设置高低穿系数与灰度偏移值，并重新生成hlpen_lut
     * 
     * @param   [IN] hlpenRatio 高低穿系数，范围[0, 1]，小于0.5为低穿系数，大于0.5为高穿系数
     * @param   [IN] yOffs 灰度偏移值，正值为+，负值为- 
     * @param   [IN] voBg 后级输出的背景亮度，范围[0, 255]，hlt_vo会根据该值做自适应调整
     */
    void SetHlpenYout(float64_t hlpenRatio=0.5, int32_t yOffs=0, uint8_t voBgl=255);

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
    int32_t SetZColorTable(uint8_t* rgb, int32_t ylvl=1024, int32_t znum=44);

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
     * @param   [IN] wtArea 处理权重图中值非0区的外接矩形框
	 * @param   [IN] zMap 原子序数映射表
     * @param   [IN] dispRange 输入的有效区间，即需显示的灰度图范围（可变吸收率），当first大于second时为反色处理
     * 
     * @return  int32_t 0：处理正常，其他：异常
     */
    int32_t PaintZColorImage(rgb8c_t& rgbOut, yuv4p_t& yuvOut, yuv4p_t& yuvTmp, Imat<uint16_t>& grayIn, Imat<uint8_t>& zIn, Imat<uint8_t>& wtIn, 
                             std::vector<Rect<int32_t>>& wtArea, std::array<int32_t, 44> &zMap, std::pair<uint16_t, uint16_t> dispRange={0, 65535});

    void SetDump(uint32_t dps=0, std::string dir="/tmp", uint32_t dpc=0) override;
};


} // namespace

#endif

