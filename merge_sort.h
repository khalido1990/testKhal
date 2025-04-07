#pragma once

#include "precomp.h"

namespace Tmpl8
{
    class MergeSort
    {
    public:
        // Sort tanks by health value using merge sort
        static void sort_tanks_health(const std::vector<Tank>& original, std::vector<const Tank*>& sorted_tanks, int begin, int end);

    private:
        // Helper method to merge two sorted vectors of tank pointers
        static void merge_tanks_by_health(std::vector<const Tank*>& sorted_tanks, std::vector<const Tank*>& left, std::vector<const Tank*>& right);
    };

} // namespace Tmpl8