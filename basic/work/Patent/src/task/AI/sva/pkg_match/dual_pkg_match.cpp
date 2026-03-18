/**
 * @file   dual_pkg_match.cpp
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  双视角包裹匹配
 * @author sunzelin
 * @date   2022/5/30
 * @note
 * @note \n History
   1.日    期: 2022/5/30
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include <iostream>
#include <vector>
#include <iterator>
#include <math.h>

/* 模块外部头文件 */
#include "dual_pkg_match.h"

using namespace std;

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define GET_VIEW_2_CHN_MAP(v) (static_cast<int>(v) - 1)

#define DPM_CHECK_RET(cond, str, val) \
    { \
        if (cond) \
        { \
            cout << str << "ret: " << val << endl; \
            return val;\
        } \
    }

#define DPM_CHECK_NO_RET(cond, str, r, val) \
    { \
        if (cond) \
        { \
            cout << str << "ret: " << val << endl; \
            r = val; \
        } \
    }

#define GET_MIN(A,B) (A > B ? B : A)
#define GET_MAX(A,B) (A > B ? A : B)
#define FLOAT_MULTI_A_THOUSAND(A) (A * 1000.0)

/* 包裹匹配缓存单元大小，当前仅有包裹id信息 */
#define DUAL_PKG_MATCH_BUF_UNIT_SIZE (sizeof(pkg_info))
/* 一维IOU默认匹配阈值 */
#define DUAL_PKG_MATCH_DEFAULT_THRES_1D (0.85f)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef unsigned int UINT;

typedef struct _DUAL_PKG_MATCH_LIST_INFO_
{
    /* 包裹id */
    int iPkgId;

	/* 是否满足上下堆叠条件 */
	bool bPkgOverlay;
	/* 匹配IOU，一维 */
	float fMatchIou1d;
} DUAL_PKG_MATCH_LIST_INFO_S;

typedef struct _DUAL_PKG_MATCH_INNER_PKG_INFO_
{
    /* 包裹是否有效标志。放入队列时置为true，当包裹完成匹配后置为false */
    bool bExist;
    /* 包裹id */
    int iPkgId;
	/* 当前目标对应的匹配包裹链表，目前也使用vector实现 */
	vector<DUAL_PKG_MATCH_LIST_INFO_S> vec;
} DUAL_PKG_MATCH_INNER_PKG_INFO_S;

typedef DUAL_PKG_MATCH_INNER_PKG_INFO_S pkg_info;

typedef struct _DUAL_PKG_MATCH_CHN_VECTOR_INFO_
{
    /* vector，用于存放通道包裹结果 */
    vector<pkg_info> vec;
    /* 包裹缓存队列静态参数 */
    DUAL_PKG_MATCH_INIT_CHN_QUE_PRM_S stQuePrm;
} DUAL_PKG_MATCH_CHN_VECTOR_INFO_S;

class PkgMatch
{
public:
	/* constructor */
	PkgMatch();
	PkgMatch(const DUAL_PKG_MATCH_INIT_PRM_S *pstInitPrm);
	
	/* destructor */
	~PkgMatch();

	/* insert object into vector */
	int insert(int idx, pkg_info& obj);

	/* update vector, erase unexist obj */
	int update(void);

	/* do pkg match */
	int match(const DUAL_PKG_MATCH_CTRL_PRM_S *pstCtrlPrm, DUAL_PKG_MATCH_RESULT_S *pstMatchResult);

	/* set match threshold */
	int set_match_thresh(float thresh);

	/* print module debug status */
	int print_debug_status(void);

private:
	/* channel cnt */
	unsigned int _cnt;
	DUAL_PKG_MATCH_CHN_VECTOR_INFO_S _vecInfo[DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM];

	/* dual view match threshold, default 0.85，如DUAL_PKG_MATCH_DEFAULT_THRES_1D */
	float _match_thresh;
};

