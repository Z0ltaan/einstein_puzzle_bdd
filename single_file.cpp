#include <algorithm>
#include <array>
#include <buddy/bdd.h>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <iterator>
#include <ostream>
#include <utility>
#include <variant>
#include <vector>

static constexpr int N = 9; // число объектов;
static constexpr int M = 4; // число свойств

static const int LOG_N = std::ceil(std::log2(N));
static const int N_VAR = N * M * LOG_N; // число булевых переменных
static std::vector< char > var(N_VAR);

namespace course
{
  namespace detail
  {
    struct delim_char
    {
      char expected;
    };

    std::istream& operator>>(std::istream& in, detail::delim_char&& rhs);
  }

  struct obj_value
  {
    size_t prop;
    size_t value;
  };

  struct constraint1
  {
    size_t prop;
    size_t obj;
    size_t value;
  };

  struct constraint3
  {
    char side;
    obj_value lhs;
    obj_value rhs;
  };

  using constraint2 = std::pair< obj_value, obj_value >;
  using constraint4 = std::pair< constraint3, constraint3 >;
  using general_constraint = std::variant< course::constraint1,
                                           course::constraint2,
                                           course::constraint3,
                                           course::constraint4 >;

  std::istream& operator>>(std::istream& in, general_constraint& rhs);
  std::istream& operator>>(std::istream& in, obj_value& lhs);
  std::istream& operator>>(std::istream& in, constraint1& lhs);
  std::istream& operator>>(std::istream& in, constraint2& lhs);
  std::istream& operator>>(std::istream& in, constraint3& lhs);
  std::istream& operator>>(std::istream& in, constraint4& lhs);

  std::ostream& operator<<(std::ostream& out, const general_constraint& rhs);
  std::ostream& operator<<(std::ostream& out, const obj_value& lhs);
  std::ostream& operator<<(std::ostream& out, const constraint1& lhs);
  std::ostream& operator<<(std::ostream& out, const constraint2& lhs);
  std::ostream& operator<<(std::ostream& out, const constraint3& lhs);
  std::ostream& operator<<(std::ostream& out, const constraint4& lhs);

  template< class bdd, size_t obj_count, size_t prop_count >
  using props_array_t =
    std::array< std::array< std::array< bdd, obj_count >, obj_count >,
                prop_count >;

} // namespace course

using props_array = course::props_array_t< bdd, N, M >;

static void
fun(char* varset, int size);
static std::ofstream out;

static int
var_index(int i, int k, int t)
{
  return i * M * LOG_N + k * LOG_N + t;
}

static bdd
bdd_eql(const bdd& a, const bdd& b)
{
  return (a & b) | (!a & !b);
}

bdd
evaluate_constraint(const props_array& props, const course::constraint1& c)
{
  return props.at(c.prop).at(c.obj).at(c.value);
}

// TODO: rewrite this shit so .at is used instead of []
bdd
evaluate_constraint(const props_array& props, const course::constraint2& c)
{
  bdd to_return = bddtrue;
  auto upper_bound = props.front().size();
  for (size_t i = 0; i < upper_bound; ++i)
  {
    to_return &= bdd_eql(props[c.first.prop][i][c.first.value],
                         props[c.second.prop][i][c.second.value]);
  }
  return to_return;
}

bdd
evaluate_constraint(const props_array& props, const course::constraint3& c)
{
  bdd to_return = bddtrue;
  static const size_t N_SQRT = static_cast< size_t >(std::sqrt(N));
  static const char* split_allowed = std::getenv("SPLIT");
  const auto upper_bound = props.front().size();
  if (c.side == 'u')
  {
    for (size_t i = 0; i < upper_bound; ++i)
    {
      if (i / N_SQRT == 0 || (i % N_SQRT == 0 && !split_allowed))
      {
        to_return &= !props[c.lhs.prop][i][c.lhs.value];
      }
      else if (i % N_SQRT == 0 && split_allowed)
      {
        to_return &= bdd_eql(props[c.lhs.prop][i][c.lhs.value],
                             props[c.rhs.prop][i - 1][c.rhs.value]);
      }
      else
      {
        to_return &= bdd_eql(props[c.lhs.prop][i][c.lhs.value],
                             props[c.rhs.prop][i - N_SQRT - 1][c.rhs.value]);
      }
    }
  }
  else if (c.side == 'd')
  {
    for (size_t i = 0; i < upper_bound; ++i)
    {
      if (i / N_SQRT >= N_SQRT - 1 || (i % N_SQRT == 0 && !split_allowed))
      {
        to_return &= !props[c.lhs.prop][i][c.lhs.value];
      }
      else if (i % N_SQRT == 0 && split_allowed)
      {
        to_return &=
          bdd_eql(props[c.lhs.prop][i][c.lhs.value],
                  props[c.rhs.prop][i + 2 * N_SQRT - 1][c.rhs.value]);
      }
      else
      {
        to_return &= bdd_eql(props[c.lhs.prop][i][c.lhs.value],
                             props[c.rhs.prop][i + N_SQRT - 1][c.rhs.value]);
      }
    }
  }

  return to_return;
}

