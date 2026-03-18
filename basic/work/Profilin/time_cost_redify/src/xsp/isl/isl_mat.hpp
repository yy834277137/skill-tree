/**
 * Image SoLution Matrix Class
 */

#ifndef __ISL_MAT_H__
#define __ISL_MAT_H__

#include <type_traits>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <string>
#include <sstream>
#include <iterator>
#include <functional>
#include <optional>
#include "isl_util.hpp"
#include "xmat.hpp"

namespace isl
{

template<typename _T>
struct Point
{
    _T x = 0, y = 0;

    Point() : x(0), y(0) {} // 默认构造
    Point(_T _x, _T _y) : x(_x), y(_y) {} // 全参数构造
    Point(const Point& that) : x(that.x), y(that.y) {} // 拷贝构造

    template<typename _U> // 转换构造
    explicit Point(const Point<_U>& that) : x(static_cast<_T>(that.x)), y(static_cast<_T>(that.y)) {}

    /* 重载=：支持对象间直接赋值 */
    Point& operator =(const Point& that)
    {
        if (this != &that)
        {
            this->x = that.x;
            this->y = that.y;
        }
        return *this;
    }

    // 重载()：读取该Point对象，通过point().x与point().y获取宽高
    Point<_T> operator ()() const
    {
        return Point(x, y);
    }

    // 重载==：判断两个Point对象是否相同
    bool operator ==(const Point& that) const
    {
        return ((that.x == this->x) && (that.y == this->y));
    }

    // 重载!=：判断两个Point对象是否不同
    bool operator !=(const Point& that) const
    {
        return !(*this == that);
    }

    // 重载<<：打印该Point坐标到标准输出，在isl_log.hpp中统一重载
    #if 0
    friend std::ostream &operator<<(std::ostream& os, const Point& _point)
    {
        return os << "(" << _point.x << ", " << _point.y << ")";
    }
    #endif
};

template<typename _T>
struct Size
{
    _T width = 0, height = 0;

    Size() : width(0), height(0) {} // 默认构造
    Size(_T _w, _T _h) : width(_w), height(_h) {} // 全参数构造
    Size(const Size& that) : width(that.width), height(that.height) {} // 拷贝构造

    // 计算面积
    _T area() const
    {
        return width * height;
    }

    /* 重载=：支持对象间直接赋值 */
    Size& operator =(const Size& that)
    {
        if (this != &that)
        {
            this->width = that.width;
            this->height = that.height;
        }
        return *this;
    }

    // 重载()：读取该Size对象，通过Size().width与Size().height获取宽高
    Size<_T> operator ()() const
    {
        return Size(width, height);
    }

    // 重载==：判断两个Size对象是否相同
    bool operator ==(const Size& that) const
    {
        return ((that.width == this->width) && (that.height == this->height));
    }

    // 重载!=：判断两个Size对象是否不同
    bool operator !=(const Size& that) const
    {
        return !(*this == that);
    }

    // 重载<：判断当前宽高是否均小于另一宽高
    bool operator <(const Size& that) const
    {
        return ((this->width < that.width) && (this->height < that.height));
    }

    // 重载<=：判断当前宽高是否均不大于另一宽高
    bool operator <=(const Size& that) const
    {
        return ((this->width <= that.width) && (this->height <= that.height));
    }

    // 重载>：判断当前宽高是否均大于另一宽高
    bool operator >(const Size& that) const
    {
        return ((this->width > that.width) && (this->height > that.height));
    }

    // 重载>=：判断当前宽高是否均不小于另一宽高
    bool operator >=(const Size& that) const
    {
        return ((this->width >= that.width) && (this->height >= that.height));
    }
};

template<typename _T>
struct Rect
{
    _T x = 0, y = 0, width = 0, height = 0;

    Rect() : x(0), y(0), width(0), height(0) {} // 默认构造
    Rect(_T _x, _T _y, _T _w, _T _h) : x(_x), y(_y), width(_w), height(_h) {} // 全参数构造
    Rect(const Rect& that) : x(that.x), y(that.y), width(that.width), height(that.height) {} // 拷贝构造

    // 矩形左上点坐标：POSition of the Top-Left corner
    Point<_T> postl() const
    {
        return Point<_T>(x, y);
    }

    // 矩形右下点坐标：POSition of the Bottom-Right corner
    Point<_T> posbr() const
    {
        return Point<_T>(x+width-1, y+height-1);
    }

    Size<_T> size() const
    {
       return Size<_T>(width, height);
    }

    #if 0
    Rect<_T> operator ()() const
    {
        return Rect<_T>(x, y, width, height);
    }
    #endif

    /* 重载=：支持对象间直接赋值 */
    Rect& operator =(const Rect& that)
    {
        if (this != &that)
        {
            this->x = that.x;
            this->y = that.y;
            this->width = that.width;
            this->height = that.height;
        }
        return *this;
    }

    // 重载==：判断两个Rect对象是否完全一致，包括Position和Size
    bool operator ==(const Rect& that) const
    {
        return ((that.x == this->x) && (that.width == this->width) && 
                (that.y == this->y) && (that.height == this->height));
    }

    // 重载!=：==取非
    bool operator !=(const Rect& that) const
    {
        return !(*this == that);
    }

    // 重载<=：判断当前Rect是否在另一内部（非严格，允许完全重合）
    bool operator <=(const Rect& that) const
    {
        return ((that.x <= this->x) && (this->posbr().x <= that.posbr().x) &&
                (that.y <= this->y) && (this->posbr().y <= that.posbr().y));
    }

