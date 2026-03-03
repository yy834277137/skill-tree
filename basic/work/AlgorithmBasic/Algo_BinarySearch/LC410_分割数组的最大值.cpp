class Solution {
public:
    int splitArray(vector<int>& nums, int k) {
        int maxElement = -1, maxSum = 0;
        for (int& num : nums)
        {
            maxElement = max(num, maxElement);
            maxSum += num;
        }

        return binary(nums, k, maxElement, maxSum);
    }

/*
 *  要找的是最小和, 即最小边界, 类似于找最小左边界或最大右边界的值, 
 *  这种情况返回满足条件的low或者high即可, 只有要在一堆数中找到确切的数时, 才返回mid 
 *  或用ans记录mid
 * 
*/
private:
    int binary(vector<int>& nums, int count, int low, int high)
    {
        int mid = 0;
        while (low <= high)
        {
            mid = low + (high - low) / 2;
            if (!valid(nums, count, mid))   low = mid + 1;
            else high = mid - 1;
        }
        return low; 
    }

    int valid(vector<int>& nums, int count, int subSum)
    {
        int sCurSum = 0, sCurCount = 1;
        for (int& num : nums)
        {
            sCurSum += num;
            if (sCurSum > subSum)
            {
                sCurSum = num;
                sCurCount++;
                if (sCurCount > count)  return false;
            }
        }
        return true;
    }
};