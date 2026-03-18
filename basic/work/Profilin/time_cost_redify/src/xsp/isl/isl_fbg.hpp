/**
 * Image SoLution FrontGround & BackGround 
 */

#ifndef __ISL_FBG_H__
#define __ISL_FBG_H__

#include <list>
#include <unordered_set>
#include "boost/any.hpp"
#include "isl_mat.hpp"
#include "isl_transform.hpp"

namespace isl
{

template<typename _T>
struct Vector2;
template<typename _T>
struct Point3
{
    _T x = 0, y = 0, z = 0;

    Point3() : x(0), y(0), z(0) {} // 默认构造
    Point3(_T _x, _T _y, _T _z) : x(_x), y(_y), z(_z) {} // 全参数构造

    // 重载()：读取该Point3对象，通过point().x、point().y、point().z获取空间坐标
    Point3<_T> operator ()() const
    {
        return Point3(x, y, z);
    }

    // 重载==：判断两个Point3对象是否相同
    bool operator ==(const Point3& that) const
    {
        return ((that.x == this->x) && (that.y == this->y) && (that.z == this->z));
    }

    // 重载!=：判断两个Point3对象是否不同
    bool operator !=(const Point3& that) const
    {
        return !(*this == that);
    }

    // 重载<<：打印该Point3坐标到标准输出
    friend std::ostream &operator<<(std::ostream& os, const Point3& _point)
    {
        return os << "(" << _point.x << ", " << _point.y << ", " << _point.z << ")";
    }
};


template<typename _T>
struct Vector3
{
    _T x, y, z; // 自由向量起点平移至原点(0, 0, 0)的原点向量
    Point3<_T> ps, pe; // 以ps为起点、pe为终点的向量

    Vector3() : x(0), y(0), z(0), ps(0, 0, 0), pe(0, 0, 0) {} // 默认构造

    Vector3(_T _x, _T _y, _T _z) : x(_x), y(_y), z(_z), ps(0, 0, 0), pe(x, y, z) {} // 以(0, 0, 0)为起点、(_x, _y, _z)为终点的向量

    Vector3(Point3<_T> _pe) : Vector3(_pe.x, _pe.y, _pe.z) {} // 以(0, 0, 0)为起点、_pe为终点的向量

    Vector3(Point3<_T> _ps, Point3<_T> _pe) : ps(_ps), pe(_pe) {x = _pe.x - _ps.x; y = _pe.y - _ps.y; z = _pe.z - _ps.z;} // 以_ps为起点、_pe为终点的向量

    Vector3(Vector2<_T> _v2) : Vector3(_v2.x, _v2.y, 0) {} // 二维转三维向量，Z坐标为0

    /**
     * @fn      Module
     * @brief   计算向量的模
     * 
     * @return  _T 向量的模
     */
    inline float64_t Module() const
    {
        return std::hypot(x, y, z);
    }

    /**
     * @fn      Module2
     * @brief   计算向量模的平方，即：x^2 + y^2 + z^2
     * 
     * @return  float64_t 向量模的平方
     */
    inline float64_t Module2() const
    {
        return static_cast<float64_t>(x * x + y * y + z * z);
    }

    /**
     * @fn      Cross
     * @brief   计算该向量this与另一向量that的叉乘（向量积）：this × that
     *  
     * @param   [IN] that 另一向量
     * 
     * @return  _T 叉积，>0：that在this的顺时针方向，<0：that在this的逆时针方向 
     * @warning 这里的顺时针和逆时针是基于图像坐标系描述，图像坐标系的Y轴是朝下的，与自然坐标系相反
     */
    inline Vector3<_T> Cross(const Vector3<_T>& that) const
    {
        return Vector3<_T>(this->y * that.z - this->z * that.y, this->z * that.x - this->x * that.z, this->x * that.y - this->y * that.x);
    }
};


template<typename _T>
struct Vector2
{
    _T x, y; // 自由向量起点平移至原点(0, 0)的原点向量
    Point<_T> ps, pe; // 以ps为起点、pe为终点的向量

    Vector2() : x(0), y(0), ps(0, 0), pe(0, 0) {} // 默认构造

    Vector2(_T _x, _T _y) : x(_x), y(_y), ps(0, 0), pe(x, y) {} // 以(0, 0)为起点、(_x, _y)为终点的向量

    Vector2(Point<_T> _pe) : Vector2(_pe.x, _pe.y) {} // 以(0, 0)为起点、_pe为终点的向量

    Vector2(Point<_T> _ps, Point<_T> _pe) : ps(_ps), pe(_pe){x = _pe.x - _ps.x; y = _pe.y - _ps.y;} // 以_ps为起点、_pe为终点的向量

    /**
     * @fn      Module
     * @brief   计算向量的模
     * 
     * @return  _T 向量的模
     */
    inline float64_t Module() const
    {
        return std::hypot(x, y);
    }

    /**
     * @fn      Module2
     * @brief   计算向量模的平方，即：x^2 + y^2
     * 
     * @return  float64_t 向量模的平方
     */
    inline float64_t Module2() const
    {
        return static_cast<float64_t>(x * x + y * y);
    }

    inline Vector2<float64_t> Normalize() const
    {
        float64_t len = Module();
        return (len > FLT_EPSILON) ? Vector2<float64_t>(x / len, y / len) : Vector2<float64_t>(0.0, 0.0);
    }

    /**
     * @fn      Cross
     * @brief   计算该向量this与另一向量that的叉乘（向量积）：this × that
     * @note    注：叉乘的结果向量垂直于this、that所在的XY平面，这里仅计算其叉积数值 
     * 
     * @param   [IN] that 另一向量
     * 
     * @return  _T 叉积，>0：that在this的顺时针方向，<0：that在this的逆时针方向 
     * @warning 这里的顺时针和逆时针是基于图像坐标系描述，图像坐标系的Y轴是朝下的，与自然坐标系相反
     */
    inline _T Cross(const Vector2<_T>& that) const
    {
        return this->x * that.y - this->y * that.x;
    }

