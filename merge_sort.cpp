#include "precomp.h" // include this in every .cpp file

namespace Tmpl8
{
    // -----------------------------------------------------------
    // Sort tanks by health value using merge sort
    // -----------------------------------------------------------
    void MergeSort::sort_tanks_health(const std::vector<Tank>& original, std::vector<const Tank*>& sorted_tanks, int begin, int end)
    {
        const int NUM_TANKS = end - begin;
        sorted_tanks.clear();
        sorted_tanks.reserve(NUM_TANKS);

        // Base case: if only one tank or no tanks, just add it to the result
        if (NUM_TANKS <= 1)
        {
            if (NUM_TANKS == 1)
            {
                sorted_tanks.push_back(&original.at(begin));
            }
            return;
        }

        // Divide the array into two halves
        int mid = begin + NUM_TANKS / 2;

        // Create temporary vectors for the two halves
        std::vector<const Tank*> left_half;
        std::vector<const Tank*> right_half;

        // Recursively sort both halves
        sort_tanks_health(original, left_half, begin, mid);
        sort_tanks_health(original, right_half, mid, end);

        // Merge the sorted halves
        merge_tanks_by_health(sorted_tanks, left_half, right_half);
    }

    // -----------------------------------------------------------
    // Merge two sorted vectors of tank pointers by health
    // -----------------------------------------------------------
    void MergeSort::merge_tanks_by_health(std::vector<const Tank*>& sorted_tanks, std::vector<const Tank*>& left, std::vector<const Tank*>& right)
    {
        size_t left_index = 0;
        size_t right_index = 0;

        // While there are still elements in both arrays
        while (left_index < left.size() && right_index < right.size())
        {
            // Compare health values and add the tank with lower health to the result
            if (left[left_index]->compare_health(*right[right_index]) <= 0)
            {
                sorted_tanks.push_back(left[left_index]);
                left_index++;
            }
            else
            {
                sorted_tanks.push_back(right[right_index]);
                right_index++;
            }
        }

        // Add any remaining elements from the left array
        while (left_index < left.size())
        {
            sorted_tanks.push_back(left[left_index]);
            left_index++;
        }

        // Add any remaining elements from the right array
        while (right_index < right.size())
        {
            sorted_tanks.push_back(right[right_index]);
            right_index++;
        }
    }

} // namespace Tmpl8