#include "constraints.hpp"
#include <functional>
#include <iostream>
#include <istream>
#include <limits>
#include <map>
#include <stdexcept>

static bool is_comment_symbol(char c) { return c == '#' || c == ';'; }

course::constraint_bind::constraint_bind() : direction(UNDEFINED) {}

course::constraint_bind::constraint_bind(char id) : direction(UNDEFINED) {
  auto iterator = value_factory.find(id);
  if (iterator != value_factory.end()) {
    direction = iterator->second;
  }
}

course::constraint_bind::constraint_bind(bind_direction direction)
    : direction(direction) {}

bool course::constraint_bind::is_undefined() const noexcept {
  return direction == UNDEFINED;
}

std::istream &course::operator>>(std::istream &in, constraint_bind &rhs) {

  std::istream::sentry guard(in);
  if (!guard) {
    return in;
  }

  constraint_bind::direction_id id = ' ';
  in >> id;

  constraint_bind tmp(id);
  if (tmp.is_undefined()) {
    in.setstate(std::ios::badbit);
  } else {
    rhs = tmp;
  }

  return in;
}

std::istream &course::operator>>(std::istream &in, general_constraint &rhs) {
  using constraint_level = char;
  using constraint_factory =
      std::map<constraint_level, std::function<general_constraint()>>;

  const static constraint_factory factory = {
      {'1', [] { return constraint1{}; }},
      {'2', [] { return constraint2{}; }},
      {'3', [] { return constraint3{}; }},
      {'4', [] { return constraint4{}; }}};

  std::istream::sentry guard(in);
  if (!guard) {
    return in;
  }

  char next_symbol{};
  course::general_constraint tmp;
  while ((in >> next_symbol), is_comment_symbol(next_symbol)) {
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  if (next_symbol == 'c') {
    in >> next_symbol;

    try {
      tmp = std::invoke(factory.at(next_symbol));
    } catch (const std::out_of_range &e) {
      in.setstate(std::ios::badbit);
    }

  } else {
    in.setstate(std::ios::badbit);
  }

  std::visit([&](auto &&var) { in >> var; }, tmp);

  if (in) {
    rhs = tmp;
  }

  return in;
}

std::ostream &course::operator<<(std::ostream &out,
                                 const general_constraint &rhs) {
  std::visit([&](auto &&value) { out << value; }, rhs);
  return out;
}

std::istream &course::detail::operator>>(std::istream &in,
                                         course::detail::delim_char &&rhs) {
  std::istream::sentry guard(in);
  if (!guard) {
    return in;
  }
  char c{};
  in >> c;
  if (rhs.expected != c) {
    in.setstate(std::ios::badbit);
  }

  return in;
}

std::istream &course::operator>>(std::istream &in, course::constraint1 &lhs) {
  std::istream::sentry guard(in);
  if (!guard) {
    return in;
  }

  using delim = course::detail::delim_char;
  constraint1 tmp{};
  in >> delim{'{'} >> tmp.prop >> delim{','} >> tmp.obj >> delim{','} >>
      tmp.value >> delim{'}'};

  if (in) {
    lhs = tmp;
  }

  return in;
}

std::istream &course::operator>>(std::istream &in, course::obj_value &rhs) {
  std::istream::sentry guard(in);
  if (!guard) {
    return in;
  }

  using delim = course::detail::delim_char;
  obj_value tmp{};
  in >> delim{'{'} >> tmp.prop >> delim{','} >> tmp.value >> delim{'}'};

  if (in) {
    rhs = tmp;
  }

  return in;
}

std::istream &course::operator>>(std::istream &in, course::constraint2 &lhs) {
  std::istream::sentry guard(in);
  if (!guard) {
    return in;
  }
  using delim = course::detail::delim_char;
  constraint2 tmp{};

  in >> delim{'{'} >> tmp.first >> delim{','} >> tmp.second >> delim{'}'};

  if (in) {
    lhs = tmp;
  }

  return in;
}

std::istream &course::operator>>(std::istream &in, course::constraint3 &lhs) {
  std::istream::sentry guard(in);
  if (!guard) {
    return in;
  }

  using delim = course::detail::delim_char;
  constraint3 tmp{};

  in >> delim{'{'} >> tmp.side >> delim{','} >> tmp.lhs >> delim{','} >>
      tmp.rhs >> delim{'}'};

  if (tmp.side.is_undefined()) {
    in.setstate(std::ios::badbit);
  }

  if (in) {
    lhs = tmp;
  }

  return in;
}

std::istream &course::operator>>(std::istream &in, course::constraint4 &lhs) {
  std::istream::sentry guard(in);
  if (!guard) {
    return in;
  }

  using delim = course::detail::delim_char;
  constraint4 tmp{};

  in >> delim{'{'} >> tmp.first >> delim{','} >> tmp.second >> delim{'}'};

  if (in) {
    lhs = tmp;
  }
  return in;
}

std::ostream &course::operator<<(std::ostream &out, const obj_value &lhs) {
  return out << '{' << lhs.prop << ", " << lhs.value << '}';
}

std::ostream &course::operator<<(std::ostream &out, const constraint1 &lhs) {
  return out << '{' << lhs.prop << ", " << lhs.obj << ", " << lhs.value << '}';
}

std::ostream &course::operator<<(std::ostream &out, const constraint2 &lhs) {
  return out << '{' << lhs.first << ", " << lhs.second << '}';
}

std::ostream &course::operator<<(std::ostream &out, const constraint3 &lhs) {
  return out << '{' << static_cast<int>(lhs.side.direction) << ", " << lhs.lhs
             << ", " << lhs.rhs << '}';
}

std::ostream &course::operator<<(std::ostream &out, const constraint4 &lhs) {
  return out << '{' << lhs.first << ", " << lhs.second << '}';
}
