/*
 * @lc app=leetcode.cn id=200 lang=cpp
 * @lcpr version=30400
 *
 * [200] 岛屿数量
 */
#include <iostream>
#include <vector>
#include <string>
#include "./common/ListNode.cpp"
#include "./common/TreeNode.cpp"
// @lc code=start
class Solution {
public:
    int numIslands(std::vector<std::vector<char>>& grid) {
        if (grid.empty() || grid[0].empty()) {
            return 0;
        }
        int height = grid.size();
        int width = grid[0].size();
        int res = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (grid[i][j] == '1') {
                    res++;
                    bfs(grid, i, j);
                }
            }
        }
        return res;
    }

private:
    void bfs(std::vector<std::vector<char>>& grid, int row, int col) {
        std::queue<std::pair<int, int>> q;
        q.push({row, col});
        // Mark as visited immediately upon adding to the queue
        grid[row][col] = '0'; 

        int height = grid.size();
        int width = grid[0].size();

        // Directions for up, down, left, right
        int dr[] = {-1, 1, 0, 0};
        int dc[] = {0, 0, -1, 1};

        while (!q.empty()) {
            std::pair<int, int> curr = q.front();
            q.pop();

            // Explore neighbors
            for (int i = 0; i < 4; ++i) {
                int next_row = curr.first + dr[i];
                int next_col = curr.second + dc[i];

                // Check for valid next cell
                if (next_row >= 0 && next_row < height &&
                    next_col >= 0 && next_col < width &&
                    grid[next_row][next_col] == '1') 
                {
                    q.push({next_row, next_col});
                    // Mark as visited to prevent re-adding
                    grid[next_row][next_col] = '0';
                }
            }
        }
    }

    void dfs(vector<vector<char>>grid, int row, int col)
    {
        if (row < 0 || row >= grid.size() || col < 0 || col >= grid[0].size()) return;
        grid[row][col] = 0;
        int dr[] = {-1, 1, 0, 0};
        int dc[] = {0, 0, -1, 1};
        int height = grid.size();
        int width = grid[0].size();
        for (int i = 0; i < 4; ++i) {
            int next_row = row + dr[i];
            int next_col = col + dc[i];

            // Check for valid next cell
            if (next_row >= 0 && next_row < height &&
                next_col >= 0 && next_col < width &&
                grid[next_row][next_col] == '1') 
            {
                dfs(grid, next_row, next_col);
            }
        }
    }
};
// @lc code=end



/*
// @lcpr case=start
// [["1","1","1","1","0"],["1","1","0","1","0"],["1","1","0","0","0"],["0","0","0","0","0"]]\n
// @lcpr case=end

// @lcpr case=start
// [["1","1","0","0","0"],["1","1","0","0","0"],["0","0","1","0","0"],["0","0","0","1","1"]]\n
// @lcpr case=end

 */