bdd
evaluate_constraint(const props_array& props, const course::constraint4& c)
{
  return evaluate_constraint(props, c.first) |
         evaluate_constraint(props, c.second);
}

static std::filesystem::path
get_program_dir(const char* program_path)
{
  std::filesystem::path p(program_path);
  return p.parent_path();
}

int
main(int argc, char** argv)
{
  static_assert((N > 0 && M > 0) && (M < N), "check your N and M values");

  std::vector< course::general_constraint > constraints;
  constraints.reserve(N + M);

  std::filesystem::path program_dir = get_program_dir(argv[0]);

  {
    std::filesystem::path config = program_dir / "constraints.ini";
    std::ifstream in(config);
    if (!in.is_open())
    {
      std::cerr << "cant open a file: " << config.c_str() << "\n";
      return -1;
    }

    std::copy(std::istream_iterator< course::general_constraint >{ in },
              std::istream_iterator< course::general_constraint >{},
              std::back_insert_iterator{ constraints });

    if (in.bad())
    {
      std::cerr << "bad config data\n";
      return -1;
    }
  }
  props_array properties;

  size_t nodenum = 1000000;
  size_t cache_size = 10000;
  bdd_init(nodenum, cache_size);
  bdd_setvarnum(N_VAR);

  // NOTE: Set function p(k, i, j)
  for (int i = 0; i < N; ++i)
  {
    for (int j = 0; j < N; ++j)
    {
      for (int k = 0; k < M; ++k)
      {
        bdd& tmp = properties[k][i][j];
        tmp = bddtrue;
        for (int t = 0; t < LOG_N; ++t)
        {
          auto idx = var_index(i, k, t);
          tmp &= ((j >> t) & 1) ? bdd_ithvar(idx) : bdd_nithvar(idx);
        }
      }
    }
  }

  // NOTE: init F
  bdd task = bddtrue;

  // NOTE: evaluate constraints
  {
    auto evaluate_general_constraint = [&](const auto& item)
    {
      task &= std::visit([&](auto&& rhs) -> bdd
                         { return evaluate_constraint(properties, rhs); },
                         item);
    };

    std::for_each(
      constraints.begin(), constraints.end(), evaluate_general_constraint);
  }

  // NOTE: lvl 5 constraint
  for (int k = 0; k < M; ++k)
  {
    for (int j = 0; j < N; ++j)
    {
      for (int i1 = 0; i1 < N; i1++)
      {
        for (int i2 = i1 + 1; i2 < N; i2++)
        {
          task &= bdd_imp(properties[k][i1][j], !properties[k][i2][j]);
        }
      }
    }
  }

  // NOTE: lvl 6 constraint
  for (int i = 0; i < N; ++i)
  {
    for (int j = 0; j < M; ++j)
    {
      bdd tmp_false = bddfalse;
      for (int k = 0; k < N; ++k)
      {
        tmp_false |= properties[j][i][k];
      }
      task &= tmp_false;
    }
  }

  double satcount = bdd_satcount(task);
  std::cout << "Solution count: " << satcount << '\n';
  std::cout
    << "if solution number is greater than 10000 file will not be created\n";
  if (satcount != 0 && satcount <= 10000)
  {
    std::filesystem::path output_file(program_dir / "out.txt");
    out.open(output_file);
    if (!out.is_open())
    {
      std::cerr << "cant open output file: " << output_file.c_str() << '\n';
      return -1;
    }
    out << satcount << " solutions:\n";
    bdd_allsat(task, fun);
  }
  bdd_done();
  return 0;
}

static void
print()
{
  for (unsigned i = 0; i < N; i++)
  {
    out << i << ": ";
    for (unsigned j = 0; j < M; j++)
    {
      unsigned J = (i * M + j) * LOG_N;
      unsigned num = 0;
      unsigned powerOf2 = 1;

      for (unsigned k = 0; k < LOG_N; k++)
      {
        if (var[J + k])
        {
          num += powerOf2;
        }
        powerOf2 *= 2;
      }
      out << num << " ";
    }
    out << '\n';
  }
  out << "\n\n";
}

static void
build(char* varset, unsigned total_bits)
{
  struct State
  {
    unsigned idx;
    int step;
  };

  std::vector< State > stack;
  stack.reserve(total_bits + 1);
  stack.push_back({ 0, 0 });

  while (!stack.empty())
  {
    State& curr = stack.back();
    unsigned i = curr.idx;

    if (i == total_bits)
    {
      print();
      stack.pop_back();
      continue;
    }

    if (varset[i] >= 0)
    {
      if (curr.step == 0)
      {
        var[i] = varset[i];
        curr.step = 1;
        stack.push_back({ i + 1, 0 });
      }
      else
      {
        stack.pop_back();
      }
    }
    else
    {
      if (curr.step == 0)
      {
        var[i] = 0;
        curr.step = 1;
        stack.push_back({ i + 1, 0 });
      }
      else if (curr.step == 1)
      {
        var[i] = 1;
        curr.step = 2;
        stack.push_back({ i + 1, 0 });
      }
      else
      {
        stack.pop_back();
      }
    }
  }
}

static void
fun(char* varset, int size)
{
  build(varset, size);
}

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

  if (!(tmp.side == 'd' || tmp.side == 'u'))
  {
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
