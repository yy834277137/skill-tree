/*
 * @lc app=leetcode.cn id=142 lang=cpp
 * @lcpr version=30202
 *
 * [142] 环形链表 II
 */
#include <iostream>
#include <vector>
#include <string>
#include "./common/ListNode.cpp"
#include "./common/TreeNode.cpp"
// @lc code=start
/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     ListNode *next;
 *     ListNode(int x) : val(x), next(NULL) {}
 * };
 */
class Solution {
public:
    ListNode *detectCycle(ListNode *head) {
        ListNode *pSlow = head, *pFast = head;

        while(pFast && pFast->next)
        {
            pSlow = pSlow->next;
            pFast = pFast->next->next;
            if (pSlow == pFast)
            {
                pSlow = head;
                while (pSlow != pFast)
                {
                    pSlow = pSlow->next;
                    pFast = pFast->next;
                }
                return pSlow;
            }
        }
        return nullptr;
    }
};
// @lc code=end



/*
// @lcpr case=start
// [3,2,0,-4]\n1\n
// @lcpr case=end

// @lcpr case=start
// [1,2]\n0\n
// @lcpr case=end

// @lcpr case=start
// [1]\n-1\n
// @lcpr case=end

 */