typedef struct _DUAL_PKG_MATCH_INNER_INFO_
{
	/* 初始化标识 */
    bool bInit;
	/* 实例类PkgMatch，用户层接口init中new一个 */
    void *pPkgMatch;
} DUAL_PKG_MATCH_INNER_INFO_S;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
static DUAL_PKG_MATCH_INNER_INFO_S g_stInnerInfo = {0};

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
#if 1  /* debug api */
static void pr_vec(DUAL_PKG_MATCH_CHN_VECTOR_INFO_S& vec_info)
{
    cout << "======== vec_info ========" << endl;
    cout << "depth: " << vec_info.stQuePrm.uiQueDepth << endl;
    /* cout << "unit_size: " << vec_info.stQuePrm.iQueUnitSize << endl; */

	cout	<< "current vector size: " << vec_info.vec.size()
			<< ", current capacity: " << vec_info.vec.capacity() << endl;

    vector<pkg_info>::iterator it = vec_info.vec.begin();
    while (it != vec_info.vec.end())
    {
		cout << "id: " << it->iPkgId
			 << ", exist: " << it->bExist << endl;
        it++;
    }

    cout << "=========================" << endl;
}
#endif

/**
 * @function   insert_vector_obj
 * @brief      向包裹队列中插入目标
 * @param[in]  vector<T> & vec  
 * @param[in]  T & obj          
 * @param[out] None
 * @return     static int
 */
template <typename T> 
static int insert_vector_obj(vector<T> & vec, T & obj)
{
	vec.push_back(obj);
	return DPM_OK;
}

template <typename T>
static bool check_vector_empty(vector<T> & vec)
{
	return vec.empty();
}

/**
 * @function   get_vector_obj
 * @brief      获取包裹队列中的目标
 * @param[in]  vector<T> & vec_info  
 * @param[in]  int idx               
 * @param[in]  T & obj               
 * @param[out] None
 * @return     static int
 */
template <typename T> 
static int get_vector_obj(vector<T> & vec_info, UINT idx, T & obj)
{
	if (idx >= vec_info.size())
	{
		cout << "invalid idx: " << idx << ", size: " << vec_info.size() << endl;
		return DPM_FAIL;
	}
	
	obj = vec_info[idx];
	return DPM_OK;
}

/**
 * @function   update_vector_obj
 * @brief      更新包裹队列中的目标
 * @param[in]  vector<T> & vec_info  
 * @param[in]  int idx               
 * @param[in]  T & obj               
 * @param[out] None
 * @return     static int
 */
template <typename T> 
static int update_vector_obj(vector<T> & vec_info, int idx, T & obj)
{
	vec_info[idx] = obj;
	return DPM_OK;
}

/**
 * @function   release_vector_obj
 * @brief      释放包裹队列目标
 * @param[in]  vector<pkg_info> & vec_info  
 * @param[in]  int iPkgId                   
 * @param[out] None
 * @return     static int
 */
static int release_vector_obj(vector<pkg_info> & vec_info, int iPkgId)
{
	vector<pkg_info>::iterator it = vec_info.begin();

	while(it != vec_info.end())
	{
		if (it->iPkgId == iPkgId)
		{
			it->bExist = false;
			return DPM_OK;
		}

		it++;
	}

	cout << "not found pkg " << iPkgId << endl;
	return DPM_FAIL;
}

/**
 * @function   cal_dual_pkg_iou_1d
 * @brief      计算双视角包裹IOU（一维）
 * @param[in]  DUAL_PKG_MATCH_RECT_INFO_S *pstMainViewRect  
 * @param[in]  DUAL_PKG_MATCH_RECT_INFO_S *pstSideViewRect  
 * @param[in]  float & fIou1d                               
 * @param[out] None
 * @return     static int
 */
