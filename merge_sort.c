#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void merge(int nums[], int tmp[], int startIndex, int midIndex, int endIndex)
{
    int i = startIndex, j = midIndex + 1, k = startIndex;
    while (i <= midIndex && j <= endIndex)
    {
        if (nums[i] < nums[j])
            tmp[k++] = nums[i++];
        else
            tmp[k++] = nums[j++];
    }
    while (i <= midIndex) tmp[k++] = nums[i++];
    while (j <= endIndex) tmp[k++] = nums[j++];
    
    for (i = 0; i <= endIndex; i++)
        nums[i] = tmp[i];
}

void merge_sort(int nums[], int tmp[], int startIndex, int endIndex)
{
    if (startIndex < endIndex)
    {
        int midIndex = (startIndex + endIndex) / 2;
        merge_sort(nums, tmp, startIndex, midIndex);
        merge_sort(nums, tmp, midIndex + 1, endIndex);
        merge(nums, tmp, startIndex, midIndex, endIndex);
    }
}

void non_recursion_merge_sort(int nums[], int tmp[], int numsSize)
{
    int i, startIndex, midIndex, endIndex;

    for (i = 1; i < numsSize; i *= 2)
    {
        for (startIndex = 0; startIndex < numsSize - i; startIndex = endIndex + 1)
        {
            midIndex = startIndex + i - 1;
            endIndex = midIndex + i;
            merge(nums, tmp, startIndex, midIndex, endIndex);
        }
    }
}

int main()
{
    int nums[] = {8,5,9,0,3,2,1,6,7,8,4,1,3,2,0,1};
    int *tmp = malloc(sizeof(nums));
    memset(tmp, 0, sizeof(nums));
    //non_recursion_merge_sort(nums, tmp, sizeof(nums) / sizeof(int));
    merge_sort(nums, tmp, 0, sizeof(nums) / sizeof(int) - 1);
    int i;
    for (i = 0; i < sizeof(nums) / sizeof(int); i++)
        printf("%d ", nums[i]);
    printf("\n");

    return 0;
}