    /**
     * @fn      Cross3
     * @brief   计算该向量this与另一向量that的叉乘（向量积）：this × that，
     * @note    注：叉乘的结果向量垂直于this、that所在的XY平面，即三维向量的x/y均为0
     * 
     * @param   [IN] that 另一向量
     * 
     * @return  Vector3<_T> 叉积
     */
    inline Vector3<_T> Cross3(const Vector2<_T>& that) const
    {
        return Vector3<_T>(*this).Cross(Vector3<_T>(that));
    }

    /**
     * @fn      Dot
     * @brief   计算该向量this与另一向量that的点乘（数量积）：this · that
     * @note    表示向量that在this方向上的投影与this的乘积，即两个向量的相似度，值越大相似度越高 
     *  
     * @param   [IN] that 另一向量
     * 
     * @return  _T 内积，>0：夹角在0°~90°之间，=0：垂直，<0：在90°~180°之间
     */
    inline _T Dot(const Vector2<_T>& that) const
    {
        return this->x * that.x + this->y * that.y;
    }

    /// 重载*，向量点乘，与Dot()功能相同
    inline _T operator*(const Vector2<_T>& that) const
    {
        return this->Dot(that);
    }

    /// 重载*，向量数乘
    inline Vector2<_T>&& operator*(const _T& ratio) const
    {
        return Vector2<_T>(this->x * ratio, this->y * ratio);
    }

    /// 重载*=，向量数乘
    inline Vector2<_T>& operator*=(const _T& ratio)
    {
        this->x *= ratio;
        this->y *= ratio;
        return *this;
    }

    /// 重载+，向量加法
    inline Vector2<_T>&& operator+(const Vector2<_T>& that) const
    {
        return Vector2<_T>(this->x + that.x, this->y + that.y);
    }

    /// 重载+=，向量加法
    inline Vector2<_T>& operator+=(const Vector2<_T>& that)
    {
        this->x += that.x;
        this->y += that.y;
        return *this;
    }

    /// 重载-，负向量
    inline Vector2<_T> operator-() const
    {
        return Vector2<_T>(this->pe, this->ps);
    }

    /// 重载-，向量减法，→OA - →OB = →BA，即 B -> A
    inline Vector2<_T>&& operator-(const Vector2<_T>& that) const
    {
        return Vector2<_T>(this->x - that.x, this->y - that.y);
    }

    /// 重载-=，向量减法
    inline Vector2<_T>& operator-=(const Vector2<_T>& that)
    {
        this->x -= that.x;
        this->y -= that.y;
        return *this;
    }

    friend std::ostream &operator<<(std::ostream& os, const Vector2& _vec)
    {
        return os << "(" << _vec.x << ", " << _vec.y << ") # " << _vec.ps << " -> " << _vec.pe;
    }
};

enum class pointseq_t
{
    na,     // 无序或未知
    cw,     // 顺时针顺序
    ccw,    // 逆时针顺序
    b2t_l2r // 从下到上，每行按从左到右顺序
};

template<typename _T>
class ConvexPolygon
{
public:
    using polygon_t = std::vector<Point<_T>>;

private:
    polygon_t vertices; // 顶点
    pointseq_t seqmode = pointseq_t::na; // 排序方式，图像坐标，非自然坐标，Y正轴朝下

public:
    ConvexPolygon() {} // 默认构造

    ConvexPolygon(const Point<_T>& _pot) : vertices({_pot}) {} // 点

    ConvexPolygon(const Point<_T>& _potA, const Point<_T>& _potB) : vertices({_potA, _potB}) {} // 线

    ConvexPolygon(const polygon_t& _vertices) : vertices(_vertices) {}

    // 注意临时ConvexPolygon对象的悬空引用
    inline const polygon_t& GetVertices() const
    {
        return this->vertices;
    }

