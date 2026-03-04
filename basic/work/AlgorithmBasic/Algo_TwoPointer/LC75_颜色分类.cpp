/*
 * @lc app=leetcode.cn id=75 lang=cpp
 * @lcpr version=30400
 *
 * [75] 颜色分类
 */
#include <iostream>
#include <vector>
#include <string>
#include "./common/ListNode.cpp"
#include "./common/TreeNode.cpp"
// @lc code=start

// move0的方式 两次循环
// class Solution {
// public:
//     void sortColors(vector<int>& nums) {
//         int left = 0, right = 0;
//         while (right < nums.size())
//         {
//             if (nums[right] != 2)
//             {
//                 nums[left] = nums[right];
//                 left++;
//             }
//             right++;
//         }

//         int sTwoStart = left;
//         for (int i = sTwoStart; i < nums.size(); i++)
//         {
//             nums[i] = 2;
//         }

//         left = 0, right = 0;
//         while (right < sTwoStart)
//         {
//             if (nums[right] != 1)
//             {
//                 nums[left] = nums[right];
//                 left++;
//             }
//             right++;
//         }
//         for (int i = left; i < sTwoStart; i++)
//         {
//             nums[i] = 1;
//         }
//     }
// };


// 一次遍历的方式
class Solution {
public:
    void sortColors(vector<int>& nums) {
        int p0 = 0, p2 = nums.size() - 1, pWork = 0;
        while (pWork <= p2)
        {
            // p0左边都是0 [0, p0), 所以两个指针都++
            if (nums[pWork] == 0)
            {
                std::swap(nums[p0], nums[pWork]);
                p0++;
                pWork++;
            }
            // pWork在中间 直接++
            else if (nums[pWork] == 1)
            {
                pWork++;
            }
            // p2的右边都是2 但是swap后的pWork不一定是什么 还需要再判断一次
            else if (nums[pWork] == 2)
            {
                std::swap(nums[pWork], nums[p2]);
                p2--;
            }
        }
    }
};

// @lc code=end



/*
// @lcpr case=start
// [2,0,2,1,1,0]\n
// @lcpr case=end

// @lcpr case=start
// [2,0,1]\n
// @lcpr case=end

 */