    // 重载<：判断当前Rect是否在另一内部（严格，但允许仅有单边不重合）
    bool operator <(const Rect& that) const
    {
        return ((*this <= that) && (*this != that));
    }

    // 两个矩形的交集区域，返回新的矩形对象
    Rect<_T> intersect(const Rect<_T>& that) const
    {
        if (this->width > 0 && this->height > 0 && that.width > 0 && that.height > 0)
        {
            _T tlX = std::max(this->x, that.x);
            _T tlY = std::max(this->y, that.y);
            _T brX = std::min(this->posbr().x, that.posbr().x);
            _T brY = std::min(this->posbr().y, that.posbr().y);
            if (tlX <= brX && tlY <= brY)
            {
                return Rect<_T>(tlX, tlY, brX - tlX + 1, brY - tlY + 1);
            }
        }
        return Rect<_T>();
    }

    // 两个矩形的最小外接矩形，直接作用于this对象
    Rect<_T>& unite(const Rect<_T>& that)
    {
        if (this->width > 0 && this->height > 0)
        {
            if (that.width > 0 && that.height > 0)
            {
                _T brX = std::max(this->posbr().x, that.posbr().x);
                _T brY = std::max(this->posbr().y, that.posbr().y);
                this->x = std::min(this->x, that.x);
                this->y = std::min(this->y, that.y);
                this->width = brX - this->x + 1;
                this->height = brY - this->y + 1;
            }
        }
        else
        {
            if (that.width > 0 && that.height > 0)
            {
                *this = that;
            }
        }
        return *this;
    }

    // this在that中的补矩形集，输入的this需要在that内部
    std::vector<Rect<_T>> complement(const Rect<_T>& that) const
    {
        std::vector<Rect<int32_t>> comp;
        if (*this < that)
        {
            int32_t toph = 0, leftw = 0;
            if (this->y > that.y) // 上
            {
                toph = this->y - that.y;
                comp.push_back(Rect<int32_t>(that.x, that.y, that.width, toph));
            }
            if (this->x > that.x) // 左中
            {
                leftw = this->x - that.x;
                comp.push_back(Rect<int32_t>(that.x, this->y, leftw, this->height));
            }
            if (this->posbr().y < that.posbr().y) // 下
            {
                comp.push_back(Rect<int32_t>(that.x, this->y + this->height, that.width, that.height - toph - this->height));
            }
            if (this->posbr().x < that.posbr().x) // 右中
            {
                comp.push_back(Rect<int32_t>(this->x + this->width, this->y, that.width - leftw - this->width, this->height));
            }
        }
        return comp;
    }

    Rect<_T> extend(const _T exHor, const _T exVer, const Rect<_T>& boundary)
    {
        // 前提条件：this必须在boundary内部
        if ((*this <= boundary) && (exHor > 0 || exVer > 0))
        {
            auto&& br = this->posbr();
            br.x = std::min(br.x + exHor, boundary.posbr().x);
            br.y = std::min(br.y + exVer, boundary.posbr().y);
            this->x = std::max(subcf0(this->x, exHor), boundary.x);
            this->y = std::max(subcf0(this->y, exVer), boundary.y);
            this->width = br.x - this->x + 1;
            this->height = br.y - this->y + 1;
        }

        return *this;
    }

    #if 0
    // 重载>：判断当前Rect是否在另一外部（严格，但允许单边重合）
    bool operator >(const Rect& that) const
    {
        return ((that.x >= this->x) && (this->posbr().x >= that.posbr().x) && (this->width > that.width) &&
                (that.y >= this->y) && (this->posbr().y >= that.posbr().y) && (this->height > that.height));
    }

    // 重载>=：判断当前Rect是否在另一外部（非严格，允许完全重合）
    bool operator >=(const Rect& that) const
    {
        return ((that.x >= this->x) && (this->posbr().x >= that.posbr().x) &&
                (that.y >= this->y) && (this->posbr().y >= that.posbr().y));
    }
    #endif
};

// Various border types, image boundaries are denoted with `|`
typedef enum
{
    BORDER_CONSTANT     = 0, //!< `iiiiii|abcdefgh|iiiiiii`  with some specified `i`
    BORDER_REPLICATE    = 1, //!< `aaaaaa|abcdefgh|hhhhhhh`
    BORDER_REFLECT      = 2, //!< `gfedcb|abcdefgh|gfedcba`
    BORDER_DEFAULT      = BORDER_REFLECT, //!< same as BORDER_REFLECT
}BORDER_TYPE;

/**
 * 填充域大小，暂仅支持整型，不支持浮点型
 * 比如：在垂直方向，总高度为H，[A, B]为有效区，则该值为std::pair{A, H-(B+1)}，
 *      即：上填充域高度为A，下填充域高度为H-(B+1)，中间有效高度为(B+1)-A
 */
using pads_t = std::pair<int32_t, int32_t>;

/**
 * Image MATrix，图像矩阵类
 * @note 支持数据类型模板，但尺寸坐标等暂仅支持整型，不支持浮点型
 */
template<typename _T>
class Imat
{
private:
    int32_t mtype = ISL_U8; // Imat类型，参考isl_hal.hpp中的定义
    int32_t rows = 0, cols = 0; // 整个Imat平面的行数和列数，包含padding区域
    pads_t vpads = {0, 0}; // 垂直方向的上下padding，first: top padding, second: bottom padding
    pads_t hpads = {0, 0}; // 水平方向的左右padding，first: left padding, second: right padding
    Rect<int32_t> _pwin; // 去除vpads和hpads的内层区域尺寸，即实际的图像处理区域，外层padding作为邻域
    _T* data = nullptr; // Imat数据的内存起始地址
    size_t memsize = 0; // Imat数据申请的内存空间总量，单位：字节
    size_t stride = 0; // Imat数据在内存中的行跨距，单位：字节
    BORDER_TYPE bordtype = BORDER_DEFAULT; // 边界处理方式：固定值、复制边界值、镜像等
    _T bordval = _T(); // 边界处理方式为固定值（BORDER_CONSTANT）时，该固定值的取值