    /**
     * @fn      TraceConvexHull
     * @brief   跟踪凸壳，形成点集的最小包围凸多边形，并将凸多边形顶点从左上开始顺时针存入this->vertices中
     * @note    函数内部描述的各种方向均相对图像坐标系而言 
     *  
     * @param   [IN] pointSet 点集，若为nullptr，则以this->vertices为默认输入 
     * @note    若为非nullptr，函数内会重新排序pointSet中的对象
     *  
     * @return  int32_t <0：失败，其他：最小包围凸多边形的顶点数
     */
    int32_t TraceConvexHull(std::vector<Point<_T>>* pointSet=nullptr)
    {
        bool bConstruct = (pointSet == nullptr || pointSet == &this->vertices);
        #if 0
        std::vector<Point<_T>> pointsTmp(bConstruct ? this->vertices : std::vector<Point<_T>>());
        std::vector<Point<_T>>& pointsRef = bConstruct ? pointsTmp : *pointSet;
        #else
        std::optional<std::vector<Point<_T>>> pointsTmp;
        if (bConstruct)
        {
            pointsTmp.emplace();
            pointsTmp->assign(this->vertices.begin(), this->vertices.end());
        }
        std::vector<Point<_T>>& pointsRef = (bConstruct) ? pointsTmp.value() : *pointSet;
        #endif

        /// 点集从左到右、从上到下排序
        auto _l2r_t2b = [](const Point<_T>& ptA, const Point<_T>& ptB) -> bool {
            return (ptA.x != ptB.x) ? (ptA.x < ptB.x) : (ptA.y < ptB.y); // left -> right, then top -> bottom
        };
        std::sort(pointsRef.begin(), pointsRef.end(), _l2r_t2b);
        this->vertices = {pointsRef.front()};
        this->seqmode = pointseq_t::cw;

        if (pointsRef.size() > 1)
        {
            auto _cross = [](const Point<_T>& ptO, const Point<_T>& ptA, const Point<_T>& ptB) -> _T {
                Vector2<_T> oa(ptO, ptA); Vector2<_T> ob(ptO, ptB); return oa.Cross(ob);
            };

            /// 以左上角为起点，顺时针遍历得到右上区域的凸边点集
            for (auto it = pointsRef.begin()+1; it != pointsRef.end(); ++it)
            {
                /// 最后一条边向量OA，其起点O与新点B构成向量OB，若OB在向量OA逆时针方向，删除点A再次计算，直至变顺时针，保留该边并插入点B
                while (vertices.size() >= 2 && _cross(*(vertices.rbegin()+1), vertices.back(), *it) <= 0)
                {
                    this->vertices.pop_back();
                }
                this->vertices.push_back(*it);
            }

            /// 以右下角为起点，顺时针遍历得到左下区域的凸边点集，最后会回到左上角
            size_t reserve = this->vertices.size() + 1; // 右下角点在上述流程中已插入
            for (auto it = pointsRef.rbegin()+1; it != pointsRef.rend(); ++it)
            {
                while (vertices.size() >= reserve && _cross(*(vertices.rbegin()+1), vertices.back(), *it) <= 0)
                {
                    this->vertices.pop_back();
                }
                this->vertices.push_back(*it);
            }
            if (this->vertices.front() == this->vertices.back()) // 避免重复插入左上角点
            {
                this->vertices.pop_back();
            }
        }

        return this->vertices.size();
    }

