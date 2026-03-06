/*
 * @lc app=leetcode.cn id=102 lang=cpp
 * @lcpr version=30400
 *
 * [102] 二叉树的层序遍历
 */
#include <iostream>
#include <vector>
#include <string>
#include "./common/ListNode.cpp"
#include "./common/TreeNode.cpp"
// @lc code=start
/**
 * Definition for a binary tree node.
 * struct TreeNode {
 *     int val;
 *     TreeNode *left;
 *     TreeNode *right;
 *     TreeNode() : val(0), left(nullptr), right(nullptr) {}
 *     TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
 *     TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
 * };
 */
class Solution {
public:
    vector<vector<int>> levelOrder(TreeNode* root) {
        vector<vector<int>> retVal;
        queue<TreeNode*> qLevelQueue;
        if (root == nullptr)
        {
            return retVal;
        }

        qLevelQueue.push(root);
        while (!qLevelQueue.empty())
        {
            int sCurLevelSize = qLevelQueue.size();
            vector<int> sVcCurrentLevel;
            for (int i = 0; i < sCurLevelSize; i++)
            {
                TreeNode* curNode = qLevelQueue.front();
                qLevelQueue.pop();
                sVcCurrentLevel.push_back(curNode->val);

                if (curNode->left)
                {
                    qLevelQueue.push(curNode->left);
                }

                if (curNode->right)
                {
                    qLevelQueue.push(curNode->right);
                }
            }
            retVal.push_back(sVcCurrentLevel);
        }

        return retVal;
    }
};
// @lc code=end


#include <vector>

// ... (TreeNode definition)

class Solution {
public:
    std::vector<std::vector<int>> levelOrder(TreeNode* root) {
        std::vector<std::vector<int>> res;
        dfs(root, res, 0);
        return res;
    }

private:
    void dfs(TreeNode* root, std::vector<std::vector<int>>& res, int height) {
        if (nullptr == root) {
            return;
        }

        // 如果当前层级是第一次达到，为它创建一个新的 vector
        if (height == res.size()) {
            res.push_back(std::vector<int>());
        }

        // 将当前节点的值加入到它所在的层级
        res[height].push_back(root->val);

        // 递归处理左子树和右子树，层级加 1
        dfs(root->left, res, height + 1);
        dfs(root->right, res, height + 1);
    }
};


/*
// @lcpr case=start
// [3,9,20,null,null,15,7]\n
// @lcpr case=end

// @lcpr case=start
// [1]\n
// @lcpr case=end

// @lcpr case=start
// []\n
// @lcpr case=end

 */

