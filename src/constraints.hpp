#ifndef CONSTRAINTS_HPP
#define CONSTRAINTS_HPP

#include <array>
#include <iosfwd>
#include <map>
#include <utility>
#include <variant>

namespace course {
namespace detail {
struct delim_char {
  char expected;
};

std::istream &operator>>(std::istream &in, detail::delim_char &&rhs);
} // namespace detail

struct constraint_bind {
  enum bind_direction {
    UPPER_LEFT,
    LOWER_LEFT,
    UNDEFINED,
    first = UPPER_LEFT,
    last = UNDEFINED
  } direction;

  using direction_id = char;
  using constraint_bind_factory = std::map<direction_id, bind_direction>;
  const inline static constraint_bind_factory value_factory = {
      {'u', UPPER_LEFT},
      {'d', LOWER_LEFT},
  };

  constraint_bind();
  constraint_bind(char id);
  constraint_bind(bind_direction direction);

  bool is_undefined() const noexcept;
};

struct obj_value {
  size_t prop;
  size_t value;
};

struct constraint1 {
  size_t prop;
  size_t obj;
  size_t value;
};

struct constraint3 {
  constraint_bind side;
  obj_value lhs;
  obj_value rhs;
};

using constraint2 = std::pair<obj_value, obj_value>;
using constraint4 = std::pair<constraint3, constraint3>;
using general_constraint =
    std::variant<course::constraint1, course::constraint2, course::constraint3,
                 course::constraint4>;

std::istream & operator>>(std::istream & in, constraint_bind & rhs);
std::istream &operator>>(std::istream &in, general_constraint &rhs);
std::istream &operator>>(std::istream &in, obj_value &lhs);
std::istream &operator>>(std::istream &in, constraint1 &lhs);
std::istream &operator>>(std::istream &in, constraint2 &lhs);
std::istream &operator>>(std::istream &in, constraint3 &lhs);
std::istream &operator>>(std::istream &in, constraint4 &lhs);

std::ostream &operator<<(std::ostream &out, const general_constraint &rhs);
std::ostream &operator<<(std::ostream &out, const obj_value &lhs);
std::ostream &operator<<(std::ostream &out, const constraint1 &lhs);
std::ostream &operator<<(std::ostream &out, const constraint2 &lhs);
std::ostream &operator<<(std::ostream &out, const constraint3 &lhs);
std::ostream &operator<<(std::ostream &out, const constraint4 &lhs);

template <class bdd, size_t obj_count, size_t prop_count>
using props_array_t =
    std::array<std::array<std::array<bdd, obj_count>, obj_count>, prop_count>;

} // namespace course

#endif
