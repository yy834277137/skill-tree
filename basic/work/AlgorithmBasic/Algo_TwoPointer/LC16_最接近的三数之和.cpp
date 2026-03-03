// 双指针法

class Solution {
public:
    int threeSumClosest(vector<int>& nums, int target) {
        int res = nums[0] + nums[1] + nums[nums.size() - 1];
        sort(nums.begin(), nums.end());
        for (int i = 0; i < nums.size() - 2; i++)
        {
            int left = i + 1, right = nums.size() - 1;
            // 绝对不能等于, 因为需要三个数 所以left和right是唯一的
            while (left < right)
            {
                int sum = nums[i] + nums[left] + nums[right];
                if (abs(sum - target) < abs(res - target)) res = sum;
                if (sum > target) right--;
                else left++;
            }
        }
        return res;
    }
};

// 二分法
sort(nums.begin(), nums.end());
int res = initial_sum; // 初始化一个足够大的差值的和

// 第一层循环，固定 nums[i]
for (int i = 0; i < nums.size() - 2; ++i) {
    // 第二层循环，固定 nums[j]
    for (int j = i + 1; j < nums.size() - 1; ++j) {
        // 计算需要寻找的第三个数的目标值
        int complement = target - nums[i] - nums[j];

        // 在 nums[j+1] 之后的部分二分查找 complement
        auto it = lower_bound(nums.begin() + j + 1, nums.end(), complement);

        // 检查 lower_bound 找到的元素
        if (it != nums.end()) {
            int sum = nums[i] + nums[j] + *it;
            if (abs(sum - target) < abs(res - target)) {
                res = sum;
            }
        }

        // 检查它前面的那个元素（如果存在）
        if (it != nums.begin() + j + 1) {
            int sum = nums[i] + nums[j] + *(--it);
            if (abs(sum - target) < abs(res - target)) {
                res = sum;
            }
        }
    }
}
return res;

// 二分法需要定两个值找第三个, 双指针只需要定一个即可