static int cal_dual_pkg_iou_1d(DUAL_PKG_MATCH_RECT_INFO_S *pstMainViewRect, 
	                           DUAL_PKG_MATCH_RECT_INFO_S *pstSideViewRect, 
	                           float & fIou1d)
{
	float fInterSec = 0.0f;
	float fUnion = 0.0f;
	
	DPM_CHECK_RET(NULL == pstMainViewRect || NULL == pstSideViewRect, "ptr null!", DUAL_PKG_MATCH_ERR_NULL_PTR);

	fInterSec = GET_MIN(pstMainViewRect->x + pstMainViewRect->w, pstSideViewRect->x + pstSideViewRect->w) \
	            - GET_MAX(pstMainViewRect->x, pstSideViewRect->x);
	fUnion = GET_MAX(pstMainViewRect->x + pstMainViewRect->w, pstSideViewRect->x + pstSideViewRect->w) \
	         - GET_MIN(pstMainViewRect->x, pstSideViewRect->x);

	/* 若双视角包裹无交叠，iou置为0 */
	if (fInterSec < fabs(1e-6))
	{
		fIou1d = 0.0;
	}
	else
	{
		fIou1d = FLOAT_MULTI_A_THOUSAND(fInterSec) / FLOAT_MULTI_A_THOUSAND(fUnion);
	}
	
	return DPM_OK;
}

/**
 * @function   get_best_match_pair
 * @brief      获取最佳包裹匹配对
 * @param[in]  const bool bNeedBestMatch                
 * @param[in]  const pkg_info & stPkgInfo               
 * @param[in]  DUAL_PKG_MATCH_RESULT_S *pstMatchResult  
 * @param[out] None
 * @return     static int
 */
static int get_best_match_pair(const bool bNeedBestMatch,
	                           pkg_info & stPkgInfo,
	                           DUAL_PKG_MATCH_RESULT_S *pstMatchResult)
{
	int iMainPkgId = 0;
	int iSidePkgId = 0;
	float fMaxIou = 0.0;

	vector<DUAL_PKG_MATCH_LIST_INFO_S>::iterator it = stPkgInfo.vec.begin();

	pstMatchResult->bHasMatch = bNeedBestMatch;
	if (pstMatchResult->bHasMatch)
	{
		iMainPkgId = stPkgInfo.iPkgId;

		while(it != stPkgInfo.vec.end())
		{	
			/* 若找到满足交叠情况的匹配对，直接break向外层输出匹配结果 */
			if (it->bPkgOverlay)
			{
				iSidePkgId = it->iPkgId;
				break;
			}
			
			if (it->fMatchIou1d > fMaxIou)
			{
				fMaxIou = it->fMatchIou1d;
				iSidePkgId = it->iPkgId;
			}
			
			it++;
		}

		pstMatchResult->auiMatchId[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_MAIN_VIEW)] = iMainPkgId;
		pstMatchResult->auiMatchId[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_SIDE_VIEW)] = iSidePkgId;
	}
	
	return DPM_OK;
}

/**
 * @function   PkgMatch.PkgMatch
 * @brief      构造函数
 * @param[out] None
 * @return     PkgMatch::
 */
PkgMatch::PkgMatch() : _cnt(0), _match_thresh(DUAL_PKG_MATCH_DEFAULT_THRES_1D)
{
    /* do nothing... */
}

/**
 * @function   PkgMatch.PkgMatch
 * @brief      构造函数
 * @param[in]  const DUAL_PKG_MATCH_INIT_PRM_S *pstInitPrm  
 * @param[out] None
 * @return     PkgMatch::
 */
PkgMatch::PkgMatch(const DUAL_PKG_MATCH_INIT_PRM_S *pstInitPrm)
    : _cnt(pstInitPrm->iInitChnCnt), _match_thresh(DUAL_PKG_MATCH_DEFAULT_THRES_1D)
{
	int iChnIdx = 0;

    cout << "init chn cnt: " << _cnt << endl;

    for (UINT i = 0; i < _cnt; i++)
    {
    	iChnIdx = GET_VIEW_2_CHN_MAP(pstInitPrm->astInitChnPrm[i].enViewType);
        _vecInfo[iChnIdx].stQuePrm.uiQueDepth = pstInitPrm->astInitChnPrm[i].stQuePrm.uiQueDepth;
    }

    cout << "constructor end!" << endl;
}

