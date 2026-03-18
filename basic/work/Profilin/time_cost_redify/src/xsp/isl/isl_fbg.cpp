/**
 * Image SoLution Front-BackGround Separation
 */

#include <vector>
#include "isl_util.hpp"
#include "spdlog/spdlog.h"
#include "isl_log.hpp"
#include "isl_fbg.hpp"
#include "isl_filter.hpp"
#include "isl_nree.hpp"

namespace isl
{


/**
 * @fn      Statistic
 * @brief   统计img图像中满足rule规则像素的包围盒、直方图和均值，有需要也输出满足rule规则的Mask图像
 * 
 * @param   [IN] img 需统计的图像
 * @param   [IN] mask 统计规则基于的mask图像，pwin()尺寸需与img相同，若无mask，可直接用img替代
 * @param   [IN] rule 统计规则，第一个参数是针对img的，第二个参数是针对mask的
 * @param   [OUT] rmsk 根据rule生成的最终规则图像，255表示符合规则的像素，0表示不符合
 * @param   [OUT] box 满足rule规则所有像素的最小包围盒，坐标是基于img图像的
 * @param   [OUT] hist 满足rule规则所有像素的直方图，256个Bins
 * @param   [OUT] avg 满足rule规则所有像素的均值
 * 
 * @return  int32_t 满足rule规则的像素个数，-1表示失败
 */
using hist8_t = std::array<uint32_t, 256>;
static int32_t Statistic(Imat<uint8_t>& img, Imat<uint8_t>& mask, std::function<bool(const uint8_t, const uint8_t)> rule, 
                         Imat<uint8_t>* rmsk, Rect<int32_t>* box, hist8_t* hist, uint8_t* avg)
{
    if (!img.isvalid() || !mask.isvalid() || img.pwin().size() != mask.pwin().size())
    {
        return -1;
    }
    if ((rmsk != nullptr) && (!rmsk->isvalid() || img.pwin().size() != rmsk->pwin().size()))
    {
        return -1;
    }

    uint64_t sum = 0, cnt = 0;
    if (hist != nullptr) {hist->fill(0);}
    if (box != nullptr) {box->x = img.width(), box->y = img.height();}
    int32_t right = 0, bottom = 0;
    for (int32_t i = 0, rowi = img.pwin().y; i < img.pwin().height; ++i, ++rowi)
    {
        uint8_t* pi = img.ptr(rowi, img.pwin().x);
        uint8_t* pm = mask.ptr(mask.pwin().y + i, mask.pwin().x);
        uint8_t* pc = (rmsk != nullptr) ? rmsk->ptr(rmsk->pwin().y + i, rmsk->pwin().x) : nullptr;
        for (int32_t j = 0, coli = img.pwin().x; j < img.pwin().width; ++j, ++coli, ++pi, ++pm)
        {
            if (rule(*pi, *pm))
            {
                cnt++;
                if (pc != nullptr) {*pc++ = 255;} // true
                if (hist != nullptr) {hist->at(*pi)++;}
                if (avg != nullptr) {sum += *pi;}
                if (nullptr != box)
                {
                    if (rowi < box->y) {box->y = rowi;}
                    if (rowi > bottom) {bottom = rowi;}
                    if (coli < box->x) {box->x = coli;}
                    if (coli > right) {right = coli;}
                }
            }
            else
            {
                if (pc != nullptr) {*pc++ = 0;} // false
            }
        }
    }
    if (cnt > 0 && nullptr != avg)
    {
        *avg = (sum + cnt / 2) / cnt;
    }
    if (nullptr != box)
    {
        box->width = right - box->x + 1;
        box->height = bottom - box->y + 1;
    }

    return static_cast<int32_t>(cnt);
}


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
int32_t Contour::Follow(Imat<uint8_t>& binz, const Point<int32_t>& ptStart, const uint8_t mVal, const std::vector<uint8_t>& mOth,
                        std::unordered_set<uint8_t>& mColl, std::optional<std::pair<Point<int32_t>, Point<int32_t>>> presuf)
{
    if (!binz.isvalid())
    {
        ISL_LOGE("binz is invalid: {}", binz);
        return -1;
    }
    if (mVal == this->bgVal || mVal == this->arVal)
    {
        ISL_LOGE("mVal is invalid: {}", mVal);
        return -1;
    }

    auto _pxval = [&binz](const Point<int32_t>& px) -> uint8_t& {return *(binz.ptr(px.y, px.x));};

    auto _swap_pt = [] (Point<int32_t>& A, Point<int32_t>& B) {std::swap(A.x, B.x); std::swap(A.y, B.y);};

    /// 8-step around a pixel clockwise
    auto _step_cw8 = [] (Point<int32_t>& current, const Point<int32_t>& center) { // 采用弱连通算法，即对角线也视为连通
        if (current.x > center.x && current.y <= center.y) // 右上->右 或 右->右下
        {
            current.y++;
        }
        else if (current.x < center.x && current.y >= center.y) // 左下->左 或 左->左上
        {
            current.y--;
        }
        else if (current.x >= center.x && current.y > center.y) // 右下->下 或 下->左下
        {
            current.x--;
        }
        else if (current.x <= center.x && current.y < center.y) // 左上->上 或 上->右上
        {
            current.x++;
        }
    };

    // 8-step around a pixel counter-clockwise
    auto _step_ccw8 = [] (Point<int32_t>& current, const Point<int32_t>& center) { // 采用弱连通算法，即对角线也视为连通
        if (current.x > center.x && current.y >= center.y) // 右下->右 或 右->右上
        {
            current.y--;
        }
        else if (current.x < center.x && current.y <= center.y) // 左上->左 或 左->左下
        {
            current.y++;
        }
        else if (current.x <= center.x && current.y > center.y) // 左下->下 或 下->右下
        {
            current.x++;
        }
        else if (current.x >= center.x && current.y < center.y) // 右上->上 或 上->左上
        {
            current.x--;
        }
    };

    // 所有轮廓点值只有2个：fgVal和mVal，且Folow完成后，均为mVal
    if (ptStart.x < 1 || ptStart.y < 1 || (_pxval(ptStart) != this->fgVal && _pxval(ptStart) != mVal))
    {
        ISL_LOGE("contour#{} @ {}: the start point({}) is invalid", mVal, ptStart, _pxval(ptStart));
        return -1;
    }

    const Point<int32_t> ptBdOff(ptStart.x-1, ptStart.y); // 起始点左边相邻的空白点，Boundary Off
    if (!presuf.has_value() && !(_pxval(ptBdOff) == this->bgVal || _pxval(ptBdOff) == this->arVal))
    {
        ISL_LOGE("contour#{} @ {}: the boundary off point{} is invalid, pixel value {}", 
                 mVal, ptStart, ptBdOff, _pxval(ptBdOff));
        return -1;
    }

    int32_t top = ptStart.y, bottom = ptStart.y, left = ptStart.x, right = ptStart.x;
    this->boundaryTrace = {ptStart};
    this->boundarySet = {ptStart};
    this->boundingBox = Rect<int32_t>(left, top, 1, 1);
    _pxval(ptStart) = mVal;

    Point<int32_t> ptCurt = presuf.has_value() ? presuf->first : ptBdOff;
    int32_t cycleCnt = 0;
    const bool bIgnoreCollision = mOth.empty(); // 若输入其他轮廓mark值，则表示与mOth中的轮廓碰撞时，直接返回，不能忽略
    mColl.clear();

    /// 定位轮廓的结束点，正常跟踪逻辑中先找到起始点，紧接着就寻找这个点，找到这个点之后才开始寻找轮廓的第二个像素点
    Point<int32_t> ptEnd; // 轮廓结束点
    if (presuf.has_value())
    {
        ptEnd = presuf->second;
        if (_pxval(ptEnd) != this->fgVal && _pxval(ptEnd) != mVal) // 初始为fgVal或traceVal，Follow后即为markVal
        {
            ISL_LOGE("contour#{} @ {}: the end point({} @ {}) is invalid, fgVal: {}", 
                     mVal, ptStart, _pxval(ptEnd), ptEnd, this->fgVal);
            return -1;
        }
    }
    else
    {
        cycleCnt = 8; // 重置cycleCnt
        while (cycleCnt-- > 0)
        {
            _step_cw8(ptCurt, ptStart); // 以起点为中心，顺时针8-邻域寻找轮廓的结束点
            if (ptCurt == ptBdOff) // 又回到起点，则为孤点
            {
                this->startPoint = ptStart;
                this->markVal = mVal;
                return this->boundaryTrace.size();
            }
            else
            {
                uint8_t& pxVal = _pxval(ptCurt);
                if (pxVal != this->bgVal && pxVal != this->arVal)
                {
                    if (pxVal != this->fgVal && pxVal != mVal)
                    {
                        mColl.insert(pxVal);
                        if (!bIgnoreCollision)
                        {
                            auto im = std::find(mOth.begin(), mOth.end(), pxVal);
                            if (mOth.end() != im) // 存在mark为pxVal的轮廓，发生碰撞
                            {
                                // 不忽略碰撞时直接退出，错误码为-2
                                ISL_LOGW("contour#{} @ {}, abort follow @ {}, collided with {}", mVal, ptStart, ptCurt, pxVal);
                                return -2;
                            }
                        }
                    }
                    ptEnd = ptCurt; // Bingo the End Point
                    break;
                }
                else
                {
                    pxVal = this->arVal;
                }
            }
        }
        if (cycleCnt < 1)
        {
            ISL_LOGE("contour#{} @ {}: search end point failed, cycleCnt {}", mVal, ptStart, cycleCnt);
            return -1;
        }
    }

    /// 以pointStart为中心，逆时针8-邻域寻找下一个轮廓边界点
    #ifdef FOLLOW_DEBUG
    std::stringstream ss;
    ss << "start " << static_cast<int32_t>(_pxval(ptStart)) << "(" << ptStart.x << ", " << ptStart.y << "), ";
    ss << "end " << static_cast<int32_t>(_pxval(ptEnd)) << "(" << ptEnd.x << ", " << ptEnd.y << ")\n";
    #endif

    int32_t retVal = -1;
    Point<int32_t> ptCent = ptStart; // 初始以ptStart为中心点，ptEnd为起始点逆时针方向查找
    Point<int32_t> ptCycle; // 轮廓循环点，判断Follow是否进入无限循环
    cycleCnt = 8; // 重置cycleCnt
    while (cycleCnt-- > 0) // 正常Trace跳出该循环后cycleCnt为[0,7]，且0只有在2个点自旋时出现
    {
        #ifdef FOLLOW_DEBUG
        if (7 == cycleCnt)
        {
            ss << "(" << ptCent.x << ", " << ptCent.y << "), (" << ptCurt.x << ", " << ptCurt.y << ") -> ";
        }
        else
        {
            ss << "(" << ptCurt.x << ", " << ptCurt.y << ") -> ";
        }
        #endif
        _step_ccw8(ptCurt, ptCent);

        uint8_t& pxVal = _pxval(ptCurt);
        if (pxVal != this->bgVal && pxVal != this->arVal) // Bingo the next Boundary Point
        {
            if (ptCycle == Point<int32_t>()) // 尚未初始化
            {
                ptCycle = ptCurt;
            }
            if (presuf.has_value() && ptCent == ptEnd) // 正常结束时，ptCurt需在ptCent（结束点）为中心点的九宫格斜上侧
            {
                if ((ptCurt.y < ptCent.y) || (ptCurt.y == ptCent.y && ptCurt.x < ptCent.x))
                {
                    retVal = this->boundaryTrace.size();
                    break;
                }
                ISL_LOGI("contour#{} @ {}, passing through the end point {}, but next {} is in the lower right corner, continue", 
                         mVal, ptStart, ptCent, ptCurt);
            }
            if (ptCurt == ptStart) // 回到起始点，判断是否结束，当起始点在一条线（One Pixel）上时，是会经过该点的，但并不是结束
            {
                //ISL_LOGI("back to the start point {}, and the cent {}, end {}", ptCurt, ptCent, ptEnd);
                if (ptCent == ptEnd) // 正常结束
                {
                    #ifdef FOLLOW_DEBUG
                    ss << "(" << ptCurt.x << ", " << ptCurt.y << ") End " << cycleCnt << "\n";
                    #endif
                    retVal = this->boundaryTrace.size();
                    break;
                }
                else
                {
                    if (presuf.has_value())
                    {
                        int32_t cycleTmp = 8;
                        Point<int32_t> ptTmp = ptCent;
                        while (cycleTmp-- > 0)
                        {
                            _step_ccw8(ptTmp, ptStart);
                            uint8_t& pxVal = _pxval(ptTmp);
                            if (ptTmp == ptCycle) // 遇到ptCycle之前，并无其他轮廓点，说明已进入死循环
                            {
                                ISL_LOGW("contour#{} @ {}, passing through the cycle point {}, but no end point {}, force to exit", 
                                         mVal, ptStart, ptCycle, ptEnd);
                                retVal = 0;
                                goto EXIT;
                            }
                            else if (pxVal == this->fgVal || pxVal == mVal) // 只是经过ptStart，继续回归正常流程
                            {
                                //ISL_LOGI("ptTmp {}, ptStart {}, ptCent {}", ptTmp, ptStart, ptCent);
                                break;
                            }
                        }
                        if (cycleTmp < 0)
                        {
                            goto EXIT;
                        }
                    }
                }
            }

            #ifdef FOLLOW_DEBUG
            ss << "(" << ptCurt.x << ", " << ptCurt.y << ((presuf.has_value() && ptEnd == ptCurt) ? ") End " : ") Next ") << cycleCnt << "\n";
            #endif
            if (pxVal != this->fgVal && pxVal != mVal)
            {
                #if 1
                mColl.insert(pxVal);
                #else
                auto insRet = mColl.insert(pxVal);
                if (insRet.second)
                {
                    ISL_LOGI("contour#{} @ {}, collided with {} @ {}, continue", mVal, ptStart, pxVal, ptCurt);
                }
                #endif
                if (!mOth.empty())
                {
                    auto im = std::find(mOth.begin(), mOth.end(), pxVal);
                    if (mOth.end() != im)
                    {
                        //ISL_LOGI("contour#{} @ {}, abort follow @ {} ~ {}, collided with {}", mVal, ptStart, ptCurt, ptCent, pxVal);
                        retVal = -2;
                        goto EXIT;
                    }
                }
            }

            this->boundaryTrace.push_back(ptCurt);
            this->boundarySet.insert(ptCurt);
            if (pxVal != mVal) pxVal = mVal;
            if (ptCurt.y < top) top = ptCurt.y;
            if (ptCurt.y > bottom) bottom = ptCurt.y;
            if (ptCurt.x < left) left = ptCurt.x;
            if (ptCurt.x > right) right = ptCurt.x;
            _swap_pt(ptCurt, ptCent);
            cycleCnt = 8; // 重置循环次数
        }
        else
        {
            pxVal = this->arVal;
        }
    }

EXIT:
    #ifdef FOLLOW_DEBUG
    std::stringstream fn;
    fn << "/mnt/dump/s" << ptStart.x << "-" << ptStart.y << "_t" << get_tick_count() << "_c" << cycleCnt << ".txt";
    std::string&& stmp = ss.str();
    write_file(fn.str(), stmp.data(), stmp.length(), true);
    #endif

    if (retVal >= 0)
    {
        this->boundingBox = Rect<int32_t>(left, top, right-left+1, bottom-top+1);
        this->startPoint = ptStart;
        this->markVal = mVal;
    }
    else
    {
        ISL_LOGE("contour#{} @ {}, abort follow @ {} ~ {}, fgVal {}, boundaryLen {} @ {}, cycleCnt {}, retVal {}", 
                 mVal, ptStart, ptCurt, ptCent, this->fgVal, this->boundaryTrace.size(), this->boundaryTrace.back(), cycleCnt, retVal);
        this->boundaryTrace.clear();
        this->boundarySet.clear();
    }

    return retVal;
}


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
int32_t Contour::Grow(Imat<uint8_t>& binz, const int32_t starty, const std::vector<uint8_t>& marks, Rect<int32_t>& bbox)
{
    if (!binz.isvalid() || this->boundaryTrace.empty())
    {
        return -1;
    }

    auto iStart = this->boundaryTrace.end(), iEnd = this->boundaryTrace.end();
    for (auto it = this->boundaryTrace.begin(); it != this->boundaryTrace.end(); ++it)
    {
        if (it->y == starty)
        {
            iStart = it;
            break;
        }
    }
    if (iStart == this->boundaryTrace.end())
    {
        ISL_LOGE("contour#{} @ {}, cannot locate start point, starty {}", this->markVal, this->startPoint, starty);
        return -1;
    }

    uint8_t *pxPre = binz.ptr(starty, this->boundingBox.x + this->boundingBox.width);
    uint8_t* pxCur = pxPre - 1;
    for (int32_t j = this->boundingBox.posbr().x; j >= this->boundingBox.x; --j, --pxPre, --pxCur)
    {
        if ((*pxPre == this->bgVal || *pxPre == this->arVal) && (*pxCur == this->markVal)) // find the end point
        {
            for (auto it = iStart; it != this->boundaryTrace.end(); ++it)
            {
                if (it->y == starty && it->x == j)
                {
                    iEnd = it;
                    break;
                }
            }
            if (iEnd != this->boundaryTrace.end())
            {
                break;
            }
            else
            {
                ISL_LOGW("contour#{} @ {}, locate end point @ {} failed, try again", 
                         this->markVal, this->startPoint, Point<int32_t>(j, starty));
            }
        }
    }
    if (iEnd == this->boundaryTrace.end())
    {
        ISL_LOGE("contour#{} @ {}, cannot locate end point, starty {}", this->markVal, this->startPoint, starty);
        return -1;
    }

    Contour growing(this->fgVal, this->bgVal, this->arVal);
    std::pair<Point<int32_t>, Point<int32_t>> presuf{
        (iStart != this->boundaryTrace.begin()) ? *std::prev(iStart) : this->boundaryTrace.back(), *iEnd};
    std::unordered_set<uint8_t> collisons;
    auto folRet = growing.Follow(binz, *iStart, this->markVal, marks, collisons, presuf);
    if (folRet > 0)
    {
        for (auto it = iStart; it != iEnd; ++it)
        {
            this->boundarySet.erase(*it);
        }
        auto ipos = this->boundaryTrace.erase(iStart, iEnd); // 先删除Grow区域的原轮廓点
        int32_t top = startPoint.y, bottom = startPoint.y, left = startPoint.x, right = startPoint.x;
        for (auto& elem : this->boundaryTrace)
        {
            if (elem.y < top) top = elem.y;
            if (elem.y > bottom) bottom = elem.y;
            if (elem.x < left) left = elem.x;
            if (elem.x > right) right = elem.x;
        }
        this->boundingBox = Rect<int32_t>(left, top, right-left+1, bottom-top+1); // 更新轮廓包围盒
        this->boundaryTrace.splice(ipos, growing.boundaryTrace, growing.boundaryTrace.begin(), std::prev(growing.boundaryTrace.end()));
        this->boundarySet.merge(growing.boundarySet);
        this->boundingBox.unite(growing.boundingBox);
        bbox = growing.boundingBox;
        #if 0
        std::cout << "after grow: ";
        for (auto elem : this->boundaryTrace)
        {
            std::cout << "(" << elem.x << ", " << elem.y << ") -> ";
        }
        std::cout << std::endl;
        #endif
        if (collisons.size() > 0)
        {
            ISL_LOGD("contour#{} @ {}, grow from {}, presuf {}, collisons {}", 
                     this->markVal, this->startPoint, growing.startPoint, presuf, collisons);
        }
        return this->fgVal;
    }
    else if (folRet == 0)
    {
        auto it = std::find(growing.boundaryTrace.begin(), growing.boundaryTrace.end(), this->startPoint);
        if (it != growing.boundaryTrace.end())
        {
            this->boundaryTrace.clear();
            this->boundaryTrace.splice(this->boundaryTrace.end(), growing.boundaryTrace, it, growing.boundaryTrace.end());
            this->boundaryTrace.splice(this->boundaryTrace.end(), growing.boundaryTrace);
            this->boundarySet.clear();
            this->boundarySet.insert(this->boundaryTrace.begin(), this->boundaryTrace.end());
            this->boundingBox = growing.boundingBox;
            bbox = growing.boundingBox;
            if (collisons.size() > 0)
            {
                ISL_LOGD("contour#{} @ {}, grow from {}, presuf {}, collisons {}", 
                         this->markVal, this->startPoint, growing.startPoint, presuf, collisons);
            }
            return this->fgVal;
        }
        else
        {
            ISL_LOGE("contour#{} @ {}, cannot find start point in growing#{} @ {}", 
                     this->markVal, this->startPoint, growing.markVal, growing.startPoint);
            return -1;
        }
    }
    else if (folRet == -2)
    {
        for (const auto& elem : marks)
        {
            if (collisons.find(elem) != collisons.end())
            {
                return elem;
            }
        }
        return -1; // 查找不到时，直接返回-1
    }
    else
    {
        ISL_LOGE("contour#{} @ {}, grow failed from {} to {}, starty {}", 
                 this->markVal, this->startPoint, presuf.first, presuf.second, starty);
        return -1;
    }
}


/**
 * @fn      Fillin
 * @brief   给当前轮廓对象内部置为mark值
 * 
 * @param   [IN] binz 二值图
 * @param   [IN] region 有输入时，轮廓在该区域内的部分才会被填充，否则填充所有轮廓区域
 */
void Contour::Fillin(Imat<uint8_t>& binz, std::optional<Rect<int32_t>> region)
{
    if (binz.isvalid() && !this->boundaryTrace.empty())
    {
        /// 从下到上从左到右排序轮廓边界点
        std::list<Point<int32_t>> boundaryB2tL2r = region.has_value() ? std::list<Point<int32_t>>() : this->boundaryTrace;
        if (region.has_value())
        {
            const int32_t top = region->postl().y;
            const int32_t bottom = region->posbr().y;
            for (auto &elem : this->boundaryTrace)
            {
                if (top <= elem.y && elem.y <= bottom)
                {
                    boundaryB2tL2r.push_back(elem);
                }
            }
        }
        auto _b2t_l2r = [](const Point<int32_t>& ptA, const Point<int32_t>& ptB) -> bool
        {
            return (ptA.y != ptB.y) ? (ptA.y > ptB.y) : (ptA.x < ptB.x); // bottom -> top, then left -> right
        };
        boundaryB2tL2r.sort(_b2t_l2r);
        boundaryB2tL2r.unique(); // 删除重复边界点

        auto _pxval = [&binz](const int32_t& y, const int32_t& x) -> uint8_t& {return *(binz.ptr(y, x));};
        for (auto iL = boundaryB2tL2r.begin(); iL != boundaryB2tL2r.end(); ++iL)
        {
            if (_pxval(iL->y, iL->x + 1) != this->arVal) // 非单点
            {
                bool bRightLimited = false;
                auto iR = std::next(iL);
                while (iR != boundaryB2tL2r.end() && iR->y == iL->y)
                {
                    if (_pxval(iR->y, iR->x + 1) != this->arVal)
                    {
                        ++iR;
                    }
                    else // 达到单次可连续赋值的最大长度
                    {
                        bRightLimited = true;
                        break;
                    }
                }
                if (!bRightLimited) // 往前倒1
                {
                    --iR;
                    ISL_LOGW("contour#{} @ {}, {} -> {}, limit on the right isnot arVal({})", 
                             this->markVal, this->startPoint, *iL, *iR, this->arVal);
                }

                if (iL != iR)
                {
                    uint8_t* px = binz.ptr(iL->y, iL->x + 1);
                    for (int32_t k = iL->x + 1; k < iR->x; ++k, ++px)
                    {
                        if (*px < this->arVal)
                        {
                            *px = this->markVal;
                        }
                    }
                    iL = iR;
                }
            }
        }
    }

    return;
}


int32_t Contour::SetVal(Imat<uint8_t>& binz, uint8_t val, std::optional<Rect<int32_t>> region)
{
    int32_t cnt = -1;

    if (binz.isvalid() && !this->boundaryTrace.empty())
    {
        Rect<int32_t> markBox = this->boundingBox;
        if (region.has_value()) // 计算region与boundingBox的交集
        {
            markBox = region->intersect(this->boundingBox);
            if (0 == markBox.width || 0 == markBox.height) // 无交集，直接退出
            {
                return 0;
            }
        }

        cnt = 0;
        for (int32_t i = 0, row = markBox.y; i < markBox.height; ++i, ++row)
        {
            uint8_t *px = binz.ptr(row, markBox.x);
            for (int32_t j = 0; j < markBox.width; ++j, ++px)
            {
                if (*px == this->markVal)
                {
                    *px = val;
                    ++cnt;
                }
            }
        }
    }

    return cnt;
}


/**
 * @fn      Remark
 * @brief   对轮廓区域重新Mark，新的Mark值为mval 
 * @note    该操作必须在Fillin之后，接口内部是根据旧的Mark值来识别并更新的 
 * 
 * @param   [IN] binz 二值图
 * @param   [IN] mval 新的mark值
 * @param   [IN] region 有输入时，轮廓在该区域内的部分才会被重新mark，否则更新所有轮廓区域
 */
void Contour::Remark(Imat<uint8_t>& binz, uint8_t mval, std::optional<Rect<int32_t>> region)
{
    this->SetVal(binz, mval, region);
    return;
}


/**
 * @fn      Clear
 * @brief   将轮廓区域图像清除为背景
 * @note    该操作必须在Fillin之后，接口内部是根据旧的Mark值来识别并清除的 
 * 
 * @param   [IN] binz 二值图
 * @param   [IN] region 有输入时，轮廓在该区域内的部分才会被清除，否则清除所有轮廓区域
 */
void Contour::Clear(Imat<uint8_t>& binz, std::optional<Rect<int32_t>> region)
{
    this->SetVal(binz, this->bgVal, region);
    return;
}


void Contour::SmoothShell(Imat<uint16_t>& shell, Imat<uint8_t>& binz, std::optional<Rect<int32_t>> region)
{
    if (binz.isvalid() && !this->boundaryTrace.empty())
    {
        const int32_t top = region.has_value() ? region->postl().y : 0;
        const int32_t bottom = region.has_value() ? region->posbr().y : binz.height()-1;
        std::vector<std::tuple<Point<int32_t>, Point<int32_t>, uint8_t>> collisons;
        auto _dilate3x3 = [this, &shell, &binz, &collisons](Point<int32_t>& center, int32_t shlvl, std::vector<Point<int32_t>>* shPot) -> auto {
            for (int32_t i = -1; i <= 1; ++i)
            {
                uint8_t* src = binz.ptr(center.y + i, center.x - 1);
                uint16_t* dst = shell.ptr(center.y + i, center.x - 1);
                for (int32_t j = -1; j <= 1; ++j, ++src, ++dst)
                {
                    if (*src == this->markVal)
                    {
                        *dst = *src;
                    }
                    else if (*src == this->bgVal)
                    {
                        *dst = (shlvl << 8) | this->markVal;
                        if (nullptr != shPot)
                        {
                            shPot->push_back(Point<int32_t>(center.x + j, center.y + i));
                        }
                    }
                    else if (*src != this->fgVal) // 与其他轮廓发生碰撞
                    {
                        collisons.push_back(std::make_tuple(center, Point<int32_t>(center.x + j, center.y + i), *src));
                    }
                }
            }
        };

        std::vector<Point<int32_t>> shellPoints;
        for (auto it : this->boundaryTrace)
        {
            if (top <= it.y && it.y <= bottom)
            {
                _dilate3x3(it, 1, &shellPoints);
            }
        }
        for (auto it : shellPoints)
        {
            _dilate3x3(it, 2, nullptr);
        }
        for (auto it : collisons)
        {
            ISL_LOGI("center: {}, other: {}, {}", std::get<0>(it), std::get<1>(it), std::get<2>(it));
        }
    }

    return;
}


/**
 * @fn      Move
 * @brief   移动轮廓
 * 
 * @param   [IN] ver 垂直移动行数，负数为向上移动，正数为向下移动
 * @param   [IN] hor 水平移动列数，负数为向左移动，正数为向右移动
 */
void Contour::Move(int32_t ver, int32_t hor)
{
    if (!this->boundaryTrace.empty())
    {
        if (ver != 0)
        {
            this->startPoint.y += ver;
            this->boundingBox.y += ver;
            for (auto& elem : this->boundaryTrace)
            {
                elem.y += ver;
            }
        }
        if (hor != 0)
        {
            this->startPoint.x += hor;
            this->boundingBox.x += hor;
            for (auto& elem : this->boundaryTrace)
            {
                elem.x += hor;
            }
        }

        this->boundarySet.clear();
        this->boundarySet.insert(this->boundaryTrace.begin(), this->boundaryTrace.end());
    }

    return;
}


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
std::pair<size_t, size_t> Contour::CoverageRate(Contour& that, std::vector<Point<int32_t>>& compSet, std::optional<Rect<int32_t>> effect)
{
    std::unordered_set<Point<int32_t>, HashPoint> effSet;
    if (effect.has_value())
    {
        const int32_t yTop = effect->y, yBot = effect->y + effect->height; // [yTop, yBot)
        std::copy_if(that.boundarySet.begin(), that.boundarySet.end(), std::inserter(effSet, effSet.begin()), 
                     [&yTop, &yBot](Point<int32_t> elem) { return (elem.y >= yTop && elem.y < yBot); });
    }

    auto& subSet = effect.has_value() ? effSet : that.boundarySet;
    size_t total = subSet.size(), hitted = 0;

    for (const auto& elem : subSet)
    {
        if (this->boundarySet.find(elem) == this->boundarySet.end())
        {
            compSet.push_back(elem);
        }
        else
        {
            hitted++;
        }
    }

    return std::make_pair(total, hitted);
}


/**
 * @fn      ResetProf
 * @brief   重置Fbg对象属性及其内存
 * 
 * @param   [IN] maxSize 需支持的最大轮廓尺寸
 * 
 * @return  int32_t 0：正常，-1：内存申请失败
 */
int32_t Fbg::ResetProf(std::optional<Size<int32_t>> maxSize)
{
    if (maxSize.has_value() && maxSize.value() != this->prof_smax)
    {
        int32_t preArea = this->prof_smax.area(); // 记录旧区域大小
        this->prof_smax = maxSize.value();
        this->prof_msize.width = this->prof_smax.width + 2 * pads_around;
        this->prof_msize.height = this->prof_smax.height + 2 * pads_around;
        if (this->prof_smax.area() > preArea) // 所需内存变大，重新申请内存
        {
            this->prof_mem = std::make_unique<uint8_t[]>(prof_smax.area() * 3 + prof_msize.area());
            if (!this->prof_mem)
            {
                ISL_LOGE("make unique_ptr failed, memory size: {}", prof_smax.area() + prof_msize.area());
                return -1;
            }
        }

        profDs = Imat<uint8_t>(prof_smax.height, prof_smax.width, 1, prof_mem.get(), (size_t)prof_smax.area());
        profEdge = Imat<uint8_t>(profDs, profDs.ptr()+profDs.get_memsize(), (size_t)prof_smax.area());
        profTh = Imat<uint8_t>(profDs, profEdge.ptr()+profEdge.get_memsize(), (size_t)prof_smax.area());
        profBinz = Imat<uint8_t>(prof_msize.height, prof_msize.width, 1, profTh.ptr()+profTh.get_memsize(), (size_t)prof_msize.area());
    }

    /// 重置profDs与profBinz的有效区域高度为0
    profDs(Rect<int32_t>(0, 0, prof_smax.width, 0));
    profEdge(Rect<int32_t>(0, 0, prof_smax.width, 0));
    profTh(Rect<int32_t>(0, 0, prof_smax.width, 0));
    profBinz(Rect<int32_t>(pads_around, pads_around, prof_smax.width, 0));
    profBinz.fill(this->bgVal);
    this->fgContours.clear();
    this->fgSliceOut.clear();

    return 0;
}

auto _get_idle_mval = [](std::vector<uint8_t>& marks, uint8_t mMin, uint8_t mMax) -> uint8_t
{
    if (!marks.empty())
    {
        auto maxe = std::max_element(marks.begin(), marks.end());
        if (*maxe < mMax)
        {
            return std::max(*maxe + 1, static_cast<int32_t>(mMin));
        }
        else
        {
            for (uint8_t i = mMin; i <= mMax; ++i)
            {
                auto it = std::find(marks.begin(), marks.end(), i);
                if (it == marks.end()) // 未找到，则为未使用
                {
                    return i;
                }
            }
            return 0; // 异常，直接返回0
        }
    }
    else
    {
        return mMin;
    }
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
int32_t Fbg::SearchContours(std::list<Contour>& contours, Imat<uint8_t>& binz, const std::optional<std::vector<std::pair<uint8_t, int32_t>>>& mark_conf)
{
    if (!binz.isvalid())
    {
        return -1;
    }

    std::vector<uint8_t> marks;
    for (auto it = contours.begin(); it != contours.end(); ++it)
    {
        marks.push_back(it->MarkVal());
    }

    /// 在区域中寻找所有轮廓
    Rect<int32_t> searchWin = binz.pwin();
    int32_t searchStartX = subcf0(searchWin.x, 1);
    int32_t contNum = 0;
    for (int32_t i = 0, row = searchWin.y; i < searchWin.height; ++i, ++row)
    {
        uint8_t* pxPre = binz.ptr(row, searchStartX);
        uint8_t* pxCur = pxPre + 1;
        for (int32_t j = 0, col = searchStartX+1; j < searchWin.width; ++j, ++col, ++pxPre, ++pxCur)
        {
            if ((*pxPre == this->bgVal || *pxPre == this->arVal) && (*pxCur == this->fgVal)) // find the contour start point
            {
                Contour contour(this->fgVal, this->bgVal, this->arVal);
                std::unordered_set<uint8_t> collisons;
                auto folRet = contour.Follow(binz, Point<int32_t>(col, row), _get_idle_mval(marks, this->fgVal+1, this->arVal-1), 
                                             std::vector<uint8_t>(), collisons);
                if (folRet > 0)
                {
                    ISL_LOGD("search in {}, contour#{} @ {}, bbox {}, len {}, collisons {}", searchWin, contour.MarkVal(), 
                             contour.StartPoint(), contour.BoundingBox(), folRet, collisons);
                    contour.Fillin(binz);
                    if (collisons.size() > 0 && mark_conf.has_value())
                    {
                        int32_t confMax = -1; // 最大置信度
                        uint8_t comark = this->bgVal; // 最大置信度对应的轮廓mark值
                        for (auto it = collisons.begin(); it != collisons.end(); ++it)
                        {
                            for (auto& elem : mark_conf.value()) // 从mark_conf中寻找最大的置信度
                            {
                                if (elem.first == *it)
                                {
                                    if (elem.second > confMax)
                                    {
                                        confMax = elem.second;
                                        comark = elem.first;
                                    }
                                    break;
                                }
                            }
                        }
                        if (confMax >= 0)
                        {
                            ISL_LOGI("search in {}, contour#{} @ {}, bbox {}, inherit attr from {} $ {}", searchWin, contour.MarkVal(), 
                                     contour.StartPoint(), contour.BoundingBox(), comark, confMax);
                            contour.setPrivate<ContAttr>(ContAttr({confMax}));
                        }
                    }
                    marks.push_back(contour.MarkVal());
                    contours.push_back(std::move(contour));
                    ++contNum;
                }
                else
                {
                    //std::string fn_profsh = fmt::format("/mnt/dump/{}", this->procCnt);
                    //binz.dump(fn_profsh, true, Rect<int32_t>(0, 0, binz.width(), binz.pwin().y+binz.pwin().height+this->pads_around));
                    ISL_LOGW("contour#{} @ {}: follow failed, pwin {}", contour.MarkVal(), contour.StartPoint(), binz.pwin());
                }
            }
        }
    }

    return contNum;
}


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
int32_t Fbg::FissionContour(std::list<Contour>& contours, Imat<uint8_t>& binz, Contour& disCont, const Rect<int32_t>& disRegion)
{
    Rect<int32_t> clearWin = disRegion.intersect(disCont.BoundingBox());
    if (!binz.isvalid() || clearWin == Rect<int32_t>())
    {
        return -1;
    }
    if (clearWin.width != disCont.BoundingBox().width) // 暂不支持将轮廓区域左右分裂
    {
        ISL_LOGE("contour#{} @ {} ~ {}, unsupport divide horizontally by {}",
                 disCont.MarkVal(), disCont.StartPoint(), disCont.BoundingBox(), disRegion);
        return -1;
    }

    disCont.Clear(binz, clearWin); // 将disRegion区域内disCont轮廓清除为背景
    if (clearWin.height == disCont.BoundingBox().height)
    {
        return 0;
    }

    std::vector<Rect<int32_t>> searchWin;
    if (disRegion.y > disCont.BoundingBox().y)
    {
        searchWin.push_back(Rect<int32_t>(disCont.BoundingBox().x, disCont.BoundingBox().y, 
                                          disCont.BoundingBox().width, disRegion.y - disCont.BoundingBox().y));
    }
    if (disRegion.posbr().y < disCont.BoundingBox().posbr().y)
    {
        searchWin.push_back(Rect<int32_t>(disCont.BoundingBox().x, disRegion.posbr().y + 1,
                                          disCont.BoundingBox().width, disCont.BoundingBox().posbr().y - disRegion.posbr().y));
    }

    std::vector<uint8_t> marks;
    for (auto it = contours.begin(); it != contours.end(); ++it)
    {
        if (it->MarkVal() != disCont.MarkVal())
        {
            marks.push_back(it->MarkVal());
        }
    }

    // 在disCont轮廓内分裂出新轮廓
    int32_t contNum = 0;
    for (auto& win : searchWin)
    {
        int32_t startX = subcf0(win.x, 1);
        for (int32_t i = 0, row = win.y; i < win.height; ++i, ++row)
        {
            uint8_t* pxPre = binz.ptr(row, startX);
            uint8_t* pxCur = pxPre + 1;
            for (int32_t j = 0, col = startX+1; j < win.width; ++j, ++col, ++pxPre, ++pxCur)
            {
                if ((*pxPre == this->bgVal || *pxPre == this->arVal) && (*pxCur == disCont.MarkVal())) // find the contour start point
                {
                    Contour contour(disCont.MarkVal(), this->bgVal, this->arVal);
                    std::unordered_set<uint8_t> collisons;
                    auto folRet = contour.Follow(binz, Point<int32_t>(col, row), _get_idle_mval(marks, disCont.MarkVal()+1, this->arVal-1), marks, collisons);
                    if (folRet > 0)
                    {
                        ISL_LOGD("fission in #{} @ {} ~ {}, new contour#{} @ {} ~ {}, collisons {}", disCont.MarkVal(), disCont.StartPoint(), 
                                 win, contour.MarkVal(), contour.StartPoint(), contour.BoundingBox(), collisons);
                        contour.Fillin(binz);
                        contour.SetFg(this->fgVal);
                        if (disCont.hasPrivate<ContAttr>())
                        {
                            contour.setPrivate<ContAttr>(ContAttr(*(disCont.getPrivate<ContAttr>()))); // 继承私有数据
                        }
                        marks.push_back(contour.MarkVal());
                        contours.push_back(std::move(contour));
                        ++contNum;
                    }
                    else
                    {
                        ISL_LOGW("contour#{} @ {}: follow failed, pwin {}", contour.MarkVal(), contour.StartPoint(), binz.pwin());
                    }
                }
            }
        }
    }

    return contNum;
}


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
int32_t Fbg::MaterObjConf(Contour& fgCont)
{
    if (fgCont.BoundingBox().height > this->cont_smax.height)
    {
        ISL_LOGW("Unsupport this contour#{} @ {} ~ {}, > 2 slices height({})", 
                 fgCont.MarkVal(), fgCont.StartPoint(), fgCont.BoundingBox(), this->cont_smax.height);
        return -1;
    }

    const int32_t acnodeArea = 9; // 单个中间像素Dilate即为9
    const uint8_t u8BgTh = _compress_16to8(this->bg_lum_th);
    const uint8_t u8FgTh = _compress_16to8(this->fg_lum_th);

    /// 统计低于bg_lum_th轮廓区域的面积与低于fg_lum_th的像素个数
    Imat<uint8_t> imgGray, imgBinz;
    Rect<int32_t> contBox = fgCont.BoundingBox();
    if (0 != this->profBinz.cropto(imgBinz, contBox))
    {
        ISL_LOGE("profBinz cropto imgBinz failed, profBinz: {}, roi: {}", this->profTh, contBox);
        return -1;
    }
    contBox.x = subcf0(contBox.x, this->pads_around);
    contBox.y = subcf0(contBox.y, this->pads_around);
    if (0 != this->profDs.cropto(imgGray, contBox))
    {
        ISL_LOGE("profDs cropto imgGray failed, profDs: {}, roi: {}", this->profDs, contBox);
        return -1;
    }

    std::function<bool(const uint8_t, const uint8_t)> isLtBg = // is less than background，fgCont.MarkVal()标记的像素都是
        [&fgCont](const uint8_t, const uint8_t binz) -> bool {return (binz == fgCont.MarkVal());};
    hist8_t objHist;
    int32_t objArea = Statistic(imgGray, imgBinz, isLtBg, nullptr, nullptr, &objHist, nullptr);
    if (objArea <= 0)
    {
        ISL_LOGE("Statistic this contour#{} @ {} ~ {} failed, Img: {}, Mask: {}", 
                 fgCont.MarkVal(), fgCont.StartPoint(), fgCont.BoundingBox(), profDs, profBinz);
        return -1;
    }
    else if (objArea <= this->fgAreaMin) // 极小区域直接过滤
    {
        ISL_LOGD("this contour#{} @ {} is too small: {} <= {}", 
                 fgCont.MarkVal(), fgCont.StartPoint(), objArea, this->fgAreaMin);
        return 0;
    }
    else
    {
        int32_t betweenFgBg = 0;
        for (int32_t i = u8FgTh; i <= u8BgTh; ++i)
        {
            betweenFgBg += objHist[i];
        }
        if (objArea - betweenFgBg <= acnodeArea)
        {
            ISL_LOGD("Most region of this contour#{} @ {} is the ultra-thin zone: {} / {}", 
                     fgCont.MarkVal(), fgCont.StartPoint(), betweenFgBg, objArea);
            return 0;
        }
    }

    /// 计算高置信度实物区
    Rect<int32_t>&& srcRegion = Rect<int32_t>(fgCont.BoundingBox().x - this->pads_around, fgCont.BoundingBox().y - this->pads_around, 
                                              fgCont.BoundingBox().width, fgCont.BoundingBox().height);
    Rect<int32_t>&& dstRegion = Rect<int32_t>(0, 0, srcRegion.width + 2 * pads_around, srcRegion.height + 2 * pads_around);
    Imat<uint8_t> imgSrc{srcRegion.height, srcRegion.width, 1, profDs.ptr(srcRegion.y, srcRegion.x), 
        profDs.get_stride() * srcRegion.height, {0, 0}, {0, 0}, profDs.get_stride()};
    Imat<uint8_t> imgErode{srcRegion.height, srcRegion.width, 1, (uint8_t*)cont_mem.get(), (size_t)srcRegion.size().area()};
    imgBinz = Imat<uint8_t>{dstRegion.height, dstRegion.width, 1, imgErode.ptr() + imgErode.get_memsize(), 
        (size_t)dstRegion.size().area(), {pads_around, pads_around}, {pads_around, pads_around}};
    imgBinz.fill(this->bgVal); // 为了填充周围padding区域，直接将整个区域都填充了，简单处理

    if (this->bNarrowFgEn)
    {
        for (int32_t i = 0; i < srcRegion.height; ++i)
        {
            uint8_t* pDs = profDs.ptr(srcRegion.y + i, srcRegion.x);
            uint8_t* pEdge = profEdge.ptr(srcRegion.y + i, srcRegion.x);
            uint8_t* pBinz = imgBinz.ptr(pads_around + i, pads_around);
            for (int32_t j = 0; j < srcRegion.width; ++j, ++pDs, ++pEdge, ++pBinz)
            {
                if (*pDs < u8FgTh || *pEdge)
                {
                    *pBinz = this->fgVal;
                }
            }
        }
    }

    std::function<bool(const uint8_t)> isOpen = [this](const uint8_t src) -> bool {return (src == this->fgVal);};
    std::function<bool(const uint8_t)> isEroding = [&u8FgTh](const uint8_t src) -> bool {return (src < u8FgTh);};
    if (0 == Filter::Erode(imgErode, this->bNarrowFgEn ? imgBinz : imgSrc, 1, this->bNarrowFgEn ? isOpen : isEroding, 
                           {this->bgVal, this->fgVal})) // 腐蚀后，背景为255，前景为0
    {
        if (0 != Filter::Dilate(imgBinz, imgErode, 1, isOpen, {this->bgVal, this->fgVal}))
        {
            ISL_LOGE("Dilate failed, imgErode: {}, imgBinz: {}", imgErode, imgBinz);
            return -1;
        }
    }
    else
    {
        ISL_LOGE("Erode failed, imgSrc: {}, imgErode: {}", this->bNarrowFgEn ? imgBinz : imgSrc, imgErode);
        return -1;
    }

    std::list<Contour> contours;
    if (SearchContours(contours, imgBinz) > 0)
    {
        std::function<bool(const uint8_t, const uint8_t)> unconditional = [](const uint8_t, const uint8_t) -> bool {return true;};
        if (Statistic(imgBinz, imgBinz, unconditional, nullptr, nullptr, &objHist, nullptr) <= 0)
        {
            return -1;
        }
        uint32_t maxArea = 0, maxIdx = 0;
        for (int32_t i = this->fgVal + 1; i < this->arVal; ++i)
        {
            if (objHist.at(i) > maxArea)
            {
                maxArea = objHist.at(i);
                maxIdx = i;
            }
        }
        auto iBiggest = contours.begin();
        for (; iBiggest != contours.end(); ++iBiggest)
        {
            if (iBiggest->MarkVal() == maxIdx)
            {
                break;
            }
        }
        if (iBiggest == contours.end())
        {
            return -1;
        }

        if (maxArea <= acnodeArea) // 排除孤点情况
        {
            ISL_LOGD("the biggest material object#{} @ {} of this contour#{} @ {} is too small: {}",
                     iBiggest->MarkVal(), iBiggest->StartPoint(), fgCont.MarkVal(), fgCont.StartPoint(), maxArea);
            return 0;
        }
        else
        {
            const float64_t areaCoef = 1.0; // (0, 2]，越大置信度越高
            const float64_t grayCoef = 0.2;
            float64_t areaWt = 0.0, grayWt = 0.0;

            if (maxArea > static_cast<uint32_t>(this->fgAreaMin))
            {
                float64_t alpha = subcf0(0.5, std::log10(1.0 + areaCoef));
                areaWt = std::log10(1.0 + areaCoef * maxArea / this->fgAreaMin) + alpha;
            }
            else
            {
                float64_t beta = 1.0 - static_cast<float64_t>(acnodeArea) / this->fgAreaMin;
                beta = 0.5 / (beta * beta * beta);
                areaWt = static_cast<float64_t>(maxArea - acnodeArea) / this->fgAreaMin;
                areaWt = areaWt * areaWt * areaWt * beta;
            }

            std::function<bool(const uint8_t, const uint8_t)> isLtFg = // is less than frontground，biggest.MarkVal()标记的像素都是
                [&maxIdx](const uint8_t, const uint8_t binz) -> bool {return (binz == maxIdx);};
            uint8_t objGray = 0;
            if (Statistic(imgSrc, imgBinz, isLtFg, nullptr, nullptr, nullptr, &objGray) > 0)
            {
                grayWt = std::log(1.0 + grayCoef * subcf0(u8FgTh, objGray)) + 1.0;
                ISL_LOGD("confidence of the biggest material object#{} @ {} ~ {} in contour#{} @ {} ~ {}: {} * {} = {}",
                         iBiggest->MarkVal(), iBiggest->StartPoint(), iBiggest->BoundingBox(), 
                         fgCont.MarkVal(), fgCont.StartPoint(), fgCont.BoundingBox(), areaWt, grayWt, areaWt * grayWt);\
                return static_cast<int32_t>(std::min(areaWt * grayWt, 1.0) * 100); // 归一化到0~100
            }
            else
            {
                ISL_LOGE("Statistic the biggest material object#{} @ {} ~ {} in contour#{} @ {} ~ {} failed, Img: {}, Mask: {}", 
                         iBiggest->MarkVal(), iBiggest->StartPoint(), iBiggest->BoundingBox(), 
                         fgCont.MarkVal(), fgCont.StartPoint(), fgCont.BoundingBox(), imgSrc, imgBinz);
                return -1;
            }
        }
    }
    else
    {
        ISL_LOGD("there's no material object in this contour#{} @ {}", fgCont.MarkVal(), fgCont.StartPoint());
        return 0;
    }
}


/**
 * @fn      SetBgLumTh
 * @brief   设置背景亮度阈值
 * 
 * @param   [IN] bgTh 背景亮度阈值，不小于55000
 */
void Fbg::SetBgLumTh(const uint16_t bgTh)
{
    this->bgThUsrSet = std::max(bgTh, static_cast<uint16_t>(55000));
    return;
}


/**
 * @fn      SetFgSensitivity
 * @brief   设置前景检测灵敏度
 * 
 * @param   [IN] sensitivity 前景检测灵敏度，范围：[0, 100]
 */
void Fbg::SetFgSensitivity(const int32_t sensitivity)
{
    this->fgSensitivity.second = std::clamp(sensitivity, 0, 100);
    return;
}


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
std::vector<Fbg::Fg4Slice> Fbg::FbgCull(Imat<uint8_t>& fgMask, Imat<uint16_t>& imgIn, Imat<uint16_t>& imgTmp, bool bRecache)
{
    std::vector<Fg4Slice> sliceResults;
    this->procCnt++;

    if (!(fgMask.isvalid() && imgIn.isvalid() && imgTmp.isvalid()))
    {
        ISL_LOGE("fgMask({}) or imgIn({}) or imgTmp({}) is invalid", fgMask, imgIn, imgTmp);
        return sliceResults;
    }
    if (fgMask.pwin().size() != imgIn.pwin().size() || fgMask.get_vpads().first < fgMask.pwin().height)
    {
        ISL_LOGE("fgMask size is invalid: {}, imgIn: {}", fgMask, imgIn);
        return sliceResults;
    }
    if (imgTmp.get_memsize() < imgIn.pwin().size().area() * imgIn.elemsize())
    {
        ISL_LOGE("imgTmp memory size({}) is too small, at least {}", imgTmp.get_memsize(), imgIn.pwin().size().area() * imgIn.elemsize());
        return sliceResults;
    }

    /// 更新外部设置的参数
    if (this->bg_lum_th != this->bgThUsrSet)
    {
        this->bg_lum_th = this->bgThUsrSet;
    }
    if (this->fgSensitivity.first != this->fgSensitivity.second)
    {
        this->fgSensitivity.first = this->fgSensitivity.second;
        this->fgAreaMin = 16 + ((this->fgSensitivity.first < 50) ? (50 - this->fgSensitivity.first) / 3 : 0);
        this->bNarrowFgEn = (this->fgSensitivity.first < 70) ? false : true;
    }
    this->fg_lum_th = this->bg_lum_th - 100 - 35 * (100 - this->fgSensitivity.first);

    /// 初始化下采样尺寸
    Size<int32_t> dsSize{static_cast<int32_t>(imgIn.pwin().width / prof_dscale.width), 
                         static_cast<int32_t>(imgIn.pwin().height / prof_dscale.height)};
    if (dsSize.width != this->prof_smax.width || dsSize.width != profDs.pwin().width) // 后面那个条件是为了内存申请失败时下次可再次申请
    {
        ISL_LOGI("RESET!! Horizontal downscaled size is changing, {} -> {}", this->prof_smax.width, dsSize.width);
        if (0 != this->ResetProf(Size<int32_t>{dsSize.width, this->prof_smax.height}))
        {
            return sliceResults;
        }
    }
    if (dsSize.width != this->cont_smax.width || dsSize.height * 2 > this->cont_smax.height)
    {
        this->cont_msize.width = dsSize.width + 2 * pads_around;
        this->cont_msize.height = dsSize.height * 2 + 2 * pads_around;
        int32_t memNeed = dsSize.area() * 2;
        if (memNeed > this->cont_smax.area()) // 内存大小不够了，需重新申请
        {
            memNeed += this->cont_msize.area();
            this->cont_mem = std::make_unique<uint8_t[]>(memNeed);
            if (!this->cont_mem)
            {
                ISL_LOGE("make unique_ptr failed, memory size: {}", memNeed);
                return sliceResults;
            }
        }
        this->cont_smax.width = dsSize.width;
        this->cont_smax.height = dsSize.height * 2;
        #if 0
        this->contTh = Imat<uint8_t>(cont_smax.height, cont_smax.width, 1, cont_mem.get(), (size_t)cont_smax.area());
        this->contBinz = Imat<uint8_t>(cont_msize.height, cont_msize.width, 1, contTh.ptr()+contTh.get_memsize(), (size_t)cont_msize.area());
        #endif
    }

    /// 下采样尺寸变化后，更新缩放表
    if (dsSize.width != static_cast<int32_t>(dsLutHor.size()))
    {
        ISL_LOGI("Downscaled width is changed, {} -> {}, rebuild horizontal resizing LUT", dsLutHor.size(), dsSize.width);
        buildDownSizingAvgLut(dsLutHor, imgIn.pwin().width, dsSize.width);
        buildResizingLut(usLutHor, 1, dsSize.width, imgIn.pwin().width);
    }
    if (dsSize.height != static_cast<int32_t>(dsLutVer.size()))
    {
        ISL_LOGI("Downscaled height is changed, {} -> {}, rebuild vertical resizing LUT", dsLutVer.size(), dsSize.height);
        buildDownSizingAvgLut(dsLutVer, imgIn.pwin().height, dsSize.height);
        buildResizingLut(usLutVer, 1, dsSize.height, imgIn.pwin().height);
    }

    /// 重置后的初始条带，即XRAY_LIB_RTCACHING状态，内部同步重置
    if (bRecache)
    {
        ISL_LOGI("RESET!! Slice is caching in RT-Preview mode, pwin: {}", imgIn);
        this->ResetProf();
    }

    if (dsSize.height + profDs.pwin().height > profDs.height()) // Buffer满
    {
        /// 更新profBinz与profDs
        const int32_t reserveNum = (fgSliceOut.size() > 1) ? static_cast<int32_t>(std::ceil(32.0 / dsSize.height)) : 1;
        const int32_t reserveHei = reserveNum * dsSize.height; // 预留高度，与条带高度对齐
        const int32_t displace = subcf0(profDs.pwin().height, reserveHei);
        Rect<int32_t> srcRegion(profBinz.pwin().x, profBinz.pwin().y+displace, profBinz.pwin().width, reserveHei);
        profBinz.move_vpads({0, displace});
        profBinz.copyto(profBinz, srcRegion, profBinz.pwin());
        Rect<int32_t> resetRegion(0, 0, profBinz.width(), this->pads_around);
        profBinz.fill(this->bgVal, resetRegion);
        resetRegion.y = profBinz.pwin().y + profBinz.pwin().height;
        resetRegion.height += displace;
        profBinz.fill(this->bgVal, resetRegion);
        srcRegion.x -= this->pads_around, srcRegion.y -= this->pads_around;
        profDs.move_vpads({0, displace});
        profDs.copyto(profDs, srcRegion, profDs.pwin());
        profEdge.move_vpads({0, displace});
        profEdge.copyto(profEdge, srcRegion, profEdge.pwin());
        profTh.move_vpads({0, displace});
        profTh.copyto(profTh, srcRegion, profTh.pwin());

        if (fgSliceOut.size() > 1) // 更新fgContours与fgSliceOut
        {
            resetRegion.y = 0, resetRegion.height = profBinz.pwin().y+profBinz.pwin().height;
            profBinz.map([this](uint8_t &val) {val = (val < this->arVal) ? this->fgVal : this->bgVal;}, &resetRegion); // 重置为二值图
            std::list<Contour> contours;
            if (SearchContours(contours, profBinz) > 0) // 重新搜索
            {
                for (auto& elem : this->fgContours)
                {
                    elem.Move(-displace);
                }
                for (auto it = contours.begin(); it != contours.end();)
                {
                    for (auto& elem : this->fgContours)
                    {
                        std::vector<Point<int32_t>> outside;
                        std::pair<size_t, size_t> rate = elem.CoverageRate(*it, outside);
                        if (rate.second > 0) // 全包含或部分包含
                        {
                            // 正常情况，outside中的点是将轮廓一分为二后暴露出来的轮廓段，这里省略二次跟踪了，认为是Coverage的
                            if (outside.size() > 0)
                            {
                                ISL_LOGD("contour#{} @ {} contains #{} @ {}, {}", elem.MarkVal(), 
                                         elem.StartPoint(), it->MarkVal(), it->StartPoint(), outside);
                            }
                            it->setPrivate<ContAttr>(ContAttr(*(elem.getPrivate<ContAttr>()))); // 转移私有数据
                            break;
                        }
                    }
                    ISL_LOGI("buffer is being full, {} contour#{} @ {} ~ {}, conf {}", 
                             it->hasPrivate<ContAttr>() ? "reserve" : "delete",
                             it->MarkVal(), it->StartPoint(), it->BoundingBox(), 
                             it->hasPrivate<ContAttr>() ? (it->getPrivate<ContAttr>())->fgConf : -1);
                    it = it->hasPrivate<ContAttr>() ? std::next(it) : contours.erase(it);
                }
                this->fgContours = contours;
                fgSliceOut.erase(fgSliceOut.begin(), std::prev(fgSliceOut.end(), reserveNum));
            }
            else
            {
                this->fgContours.clear();
                fgSliceOut.erase(fgSliceOut.begin(), std::prev(fgSliceOut.end()));
            }
        }
        else
        {
            for (auto& elem : this->fgContours)
            {
                elem.Move(-displace);
            }
        }
    }

    /// 输入低能RAW图下采样到8Bit灰度图
    /// _u16_2_u8采用非线性映射，中间亮度区被压缩，低、高区向右移6Bit，中区向右移10Bit
    /// 亮度从低段到高段输入记为a、b、65535，输出记为x、y、255，则：x=a/64，b=a+52428
    const uint8_t u8BgTh = _compress_16to8(this->bg_lum_th);
    profDs.move_vpads({profDs.pwin().height, -dsSize.height});
    Imat<uint16_t> imgTmpDs(imgIn.pwin().height, imgIn.pwin().width, 1, imgTmp.ptr(), imgTmp.get_memsize());
    if (0 != imgAnyScale(profDs, imgIn, &imgTmpDs, &dsLutHor, &dsLutVer, _compress_16to8))
    {
        ISL_LOGE("imgAnyScale failed, profDs: {}, imgIn: {}, imgTmpDs: {}, dsLutHor: {}, dsLutVer: {}", 
                 profDs, imgIn, imgTmpDs, dsLutHor.size(), dsLutVer.size());
        return sliceResults;
    }

    profEdge.move_vpads({profEdge.pwin().height, -dsSize.height});
    Imat<uint8_t> imgDs(profDs.pwin().y+profDs.pwin().height, profDs.width(), 1, profDs.ptr(), profDs.get_memsize(), {profDs.pwin().y, 0}); // 下邻域置0
    Imat<uint8_t> imgEdge = profEdge;
    imgDs.move_vpads({-1, 0});
    imgEdge.move_vpads({-1, 0}); // 修复上条带的最后一行，Sobel邻域只有1 Pixel
    Imat<uint8_t> imgTmpE(imgEdge.pwin().height, imgEdge.pwin().width, 1, (uint8_t*)imgTmp.ptr(), imgEdge.pwin().size().area());
    if (0 != Nree::EdgeDetectSobel(imgEdge, imgDs, imgTmpE, std::make_pair<uint8_t, uint8_t>(25, 127), 
                                   std::pair{_compress_16to8(this->fg_lum_th), static_cast<uint8_t>(255)})) // 这里只计算非前景区的边缘
    {
        ISL_LOGE("detect edge in profDs failed, imgDs: {}, imgEdge: {}", imgDs, imgEdge);
        return sliceResults;
    }

    /// 加强边缘区域的轮廓，防止细边被腐蚀后消除
    profTh.move_vpads({profTh.pwin().height, -dsSize.height});
    Imat<uint8_t> imgTh = profTh;
    imgTh.move_vpads({-1, 0});
    for (int32_t i = 0; i < imgTh.pwin().height; ++i)
    {
        uint8_t* pDs = imgDs.ptr(imgDs.pwin().y+i, imgDs.pwin().x);
        uint8_t* pEdge = imgEdge.ptr(imgEdge.pwin().y+i, imgEdge.pwin().x);
        uint8_t* pTh = imgTh.ptr(imgTh.pwin().y+i, imgTh.pwin().x);
        for (int32_t j = 0; j < imgTh.pwin().width; ++j, ++pDs, ++pEdge, ++pTh)
        {
            *pTh = (*pDs < u8BgTh || *pEdge) ? 0 : 255;
        }
    }

    /// 从profTh中截取需处理的阈值图
    const int32_t meanRadius = 3, openRadius = 1;
    const pads_t& vdiff = profTh.move_vpads({-2 * (meanRadius + openRadius), 0}); // 上一个条带中，需要有下邻域的区域，未精确计算，现重新处理
    Imat<uint8_t> imgBTh;
    if (0 != profTh.cropto(imgBTh, profTh.pwin()))
    {
        ISL_LOGE("profTh cropto imgBTh failed, profTh: {}", profTh);
        return sliceResults;
    }

    const int32_t meanHeight = std::min(-vdiff.first, meanRadius + openRadius * 2) + dsSize.height;
    const int32_t openHeight = std::min(-vdiff.first, meanRadius + openRadius) + dsSize.height;
    imgBTh(Rect<int32_t>(imgBTh.pwin().x, imgBTh.height() - meanHeight, imgBTh.pwin().width, meanHeight));
    if (imgBTh.isize().area() + imgBTh.pwin().size().area() > static_cast<int32_t>(imgTmp.get_memsize()))
    {
        ISL_LOGE("imgTmp out of memory, imgTmp: {}, request: {}", imgTmp, imgBTh.isize().area() + imgBTh.pwin().size().area());
        return sliceResults;
    }
    Imat<uint8_t> imgBTmp{imgBTh, (uint8_t*)imgTmp.ptr(), static_cast<size_t>(imgBTh.isize().area())};
    Imat<uint8_t> imgBAvg{imgBTh.pwin().height, imgBTh.pwin().width, 1, imgBTmp.ptr()+imgBTmp.get_memsize(), (size_t)imgBTh.pwin().size().area()};
    if (0 != Filter::Mean(imgBAvg, imgBTh, imgBTmp, meanRadius))
    {
        ISL_LOGE("Mean failed, imgBAvg: {}, imgBTh: {}, imgBTmp: {}", imgBAvg, imgBTh, imgBTmp);
        return sliceResults;
    }

    imgBAvg.move_vpads({meanHeight - openHeight, 0});
    Imat<uint8_t> imgErode{imgBAvg.pwin().height, imgBAvg.pwin().width, 1, (uint8_t*)imgTmp.ptr(), (size_t)imgBAvg.pwin().size().area()};
    std::function<bool(const uint8_t)> isEroding = [](const uint8_t src) -> bool {return (src < 160);};
    if (0 != Filter::Erode(imgErode, imgBAvg, openRadius, isEroding, {this->bgVal, this->fgVal})) // 腐蚀后，背景为255，前景为0
    {
        ISL_LOGE("Erode failed, imgErode: {}, imgBAvg: {}", imgErode, imgBAvg);
        return sliceResults;
    }

    profBinz.move_vpads({profBinz.pwin().height, -dsSize.height}); // 定位到实际处理区域，高度为dsSize.height
    const Rect<int32_t> binzWinNew(profBinz.pwin());
    const Rect<int32_t> binzWinPre(binzWinNew.x, std::max(binzWinNew.y - binzWinNew.height, this->pads_around), 
                                   binzWinNew.width, (binzWinNew.y > binzWinNew.height) ? binzWinNew.height : 0);

    profBinz.move_vpads({binzWinNew.height - imgErode.pwin().height, 0}); // 重写上一个条带中需要有下邻域的区域
    std::function<bool(const uint8_t)> isDilating = [this](const uint8_t src) -> bool {return (src == this->fgVal);};
    if (0 != Filter::Dilate(profBinz, imgErode, openRadius, isDilating, {this->bgVal, this->fgVal}))
    {
        ISL_LOGE("Dilate failed, profBinz: {}, imgErode: {}", profBinz, imgErode);
        return sliceResults;
    }

    for (int32_t i = 0, y = profBinz.pwin().y; i < profBinz.pwin().height; ++i, ++y)
    {
        uint8_t* pBinz = profBinz.ptr(y, profBinz.pwin().x);
        uint8_t* pDs = profDs.ptr(y-this->pads_around, profDs.pwin().x);
        int32_t step = 0, bgCnt = 0;
        for (; step < profDs.pwin().width; ++step, ++pBinz, ++pDs)
        {
            if (*pBinz < this->arVal) // 前景
            {
                if (*pDs < u8BgTh) // 确定是前景
                {
                    break;
                }
            }
            else // 背景
            {
                ++bgCnt;
            }
        }
        if (step == profDs.pwin().width && bgCnt < step) // 实际整行均为背景
        {
            std::memset(pBinz-step, this->bgVal, step);
        }
    }

    /// 已有轮廓的生长：第一个轮廓直接生长，其他轮廓先查找是否已被合并，未合并则生长 
    std::vector<Rect<int32_t>> growBBoxs;
    std::vector<uint8_t> mExisted;
    std::vector<std::pair<uint8_t, int32_t>> mark_conf;
    for (auto it = this->fgContours.begin(); it != this->fgContours.end();)
    {
        bool bMerged = false; // 是否已被其他轮廓合并了
        Rect<int32_t> remarkArea = it->BoundingBox(); // 当轮廓A被轮廓B吞并或覆盖，需要重置轮廓A为轮廓B的mark值
        if (remarkArea.posbr().y >= profBinz.pwin().y) // 只需Remark profBinz.pwin()的上方区域，profBinz.pwin()区域会用fillin来mark
        {
            remarkArea.height = profBinz.pwin().y - remarkArea.y;
        }

        // 备份轮廓置信度
        ContAttr* pAttr = it->getPrivate<ContAttr>();
        mark_conf.push_back(std::make_pair(it->MarkVal(), (nullptr != pAttr) ? pAttr->fgConf : -1));

        for (auto iExisted = this->fgContours.begin(); iExisted != it; ++iExisted)
        {
            // 与{iExisted}的生长BoundingBox无交集，则肯定未被合并，直接尝试下一个
            auto overlap = it->BoundingBox().intersect(growBBoxs.at(std::distance(this->fgContours.begin(), iExisted)));
            if (overlap.size().area() == 0)
            {
                continue;
            }

            std::vector<Point<int32_t>> outside;
            std::pair<size_t, size_t> rate = iExisted->CoverageRate(*it, outside, remarkArea);
            if (rate.second > 0) // 全包含或部分包含，部分包含时，若it的起始点在outside中，则需要Grow操作，若不在，暂直接remark处理
            {
                if (outside.size() == 0 || std::find(outside.begin(), outside.end(), it->StartPoint()) == outside.end())
                {
                    it->Remark(profBinz, iExisted->MarkVal(), remarkArea);
                    bMerged = true;
                }
                ISL_LOGD("contour#{} @ {} {} contain #{} @ {}, {} / {}, {}", iExisted->MarkVal(), iExisted->StartPoint(), 
                         bMerged ? "do" : "don't", it->MarkVal(), it->StartPoint(), rate.second, rate.first, outside);
                break;
            }
        }

        if (!bMerged)
        {
            auto& bbox = growBBoxs.emplace_back();
            if (it->BoundingBox().posbr().y >= profBinz.pwin().y - 1)
            {
                /// 这里直接从profBinz.pwin().y-1开始Grow也是可以的，但遇到的异常情况会多一些，最终结果是正常的，为提高效率，Grow起始行往上提了几个像素
                int32_t growRet = it->Grow(profBinz, (it->BoundingBox().y < binzWinPre.y) ? binzWinPre.y : profBinz.pwin().y-1, mExisted, bbox);
                if (growRet >= 0)
                {
                    if (growRet == this->fgVal)
                    {
                        mExisted.push_back(it->MarkVal());
                        if (bbox.size().area() > 0)
                        {
                            it->Fillin(profBinz, profBinz.pwin());
                        }
                        ++it;
                        continue;
                    }
                    else // 与其他轮廓发生碰撞，重置为碰撞轮廓的mark值：内轮廓被外轮廓包含，内轮廓Grow时会出现这种情况
                    {
                        ISL_LOGW("contour#{} @ {}: collides with contour#{}, grow aborted", it->MarkVal(), it->StartPoint(), growRet);
                        it->Remark(profBinz, growRet, remarkArea);
                    }
                }
                else
                {
                    // TODO: 这里未完善，需根据Grow失败的原因做不同的处理（有些情况也不用处理），暂只输出一个警告
                    ISL_LOGW("contour#{} @ {}: grow failed, pwin {}", it->MarkVal(), it->StartPoint(), profBinz.pwin());
                    //std::string fn_profsh = fmt::format("/mnt/dump/{}", this->procCnt);
                    //profBinz.dump(fn_profsh, true, Rect<int32_t>(0, 0, profBinz.width(), profBinz.pwin().y+profBinz.pwin().height+this->pads_around));
                }
            }
            else
            {
                ++it;
                continue;
            }
        }
        it = this->fgContours.erase(it);
    }

    profBinz.move_vpads({0, meanRadius + openRadius}); // 最后几行邻域数据，下个条带输入后会被更新的，所以就不搜索了
    SearchContours(this->fgContours, profBinz, mark_conf); // 在新区域中继续寻找新的轮廓
    if (!this->fgContours.empty())
    {
        const auto iEnd = this->fgContours.end(); // 备份end()迭代器，下面for循环中会对list做push_back操作，导致end()自动后移
        for (auto it = this->fgContours.begin(); it != iEnd;)
        {
            if (!it->hasPrivate<ContAttr>())
            {
                it->setPrivate<ContAttr>(ContAttr());
            }
            ContAttr* pAttr = it->getPrivate<ContAttr>();
            if (it->StartPoint().y >= subcf0(binzWinNew.y, binzWinNew.height) && // 算法内部至少缓存一个条带，往前倒一
                pAttr != nullptr && pAttr->fgConf <= this->fgConfMin) // 置信度过低时继续更新
            {
                pAttr->fgConf = MaterObjConf(*it);
                ISL_LOGD("contour#{} @ {}, bbox {}, pwin {}, conf {}", 
                         it->MarkVal(), it->StartPoint(), it->BoundingBox(), profBinz.pwin(), pAttr->fgConf);
                if (pAttr->fgConf <= this->fgConfMin) // 置信度仍较低
                {
                    if (it->BoundingBox().posbr().y <= profBinz.pwin().posbr().y) // 无继续延伸趋势，直接清除
                    {
                        ISL_LOGD("contour#{} @ {} ~ {} & {}: no extension tendency, delete this", 
                                 it->MarkVal(), it->StartPoint(), it->BoundingBox(), profBinz.pwin());
                        it->Clear(profBinz, it->BoundingBox());
                        it = this->fgContours.erase(it);
                        continue;
                    }
                    else if (it->StartPoint().y < binzWinNew.y) // 上个带条结果将要输出了，清除该部分数据，并更新轮廓
                    {
                        // 在该轮廓范围内，重新寻找新轮廓，上面的清除操作可能会将单个分成多个轮廓
                        if (FissionContour(this->fgContours, profBinz, *it, binzWinPre) < 0)
                        {
                            ISL_LOGI("fission contour#{} @ {} ~ {} failed, clear this directly", 
                                     it->MarkVal(), it->StartPoint(), it->BoundingBox());
                            it->Clear(profBinz, it->BoundingBox());
                        }
                        it = this->fgContours.erase(it);
                        continue;
                    }
                }
            }
            ++it;
        }
    }
    profBinz(binzWinNew);

    /// 合并所有轮廓的结果，统一输出
    std::vector<std::list<Contour>::iterator> iFgResults;
    std::vector<std::pair<int32_t, int32_t>> verRangeSeq; // 垂直方向连续前景段，连续的含义：A与B正好连续或有部分重叠或包含
    if (!this->fgContours.empty())
    {
        for (auto it = this->fgContours.begin(); it != this->fgContours.end(); ++it)
        {
            ContAttr *pAttr = it->getPrivate<ContAttr>();
            ISL_LOGD("out fg {}: #{} @ {} ~ {} $ {}, confidence: {}", std::distance(this->fgContours.begin(), it),
                     it->MarkVal(), it->StartPoint(), it->BoundingBox(), it->BoundaryLen(), pAttr ? pAttr->fgConf : -1);
            if (pAttr && (pAttr->fgConf > this->fgConfMin))
            {
                iFgResults.push_back(it);
            }
        }
        if (!iFgResults.empty())
        {
            auto _t2b = [](const std::list<Contour>::iterator &iA, const std::list<Contour>::iterator &iB) -> bool {
                return iA->BoundingBox().y < iB->BoundingBox().y; // top -> bottom
            };
            std::sort(iFgResults.begin(), iFgResults.end(), _t2b);
        }

        for (auto& iv : iFgResults)
        {
            const Rect<int32_t>& boxRef = iv->BoundingBox();
            int32_t bottom = verRangeSeq.empty() ? 0 : verRangeSeq.back().second;
            if (boxRef.y > bottom) // separate
            {
                verRangeSeq.push_back({boxRef.y, boxRef.posbr().y});
                //if (boxRef.posbr().y >= binzWinNew.y) present.mark = {it->MarkVal()}; // 当前条带中有多个分离的轮廓，仅记录最后一组
            }
            else if (boxRef.y <= bottom && bottom < boxRef.posbr().y) // overlap
            {
                verRangeSeq.back().second = boxRef.posbr().y;
                //if (!present.mark.empty()) present.mark.push_back(it->MarkVal()); // 仅追加，不创建
            }
        }
    }

    auto _update_result = [this, &iFgResults, &verRangeSeq](Rect<int32_t> sliceRegion, Imat<uint8_t>& sliceMask) -> Fg4Slice 
    {
        fg_morph_t sliceMorgh = morph_none;
        if (!(verRangeSeq.empty() || verRangeSeq.front().first > sliceRegion.posbr().y || verRangeSeq.back().second < sliceRegion.y))
        {
            for (auto& it : verRangeSeq)
            {
                Rect<int32_t> overlap = sliceRegion.intersect(Rect<int32_t>(sliceRegion.x, it.first, sliceRegion.width, it.second-it.first+1)); 
                if (overlap.height > 0)
                {
                    if (overlap.y == sliceRegion.y) // 上沿重合
                    {
                        if (overlap.height == sliceRegion.height) // 下沿也重合
                        {
                            if (it.second == sliceRegion.posbr().y)
                            {
                                sliceMorgh = (it.first == sliceRegion.y) ? morph_indep : morph_end;
                            }
                            else
                            {
                                sliceMorgh = (it.first == sliceRegion.y) ? morph_start : morph_middle;
                            }
                            break;
                        }
                        else // 下沿不重合
                        {
                            sliceMorgh = (it.first == sliceRegion.y) ? morph_indep : morph_end;
                        }
                    }
                    else // overlap.y > sliceRegion.y，上沿不重合
                    {
                        if (overlap.posbr().y == sliceRegion.posbr().y) // 下沿重合
                        {
                            sliceMorgh = (it.second == sliceRegion.posbr().y) ? morph_indep : ((sliceMorgh == morph_end) ? morph_stend : morph_start);
                            break;
                        }
                        else // 下沿也不重合
                        {
                            if (sliceMorgh != morph_end)
                            {
                                sliceMorgh = morph_indep;
                            }
                        }
                    }
                }
                else
                {
                    if (sliceMorgh != morph_none)
                    {
                        break;
                    }
                }
            }
        }

        Fg4Slice result;
        result.status = sliceMorgh;
        if (sliceMorgh != morph_none)
        {
            auto& win1 = result.bboxes.emplace_back(sliceRegion);
            if (sliceMorgh == morph_stend)
            {
                auto iWin2 = verRangeSeq.rend();
                for (auto it = verRangeSeq.rbegin()+1; it != verRangeSeq.rend(); ++it)
                {
                    if (win1.y <= it->second && it->second < sliceRegion.posbr().y)
                    {
                        win1.height = it->second - win1.y + 1;
                        iWin2 = std::prev(it);
                        break;
                    }
                }
                if (iWin2 != verRangeSeq.rend() && iWin2->first <= sliceRegion.posbr().y)
                {
                    result.bboxes.push_back(Rect<int32_t>(sliceRegion.x, iWin2->first, sliceRegion.width, sliceRegion.posbr().y - iWin2->first + 1));
                }
            }

            // 非result.bboxes区域置为0
            Rect<int32_t> bgWin = sliceMask.pwin();
            if (result.bboxes.front().y > sliceRegion.y)
            {
                bgWin.height = result.bboxes.front().y - sliceRegion.y;
                sliceMask.fill(0, bgWin);
            }
            for (auto it0 = result.bboxes.begin(), it1 = std::next(it0); it1 != result.bboxes.end(); ++it0, ++it1)
            {
                bgWin.y = sliceMask.pwin().y + (it0->y + it0->height - sliceRegion.y);
                bgWin.height = it1->y - it0->y - it0->height;
                sliceMask.fill(0, bgWin);
            }

            for (auto& win : result.bboxes)
            {
                Imat<uint8_t> statBinz(this->profBinz, win);
                win.x -= this->pads_around;
                win.y -= this->pads_around;
                Imat<uint8_t> statDs(this->profDs, win);
                win.x = sliceMask.pwin().x;
                win.y = statBinz.pwin().y - sliceRegion.y + sliceMask.pwin().y;
                Imat<uint8_t> maskOut(sliceMask, win);
                #if 0 // 仅统计置信度高的前景区域
                std::function<bool(const uint8_t, const uint8_t)> isMarked = [&iFgResults](const uint8_t, const uint8_t binz) -> 
                    bool { for (auto& iv : iFgResults) if (binz == iv->MarkVal()) return true; return false; };
                #else // 统计所有前景区域
                std::function<bool(const uint8_t, const uint8_t)> isMarked = [this](const uint8_t, const uint8_t binz) -> 
                    bool { if (this->fgVal < binz && binz < this->arVal) return true; return false; };
                #endif
                if (Statistic(statDs, statBinz, isMarked, &maskOut, &win, nullptr, nullptr) > 0)
                {
                    win.x *= prof_dscale.width;
                    win.y -= sliceRegion.y - this->pads_around;
                    win.y *= prof_dscale.height;
                    win.width *= prof_dscale.width;
                    win.height *= prof_dscale.height;
                }
                else
                {
                    ISL_LOGE("statistic failed, statDs: {}, statBinz: {}, sliceMask {}, win {}", statDs, statBinz, sliceMask, win);
                }
            }
        }
        else
        {
            sliceMask.fill(0, sliceMask.pwin()); // no foreground
        }

        return result;
    };

    Imat<uint8_t> imgMask(dsSize.height, dsSize.width, 1, (uint8_t*)imgTmp.ptr(), dsSize.area()); // 这里将imgTmp拆成了2块，均为uint8_t类型，内存足够
    Imat<uint8_t> imgTmpS(fgMask.pwin().height, fgMask.pwin().width, 1, imgMask.ptr()+imgMask.get_memsize(), fgMask.pwin().size().area());
    std::function<uint8_t(const uint8_t)> _b2mask = [this](const uint8_t src) -> uint8_t {return (src ? this->fgVal : this->bgVal);};
    fgMask.move_vpads({-fgMask.pwin().height, fgMask.pwin().height});
    if (!fgSliceOut.empty())
    {
        fgSliceOut.back() = _update_result(binzWinPre, imgMask);
        if (0 != imgAnyScale(fgMask, imgMask, &imgTmpS, &usLutHor, &usLutVer, _b2mask))
        {
            ISL_LOGW("imgAnyScale failed, fgMask {}, imgMask {}, imgTmpS {}", fgMask, imgMask, imgTmpS);
        }
        sliceResults.push_back(fgSliceOut.back());
    }
    else
    {
        fgMask.fill(this->bgVal, fgMask.pwin());
    }
    fgMask.move_vpads({fgMask.pwin().height, -fgMask.pwin().height});
    fgSliceOut.emplace_back(_update_result(binzWinNew, imgMask));
    if (0 != imgAnyScale(fgMask, imgMask, &imgTmpS, &usLutHor, &usLutVer, _b2mask))
    {
        ISL_LOGW("imgAnyScale failed, fgMask {}, imgMask {}, imgTmpS {}", fgMask, imgMask, imgTmpS);
    }
    sliceResults.push_back(fgSliceOut.back());

    /// 恢复profDs与profBinz的有效图像区域
    //profDs.reshape(Size<int32_t>(this->prof_smax.width, this->prof_smax.height), 1, {0, this->prof_smax.height-profDs.height()});
    profDs.move_vpads({-profDs.pwin().y, 0});
    profEdge.move_vpads({-profEdge.pwin().y, 0});
    profTh.move_vpads({-profTh.pwin().y, 0});
    profBinz({pads_around, pads_around, dsSize.width, profBinz.pwin().y + profBinz.pwin().height - pads_around});

    #if 0
    std::string dump_dir = "/mnt/dump/";
    std::string fn_profds = fmt::format("{}/profds/{}", dump_dir, this->procCnt);
    profDs.dump(fn_profds, true, Rect<int32_t>(0, 0, profDs.pwin().width, profDs.pwin().height));
    std::string fn_profed = fmt::format("{}/profed/{}", dump_dir, this->procCnt);
    profEdge.dump(fn_profed, true, Rect<int32_t>(0, 0, profEdge.pwin().width, profEdge.pwin().height));
    std::string fn_profth = fmt::format("{}/profth/{}", dump_dir, this->procCnt);
    profTh.dump(fn_profth, true, Rect<int32_t>(0, 0, profTh.pwin().width, profTh.pwin().height));
    std::string fn_profbz = fmt::format("{}/profbz/{}", dump_dir, this->procCnt);
    profBinz.dump(fn_profbz, true, Rect<int32_t>(0, 0, profBinz.width(), profBinz.pwin().height+this->pads_around*2));
    std::string fn_fgmask = fmt::format("{}/fgmask/{}", dump_dir, this->procCnt);
    fgMask.dump(fn_fgmask, true);

    std::string searchTag = "_t";
    std::string formatted = fmt::format("{} - {} / {}, results: ", (void*)this, this->procCnt, 
                                        fn_fgmask.substr(fn_fgmask.find(searchTag)+searchTag.length(), 10));
    for (auto it = sliceResults.rbegin(); it != sliceResults.rend(); ++it)
    {
        formatted += fmt::format("{}", static_cast<int32_t>(it->status));
        for (auto& ib : it->bboxes)
        {
            formatted += fmt::format(", {}", ib);
        }
        formatted += fmt::format(" | ");
    }
    ISL_LOGI("{}", formatted);
    #endif

    if (fgSliceOut.size() >= 2)
    {
        auto iSlicePre = std::next(fgSliceOut.rbegin());
        if (iSlicePre->status == morph_none || iSlicePre->status == morph_end || iSlicePre->status == morph_indep)
        {
            /// 只清用于记录的fgContours和fgSliceOut，不清数据
            for (auto it = this->fgContours.begin(); it != this->fgContours.end();)
            {
                if (it->BoundingBox().y >= binzWinNew.y) // 保留
                {
                    ++it;
                }
                else // 清除
                {
                    it = this->fgContours.erase(it);
                }
            }
            fgSliceOut.erase(fgSliceOut.begin(), std::prev(fgSliceOut.end())); // 仅保留最后一个条带
        }
        else if (iSlicePre->status == morph_start || iSlicePre->status == morph_stend)
        {
            /// 不仅清用于记录的fgContours和fgSliceOut，而且清数据：将最后2个条带移到最上，然后其他部分置为背景
            const int32_t displace = subcf0(profDs.pwin().height, dsSize.height * 2); // 需要移除的图像高度
            Imat<uint8_t> sliceDs = profDs;
            sliceDs.move_vpads({displace, 0});
            profDs.move_vpads({0, displace});
            sliceDs.copyto(profDs, sliceDs.pwin(), profDs.pwin());

            Imat<uint8_t> sliceEdge = profEdge;
            sliceEdge.move_vpads({displace, 0});
            profEdge.move_vpads({0, displace});
            sliceEdge.copyto(profEdge, sliceEdge.pwin(), profEdge.pwin());

            Imat<uint8_t> sliceTh = profTh;
            sliceTh.move_vpads({displace, 0});
            profTh.move_vpads({0, displace});
            sliceTh.copyto(profTh, sliceTh.pwin(), profTh.pwin());

            Imat<uint8_t> sliceBinz(profBinz); // 上一个条带的Binz图
            sliceBinz.move_vpads({displace, 0});
            profBinz.move_vpads({0, displace}); // 定位到最上一个条带
            sliceBinz.copyto(profBinz, sliceBinz.pwin(), profBinz.pwin());
            Rect<int32_t> resetRegion(0, 0, profBinz.width(), this->pads_around);
            profBinz.fill(this->bgVal, resetRegion);
            resetRegion.y = profBinz.pwin().y + profBinz.pwin().height;
            resetRegion.height += displace;
            profBinz.fill(this->bgVal, resetRegion);

            /// 更新fgContours与fgSliceOut
            for (auto it = this->fgContours.begin(); it != this->fgContours.end();)
            {
                if (it->BoundingBox().y >= sliceBinz.pwin().y)
                {
                    it->Move(-displace);
                    ++it;
                }
                else
                {
                    it = this->fgContours.erase(it);
                }
            }
            fgSliceOut.erase(fgSliceOut.begin(), std::prev(fgSliceOut.end(), 2));
        }
    }

    return sliceResults;
}


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
std::vector<Rect<int32_t>> Fbg::SmoothWeight(Imat<uint8_t>& weightOut, Imat<uint8_t>& maskIn, int32_t radius, const std::optional<std::vector<Rect<int32_t>>>& regions)
{
    if (!(weightOut.isvalid() && maskIn.isvalid()))
    {
        ISL_LOGE("weightOut({}) or maskIn({}) is invalid", weightOut, maskIn);
        return std::vector<Rect<int32_t>>();
    }
    if (maskIn.pwin().size() != weightOut.pwin().size())
    {
        ISL_LOGE("weightOut size is invalid: {}, maskIn: {}", weightOut, maskIn);
        return std::vector<Rect<int32_t>>();
    }

    radius = std::min(radius, 10); // 平滑半径最大不超过10
    std::list<Rect<int32_t>> fgWins;

    if (!regions.has_value()) // 无regions输入，内部遍历maskIn
    {
        int32_t horMin = maskIn.pwin().posbr().x, horMax = maskIn.pwin().x, verMin = -1, verMax = -1; // 初始化为非法值
        for (int32_t i = 0, y = maskIn.pwin().y; i < maskIn.pwin().height; ++i, ++y)
        {
            uint8_t *pMask = maskIn.ptr(y);
            bool bFgIn = false; // 当前行是否有包裹
            for (int32_t j = 0, x = maskIn.pwin().x; j < maskIn.pwin().width; ++j, ++x, ++pMask)
            {
                if (!(*pMask & 0x80)) // 前景
                {
                    if (!bFgIn) bFgIn = true;
                    if (x < horMin)
                    {
                        horMin = x;
                    }
                    else if (x > horMax)
                    {
                        horMax = x;
                    }
                }
            }
            if (bFgIn)
            {
                if (verMin < 0) {verMin = y;} // 从无到有，开始
            }
            else
            {
                if (verMin >= 0 && verMax < 0) // 从有到无，结束
                {
                    verMax = y - 1;
                    fgWins.push_back(Rect<int32_t>(horMin, verMin, horMax - horMin + 1, verMax - verMin + 1));
                    horMin = maskIn.pwin().posbr().x, horMax = maskIn.pwin().x, verMin = -1, verMax = -1; // 重置
                }
            }
        }
        if (verMin >= 0 && verMax < 0) // 最后一行仍是包裹
        {
            fgWins.push_back(Rect<int32_t>(horMin, verMin, horMax - horMin + 1, maskIn.pwin().posbr().y - verMin + 1));
        }
    }
    else if (!regions->empty())
    {
        for (auto& ir : regions.value())
        {
            fgWins.push_back(ir.intersect(maskIn.pwin())); // 先只计算在maskIn.pwin()内部的区域
        }
    }

    if (!fgWins.empty())
    {
        for (auto& it : fgWins)
        {
            it.extend(radius, radius, maskIn.pwin()); // 对每个矩形区扩边
        }

        // 左上顶点从上到下、从左到右排序
        auto _t2b_l2r = [](const Rect<int32_t>& rectA, const Rect<int32_t>& rectB) -> bool
        {
            return (rectA.y != rectB.y) ? (rectA.y < rectB.y) : (rectA.x < rectB.x); // top -> bottom, then left -> right
        };
        fgWins.sort(_t2b_l2r);

        // 合并垂直方向有交叉的矩形区
        auto iPrev = fgWins.begin(), iCurt = std::next(iPrev);
        for (; iCurt != fgWins.end(); ++iCurt)
        {
            if (iPrev->y <= iCurt->y && iCurt->y <= iPrev->posbr().y) // 两矩形在纵向上有交叉，则合并
            {
                iCurt->unite(*iPrev);
                fgWins.erase(iPrev);
            }
            iPrev = iCurt;
        }
    }

    /// 若矩形区未覆盖pwin()中靠近上下邻域的radius行，则遍历上下邻域中靠近处理区域的radius行，用于判定是否需要计算该区域的权重图
    if (fgWins.empty() || fgWins.front().y > maskIn.pwin().y) // 计算maskIn上邻域中靠近中间处理区域的radius行内的包裹区域
    {
        int32_t horMin = maskIn.pwin().posbr().x, horMax = maskIn.pwin().x, verMin = -1, verMax = -1; // 初始化为非法值
        for (int32_t i = std::min(maskIn.get_vpads().first, radius)-1, j = 1; i >= 0; --i, ++j) // 上邻域从下往上倒序遍历
        {
            int32_t left = maskIn.pwin().x, right = maskIn.pwin().posbr().x;
            uint8_t* pMask = maskIn.ptr(maskIn.pwin().y - j, left);
            for (; left <= right; ++left, ++pMask) // 左边界
            {
                if (0 == (*pMask & 0x80)) // 最高位为0，前景
                {
                    break;
                }
            }
            if (left <= right) // 左边界存在，则查询右边界
            {
                pMask = maskIn.ptr(maskIn.pwin().y - j, right);
                for (; right > left; --right, --pMask) // 右边界
                {
                    if (0 == (*pMask & 0x80)) // 最高位为0，前景
                    {
                        break;
                    }
                }
                if (left < horMin) horMin = left;
                if (right > horMax) horMax = right;
                if (verMin < 0 || verMax < 0) // 异常则赋值，赋值一次即可
                {
                    verMin = maskIn.pwin().y;
                    verMax = verMin + i;
                }
            }
        }
        if (horMax >= horMin && verMax >= verMin)
        {
            fgWins.push_front(Rect<int32_t>(horMin, verMin, horMin - horMin + 1, verMax - verMin + 1));
        }
    }

    if (fgWins.empty() || fgWins.back().posbr().y < maskIn.pwin().posbr().y) // 计算matMask下邻域中靠近中间处理区域的smoothRadius行内的包裹区域
    {
        int32_t horMin = maskIn.pwin().posbr().x, horMax = maskIn.pwin().x, verMin = -1, verMax = -1; // 初始化为非法值
        for (int32_t i = std::min(maskIn.get_vpads().second, radius)-1, j = 1; i >= 0; --i, ++j) // 下邻域从上往下正序遍历
        {
            int32_t left = maskIn.pwin().x, right = maskIn.pwin().posbr().x;
            uint8_t* pMask = maskIn.ptr(maskIn.pwin().posbr().y + j, left);
            for (; left <= right; ++left, ++pMask) // 左边界
            {
                if (0 == (*pMask & 0x80)) // 最高位为0，前景
                {
                    break;
                }
            }
            if (left <= right) // 左边界存在，则查询右边界
            {
                pMask = maskIn.ptr(maskIn.pwin().posbr().y + j, right);
                for (; right > left; --right, --pMask) // 右边界
                {
                    if (0 == (*pMask & 0x80)) // 最高位为0，前景
                    {
                        break;
                    }
                }
                if (left < horMin) horMin = left;
                if (right > horMax) horMax = right;
                if (verMin < 0 || verMax < 0) // 异常则赋值，赋值一次即可
                {
                    verMax = maskIn.pwin().posbr().y;
                    verMin = verMax - i;
                }
            }
        }
        if (horMax >= horMin && verMax >= verMin)
        {
            fgWins.push_back(Rect<int32_t>(horMin, verMin, horMin - horMin + 1, verMax - verMin + 1));
        }
    }

    if (fgWins.empty())
    {
        weightOut.fill(0, weightOut.pwin()); // 输出置0
        return std::vector<Rect<int32_t>>(); // 返回一个空Vector
    }
    else
    {
        // 计算背景矩形区，并置权重为0，注：bgWins的坐标先是基于maskIn，然后再转到基于weightOut
        std::vector<Rect<int32_t>> bgWins;
        auto iCurt = fgWins.begin();
        if (iCurt->y > maskIn.pwin().y)
        {
            bgWins.push_back(Rect<int32_t>(maskIn.pwin().x, maskIn.pwin().y, maskIn.pwin().width, iCurt->y - maskIn.pwin().y));
        }
        for (; iCurt != fgWins.end(); ++iCurt)
        {
            auto iNext = std::next(iCurt);
            Rect<int32_t> boundary(maskIn.pwin().x, iCurt->y, maskIn.pwin().width, 
                                   (iNext != fgWins.end() ? iNext->y : maskIn.pwin().posbr().y+1) - iCurt->y);
            std::vector<Rect<int32_t>>&& comp = iCurt->complement(boundary);
            bgWins.insert(bgWins.end(), comp.begin(), comp.end());
        }

        for (auto& it : bgWins)
        {
            it.x += weightOut.pwin().x - maskIn.pwin().x; // 转换为基于weightOut的坐标，宽高不变
            it.y += weightOut.pwin().y - maskIn.pwin().y;
            weightOut.fill(0, it); // 输出置0
        }
    }

    std::map<int32_t, uint8_t> distM2Wt; // 距离与权重的映射关系，这里采用欧式距离
    const float64_t smoothRatio = 1.5; // 平滑率，不小于1.0，值越小，高对比度时从低亮过渡到高亮会越突兀些，1.2~2.0较合理
    for (int32_t i = 0; i <= radius; ++i)
    {
        for (int32_t j = i; j <= radius; ++j)
        {
            int32_t distSq = i * i + j * j; // Square Distance，欧氏距离的平方
            float64_t distEu = std::sqrt(distSq); // Euclidean Distance，欧氏距离
            if (static_cast<float64_t>(radius+1) - distEu > FLT_EPSILON)
            {
                distM2Wt[distSq] = std::round(subcf0(1.0, std::pow(distEu / (radius+1), smoothRatio)) * 255);
            }
            else
            {
                distM2Wt[distSq] = 0;
            }
        }
    }

    std::vector<int32_t> dist2;
    for (int32_t i = -radius; i <= radius; ++i)
    {
        for (int32_t j = -radius; j <= radius; ++j)
        {
            dist2.push_back(i * i + j * j);
        }
    }

    for (auto& iw : fgWins)
    {
        for (int32_t i = 0, y = iw.y; i < iw.height; ++i, ++y)
        {
            std::vector<Imat<uint8_t>::Rptr> rMask;
            for (int32_t k = -radius; k <= radius; ++k)
            {
                rMask.push_back(maskIn.rptr(y + k));
            }

            uint8_t* pSrc = maskIn.ptr(y, iw.x);
            uint8_t* pDst = weightOut.ptr(weightOut.pwin().y + (y - maskIn.pwin().y), weightOut.pwin().x + (iw.x - maskIn.pwin().x));
            for (int32_t j = 0, x = iw.x; j < iw.width; ++j, ++x, ++pSrc, ++pDst)
            {
                if (*pSrc & 0x80) // 最高位为1，背景
                {
                    auto _m2wt = [&radius, &dist2, &distM2Wt, &rMask, &x]() -> uint8_t // 该lambda只是为了跳出多层循环
                    {
                        int32_t distMin = std::numeric_limits<int32_t>::max();
                        int32_t n = 0;
                        for (auto& row : rMask)
                        {
                            for (int32_t k = -radius; k <= radius; ++k, ++n)
                            {
                                if (!(row[x+k] & 0x80) && dist2[n] < distMin) // 遇到前景像素时计算距离
                                {
                                    distMin = dist2[n];
                                    if (1 == distMin) // 以达到最小值，直接跳出
                                    {
                                        return distM2Wt.at(1);
                                    }
                                }
                            }
                        }
                        return (distMin < std::numeric_limits<int32_t>::max()) ? distM2Wt.at(distMin) : 0;
                    };
                    *pDst = _m2wt();
                }
                else // 最高位为0，前景
                {
                    *pDst = 255;
                }
            }
        }
    }

    return std::vector<Rect<int32_t>>(fgWins.begin(), fgWins.end());
}

} // namespace

