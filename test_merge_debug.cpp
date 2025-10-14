#include "include/ranked_belief/ranking_function.hpp"
#include "include/ranked_belief/constructors.hpp"
#include "include/ranked_belief/operations/merge.hpp"
#include <iostream>
#include <vector>

using namespace ranked_belief;

std::vector<int> collect_values(const RankingFunction<int>& rf, std::size_t max_count = 100) {
    std::vector<int> result;
    auto it = rf.begin();
    auto end = rf.end();
    
    for (std::size_t i = 0; i < max_count && it != end; ++i, ++it) {
        result.push_back((*it).first);
    }
    
    return result;
}

int main() {
    try {
        std::cout << "Creating ranking functions..." << std::endl;
        std::vector<int> values = {1, 2, 3};
        auto rf1 = from_values_sequential(values);
        auto rf2 = from_values_sequential(values);
        
        std::cout << "Merging with deduplication enabled..." << std::endl;
        auto merged_dedup = merge(rf1, rf2);  // deduplicate = true (default)
        auto result_dedup = collect_values(merged_dedup);
        std::cout << "Dedup result size: " << result_dedup.size() << std::endl;
        
        std::cout << "Merging with deduplication disabled..." << std::endl;
        auto merged_no_dedup = merge(rf1, rf2, false);
        auto result_no_dedup = collect_values(merged_no_dedup);
        std::cout << "No dedup result size: " << result_no_dedup.size() << std::endl;
        
        std::cout << "Success!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
