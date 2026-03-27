#include "props_array.hpp"
#include <algorithm>
#include <iterator>
#include <vector>

namespace course {
namespace detail {
class props_filler {
  using props_array_value_t = course::props_array::value_type;

public:
  props_filler() = delete;
  props_filler(size_t array_size) : return_value_(array_size) {
    std::for_each(return_value_.begin(), return_value_.end(),
                  [=](props_array_value_t::value_type &value) {
                    value.reserve(array_size);
                    std::generate_n(std::back_insert_iterator{value},
                                    array_size, [] { return bdd{}; });
                  });
  }

  const props_array_value_t &operator()() const noexcept {
    return return_value_;
  }

private:
  props_array_value_t return_value_;
};
} // namespace detail
} // namespace course

course::props_array course::create_sized_props_array(size_t object_count,
                                                     size_t properties_count) {
  props_array properties;
  properties.reserve(properties_count);
  std::generate_n(std::back_insert_iterator{properties}, properties_count,
                  detail::props_filler(object_count));

  return properties;
}