    /**
     * @fn      Distance2Point
     * @brief   计算点与凸多边形间的最短距离
     * @note    函数内部可能会对输入的凸多边形顶点进行顺时针排序 
     * @note    函数内部描述的各种方向均相对图像坐标系而言
     *  
     * @param   [IN] pot 点
     * @param   [OUT] closest 凸多边形上离pot最近的点
     * @param   [OUT] segment 凸多边形上离pot最近的边，点closest也在这条边上
     * 
     * @return  float64_t <0：点在凸多边形内；=0：在凸多边形某条边上，>0：在凸多边形外
     */
    float64_t Distance2Point(const Point<_T>& pot, Point<float64_t>* closest=nullptr, std::pair<Point<_T>, Point<_T>>* segment=nullptr)
    {
        int32_t vnum = this->vertices.size();
        if (0 == vnum)
        {
            return std::numeric_limits<float64_t>::max();
        }
        else if (1 == vnum) // 点到点
        {
            Point<_T>& potA = this->vertices.front();
            Vector2<_T> vecAP(potA, pot);
            if (closest != nullptr)
            {
                *closest = Point<float64_t>(potA.x, potA.y); // 最近的点当然是另一个点
            }
            if (segment != nullptr) // 递归时给其他条件（线段、凸多边形）用的
            {
                *segment = {potA, potA};
            }
            return vecAP.Module();
        }
        else if (2 == vnum) // 点到线段
        {
            Point<_T>& potA = this->vertices.front();
            Point<_T>& potB = this->vertices.back();
            Vector2<_T> vecAB(potA, potB);
            if (vecAB.x == 0 && vecAB.y == 0) // A与B重合，退化为点到点的距离
            {
                return ConvexPolygon(potA).Distance2Point(pot, closest, segment);
            }

            Point<float64_t> potTmp;
            Point<float64_t>& closestPot = (closest != nullptr) ? (*closest) : potTmp;
            Vector2<_T> vecAP(potA, pot);
            float64_t t = static_cast<float64_t>(vecAB.Dot(vecAP)) / vecAB.Module2();
            if (t <= 0.0) // 垂足位于A点之前 → 最近点为A
            {
                closestPot = Point<float64_t>(potA);
                if (segment != nullptr)
                {
                    *segment = {potA, potA}; // 坍缩为点A
                }
            }
            else if (t >= 1.0) // 垂足位于B点之后 → 最近点为B
            {
                closestPot = Point<float64_t>(potB);
                if (segment != nullptr)
                {
                    *segment = {potB, potB}; // 坍缩为点B
                }
            }
            else // 垂足在线段AB上 → 最近点为垂足
            {
                closestPot = Point<float64_t>(t * vecAB.x + potA.x, t * vecAB.y + potA.y);
                if (segment != nullptr)
                {
                    *segment = {potA, potB};
                }
            }
            return std::hypot(closestPot.x - pot.x, closestPot.y - pot.y);
        }
        else // 点到凸多边形
        {
            if (this->seqmode != pointseq_t::cw) // 非顺时针先排序
            {
                vnum = this->TraceConvexHull();
                if (vnum <= 2)
                {
                    return this->Distance2Point(pot, closest, segment);
                }
            }

            bool bPotInThis = true; // 输入的点是否在多边形内部
            float64_t prev = 0.0;
            for (int32_t i = 0; i < vnum; ++i)
            {
                Point<_T>& potA = this->vertices.at(i);
                Point<_T>& potB = this->vertices.at((i + 1) % vnum);
                Vector2<_T> vecAB(potA, potB);
                Vector2<_T> vecAP(potA, pot);
                float64_t curt = vecAB.Cross(vecAP);
                if (std::abs(curt) <= FLT_EPSILON) // 点P在AB上，直接返回
                {
                    if (closest != nullptr)
                    {
                        *closest = Point<float64_t>(pot.x, pot.y); // 最近的点就是自身
                    }
                    if (segment != nullptr)
                    {
                        *segment = {potA, potB};
                    }
                    return 0.0;
                }
                else
                {
                    if (std::abs(prev) <= FLT_EPSILON) // 初始化叉积符号
                    {
                        prev = curt;
                    }
                    else
                    {
                        if (prev * curt < 0.0) // 有存在符号相反，则点在凸多边形外部
                        {
                            bPotInThis = false;
                            break;
                        }
                    }
                }
            }

            if (bPotInThis) // 在凸多边形内，点P到凸多边形的最短距离一定出现在某条边上（而不是顶点）
            {
                float64_t dmin = std::numeric_limits<float64_t>::max();
                for (int32_t i = 0; i < vnum; ++i)
                {
                    Point<_T>& potA = this->vertices.at(i);
                    Point<_T>& potB = this->vertices.at((i + 1) % vnum);
                    Vector2<_T> vecAB(potA, potB);
                    Vector2<_T> vecAP(potA, pot);
                    _T product = vecAB.Dot(vecAP);
                    if (product >= 0)
                    {
                        float64_t t = static_cast<float64_t>(product) / vecAB.Module2();
                        if (t <= 1.0)
                        {
                            Point<float64_t> pedal(t * vecAB.x + potA.x, t * vecAB.y + potA.y);
                            float64_t len = std::hypot(pedal.x - pot.x, pedal.y - pot.y);
                            if (len < dmin)
                            {
                                dmin = len;
                                if (closest != nullptr)
                                {
                                    *closest = pedal;
                                }
                                if (segment != nullptr)
                                {
                                    *segment = {potA, potB};
                                }
                            }
                        }
                    }
                }
                return -dmin;
            }
            else // 在凸多边形外
            {
                #if 1
                ConvexPolygon<_T> polyTmp;
                std::vector<Point<_T>> pointSet(this->vertices);
                pointSet.push_back(pot);
                polyTmp.TraceConvexHull(&pointSet);
                const polygon_t &polyVertices = polyTmp.GetVertices();
                #if 0
                std::cout << "new point: " << pot << " -> ";
                for (auto it = polyVertices.begin(); it != polyVertices.end(); ++it)
                {
                    std::cout << *it << ", ";
                }
                std::cout << std::endl;
                #endif

                auto it = std::find(polyVertices.begin(), polyVertices.end(), pot);
                if (it != polyVertices.end())
                {
                    int32_t nnum = polyVertices.size(); // new polygon vertices number
                    int32_t idx = std::distance(polyVertices.begin(), it);
                    Point<_T> potPrev(polyVertices.at((idx - 1 + nnum) % nnum));
                    Point<_T> potNext(polyVertices.at((idx + 1) % nnum));
                    if (nnum <= vnum) // 点Pot代替了原多边形中的某些点
                    {
                        // 在“this”中查找被代替的那些点的前一个点和后一个点，点Pot到凸多边形的最短距离即为到那些点组成的多边形的距离
                        auto iPrev = std::find(this->vertices.begin(), this->vertices.end(), potPrev);
                        auto iNext = std::find(this->vertices.begin(), this->vertices.end(), potNext);
                        if (iPrev != this->vertices.end() && iNext != this->vertices.end())
                        {
                            polygon_t lite;
                            if (iPrev < iNext) // [iPrev+1, iNext)
                            {
                                lite.assign(iPrev+1, iNext);
                            }
                            else // [iPrev+1, end) + [begin, iNext)
                            {
                                lite.assign(iPrev+1, this->vertices.end());
                                lite.insert(lite.end(), this->vertices.begin(), iNext);
                            }
                            return ConvexPolygon<_T>(lite).Distance2Point(pot, closest, segment);
                        }
                        else
                        {
                            return std::numeric_limits<float64_t>::max();
                        }
                    }
                    else // 点Pot与原多边形组成了一个新的凸多边形
                    {
                        // 点Pot到凸多边形的最短距离即为到前后两个相邻点构成的边的距离（不一定是垂足，也可能是这两个点）
                        return ConvexPolygon<_T>(potPrev, potNext).Distance2Point(pot, closest, segment);
                    }
                }
                else
                {
                    return std::numeric_limits<float64_t>::max();
                }
                #else // 遍历所有的边
                {
                    float64_t dmin = std::numeric_limits<float64_t>::max();
                    Point<float64_t> potTmp;
                    for (size_t i = 0; i < vnum; ++i)
                    {
                        Point<_T> &curt = this->vertices.at(i);
                        Point<_T> &next = this->vertices.at((i + 1) % vnum);
                        auto dist = ConvexPolygon(curt, next).Distance2Point(pot, &potTmp);
                        if (dist < dmin)
                        {
                            dmin = dist;
                            if (closest != nullptr)
                            {
                                *closest = potTmp;
                            }
                            if (segment != nullptr)
                            {
                                *segment = { curt, next };
                            }
                        }
                    }
                    return dmin;
                }
                #endif
            }
        }
    }


