#pragma once

#include <unordered_map>
#include <vector>

#include "kaacore/draw_unit.h"

namespace kaacore {

class DrawQueue {
    typedef std::unordered_map<DrawBucketKey, DrawBucket>::const_iterator const_iterator;

  public:
    void enqueue_modification(DrawUnitModification&& draw_unit_mod);
    void process_modifications();

    const_iterator begin() const;
    const_iterator end() const;

  private:
    std::unordered_map<DrawBucketKey, DrawBucket> _buckets_map;
    std::vector<DrawUnitModification> _modifications_queue;
};

} // namespace kaacore