    inline void reconstruct_pwin()
    {
        _pwin.x = hpads.first;
        _pwin.y = vpads.first;
        _pwin.width = subcf0(cols, hpads.first+hpads.second);
        _pwin.height = subcf0(rows, vpads.first+vpads.second);
    }

public:

    /// 默认构造，注：生成的Imat对象是异常的，需再次初始化
    Imat() : rows(0), cols(0), vpads({0, 0}), hpads({0, 0}), data(nullptr), memsize(0), stride(0), bordtype(BORDER_DEFAULT), bordval(0)
    {
        constexpr size_t depth = sizeof(typename std::decay<_T>::type);
        constexpr int32_t basicType = std::is_unsigned_v<_T> ? ISL_TPU : (std::is_signed_v<_T> ? ISL_TPS : ISL_TPF); // 默认为Float类型
        mtype = ISL_MAKETYPE(depth, basicType, 0);
    }

    /// 全参数构造
    Imat(int32_t _rows, int32_t _cols, int32_t _channels, _T* _data, size_t _memsize, pads_t _vpads={0, 0}, pads_t _hpads={0, 0}, size_t _stride=0)
    {
        rows = _rows;
        cols = _cols;
        vpads = _vpads;
        hpads = _hpads;
        data = _data;
        memsize = _memsize;
        reconstruct_pwin();

        constexpr size_t depth = sizeof(typename std::decay<_T>::type);
        constexpr int32_t basicType = std::is_unsigned_v<_T> ? ISL_TPU : (std::is_signed_v<_T> ? ISL_TPS : ISL_TPF); // 默认为Float类型
        mtype = ISL_MAKETYPE(depth, basicType, _channels);
        const size_t rbyte = cols * ISL_ELEM_SIZE(mtype); // 单行有效字节数
        if (memsize >= rows * rbyte)
        {
            stride = (_stride >= rbyte && _stride * rows <= memsize) ? _stride : rbyte;
        }
        else // 异常
        {
            mtype = ISL_MAKETYPE(depth, basicType, 0); // 置异常参数
            stride = 0;
        }
    }

    /// 拷贝构造，但stride与宽存储大小相同，即内存连续
    template <typename _Y> Imat(Imat<_Y>& im) :
        Imat(im.height(), im.width(), im.channels(), (_T *)im.ptr(), im.get_memsize(), im.get_vpads(), im.get_hpads()) {}

    /// 相同图像结构不同内存空间的构造
    template <typename _Y> Imat(Imat<_Y>& im, _T* _data, size_t _memsize, size_t _stride=0) :
        Imat(im.height(), im.width(), im.channels(), _data, _memsize, im.get_vpads(), im.get_hpads(), _stride) {}

    /// 相同内存空间不同图像结构的构造
    Imat(Imat& im, const Rect<int32_t>& roi)
    {
        if (im.isvalid() && roi <= Rect<int32_t>(0, 0, im.cols, im.rows))
        {
            *this = im;
            (*this)(roi);
        }
    }
    Imat(Imat& im, pads_t _vpads, pads_t _hpads={0, 0})
    {
        if (im.isvalid() && _vpads.first + _vpads.second <= im.rows && _hpads.first + _hpads.second <= im.cols)
        {
            (*this)(im.rows, im.cols, im.channels(), im.data, im.memsize, _vpads, _hpads, im.stride);
        }
    }

    /// 从XRay模块的XMat对象构造，上下邻域从输入的xm中获取，上下邻域相等，无左右邻域
    Imat(const XMat& xm)
    {
        // 数据类型占用的bit位相同即允许转换构造
        if (((XSP_MAT_DEPTH(xm.type) == XSP_8U || XSP_MAT_DEPTH(xm.type) == XSP_8S) && (is_u8_v<_T> || is_s8_v<_T>)) ||
            ((XSP_MAT_DEPTH(xm.type) == XSP_16U || XSP_MAT_DEPTH(xm.type) == XSP_16S) && (is_u16_v<_T> || is_s16_v<_T>)) ||
            ((XSP_MAT_DEPTH(xm.type) == XSP_32U || XSP_MAT_DEPTH(xm.type) == XSP_32S) && (is_u32_v<_T> || is_s32_v<_T>)) ||
            (XSP_MAT_DEPTH(xm.type) == XSP_32F && is_f32_v<_T>) ||
            (XSP_MAT_DEPTH(xm.type) == XSP_64F && is_f64_v<_T>))
        {
            constexpr size_t depth = sizeof(typename std::decay<_T>::type);
            constexpr int32_t basicType = std::is_unsigned_v<_T> ? ISL_TPU : (std::is_signed_v<_T> ? ISL_TPS : ISL_TPF); // 默认为Float类型
            this->mtype = ISL_MAKETYPE(depth, basicType, XSP_MAT_CN(xm.type));
            this->rows = xm.hei;
            this->cols = xm.wid;
            this->vpads = std::make_pair(xm.tpad, xm.bpad);
            this->hpads = std::make_pair(0, 0);
            this->data = (_T*)(xm.data.ptr);
            this->memsize = xm.size;
            this->stride = (xm.step >= xm.wid * static_cast<int32_t>(XSP_ELEM_SIZE(xm.type))) ? xm.step : xm.wid * XSP_ELEM_SIZE(xm.type);
            reconstruct_pwin();
        }
    }