    /**
     * @fn      Distance2Polygon
     * @brief   计算两个凸多边形间的最短距离，GJK算法
     * @note    两个凸多边形所有顶点构成的闵可夫斯基差集C=a-b(a∈A,b∈B)，
     *          若C包含原点，则A和B相交；否则，原点到C的距离即两多边形的最短距离。
     *  
     * @param   [IN] that 另一个凸多边形
     * 
     * @return  float64_t 最短距离，=0表示两凸多边形发生碰撞或嵌入
     */
    float64_t Distance2Polygon(const ConvexPolygon<_T>& that)
    {
        auto _support = [](const ConvexPolygon<_T>& poly, const Vector2<_T>& dir) -> Point<_T> {
            _T dotMax = std::numeric_limits<_T>::min();
            size_t furthest = 0, vnum = poly.vertices.size();
            for (size_t i = 0; i < vnum; ++i) {
                Vector2<_T> vec(poly.vertices.at(i)); // 以原点(0, 0)为起点的点向量
                _T product = vec.Dot(dir);
                if (product > dotMax) {
                    dotMax = product;
                    furthest = i;
                }
            }
            return poly.vertices.at(furthest);
        };

        auto _minkowskiDiff = [this, _support](const ConvexPolygon<_T>& that, Vector2<_T>& dir) -> Point<_T> { // 闵可夫斯基差
            Vector2<_T>&& supVec = Vector2<_T>(_support(that, -dir), _support(*this, dir));
            return Point<_T>(supVec.x, supVec.y);
        };

        const Point<_T> potO(0, 0); // 原点
        Vector2<_T> dir(1, 0); // 初始方向
        polygon_t minkowskiSet({_minkowskiDiff(that, dir)}); // 获得闵可夫斯基差集的第1个顶点
        dir = Vector2<_T>(minkowskiSet.front(), potO); // 以顶点指向原点的方向作为第2次迭代方向
        //std::cout << "1th point: " << minkowskiSet.back() << ", next dir: " << dir << std::endl;

        size_t itCnt = 0;
        while (++itCnt)
        {
            Point<_T> newVertix = _minkowskiDiff(that, dir); // 继续第N次迭代，获得闵可夫斯基差集的第N个顶点
            if (minkowskiSet.end() == std::find(minkowskiSet.begin(), minkowskiSet.end(), newVertix)) // not found
            {
                #if 0
                if (Vector2<_T>(newVertix).dot(dir) >= 0)
                {
                    // >=0：可能过原点或单纯形包含原点（即可能发生碰撞），<0：一定不会碰撞
                }
                #endif

                minkowskiSet.push_back(newVertix);
                const size_t snum = std::min(minkowskiSet.size(), static_cast<size_t>(3));
                ConvexPolygon simplex(polygon_t(minkowskiSet.rbegin(), minkowskiSet.rbegin() + snum)); // 最新的3个点构建单纯形
                Point<float64_t> closestPot;
                std::pair<Point<_T>, Point<_T>> closestSeg;
                float64_t s2o = simplex.Distance2Point(potO, &closestPot, &closestSeg); // 原点到simplex的距离
                if (s2o > FLT_EPSILON && snum == simplex.GetVertices().size())
                {
                    /// 更新迭代方向为单纯形上距离原点最近的点指向原点的向量
                    if constexpr (std::is_floating_point_v<_T>) // 浮点类型直接计算向量
                    {
                        dir = Vector2<_T>(static_cast<Point<_T>>(closestPot), potO);
                    }
                    else // 整数类型计算整数向量，避免浮点误差
                    {
                        Point<_T>& potA = closestSeg.first;
                        Point<_T>& potB = closestSeg.second;
                        Vector2<float64_t> vecPA(closestPot, Point<float64_t>(potA));
                        Vector2<float64_t> vecPB(closestPot, Point<float64_t>(potB));
                        if (vecPA.Module2() <= FLT_EPSILON)
                        {
                            dir = Vector2<_T>(potA, potO);
                        }
                        else if (vecPB.Module2() <= FLT_EPSILON)
                        {
                            dir = Vector2<_T>(potB, potO);
                        }
                        else
                        {
                            #if 1
                            Vector2<_T> vecAO(potA, potO), vecAB(potA, potB);
                            Vector2<_T> perpAB(-vecAB.y, vecAB.x); // 计算AB的法方向
                            dir = (perpAB.Dot(vecAO) < 0) ? -perpAB : perpAB; // 使AB的法方向指向原点
                            #else // 二维叉乘转三维再转二维得到AB指向原点的法向量
                            Vector3<_T> cp3 = vecAB.Cross3(vecAO).Cross(vecAB);
                            dir = Vector2<_T>(cp3.x, cp3.y);
                            #endif
                        }
                        //std::cout << itCnt+1 << "th point: " << minkowskiSet.back() << ", closest segment: " << potA << " -> " << potB << ", next dir:" << dir << std::endl;
                    }
                }
                else // 原点在单纯形内部或边上，或单纯形本身并不是凸多边形
                {
                    #if 0
                    std::cout << itCnt+1 << "th point: " << minkowskiSet.back() << ", closest: " << closestPot << " @ " << closestSeg.first << " -> " << closestSeg.second << std::endl;
                    for (auto it = simplex.vertices.begin(); it != simplex.vertices.end(); ++it)
                    {
                        std::cout << *it << ", ";
                    }
                    std::cout << std::endl;
                    #endif
                    return std::max(s2o, 0.0);
                }
            }
            else
            {
                //std::cout << itCnt+1 << "th point: " << newVertix << ", duplicated! End!" << std::endl;
                return ConvexPolygon<_T>(minkowskiSet).Distance2Point(potO);
            }
        }

        return 0.0;
    }
};


class Contour
{
public:
    struct HashPoint
    {
        size_t operator()(const Point<int32_t>& p) const
        {
            return std::hash<int32_t>()(p.x) ^ (std::hash<int32_t>()(p.y) << 1);
        }
    };

private:
    uint8_t fgVal; // ForeGround，轮廓区域（包含边界与内部）前景值
    uint8_t bgVal; // BackGround，默认背景值
    uint8_t arVal; // ARound，外轮廓环绕值，与背景值接近
    uint8_t markVal = 0; // 轮廓区域（包含边界与内部）的Mark值，取值范围：(fgVal, bgVal)
    Point<int32_t> startPoint; // 轮廓起始点，注：相对于二值图坐标

