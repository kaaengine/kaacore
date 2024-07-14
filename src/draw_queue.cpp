#include <algorithm>

#include "kaacore/draw_queue.h"

namespace kaacore {

void
DrawQueue::enqueue_modification(DrawUnitModification&& draw_unit_mod)
{
    this->_modifications_queue.push_back(std::move(draw_unit_mod));
}

void
DrawQueue::process_modifications()
{
    std::sort(
        this->_modifications_queue.begin(), this->_modifications_queue.end()
    );
    auto it_begin = this->_modifications_queue.begin();
    const auto queue_end = this->_modifications_queue.end();
    while (it_begin != queue_end) {
        auto it_end = std::partition_point(
            it_begin, queue_end,
            [key = it_begin->lookup_key](const DrawUnitModification& du_mod) {
                return du_mod.lookup_key == key;
            }
        );
        auto [bucket_it, created] =
            this->_buckets_map.try_emplace(it_begin->lookup_key);
        std::get<DrawBucket>(*bucket_it)
            .consume_modifications(it_begin, it_end);

        it_begin = it_end;
    }
    this->_modifications_queue.clear();
}

DrawQueue::const_iterator
DrawQueue::begin() const
{
    return this->_buckets_map.cbegin();
}

DrawQueue::const_iterator
DrawQueue::end() const
{
    return this->_buckets_map.cend();
}

} // namespace kaacore
