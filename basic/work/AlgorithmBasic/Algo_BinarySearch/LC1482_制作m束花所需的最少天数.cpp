/*
 * @lc app=leetcode.cn id=1482 lang=cpp
 * @lcpr version=30400
 *
 * [1482] 制作 m 束花所需的最少天数
 */
#include <iostream>
#include <vector>
#include <string>
#include "./common/ListNode.cpp"
#include "./common/TreeNode.cpp"
// @lc code=start
class Solution {
public:
    int minDays(vector<int>& bloomDay, int m, int k) {
        // long long size = static_cast<long long>(m) * static_cast<long long>(k);
        // 规避溢出风险
        if (1LL * m * k > bloomDay.size())    return -1;
        int minDays = INT_MAX, maxDays = INT_MIN;
        for (int& day : bloomDay)
        {
            minDays = min(minDays, day);
            maxDays = max(maxDays, day);
        }

        return binary(bloomDay, m, k, minDays, maxDays);
    }

private:
    int binary(vector<int>& days, int m, int k, int low, int high)
    {
        int mid = 0, ans = 0;
        while (low <= high)
        {
            mid = low + (high - low) / 2;
            if (valid(days, m, k, mid))
            {
                ans = mid;
                high = mid - 1;
            }
            else
            {
                low = mid + 1;
            }
        }
        return ans;
    }

    bool valid(vector<int>& days, int m, int k, int subDays)
    {
        int curBloomGroup = 0, curBloomSize = 0;
        for (int d : days)
        {
            if (d <= subDays)
            {
                curBloomSize++;
                if (curBloomSize == k)
                {
                    curBloomGroup++;
                    curBloomSize = 0;
                }
            }
            else    // 只要有一个不相邻, 当前花束归0
            {
                curBloomSize = 0;
            }
        }
        return curBloomGroup >= m;
    }
};
// @lc code=end



/*
// @lcpr case=start
// [1,10,3,10,2]\n3\n1\n
// @lcpr case=end

// @lcpr case=start
// [1,10,3,10,2]\n3\n2\n
// @lcpr case=end

// @lcpr case=start
// [7,7,7,7,12,7,7]\n2\n3\n
// @lcpr case=end

 */

