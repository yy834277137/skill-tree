class Solution {
public:
    int maxSideLength(vector<vector<int>>& mat, int threshold) {
        int sMaxLen = std::min(mat[0].size(), mat.size());
        int sMinLen = 1;
        return binary(mat, threshold, sMinLen, sMaxLen);   
    }

private:
    int binary(vector<vector<int>>& mat, int threshold, int low, int high)
    {
        int mid = 0, ans = 0;
        while (low <= high)
        {
            mid = low + (high - low) / 2;
            if (valid(mat, threshold, mid))
            {
                ans = mid;
                low = mid + 1;
            }
            else
            {
                high = mid - 1;
            }
        }
        return ans;
    }

    int valid(vector<vector<int>>& mat, int threshold, int subLen)
    {
        int m = mat.size(), n = mat[0].size();

        if (subLen == 0) return true;
        vector<vector<int>> pre(m + 1, vector<int>(n + 1, 0));
        for (int i = 1; i <= m; i++)
        {
            for (int j = 1; j <= n; j++)
            {
                pre[i][j] = pre[i - 1][j] + pre[i][j - 1] - pre[i - 1][j - 1] + mat[i -1][j - 1];
            }
        }

        // 这里是找从最小边长开始, 是否存在满足sum < threshold的正方形, 如果存在, 则证明这个边长满足要求
        for (int i = subLen; i <= m; i++)
        {
            for (int j = subLen; j <= n; j++)
            {
                int sSum = pre[i][j] - pre[i - subLen][j] - pre[i][j - subLen] + pre[i - subLen][j - subLen];
                if (sSum <= threshold) return true;
            }
        }

        return false;
    }
};