/**
 * @function   PkgMatch.~PkgMatch
 * @brief      析构函数
 * @param[out] None
 * @return     PkgMatch::
 */
PkgMatch::~PkgMatch()
{
    for (UINT i = 0; i < _cnt; i++)
    {
    	vector<pkg_info>::iterator it = _vecInfo[i].vec.begin();
		while(it != _vecInfo[i].vec.end())
		{
			it->vec.swap(it->vec);
			it++;
		}
		_vecInfo[i].vec.swap(_vecInfo[i].vec);
    }

    cout << "destructor end!" << endl;
}

/**
 * @function   PkgMatch.insert
 * @brief      向特定视角的包裹队列中插入新目标
 * @param[in]  int idx        
 * @param[in]  pkg_info& obj  
 * @param[out] None
 * @return     int
 */
inline
int PkgMatch::insert(int idx, pkg_info& obj)
{
    /* checker1: 索引值校验 */
	if (idx >= DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM)
	{
        cout << "invalid idx: " << idx << " >= DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM" << endl;
		return DUAL_PKG_MATCH_ERR_ILLEGAL_VIEW_TYPE;
	}
		
    /* checker2: if vec size need expand, warning but continue */
    if (_vecInfo[idx].vec.size() >= _vecInfo[idx].stQuePrm.uiQueDepth)
    {
        cout << "queue size need expand! current len: " << _vecInfo[idx].stQuePrm.uiQueDepth << endl;
    }

    _vecInfo[idx].vec.push_back(obj);
    //pr_vec(_vecInfo[idx]); /* debug print vector info */

    return DPM_OK;
}

/**
 * @function   PkgMatch.update
 * @brief      更新包裹容器信息
 * @param[in]  void  
 * @param[out] None
 * @return     int
 */
inline
int PkgMatch::update(void)
{
	for (UINT i = 0; i < _cnt; i++)
	{
	    vector<pkg_info>::iterator it = _vecInfo[i].vec.begin();
	    while (it != _vecInfo[i].vec.end())
	    {
	        if (false == it->bExist)
	        {
	            _vecInfo[i].vec.erase(it);
	        }
	        else
	        {
	            it++;
	        }
	    }
		
	    //pr_vec(_vecInfo[i]);
	}

    return DPM_OK;
}

/**
 * @function   PkgMatch.match
 * @brief      执行一次包裹匹配
 * @param[in]  const DUAL_PKG_MATCH_CTRL_PRM_S *pstCtrlPrm  
 * @param[in]  DUAL_PKG_MATCH_RESULT_S *pstMatchResult      
 * @param[out] None
 * @return     int
 */