    /// Imat对象是否合法，包括行列属性、处理区域、内存属性等
    bool isvalid() const
    {
        if (ISL_MAT_CN(mtype) > 0 && rows > 0 && cols > 0 && _pwin <= Rect<int32_t>(0, 0, cols, rows) &&
            data != nullptr && static_cast<int32_t>(stride) >= ISL_ELEM_SIZE(mtype) * cols && memsize >= stride * rows)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /// 获取Imat的数据类型：比如ISL_U8C1、ISL_U16C1等
    int32_t itype() const
    {
        return ISL_MAT_ITP(mtype);
    }

    /// 获取Imat中数据的基础类型：比如Unsigned、Signed、Float等
    int32_t btype() const
    {
        return ISL_MAT_BTP(mtype);
    }

    /// 获取Imat中数据的完整类型：比如ISL_U8、ISL_U16、ISL_F64等
    int32_t dtype() const
    {
        return ISL_MAT_DTP(mtype);
    }

    /// 获取Imat中数据占用的字节数：比如ISL_U8占用1个字节，ISL_F64占用8个字节
    int32_t depth() const
    {
        return ISL_MAT_DEPTH(mtype);
    }

    /// 获取单个元素（像素）的通道数，比如RGB888交叉格式单像素有3个通道（ISL_U8C3）
    int32_t channels() const
    {
        return ISL_MAT_CN(mtype);
    }

    /// 获取单个元素（像素）占用的总字节数
    size_t elemsize() const
    {
        return ISL_ELEM_SIZE(mtype);
    }

    /// 获取Imat对象使用的内存空间总量
    size_t get_memsize() const
    {
        return memsize;
    }

    /// 获取Imat对象使用的内存单行跨距
    size_t get_stride() const
    {
        return stride;
    }

    /// 获取Imat对象垂直方向（上下）Padding
    const pads_t& get_vpads() const
    {
        return vpads;
    }

    /// 获取Imat对象水平方向（左右）Padding
    const pads_t& get_hpads() const
    {
        return hpads;
    }

    /// 获取Imat对象的高度
    const int32_t& height() const
    {
        return this->rows;
    }

    /// 获取Imat对象的宽度
    const int32_t& width() const
    {
        return this->cols;
    }

    /// 获取整个Imat外层区域尺寸（Imat Size），即cols × rows或width × height
    Size<int32_t> isize() const
    {
        return Size<int32_t>(this->cols, this->rows);
    }

    /// 获取处理窗口（Processing Window）的矩形区域，即去除vpads和hpads的内层区域
    const Rect<int32_t>& pwin() const
    {
         return this->_pwin;
    }

    /// 重载=：Imat可直接赋值，基础数据类型需要相同
    Imat& operator=(const Imat& im)
    {
        if (this != &im && im.isvalid())
        {
            this->mtype = im.mtype;
            this->rows = im.rows;
            this->cols = im.cols;
            this->vpads = im.vpads;
            this->hpads = im.hpads;
            this->_pwin = im._pwin;
            this->data = im.data;
            this->memsize = im.memsize;
            this->stride = im.stride;
            this->bordtype = im.bordtype;
            this->bordval = im.bordval;
        }

        return *this;
    }

    /// 重载()：修改实际的图像处理区域（即垂直与水平Padding）
    Imat& operator()(const Rect<int32_t>& roi)
    {
        this->vpads = std::make_pair(std::min(roi.y, this->rows), std::max(this->rows - (roi.y + roi.height), 0));
        this->hpads = std::make_pair(std::min(roi.x, this->cols), std::max(this->cols - (roi.x + roi.width), 0));
        reconstruct_pwin();

        return *this;
    }

    /**
     * @fn      make_border
     * @brief   设置边界的处理方式，默认为镜像方式
     * 
     * @param   [IN] _type 边界的处理方式
     * @param   [IN] _value 当处理方式为BORDER_CONSTANT时，采用的固定值
     */
    void make_border(BORDER_TYPE _type, _T _value=0) const
    {
        this->bordtype = _type;
        this->bordval = _value;
        return;
    }

    /**
     * @fn      move_vpads
     * @brief   移动垂直方向（上下）Padding，正值为增加Padding（缩小_pwin），负值为减少Padding（扩大_pwin）
     * 
     * @param   [IN] _vdiff 相对当前Padding的变化，正值为增加Padding（缩小_pwin），负值为减少Padding（扩大_pwin） 
     *  
     * @return  实际Padding的变化 
     */
    pads_t move_vpads(const pads_t _vdiff={0, 0})
    {
        pads_t vpre{this->vpads.first, this->vpads.second};
        this->vpads.first = std::clamp(this->vpads.first + _vdiff.first, 0, this->rows);
        this->vpads.second = std::clamp(this->vpads.second + _vdiff.second, 0, this->rows - this->vpads.first);
        reconstruct_pwin();
        return pads_t(this->vpads.first - vpre.first, this->vpads.second - vpre.second);
    }

    /**
     * @fn      move_hpads
     * @brief   移动水平方向（左右）Padding，正值为增加Padding（缩小_pwin），负值为减少Padding（扩大_pwin）
     * 
     * @param   [IN] _hdiff 相对当前Padding的变化，正值为增加Padding（缩小_pwin），负值为减少Padding（扩大_pwin） 
     *  
     * @return  实际Padding的变化 
     */
    pads_t move_hpads(const pads_t _hdiff={0, 0})
    {
        pads_t hpre{this->hpads.first, this->hpads.second};
        this->hpads.first = std::clamp(this->hpads.first + _hdiff.first, 0, this->cols);
        this->hpads.second = std::clamp(this->hpads.second + _hdiff.second, 0, this->cols - this->hpads.first);
        reconstruct_pwin();
        return pads_t(this->hpads.first - hpre.first, this->hpads.second - hpre.second);
    }

    /**
     * @fn      reshape
     * @brief   基于当前Imat对象，构造新的行列、通道数、vpads等参数 
     * @note    不可更改数据类型 
     * 
     * @param   [IN] _shape Imat对象的行数和列数，注意不能超过内存大小
     * @param   [IN] _channels 通道数
     * @param   [IN] _vpads 垂直方向（上下）Padding，默认为无Padding
     * @param   [IN] _hpads 水平方向（左右）Padding，默认为无Padding
     */
    Imat& reshape(const Size<int32_t>& _shape, const int32_t _channels, const pads_t _vpads={0, 0}, const pads_t _hpads={0, 0})
    {
        if (_channels > 0 && _vpads.first + _vpads.second < _shape.height && _hpads.first + _hpads.second < _shape.width &&
            _shape.width * _shape.height * ISL_MAT_DEPTH(this->mtype) * _channels <= static_cast<int32_t>(this->memsize))
        {
            this->mtype = ISL_MAKETYPE(ISL_MAT_DEPTH(this->mtype), ISL_MAT_BTP(this->mtype), _channels);
            this->rows = _shape.height;
            this->cols = _shape.width;
            this->vpads = _vpads;
            this->hpads = _hpads;
            this->stride = _shape.width * ISL_ELEM_SIZE(this->mtype); // 默认stride与宽存储大小相同，内存连续
            reconstruct_pwin();
        }

        return *this;
    }

    /// 行指针类，指向行数据的起始，支持访问邻域像素：
    /// 比如Rptr[-1]，当边界处理方式为镜像时，则读取正常行索引[1]处的数据
    class Rptr
    {
    private:
        const Imat<_T>* that;
        const _T* prow;

    public:
        Rptr() : that(nullptr), prow(nullptr) {}
        Rptr(const Imat<_T>* pim, _T* ptr=nullptr) : that(pim), prow(ptr) {}

        const _T& operator[](int32_t i) const
        {
            if (i >= 0 && i < that->cols)
            {
                return prow[i];
            }
            else if (i < 0)
            {
                if (BORDER_CONSTANT == that->bordtype)
                {
                    return that->bordval;
                }
                else if (BORDER_REPLICATE == that->bordtype)
                {
                    return prow[0];
                }
                else
                {
                    return prow[-i];
                }
            }
            else // if (i >= that->cols)
            {
                if (BORDER_CONSTANT == that->bordtype)
                {
                    return that->bordval;
                }
                else if (BORDER_REPLICATE == that->bordtype)
                {
                    return prow[that->cols-1];
                }
                else
                {
                    return prow[(that->cols-1) * 2 - i];
                }
            }
        }
    };

    /// 获取行对象，支持访问邻域行： 
    /// 比如rptr(-1)，当边界处理方式为镜像时，则读取第[1]行数据 
    Rptr rptr(int32_t row=0) const
    {
        if (row >= 0 && row < this->rows)
        {
            return Rptr(this, (_T*)((uint8_t*)(this->data) + row * this->stride));
        }
        else if (row < 0)
        {
            if (BORDER_REFLECT == this->bordtype)
            {
                return Rptr(this, (_T*)((uint8_t*)(this->data) - row * this->stride));
            }
            else
            {
                return Rptr(this, this->data); // 指向第一行
            }
        }
        else // if (row >= this->rows)
        {
            if (BORDER_REFLECT == this->bordtype)
            {
                return Rptr(this, (_T*)((uint8_t*)(this->data) + ((this->rows-1) * 2 - row) * this->stride));
            }
            else
            {
                return Rptr(this, (_T*)((uint8_t*)(this->data) + (this->rows-1) * this->stride)); // 指向最后一行
            }
        }
    }

    /// 获取第row行第col列元素的地址
    _T* ptr(int32_t row=0, int32_t col=0) const
    {
        return (_T*)((uint8_t*)(this->data) + row * this->stride) + col * ISL_MAT_CN(this->mtype);
    }

    /**
     * @fn      cropto
     * @brief   基于当前Imat对象，以矩形区域裁剪生成新的Imat对象（matOut） 
     * @note    新的Imat对象与当前Imat对象共享同一块内存，垂直与水平Padding均重置为0，浅拷贝
     * 
     * @param   [OUT] matOut 新的Imat对象，其数据类型、内存属性会修改为与当前Imat对象一致
     * @param   [IN] roi 矩形裁剪区域
     * 
     * @return  int32_t 0：生成新Imat对象成功，其他：异常
     */
    int32_t cropto(Imat<_T>& matOut, const Rect<int32_t>& roi)
    {
        if (roi <= Rect<int32_t>(0, 0, this->cols, this->rows))
        {
            matOut.mtype = this->mtype;
            matOut.cols = roi.width;
            matOut.rows = roi.height;
            matOut.vpads = {0, 0};
            matOut.hpads = {0, 0};
            matOut.data = ptr(roi.y, roi.x);
            matOut.memsize = roi.height * this->stride;
            matOut.stride = this->stride;
            matOut.reconstruct_pwin();
            return 0;
        }
        else
        {
            return -1;
        }
    }

    /**
     * @fn      copyto
     * @brief   将当前Imat对象中指定区域（rectSrc）数据，拷贝到目的对象（matDst）中另一区域（rectDst）
     * @note    当前Imat对象与目的对象的类型相同，支持两对象内存有重叠，深拷贝 
     * @warning 拷贝的数据是指定区域，并非是图像实际处理区域（pwin）
     * 
     * @param   [OUT] matDst 目的对象，类型需与当前Imat对象一致
     * @param   [IN] rectSrc 当前Imat对象中指定区域，尺寸需与rectDst一致
     * @param   [IN] rectDst 目的对象区域，尺寸需与rectSrc一致
     * 
     * @return  int32_t 0：拷贝成功，其他：异常
     */
    int32_t copyto(Imat<_T>& matDst, const Rect<int32_t>& rectSrc, const Rect<int32_t>& rectDst)
    {
        if (matDst.isvalid() && rectSrc.width > 0 && rectSrc.height > 0 && rectSrc <= Rect<int32_t>(0, 0, this->cols, this->rows) && 
            rectDst <= Rect<int32_t>(0, 0, matDst.width(), matDst.height()) && rectSrc.size() == rectDst.size())
        {
            _T* dst_start = matDst.ptr(rectDst.y, rectDst.x);
            _T* dst_end = matDst.ptr(rectDst.posbr().y, rectDst.posbr().x);
            _T* src_start = this->ptr(rectSrc.y, rectSrc.x);
            _T* src_end = this->ptr(rectSrc.posbr().y, rectSrc.posbr().x);
            if (dst_start != src_start) // 当为同一内存时，直接返回
            {
                if (rectSrc.width == this->cols && this->cols * this->elemsize() == this->stride && // 连续内存 -> 连续内存
                    rectDst.width == matDst.width() && matDst.width() * matDst.elemsize() == matDst.get_stride())
                {
                    if ((src_start < dst_start && dst_start <= src_end) || (dst_start < src_start && src_start <= dst_end)) // 重叠内存
                    {
                        std::memmove(dst_start, src_start, this->stride * rectSrc.height);
                    }
                    else // 非重叠内存
                    {
                        std::memcpy(dst_start, src_start, this->stride * rectSrc.height);
                    }
                }
                else // src或dst至少有一块是非连续内存
                {
                    size_t line_size = rectSrc.width * this->elemsize();
                    if (subabs(dst_start, src_start) < line_size) // 重叠内存
                    {
                        for (int32_t i = 0, rowSrc = rectSrc.y, rowDst = rectDst.y; i < rectSrc.height; ++i, ++rowSrc, ++rowDst)
                        {
                            std::memmove(matDst.ptr(rowDst, rectDst.x), this->ptr(rowSrc, rectSrc.x), line_size);
                        }
                    }
                    else // 非重叠内存
                    {
                        for (int32_t i = 0, rowSrc = rectSrc.y, rowDst = rectDst.y; i < rectSrc.height; ++i, ++rowSrc, ++rowDst)
                        {
                            std::memcpy(matDst.ptr(rowDst, rectDst.x), this->ptr(rowSrc, rectSrc.x), line_size);
                        }
                    }
                }
            }
            return 0;
        }
        else
        {
            return -1;
        }
    }

    /**
     * @fn      map
     * @brief   对当前Imat对象数据做逐元素处理 
     * 
     * @param   [IN] fmap 处理函数，注：处理函数不能依赖域邻元素
     * @param   [IN] rect 需处理的区域，当为nullptr时则处理当前Imat对象的实际处理区域（pwin）
     * 
     * @return  int32_t 0：处理正常，其他：异常
     */
    int32_t map(std::function<void(_T&)> fmap, const Rect<int32_t>* rect=nullptr)
    {
        if (this->isvalid())
        {
            if (nullptr == rect)
            {
                rect = &this->_pwin;
            }
            else if (!(*rect <= Rect<int32_t>(0, 0, this->cols, this->rows)))
            {
                return -1;
            }
            for (int32_t i = 0, row = rect->y; i < rect->height; ++i, ++row)
            {
                _T* src = this->ptr(row, rect->x);
                for (int32_t j = 0; j < rect->width; ++j, ++src)
                {
                    fmap(*src);
                }
            }
            return 0;
        }
        else
        {
            return -1;
        }
    }

    /**
     * @fn      mapto
     * @brief   将当前Imat对象中指定区域（rectSrc）数据，做逐元素处理并拷贝到目的对象（matDst）中另一区域（rectDst） 
     * @note    两对象内存不允许有重叠 
     *  
     * @param   [OUT] matDst 目的对象
     * @param   [IN] fmap 处理函数
     * @param   [IN] rectSrc 当前Imat对象中指定区域，尺寸需与rectDst一致，默认为pwin()
     * @param   [IN] rectDst 目的对象区域，尺寸需与rectSrc一致，默认为pwin()
     * 
     * @return  int32_t 0：处理正常，其他：异常
     */
    template <typename _P>
    int32_t mapto(Imat<_P>& matDst, std::function<_P(const _T)> fmap, const Rect<int32_t>* rectSrc=nullptr, const Rect<int32_t>* rectDst=nullptr)
    {
        if (matDst.isvalid() && this->isvalid())
        {
            if (nullptr == rectDst)
            {
                rectDst = &matDst.pwin();
            }
            else if (!(*rectDst <= Rect<int32_t>(0, 0, matDst.width(), matDst.height())))
            {
                return -1;
            }
            if (nullptr == rectSrc)
            {
                rectSrc = &this->_pwin;
            }
            else if (!(*rectSrc <= Rect<int32_t>(0, 0, this->cols, this->rows)))
            {
                return -1;
            }
            if (rectSrc->size() != rectDst->size())
            {
                return -1;
            }

            for (int32_t i = 0, rowSrc = rectSrc->y, rowDst = rectDst->y; i < rectSrc->height; ++i, ++rowSrc, ++rowDst)
            {
                _P* dst = matDst.ptr(rowDst, rectDst->x);
                _T* src = this->ptr(rowSrc, rectSrc->x);
                for (int32_t j = 0; j < rectSrc->width; ++j, ++src, ++dst)
                {
                    *dst = fmap(*src);
                }
            }
            return 0;
        }
        else
        {
            return -1;
        }
    }

    /**
     * @fn      fill
     * @brief   对当前Imat对象中指定区域（rectSrc）赋值 
     * @note    该方法更适用于单通道的情况，多通道时每个通道的值都为val 
     * 
     * @param   [IN] val 需要赋的值
     * @param   [IN] rect 是否指定区域，可选，默认为Imat对象全区域
     * 
     * @return  int32_t 0：处理正常，其他：异常
     */
    int32_t fill(_T val, std::optional<Rect<int32_t>> rect=std::nullopt)
    {
        if (this->isvalid())
        {
            Rect<int32_t>&& frame = Rect<int32_t>(0, 0, cols, rows);
            Rect<int32_t>& win = (rect.has_value() && rect.value() <= frame) ? rect.value() : frame; // rect存在且在平面区域内，则取rect，否则取整平面

            if (win.width == cols && cols * elemsize() == stride) // 连续内存，一层for循环
            {
                int32_t ws = win.size().area() * this->channels();
                _T* wp = this->ptr(win.y, win.x);
                if (1 == this->depth())
                {
                    std::memset(wp, val, ws);
                }
                else
                {
                    for (int32_t i = 0; i < ws; ++i)
                    {
                        *wp++ = val;
                    }
                }
            }
            else // 非连续内存，两层for循环，逐行赋值
            {
                int32_t ws = win.width * this->channels();
                for (int32_t i = 0, r = win.y; i < win.height; ++i, ++r)
                {
                    _T* wp = this->ptr(r, win.x);
                    if (1 == this->depth())
                    {
                        std::memset(wp, val, ws);
                    }
                    else
                    {
                        for (int32_t j = 0; j < ws; ++j)
                        {
                            *wp++ = val;
                        }
                    }
                }
            }

            return 0;
        }
        else
        {
            return -1;
        }
    }


    /// 该方法更适用于多通道的情况，单通道也能使用
    template <size_t _N>
    int32_t fill(const std::array<_T, _N>& val, std::optional<Rect<int32_t>> rect=std::nullopt)
    {
        if (this->isvalid() && this->channels() == _N)
        {
            Rect<int32_t>&& frame = Rect<int32_t>(0, 0, cols, rows);
            Rect<int32_t>& win = (rect.has_value() && rect.value() <= frame) ? rect.value() : frame; // rect存在且在平面区域内，则取rect，否则取整平面

            if (win.width == cols && cols * elemsize() == stride) // 连续内存，两层for循环
            {
                int32_t ws = win.size().area();
                _T* wp = this->ptr(win.y, win.x);
                for (int32_t i = 0; i < ws; ++i)
                {
                    for (auto& v : val)
                    {
                        *wp++ = v;
                    }
                }
            }
            else // 非连续内存，三层for循环，逐行赋值
            {
                for (int32_t i = 0, r = win.y; i < win.height; ++i, ++r)
                {
                    _T* wp = this->ptr(r, win.x);
                    for (int32_t j = 0; j < win.width; ++j)
                    {
                        for (auto& v : val)
                        {
                            *wp++ = v;
                        }
                    }
                }
            }

            return 0;
        }
        else
        {
            return -1;
        }
    }

    /**
     * @fn      dump
     * @brief   保存Imat对象数据到文件
     * 
     * @param   [IN/OUT] filename 需保存到的文件名
     * @param   [IN] renew 是否重新创建文件，true-创建，false-追加
     * @param   [IN] rect 是否指定区域，可选，默认为Imat对象全区域
     */
    void dump(std::string& filename, bool renew, std::optional<Rect<int32_t>> rect=std::nullopt)
    {
        if (this->isvalid())
        {
            Rect<int32_t>&& frame = Rect<int32_t>(0, 0, cols, rows);
            Rect<int32_t>& win = (rect.has_value() && rect.value() <= frame) ? rect.value() : frame; // rect存在且在平面区域内，则取rect，否则取整平面

            if (renew)
            {
                std::stringstream ss;
                ss << filename << ((filename.back() == '_') ? "t" : "_t") << get_tick_count() << "_";
                ss << ((btype() == ISL_TPU) ? "u" : ((btype() == ISL_TPS) ? "s" : "f")) << depth() * 8 << "_";
                ss << win.width << "x" << win.height << ".raw";
                filename = ss.str();
            }

            uint8_t* start = (uint8_t*)data + win.y * stride + win.x * elemsize();
            if (win.width == cols && cols * elemsize() == stride) // 连续内存
            {
                write_file(filename, start, win.height * stride, renew);
            }
            else // 非连续内存，逐行写入
            {
                size_t len = win.width * elemsize();
                pofs_t fs;
                for (int32_t i = 0; i < win.height; ++i)
                {
                    write_file(filename, start, len, (i == 0) ? renew : false, (i == win.height-1), &fs);
                    start += stride;
                }
            }
        }

        return;
    }
};

template<typename _Y, typename _C>
struct YCbCr
{
    Imat<_Y> y;
    Imat<_C> cb;
    Imat<_C> cr;

