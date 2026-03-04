/*
 * @lc app=leetcode.cn id=283 lang=cpp
 * @lcpr version=30307
 *
 * [283] 移动零
 */
#include <iostream>
#include <vector>
#include <string>
#include "./common/ListNode.cpp"
#include "./common/TreeNode.cpp"
// @lc code=start
class Solution {
public:
    void moveZeroes(vector<int>& nums) {
        int left = 0, right = 0;
        while (right < nums.size())
        {
            if (nums[right] != 0)
            {
                nums[left] = nums[right];
                left++;
            }
            right++;
        }
        for (int i = left; i < nums.size(); i++)
        {
            nums[i] = 0;
        }
    }
};
// @lc code=end



/*
// @lcpr case=start
// [0,1,0,3,12]\n
// @lcpr case=end

// @lcpr case=start
// [0]\n
// @lcpr case=end

 */

