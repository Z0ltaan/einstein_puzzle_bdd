#ifndef PROPS_ARRAY_HPP
#define PROPS_ARRAY_HPP

#include <bdd.h>
#include <vector>

namespace course {
using props_array = std::vector<std::vector<std::vector<bdd>>>;
props_array create_sized_props_array(size_t object_count,
                                     size_t properties_count);
} // namespace course

#endif
