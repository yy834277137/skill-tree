class Solution {
public:
    int maxDistance(vector<int>& position, int m) {
        sort(position.begin(), position.end());
        int minDist = 1;
        int maxDist = position.back() - position.front();

        return binary(position, m, minDist, maxDist);
    }
private:
    int binary(vector<int>& nums, int count, int low, int high)
    {
        int mid = 0, ans = 0;
        while (low <= high)
        {
            mid = low + (high - low) / 2;
            if (valid(nums, count, mid)) 
            {
                low = mid + 1;
                ans = mid;
            }
            else high = mid - 1;
        }
        return ans; // 只返回合理的ans
    }

    bool valid(vector<int>& nums, int m, int subDist)
    {
        int lastPostion = 0, curCount = 1;
        for (int i = 1; i < nums.size(); i++)
        {
            // 如果当前间距大于预估的间距, 则说明当前的间距预估的有点小
            if (nums[i] - nums[lastPostion] >= subDist) 
            {
                curCount++;
                lastPostion = i;
                // 如果curCount >= m 说明当前完全可以放下m个球 但间距还可以更大
                // 比m多的球都可以放的下 说明 m个球肯定可以放的下
                if (curCount >= m) return true;
            }
        }
        return false;
    }
};