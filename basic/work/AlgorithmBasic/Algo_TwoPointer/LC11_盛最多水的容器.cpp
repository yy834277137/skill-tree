/*
 * @lc app=leetcode.cn id=11 lang=cpp
 * @lcpr version=30400
 *
 * [11] 盛最多水的容器
 */
#include <iostream>
#include <vector>
#include <string>
#include "./common/ListNode.cpp"
#include "./common/TreeNode.cpp"
// @lc code=start
class Solution {
public:
    int maxArea(vector<int>& height) {
        int left = 0, right = height.size() - 1;
        int sMaxWaterSize = 0;
        while (left < right)
        {
            int sCurWater = (right - left) * std::min(height[left], height[right]);
            sMaxWaterSize = std::max(sMaxWaterSize, sCurWater);
            // 因为他小 所以把他换了之后说不定会变大
            if (height[left] < height[right])
            {
                left++;
            }
            else
            {
                right--;
            }
        }

        return sMaxWaterSize;
    }
};
// @lc code=end



/*
// @lcpr case=start
// [1,8,6,2,5,4,8,3,7]\n
// @lcpr case=end

// @lcpr case=start
// [1,1]\n
// @lcpr case=end

 */

