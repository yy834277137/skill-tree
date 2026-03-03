class Solution {
public:
    int smallestDivisor(vector<int>& nums, int threshold) {
        int minDivisor = 1, maxDivisor = nums[0];
        for (int& num : nums)
        {
            maxDivisor = max(maxDivisor, num);
        }
        return binary(nums, threshold, minDivisor, maxDivisor);
    }

private:
    int binary(vector<int>& nums, int threshold, int low, int high)
    {
        int mid = 0, ans = high;
        while (low <= high)
        {
            mid = low + (high - low) / 2;
            if (valid(nums, threshold, mid))
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

    bool valid(vector<int>& nums, int threshold, int subDivisor)
    {
        if (0 >= subDivisor) return false;

        long long curSum = 0, curAns = 0;
        for (int& num : nums)
        {
            curSum += (num + subDivisor - 1) / subDivisor;
            if (curSum > threshold) return false;
        }

        return curSum <= threshold;
    }
};