    std::list<Point<int32_t>> boundaryTrace; // 轮廓边界点跟踪列表，注：相对于二值图坐标
    std::unordered_set<Point<int32_t>, HashPoint> boundarySet; // 轮廓边界点集合（无重复）
    Rect<int32_t> boundingBox; // 轮廓矩形包围盒，注：相对于二值图坐标
    boost::any privateData; // 私有数据

    int32_t SetVal(Imat<uint8_t>& binz, uint8_t val, std::optional<Rect<int32_t>> region);

public:
    //Contour() noexcept = default;
    Contour(uint8_t fg, uint8_t bg, uint8_t ar) : fgVal(fg), bgVal(bg), arVal(ar) {}

    // 当显式声明了移动构造函数（或移动赋值），编译器不会自动生成拷贝构造函数，需要显示指定
    Contour(Contour&&) noexcept = default; // 生成默认的移动构造函数
    Contour& operator=(Contour&&) noexcept = default; // 移动赋值

    Contour(const Contour&) = default; // 生成默认的拷贝构造函数
    Contour& operator=(const Contour&) = default; // 拷贝赋值

    inline const Point<int32_t>& StartPoint(void) const
    {
        return this->startPoint;
    }

    inline void SetFg(const uint8_t fg)
    {
        this->fgVal = fg;
        return;
    }

    inline const uint8_t& MarkVal(void) const
    {
        return this->markVal;
    }

    /// 轮廓边界长度，即点的数量
    inline size_t BoundaryLen(void) const
    {
        return this->boundaryTrace.size();
    }

    inline const Rect<int32_t>& BoundingBox(void) const
    {
        return this->boundingBox;
    }

    /// 存入私有数据
    template <typename T>
    void setPrivate(T&& data)
    {
        this->privateData = boost::any(std::forward<T>(data));
    }

    /// 是否已存入私有数据
    template<typename T>
    bool hasPrivate(void) const
    {
        /// 使用C++标准库的写法：privateData.type() == typeid(T) && privateData.has_value();
        return boost::any_cast<T>(&privateData) != nullptr;
    }

    /// 获取私有数据的指针，未存入时返回nullptr
    template<typename T>
    T* getPrivate(void)
    {
        return boost::any_cast<T>(&privateData);
    }

    /// 清除私有数据
    void clearPrivate(void)
    {
        /// 使用C++标准库的写法：this->privateData.reset();
        this->privateData = boost::any();
    }

    #if 0
    inline std::vector<Point<int32_t>> GetBoundaryPoints(void) const
    {
        std::vector<Point<int32_t>> vtmp(this->boundaryPoints.begin(), this->boundaryPoints.end());
        return vtmp;
    }
    #endif

    /**
     * @fn      Follow
     * @brief   轮廓跟踪，生成的轮廓边界点集是从this->startPoint开始，逆时针方向围绕，像素为非背景值
     *          跟踪完成后，轮廓边界点像素值会自动更新为this->markVal，以标识轮廓范围
     * 
     * @param   [IN] binz 二值图
     * @param   [IN] ptStart 轮廓跟踪起始点，该点像素值为fgVal或mVal
     * @param   [IN] mVal 轮廓mark值，用于标识轮廓范围
     * @param   [IN] mOth 当前存在的其他轮廓mark列表，非空时，检测到与该列表中的轮廓发生碰撞则直接返回
     * @param   [OUT] mColl 与哪些轮廓发生碰撞的轮廓mark列表
     * @param   [IN] presuf 跟踪非闭合轮廓时需输入，first：起始点的前一个点，second：跟踪结束点
     * 
     * @return  int32_t >0：跟踪得到的轮廓点数，=0：跟踪结束，但并未经过结束点presuf.second，-1：异常，-2：与其他轮廓发生碰撞
     */
    int32_t Follow(Imat<uint8_t>& binz, const Point<int32_t>& ptStart, const uint8_t mVal, const std::vector<uint8_t>& mOth,
                   std::unordered_set<uint8_t>& mColl, std::optional<std::pair<Point<int32_t>, Point<int32_t>>> presuf=std::nullopt);

    /**
     * @fn      Grow
     * @brief   轮廓的向下生长，并更新该对象的boundaryTrace、boundarySet、boundingBox属性
     * 
     * @param   [IN] binz 二值图
     * @param   [IN] starty 向下生长的起始行
     * @param   [IN] marks 其他轮廓mark列表
     * @param   [OUT] bbox 生长段轮廓的boundingBox
     * 
     * @return  int32_t fgVal：正常，-1：异常，其他：发生碰撞轮廓mark值
     */
    int32_t Grow(Imat<uint8_t>& binz, const int32_t starty, const std::vector<uint8_t>& marks, Rect<int32_t>& bbox);

    /**
     * @fn      Fillin
     * @brief   给当前轮廓对象内部置为mark值
     * 
     * @param   [IN] binz 二值图
     * @param   [IN] region 有输入时，轮廓在该区域内的部分才会被填充，否则填充所有轮廓区域
     */
    void Fillin(Imat<uint8_t>& binz, std::optional<Rect<int32_t>> region=std::nullopt);

    void SmoothShell(Imat<uint16_t>& shell, Imat<uint8_t>& binz, std::optional<Rect<int32_t>> region=std::nullopt);

    /**
     * @fn      Remark
     * @brief   对轮廓区域重新Mark，新的Mark值为mval 
     * @note    该操作必须在Fillin之后，接口内部是根据旧的Mark值来识别并更新的 
     * 
     * @param   [IN] binz 二值图
     * @param   [IN] mval 新的mark值
     * @param   [IN] region 有输入时，轮廓在该区域内的部分才会被重新mark，否则更新所有轮廓区域
     */
    void Remark(Imat<uint8_t>& binz, uint8_t mval, std::optional<Rect<int32_t>> region=std::nullopt);

