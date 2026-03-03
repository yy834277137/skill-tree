/*
 * @lc app=leetcode.cn id=278 lang=cpp
 * @lcpr version=30400
 *
 * [278] 第一个错误的版本
 */

// @lc code=start
// The API isBadVersion is defined for you.
// bool isBadVersion(int version);

class Solution {
public:
    int firstBadVersion(int n) {
        int left = 1, right = n;
        while (left <= right)
        {
            int mid = left + (right - left) / 2;
            if (!isBadVersion(mid)) left = mid + 1;
            else
                right = mid - 1;
        }
        return left;
    }
};