    YCbCr() {} // 默认构造

    YCbCr(Imat<_Y>& _y, Imat<_C>& _cb, Imat<_C>& _cr) : y(_cr), cb(_cb), cr(_cr) {} // 全参数构造

    YCbCr(int32_t _width, int32_t _height, void* _data, pads_t _vpads={0, 0}, pads_t _hpads={0, 0}, size_t _step=0) :
        y(_height, _width, 1, (_Y*)_data, std::max((int32_t)_step, _width) * _height * sizeof(_Y), _vpads, _hpads, _step * sizeof(_Y)),
        cb(_height, _width, 1, (_C*)((uint8_t*)y.ptr() + y.get_memsize()), std::max((int32_t)_step, _width) * _height * sizeof(_C), _vpads, _hpads, _step * sizeof(_C)),
        cr(_height, _width, 1, (_C*)((uint8_t*)cb.ptr() + cb.get_memsize()), std::max((int32_t)_step, _width) * _height * sizeof(_C), _vpads, _hpads, _step * sizeof(_C)) {}

    static constexpr size_t min_memsize(int32_t _width, int32_t _height)
    {
        return _width * _height * (sizeof(_Y) + sizeof(_C) * 2);
    }

    bool isvalid() const
    {
        return (y.isvalid() && cb.isvalid() && cr.isvalid() && y.isize() == cb.isize() && cb.isize() == cr.isize() &&
                y.pwin() == cb.pwin() && cb.pwin() == cr.pwin());
    }