    /**
     * @fn      Clear
     * @brief   将轮廓区域图像清除为背景
     * @note    该操作必须在Fillin之后，接口内部是根据旧的Mark值来识别并清除的 
     * 
     * @param   [IN] binz 二值图
     * @param   [IN] region 有输入时，轮廓在该区域内的部分才会被清除，否则清除所有轮廓区域
     */
    void Clear(Imat<uint8_t>& binz, std::optional<Rect<int32_t>> region=std::nullopt);

    /**
     * @fn      Move
     * @brief   移动轮廓
     * 
     * @param   [IN] ver 垂直移动行数，负数为向上移动，正数为向下移动
     * @param   [IN] hor 水平移动列数，负数为向左移动，正数为向右移动
     */
    void Move(int32_t ver=0, int32_t hor=0);

    /**
     * @fn      CoverageRate
     * @brief   该轮廓对象与另一轮廓的交叉覆盖率 
     * @note    只计算轮廓点集的交叉率，不计算轮廓内部区域的覆盖率，因此只针对轮廓A吞并邻旁的轮廓B，不适合轮廓B是A的内轮廓情况
     * 
     * @param   [IN] that 另一轮廓对象
     * @param   [OUT] compSet 两轮廓跟踪点的交集
     * @param   [IN] effect that轮廓对象需计算的有效区域
     * 
     * @return  std::pair<size_t, size_t> fist：that在effect区域内的点数，second：this在that有效区中的点的命中数
     */
    std::pair<size_t, size_t> CoverageRate(Contour& that, std::vector<Point<int32_t>>& compSet, std::optional<Rect<int32_t>> effect=std::nullopt);
};


class Fbg
{
public:
    enum fg_morph_t {morph_none, morph_start, morph_middle, morph_end, morph_indep/*| --- |*/, morph_stend/*|-  --|*/};
    struct Fg4Slice
    {
        fg_morph_t status = morph_none;
        std::vector<Rect<int32_t>> bboxes; // bounding boxes
    };
    inline static const uint8_t fgVal = 0; // ForeGround，轮廓区域（包含边界与内部）前景值
    inline static const uint8_t bgVal = 0xFF; // BackGround，默认背景值
    inline static const uint8_t arVal = 0xFE; // ARound，外轮廓环绕值，与背景值接近

private:
    uint16_t bg_lum_th = 62800; // 背景亮度阈值，大于此值认为绝对背景
    uint16_t fg_lum_th = 60800; // 前景亮度阈值，小于此值认为绝对前景，介于fg_lum_th和bg_lum_th为不确定前背景区
    const int32_t pads_around = 2;
    const Size<float64_t> prof_dscale{2.0, 2.0}; // Profile Downsampling Scaling Factor by Original XRaw
    std::vector<px_intp_t> dsLutHor, dsLutVer; // 下采样查找表
    std::vector<px_intp_t> usLutHor, usLutVer; // 上采样查找表
    Size<int32_t> prof_smax{192, 800}; // Profile Size Max
    Size<int32_t> prof_msize{prof_smax.width + 2 * pads_around, prof_smax.height + 2 * pads_around}; // Profile Memory Size
    std::unique_ptr<uint8_t[]> prof_mem = std::make_unique<uint8_t[]>(prof_smax.area() * 3 + prof_msize.area()); // 连续4块
    Imat<uint8_t> profDs{prof_smax.height, prof_smax.width, 1, prof_mem.get(), (size_t)prof_smax.area(), 
                         {0, prof_smax.height}}; // 下采样图像，1st of prof_mem
    Imat<uint8_t> profEdge{profDs, profDs.ptr()+profDs.get_memsize(), (size_t)prof_smax.area()}; // 非前景区的边缘，2nd of prof_mem
    Imat<uint8_t> profTh{profDs, profEdge.ptr()+profEdge.get_memsize(), (size_t)prof_smax.area()}; // 阈值图像，3nd of prof_mem
    Imat<uint8_t> profBinz{prof_msize.height, prof_msize.width, 1, profTh.ptr()+profTh.get_memsize(), (size_t)prof_msize.area(),
                           {pads_around, prof_msize.height-pads_around}, {pads_around, pads_around}}; // 二值化图像，3rd of prof_mem
    Size<int32_t> cont_smax{192, 16}; // Max height for 2 downsample slices
    Size<int32_t> cont_msize{cont_smax.width + 2 * pads_around, cont_smax.height + 2 * pads_around};
    std::unique_ptr<uint8_t[]> cont_mem = std::make_unique<uint8_t[]>(cont_smax.area() + cont_msize.area()); // 临时内存

    struct ContAttr
    {
        //bool bPrim = false; // 是否为主轮廓
        int32_t fgConf = -1; // 前景置信度
        ContAttr() noexcept = default;
    };

    uint16_t bgThUsrSet = bg_lum_th; // 用户设置的背景亮度阈值
    std::pair<int32_t, int32_t> fgSensitivity{50, 50}; // 前景检测灵敏度，first：当前生效，second：用户设置
    uint64_t procCnt = 0; // 计数器，记录FbgCull()的调用次数，用于调试
    int32_t fgAreaMin = 16; // 最小前景面积阈值，小于该值则认为噪声区，不小于9
    int32_t fgConfMin = 50; // 前景最低置信度，小于该值则认为噪声区
    bool bNarrowFgEn = false; // 是否开启极窄（一根细线等）前景的判定，开启后皮带边缘偏移引起的点状噪声也会判定为包裹
    std::list<Contour> fgContours; // 前景轮廓
    std::vector<Fg4Slice> fgSliceOut; // profDs中所有条带的前景属性

