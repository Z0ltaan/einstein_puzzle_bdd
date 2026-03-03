#include "constraints.hpp"
#include <iostream>
#include <limits>

static bool
is_comment_symbol(char c)
{
  return c == '#' || c == ';';
}

std::istream&
course::operator>>(std::istream& in, general_constraint& rhs)
{
  std::istream::sentry guard(in);
  if (!guard)
  {
    return in;
  }

  char next_def{};
  course::general_constraint tmp;
  while ((in >> next_def), is_comment_symbol(next_def))
  {
    in.ignore(std::numeric_limits< std::streamsize >::max(), '\n');
  }

  if (next_def == 'c')
  {
    in >> next_def;
    // TODO: get rid of switch case
    switch (next_def)
    {
      case '1':
        tmp = course::constraint1{};
        break;
      case '2':
        tmp = course::constraint2{};
        break;
      case '3':
        tmp = course::constraint3{};
        break;
      case '4':
        tmp = course::constraint4{};
        break;
      default:
        in.setstate(std::ios::badbit);
        break;
    }
  }
  else
  {
    in.setstate(std::ios::badbit);
  }

  std::visit([&](auto&& var) { in >> var; }, tmp);

  if (in)
  {
    rhs = tmp;
  }

  return in;
}

std::ostream&
course::operator<<(std::ostream& out, const general_constraint& rhs)
{
  std::visit([&](auto&& value) { out << value; }, rhs);
  return out;
}

std::istream&
course::detail::operator>>(std::istream& in, course::detail::delim_char&& rhs)
{
  std::istream::sentry guard(in);
  if (!guard)
  {
    return in;
  }

  char c{};
  in >> c;
  if (rhs.expected != c)
  {
    in.setstate(std::ios::badbit);
  }

  return in;
}

std::istream&
course::operator>>(std::istream& in, course::constraint1& lhs)
{
  std::istream::sentry guard(in);
  if (!guard)
  {
    return in;
  }

  using delim = course::detail::delim_char;
  constraint1 tmp{};
  in >> delim{ '{' } >> tmp.prop >> delim{ ',' } >> tmp.obj >> delim{ ',' } >>
    tmp.value >> delim{ '}' };

  if (in)
  {
    lhs = tmp;
  }

  return in;
}

std::istream&
course::operator>>(std::istream& in, course::obj_value& rhs)
{
  std::istream::sentry guard(in);
  if (!guard)
  {
    return in;
  }

  using delim = course::detail::delim_char;
  obj_value tmp{};
  in >> delim{ '{' } >> tmp.prop >> delim{ ',' } >> tmp.value >> delim{ '}' };

  if (in)
  {
    rhs = tmp;
  }

  return in;
}

std::istream&
course::operator>>(std::istream& in, course::constraint2& lhs)
{
  std::istream::sentry guard(in);
  if (!guard)
  {
    return in;
  }
  using delim = course::detail::delim_char;
  constraint2 tmp{};

  in >> delim{ '{' } >> tmp.first >> delim{ ',' } >> tmp.second >> delim{ '}' };

  if (in)
  {
    lhs = tmp;
  }

  return in;
}

std::istream&
course::operator>>(std::istream& in, course::constraint3& lhs)
{
  std::istream::sentry guard(in);
  if (!guard)
  {
    return in;
  }

  using delim = course::detail::delim_char;
  constraint3 tmp{};

  in >> delim{ '{' } >> tmp.side >> delim{ ',' } >> tmp.lhs >> delim{ ',' } >>
    tmp.rhs >> delim{ '}' };

  if (!(tmp.side == 'd' || tmp.side == 'u')) {
    in.setstate(std::ios::badbit);
  }

  if (in)
  {
    lhs = tmp;
  }
  return in;
}

std::istream&
course::operator>>(std::istream& in, course::constraint4& lhs)
{
  std::istream::sentry guard(in);
  if (!guard)
  {
    return in;
  }

  using delim = course::detail::delim_char;
  constraint4 tmp{};

  in >> delim{ '{' } >> tmp.first >> delim{ ',' } >> tmp.second >> delim{ '}' };

  if (in)
  {
    lhs = tmp;
  }
  return in;
}

std::ostream&
course::operator<<(std::ostream& out, const obj_value& lhs)
{
  return out << '{' << lhs.prop << ", " << lhs.value << '}';
}

std::ostream&
course::operator<<(std::ostream& out, const constraint1& lhs)
{
  return out << '{' << lhs.prop << ", " << lhs.obj << ", " << lhs.value << '}';
}

std::ostream&
course::operator<<(std::ostream& out, const constraint2& lhs)
{
  return out << '{' << lhs.first << ", " << lhs.second << '}';
}

std::ostream&
course::operator<<(std::ostream& out, const constraint3& lhs)
{
  return out << '{' << lhs.side << ", " << lhs.lhs << ", " << lhs.rhs << '}';
}

std::ostream&
course::operator<<(std::ostream& out, const constraint4& lhs)
{
  return out << '{' << lhs.first << ", " << lhs.second << '}';
}