inline
int PkgMatch::match(const DUAL_PKG_MATCH_CTRL_PRM_S *pstCtrlPrm,
                    DUAL_PKG_MATCH_RESULT_S *pstMatchResult)
{
    int iRet = DPM_FAIL;

	UINT i,j;
	float fIou1d;
	bool bNeedBestMatch = false;
	
	DUAL_PKG_MATCH_RECT_INFO_S stMainViewRect = {0};
	DUAL_PKG_MATCH_RECT_INFO_S stSideViewRect = {0};

	pkg_info main_view_obj_info;
	pkg_info side_view_obj_info;
	DUAL_PKG_MATCH_LIST_INFO_S stMatchListInfo = {0};

    /* checker1: if 2 channels are initialized */
    if (DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM != _cnt)
    {
        cout << "cnt of initialized channels is " << _cnt << " less than 2!" << endl;
        return DUAL_PKG_MATCH_ERR_STATIC_PRM;
    }

	/* 若两个视角包裹队列任一为空，直接返回成功无需进行后续处理 */
	if (check_vector_empty(_vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_MAIN_VIEW)].vec)
		|| check_vector_empty(_vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_MAIN_VIEW)].vec))
	{
		return DPM_OK;
	}

	/*
	  策略说明:
	  a. 对于当前主视角队列中的所有包裹依次取出，遍历所有侧视角队列中的包裹，寻找是否存在匹配对，匹配结果返回外部业务模块
	  b. 队列中旧包裹在遍历完成后会进行更新删除。
	*/
	i = 0;
	while(i < _vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_MAIN_VIEW)].vec.size())
	{	
		/* reset */
		bNeedBestMatch = false;
	
		/* get main view obj */ 
		iRet = get_vector_obj(_vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_MAIN_VIEW)].vec, i, main_view_obj_info);
		DPM_CHECK_RET(DPM_OK != iRet, "get que obj failed!", DPM_FAIL);
		
		iRet = pstCtrlPrm->pProcFunc(pstCtrlPrm->pRawOutInfo, 
									 DUAL_PKG_MATCH_MAIN_VIEW, 
									 main_view_obj_info.iPkgId,
									 &stMainViewRect);
		if (DPM_OK != iRet)
		{
			//cout << "get main pkg rect failed! id: " << main_view_obj_info.iPkgId << endl;
		
			/* set pkg obj as unexist */
			main_view_obj_info.bExist = false;
			update_vector_obj(_vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_MAIN_VIEW)].vec, i, main_view_obj_info);

			i++;
			continue;
		}

		{
			j = 0;
			/* as to each obj in side view queue, we do match operation */
			while(j < _vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_SIDE_VIEW)].vec.size())
			{	
				/* get side view obj */
				iRet = get_vector_obj(_vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_SIDE_VIEW)].vec, j, side_view_obj_info);
				DPM_CHECK_RET(DPM_OK != iRet, "get que obj failed!", DPM_FAIL);

				iRet = pstCtrlPrm->pProcFunc(pstCtrlPrm->pRawOutInfo, 
					                        DUAL_PKG_MATCH_SIDE_VIEW, 
					                        side_view_obj_info.iPkgId,
					                        &stSideViewRect);
				if (DPM_OK != iRet)
				{
					//cout << "get side pkg rect failed! id: " << side_view_obj_info.iPkgId << endl;

					/* set pkg obj as unexist */
					side_view_obj_info.bExist = false;
					update_vector_obj(_vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_SIDE_VIEW)].vec, j, side_view_obj_info);

					j++;
					continue;
				}
				
				fIou1d = 0.0f;
				iRet = cal_dual_pkg_iou_1d(&stMainViewRect, &stSideViewRect, fIou1d);
				if (DPM_OK == iRet)
				{
					/* 若满足1d匹配阈值，则放入链表 */
					if (fIou1d > _match_thresh)
					{
						stMatchListInfo.iPkgId = side_view_obj_info.iPkgId;
						stMatchListInfo.fMatchIou1d = fIou1d;
						stMatchListInfo.bPkgOverlay = false;

						cout << "j: " << j << ", get match iou 1d: " << fIou1d 
							 << ", thresh: " << _match_thresh
							 << endl;
						
						/* 增加匹配包裹链表 */
						iRet = insert_vector_obj(main_view_obj_info.vec, stMatchListInfo);
						if (DPM_OK == iRet)
						{
							/* 标记需要对匹配链表执行最佳匹配 */ 
							bNeedBestMatch = true;
						}
					}
					/* 包裹Y方向堆叠时，考虑主视角包裹X方向中心是否在侧视角X范围内 */
					else if ((stMainViewRect.x + stMainViewRect.w / 2.0) > stSideViewRect.x
						     && (stMainViewRect.x + stMainViewRect.w / 2.0) < stSideViewRect.x + stSideViewRect.w)
					{
						stMatchListInfo.iPkgId = side_view_obj_info.iPkgId;
						stMatchListInfo.fMatchIou1d = fIou1d;
						stMatchListInfo.bPkgOverlay = true;
					
						cout << "j: " << j << ", get pkg overlay! match iou 1d: " << fIou1d 
							 << ", thresh: " << _match_thresh
							 << endl;

						/* 增加匹配包裹链表 */
						iRet = insert_vector_obj(main_view_obj_info.vec, stMatchListInfo);
						if (DPM_OK == iRet)
						{
							/* 标记需要对匹配链表执行最佳匹配 */ 
							bNeedBestMatch = true;
						}
					}
				}

				j++;
			}

			/* 获取包裹最佳匹配对 */
			iRet = get_best_match_pair(bNeedBestMatch, main_view_obj_info, pstMatchResult);
			DPM_CHECK_RET(DPM_OK != iRet, "get que obj failed!", DPM_FAIL);	

			/* 若包裹匹配成功，需要将对应id的包裹从匹配队列中删除 */
			if (pstMatchResult->bHasMatch)
			{
				cout << "after best match! " << pstMatchResult->bHasMatch
					 << ", main: " << pstMatchResult->auiMatchId[0]
					 << ", side: " << pstMatchResult->auiMatchId[1] << endl;
				
				release_vector_obj(_vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_MAIN_VIEW)].vec, pstMatchResult->auiMatchId[0]);

				/* 侧视角的包裹在匹配成功后不进行释放，考虑到Y方向存在包裹堆叠的情况，侧视角的包裹目标需要多次匹配 */
				//release_vector_obj(_vecInfo[GET_VIEW_2_CHN_MAP(DUAL_PKG_MATCH_SIDE_VIEW)].vec, pstMatchResult->auiMatchId[1]);

				/* 单帧仅返回一组匹配结果，若有匹配对则退出处理，剩余主视角包裹交由下一帧处理 */
				break;
			}	
		}

		i++;
	}

    return DPM_OK;
}