    std::function<uint8_t(const uint16_t)> _compress_16to8 = [](const uint16_t src) -> uint8_t {
        return (src<5120) ? (src>>6) : ((src<57548) ? 80+((src-5120)>>10) : 131+((src-57548)>>6));
    };

    /**
     * @fn      SearchContours
     * @brief   搜索轮廓，搜索到的新轮廓会追加到列表contours尾
     * 
     * @param   [OUT] contours 轮廓列表，搜索到的轮廓会追加到列表尾
     * @param   [IN] binz 二值图
     * @param   [IN] mark_conf 当前所有轮廓的mark值和置信度对，可能会比contours中的数量多
     * 
     * @return  int32_t 搜索到的新轮廓个数，异常时返回-1
     */
    int32_t SearchContours(std::list<Contour>& contours, Imat<uint8_t>& binz, 
                           const std::optional<std::vector<std::pair<uint8_t, int32_t>>>& mark_conf=std::nullopt);

    /**
     * @fn      FissionContour
     * @brief   分裂轮廓：先清除轮廓disCont在区域disRegion内图像，然后在disCont残余的图像中重新搜索轮廓 
     *          一个轮廓可能会分裂出多个轮廓，分裂出的新轮廓追加到contours列表后，且继承disCont的privateData
     * 
     * @param   [IN/OUT] contours 轮廓列表，分裂出的新轮廓追加到contours列表后，但若disCont在该列表中，是不会自动删除的，需要接口外手动删除
     * @param   [IN] binz 二值图
     * @param   [IN] disCont 需分裂的轮廓
     * @param   [IN] disRegion 需清除的矩形区域，该矩形区域与轮廓区域的交集即为实际清除的图像区域
     * 
     * @return  int32_t 分裂出的新轮廓数，≤0表示分裂失败
     */
    int32_t FissionContour(std::list<Contour>& contours, Imat<uint8_t>& binz, Contour& disCont, const Rect<int32_t>& disRegion);

    /**
     * @fn      MaterObjConf
     * @brief   计算轮廓中存在实际包裹的置信度
     * @note    尽量排除以下无实际包裹情况： 
     *            1. 源剂量波动 
     *            2. 源打火 
     *            3. 皮带接缝区 
     *            4. 皮带边缘翘曲或磨损 
     * 
     * @param   [IN] fgCont 前景轮廓对象
     * 
     * @return  int32_t [0, 100]：置信度，值越大置信度越高，-1：出现异常
     */
    int32_t MaterObjConf(Contour& fgCont); // material object confidence

    /**
     * @fn      ResetProf
     * @brief   重置Fbg对象属性及其内存
     * 
     * @param   [IN] maxSize 需支持的最大轮廓尺寸
     * 
     * @return  int32_t 0：正常，-1：内存申请失败
     */
    int32_t ResetProf(std::optional<Size<int32_t>> smax=std::nullopt);

public:
    /**
     * @fn      SetBgLumTh
     * @brief   设置背景亮度阈值
     * 
     * @param   [IN] bgTh 背景亮度阈值，不小于55000
     */
    void SetBgLumTh(const uint16_t bgTh);

    /**
     * @fn      SetFgSensitivity
     * @brief   设置前景检测灵敏度
     * 
     * @param   [IN] sensitivity 前景检测灵敏度，范围：[0, 100]
     */
    void SetFgSensitivity(const int32_t sensitivity);

    /**
     * @fn      FbgCull
     * @brief   前背景分离（包裹分割）模块
     * 
     * @param   [OUT] fgMask 前景Mask，包裹区填充为fgVal，背景区填充为bgVal，处理区域尺寸与imgIn的pwin()相同，且必须包含上邻域条带区 
     * @warning              不仅输出当前处理区域条带的Mask，也同时会更新上邻域条带的Mask（因为前一次计算缺乏下邻域导致不准确）
     * @param   [IN] imgIn 条带的低能图像
     * @param   [IN] imgTmp 处理使用的临时内存，内存大小不小于imgIn.pwin()内存大小
     * @param   [IN] bRecache 是否重置，重新cache条带时置true，模块内部同步重新缓存
     * 
     * @return  std::vector<Fbg::Fg4Slice> 条带的前景信息，异常时为空，Recache时仅输出当前1个条带分割信息，其他情况输出的size为2
     */
    std::vector<Fg4Slice> FbgCull(Imat<uint8_t>& fgMask, Imat<uint16_t>& imgIn, Imat<uint16_t>& imgTmp, bool bRecache);

    /**
     * @fn      SmoothWeight
     * @brief   包裹Mask图转图像后处理的输出权重图，且在包裹边界区增加了平滑
     * 
     * @param   [OUT] weightOut 图像后处理的输出权重图，255为全使用后处理图像，0为全背景，与maskIn不可为同一内存
     * @param   [IN] maskIn 包裹Mask图，最高位0表示前景，1表示背景，实际处理区域是maskIn.pwin()与region的交集
     * @param   [IN] radius 包裹边界区的平滑半径，采用欧式距离，背景与包裹边界小于(radius+1)为实际平滑区域
     * @param   [IN] regions 实际前景区，用于加速，若输入vector为空，则无前景区；若不输入，则会根据maskIn自动计算
     * 
     * @return  std::vector<Rect<int32_t>> weightOut图中非0矩形区域集合
     */
    static std::vector<Rect<int32_t>> SmoothWeight(Imat<uint8_t>& weightOut, Imat<uint8_t>& maskIn, int32_t radius, 
                                                   const std::optional<std::vector<Rect<int32_t>>>& regions=std::nullopt);
};

} // namespace

#endif

