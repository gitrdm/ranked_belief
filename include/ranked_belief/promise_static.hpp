// Static member definition for Promise<T>
#pragma once
#include <atomic>
namespace ranked_belief {
    template<typename T>
    std::atomic<size_t> Promise<T>::global_id_counter_{0};
}