/**
 * @function   PkgMatch.set_match_thresh
 * @brief      设置匹配阈值，默认0.85
 * @param[in]  float thresh = DUAL_PKG_MATCH_DEFAULT_THRES_1D  
 * @param[out] None
 * @return     int PkgMatch::
 */
inline
int PkgMatch::set_match_thresh(float thresh = DUAL_PKG_MATCH_DEFAULT_THRES_1D)
{
	if (thresh < 0.0 || thresh > 1.0)
	{
		thresh = DUAL_PKG_MATCH_DEFAULT_THRES_1D;
		cout << "invalid thresh: " << thresh << ", use default 0.9!" << endl;
	}

	_match_thresh = thresh;
	
	cout << "set match thresh: " << thresh << endl;	
	return DPM_OK;
}

/**
 * @function   PkgMatch.print_debug_status
 * @brief      打印模块调试状态
 * @param[in]  void  
 * @param[out] None
 * @return     int PkgMatch::
 */
inline
int PkgMatch::print_debug_status(void)
{
	for (UINT i = 0; i < _cnt; i++)
	{		
		pr_vec(_vecInfo[i]);
	}

	return DPM_OK;
}

/**
 * @function   dpm_init
 * @brief      包裹匹配模块初始化
 * @param[in]  DUAL_PKG_MATCH_INIT_PRM_S *pstInitPrm
 * @param[out] None
 * @return     int
 */
int dpm_init(const DUAL_PKG_MATCH_INIT_PRM_S *pstInitPrm)
{
	DPM_CHECK_RET(NULL == pstInitPrm, "pstInitPrm == null!", DUAL_PKG_MATCH_ERR_NULL_PTR);
	
	if (1 == g_stInnerInfo.bInit)
	{
		return DPM_OK;
	}

	g_stInnerInfo.pPkgMatch = new PkgMatch(pstInitPrm);
	DPM_CHECK_RET(NULL == g_stInnerInfo.pPkgMatch, "alloc mem failed!", DUAL_PKG_MATCH_ERR_ALLOC_MEM);	

	g_stInnerInfo.bInit = 1;
	
    cout << "dpm init end! ptr: " << g_stInnerInfo.pPkgMatch << endl;
    return DPM_OK;
}

/**
 * @function   dpm_deinit
 * @brief      包裹匹配模块去初始化
 * @param[in]  void
 * @param[out] None
 * @return     int
 */