    const int32_t& height() const
    {
        return y.height();
    }

    const int32_t& width() const
    {
        return y.width();
    }

    const Rect<int32_t>& pwin() const
    {
         return y.pwin();
    }

    size_t get_memsize() const
    {
        return y.get_memsize() + cb.get_memsize() + cr.get_memsize();
    }

    // 修改实际的图像处理区域（即垂直与水平Padding）
    YCbCr& operator ()(const Rect<int32_t>& roi)
    {
        y(roi), cb(roi), cr(roi);
        return *this;
    }

    int32_t copyto(YCbCr<_Y, _C>& ycDst, const Rect<int32_t>& rectSrc, const Rect<int32_t>& rectDst)
    {
        if (this->y.copyto(ycDst.y, rectSrc, rectDst) < 0 || 
            this->cb.copyto(ycDst.cb, rectSrc, rectDst) < 0 || 
            this->cr.copyto(ycDst.cr, rectSrc, rectDst) < 0)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
};

#pragma pack(push, 1)
template<typename _Y, typename _C>
struct YCbCrInterPixel
{
    _Y y;
    _C cb;
    _C cr;
};
#pragma pack(pop)

template<typename _Y, typename _C>
struct YCbCrInter
{
    Imat<YCbCrInterPixel<_Y, _C>> data;
    YCbCrInter(){};
    YCbCrInter(int32_t _width, int32_t _height, void* _data, pads_t _vpads={0, 0}, pads_t _hpads={0, 0}, size_t _step=0) :
    data(_height, _width, 1, (YCbCrInterPixel<_Y, _C>*)_data, std::max(static_cast<size_t>(_step), _width * sizeof(YCbCrInterPixel<_Y, _C>)) * _height,
        _vpads, _hpads, _step * sizeof(YCbCrInterPixel<_Y, _C>)) {}

    YCbCrInter(const YCbCr<_Y, _C>& ycbcr)
    {
        fromYCbCr(ycbcr);
    }

    void fromYCbCr(const YCbCr<_Y, _C>& ycbcr)
    {
        if (!ycbcr.isvalid()) return;

        const int32_t width = ycbcr.width();
        const int32_t height = ycbcr.height();
        const size_t elem_size = sizeof(_Y) + sizeof(_C) * 2;
        

        for (int32_t row = 0; row < height; row++)
        {
            const _Y* y_src = ycbcr.y.ptr(row);
            const _C* cb_src = ycbcr.cb.ptr(row);
            const _C* cr_src = ycbcr.cr.ptr(row);
            struct YCbCrInterPixel<_Y, _C>* dst_row = data.ptr(row);
            for (int32_t col = 0; col < width; col++)
            {
                dst_row[col].y = y_src[col];
                dst_row[col].cb = cb_src[col];
                dst_row[col].cr = cr_src[col];
            }
        }
    }

};

} // namespace

#endif

