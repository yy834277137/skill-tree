/*
 * @lc app=leetcode.cn id=9 lang=cpp
 * @lcpr version=30400
 *
 * [9] 回文数
 */
#include <iostream>
#include <vector>
#include <string>
#include "./common/ListNode.cpp"
#include "./common/TreeNode.cpp"
// @lc code=start
class Solution {
public:
    bool isPalindrome(int x) {
        if  (x < 0) return false;
        int temp = x;
        long long y = 0;
        while (temp > 0)
        {
            int last_num = temp % 10;
            temp = temp / 10;
            y = y * 10 + last_num;
        }

        return y == x;
    }
};
// @lc code=end



/*
// @lcpr case=start
// 121\n
// @lcpr case=end

// @lcpr case=start
// -121\n
// @lcpr case=end

// @lcpr case=start
// 10\n
// @lcpr case=end

 */