int dpm_deinit(void)
{
	if (NULL == g_stInnerInfo.pPkgMatch)
	{
		cout << "global ptr null!" << endl;
		return DPM_FAIL;
	}

	cout << "before delete! ptr: " << g_stInnerInfo.pPkgMatch << endl;
	
	delete reinterpret_cast<PkgMatch *>(g_stInnerInfo.pPkgMatch);
	g_stInnerInfo.bInit = 0;
	
    cout << "dpm deinit end!" << endl;
    return DPM_OK;
}

/**
 * @function   dpm_insert_pkg_queue
 * @brief      向包裹匹配队列插入新包裹
 * @param[in]  DUAL_PKG_MATCH_VIEW_TYPE_E enViewType        
 * @param[in]  DUAL_PKG_MATCH_OUTER_PKG_INFO_S *pstPkgInfo  
 * @param[out] None
 * @return     int
 */
int dpm_insert_pkg_queue(DUAL_PKG_MATCH_VIEW_TYPE_E enViewType,
                         DUAL_PKG_MATCH_OUTER_PKG_INFO_S *pstPkgInfo)
{
    int iRet = DPM_FAIL;
	
	PkgMatch *pstPkgMatch = reinterpret_cast<PkgMatch *>(g_stInnerInfo.pPkgMatch);
	pkg_info stPkgInfo = {0};

	DPM_CHECK_RET(NULL == pstPkgInfo, "pstPkgInfo == null!", DUAL_PKG_MATCH_ERR_NULL_PTR);
	
	stPkgInfo.bExist = 1;
	stPkgInfo.iPkgId = pstPkgInfo->uiPkgId;

	iRet = pstPkgMatch->insert(GET_VIEW_2_CHN_MAP(enViewType), stPkgInfo);

    cout << "dpm insert pkg queue end!, view: " << enViewType << ", id: " << pstPkgInfo->uiPkgId << endl;
    return iRet;
}

/**
 * @function   dpm_do_pkg_match
 * @brief      进行包裹匹配处理      
 * @param[in]  DUAL_PKG_MATCH_CTRL_PRM_S *pstCtrlPrm        
 * @param[out] None
 * @return     int
 */
int dpm_do_pkg_match(DUAL_PKG_MATCH_CTRL_PRM_S *pstCtrlPrm, DUAL_PKG_MATCH_RESULT_S *pstMatchResult)
{
    int iRet = DPM_FAIL;
	
	PkgMatch *pstPkgMatch = reinterpret_cast<PkgMatch *>(g_stInnerInfo.pPkgMatch);

	DPM_CHECK_RET(NULL == pstCtrlPrm || NULL == pstCtrlPrm->pRawOutInfo || NULL == pstCtrlPrm->pProcFunc || NULL == pstMatchResult, 
		          "ptr null!", 
		          DUAL_PKG_MATCH_ERR_NULL_PTR);

	iRet = pstPkgMatch->match(pstCtrlPrm, pstMatchResult);

	//if (pstMatchResult->bHasMatch)
	{
		/* 每一次包裹匹配都需要更新匹配队列 */
		iRet |= pstPkgMatch->update();
	}

    return iRet;
}

/**
 * @function   dpm_set_match_thresh
 * @brief      设置包裹匹配阈值
 * @param[in]  float thresh  
 * @param[out] None
 * @return     int
 */
int dpm_set_match_thresh(float thresh)
{
	PkgMatch *pstPkgMatch = reinterpret_cast<PkgMatch *>(g_stInnerInfo.pPkgMatch);
	return pstPkgMatch->set_match_thresh(thresh);
}

/**
 * @function   dpm_print_debug_status
 * @brief      打印匹配模块调试信息
 * @param[in]  void  
 * @param[out] None
 * @return     int
 */
int dpm_print_debug_status(void)
{
	PkgMatch *pstPkgMatch = reinterpret_cast<PkgMatch *>(g_stInnerInfo.pPkgMatch);
	return pstPkgMatch->print_debug_status();
}

