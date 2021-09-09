#pragma once

#include <bitset>
#include <unordered_set>
#include <vector>

namespace kaacore {

template<size_t N>
class IndexSet {
  public:
    IndexSet() = default;
    ~IndexSet() = default;
    IndexSet(const IndexSet&) = default;
    IndexSet(IndexSet&&) = default;
    IndexSet& operator=(const IndexSet&) = default;
    IndexSet& operator=(IndexSet&&) = default;
    IndexSet(const std::unordered_set<int16_t>& indices_set)
    {
        for (auto index : indices_set) {
            this->_bitset.set(index);
        }
    }
    bool operator==(const IndexSet& other) const
    {
        return this->_bitset == other._bitset;
    }
    bool operator<(const IndexSet& other) const
    {
        // make sure that all bits can be contained in `unsigned long`
        // during comparison
        static_assert(sizeof(unsigned long) * 8 >= N);
        return this->_bitset.to_ulong() < other._bitset.to_ulong();
    }
    IndexSet operator|(const IndexSet& other) const
    {
        return IndexSet{this->_bitset | other._bitset};
    }
    IndexSet operator&(const IndexSet& other) const
    {
        return IndexSet{this->_bitset & other._bitset};
    };
    IndexSet& operator|=(const IndexSet& other)
    {
        this->_bitset |= other._bitset;
        return *this;
    }
    IndexSet& operator&=(const IndexSet& other)
    {
        this->_bitset &= other._bitset;
        return *this;
    }
    operator std::unordered_set<int16_t>() const
    {
        std::unordered_set<int16_t> active_indices_set;
        this->each_active_index([&active_indices_set](int16_t index) {
            active_indices_set.insert(index);
        });
        return active_indices_set;
    }
    operator std::vector<int16_t>() const
    {
        std::vector<int16_t> active_indices_vector;
        this->each_active_index([&active_indices_vector](int16_t index) {
            active_indices_vector.push_back(index);
        });
        return active_indices_vector;
    }

    template<typename Func>
    void each_active_index(Func&& func) const
    {
        for (auto index = 0; index < this->_bitset.size(); index++) {
            if (this->_bitset.test(index)) {
                func(index);
            }
        }
    }

  protected:
    std::bitset<N> _bitset;

    IndexSet(std::bitset<N> bitset_) : _bitset(bitset_) {}

    friend struct std::hash<IndexSet<N>>;
};

} // namespace kaacore
