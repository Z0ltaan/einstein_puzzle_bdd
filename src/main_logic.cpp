#include "main_logic.hpp"
#include "constraints.hpp"
#include <algorithm>
#include <bdd.h>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <ostream>
#include <vector>

static constexpr int N = 9; // число объектов;
static constexpr int M = 4; // число свойств
static constexpr size_t nodenum = 1000000;
static constexpr size_t cache_size = 10000;

static const int LOG_N = std::ceil(std::log2(N));
static const int N_VAR = N * M * LOG_N; // число булевых переменных
static std::vector<char> var(N_VAR);

using props_array = course::props_array_t<bdd, N, M>;

static void fun(char *varset, int size);
static std::ofstream out;

static int var_index(int i, int k, int t) {
  return i * M * LOG_N + k * LOG_N + t;
}

static bdd bdd_eql(const bdd &a, const bdd &b) { return (a & b) | (!a & !b); }

static bdd evaluate_constraint(const props_array &props,
                               const course::constraint1 &c) {
  return props.at(c.prop).at(c.obj).at(c.value);
}

static bdd evaluate_constraint(const props_array &props,
                               const course::constraint2 &c) {
  bdd evaluated_constraint = bddtrue;
  auto upper_bound = props.front().size();
  for (size_t i = 0; i < upper_bound; ++i) {
    evaluated_constraint &=
        bdd_eql(props.at(c.first.prop)[i].at(c.first.value),
                props.at(c.second.prop)[i].at(c.second.value));
  }
  return evaluated_constraint;
}

static bdd evaluate_constraint(const props_array &props,
                               const course::constraint3 &c) {
  static const char *split_allowed = std::getenv("SPLIT");

  const static std::map<
      course::constraint_bind::bind_direction,
      std::function<bdd(const props_array &, const course::constraint3 &c,
                        bool split_allowed)>>
      bind_evaluation = {
          {course::constraint_bind::bind_direction::UPPER_LEFT,
           [](const props_array &props, const course::constraint3 &c,
              bool split_allowed) -> bdd {
             bdd evaluated_constraint = bddtrue;
             const size_t N_SQRT = static_cast<size_t>(std::sqrt(N));
             const auto upper_bound = props.front().size();
             for (size_t i = 0; i < upper_bound; ++i) {
               if (i / N_SQRT == 0 || (i % N_SQRT == 0 && !split_allowed)) {
                 evaluated_constraint &=
                     !props.at(c.lhs.prop)[i].at(c.lhs.value);
               } else if (i % N_SQRT == 0 && split_allowed) {
                 evaluated_constraint &=
                     bdd_eql(props.at(c.lhs.prop)[i].at(c.lhs.value),
                             props.at(c.rhs.prop).at(i - 1).at(c.rhs.value));
               } else {
                 evaluated_constraint &= bdd_eql(
                     props.at(c.lhs.prop)[i].at(c.lhs.value),
                     props.at(c.rhs.prop).at(i - N_SQRT - 1).at(c.rhs.value));
               }
             }
             return evaluated_constraint;
           }},
          {course::constraint_bind::bind_direction::LOWER_LEFT,
           [](const props_array &props, const course::constraint3 &c,
              bool split_allowed) -> bdd {
             bdd evaluated_constraint = bddtrue;
             const size_t N_SQRT = static_cast<size_t>(std::sqrt(N));
             const auto upper_bound = props.front().size();
             for (size_t i = 0; i < upper_bound; ++i) {
               if (i / N_SQRT >= N_SQRT - 1 ||
                   (i % N_SQRT == 0 && !split_allowed)) {
                 evaluated_constraint &=
                     !props.at(c.lhs.prop)[i].at(c.lhs.value);
               } else if (i % N_SQRT == 0 && split_allowed) {
                 evaluated_constraint &=
                     bdd_eql(props.at(c.lhs.prop)[i].at(c.lhs.value),
                             props.at(c.rhs.prop)
                                 .at(i + 2 * N_SQRT - 1)
                                 .at(c.rhs.value));
               } else {
                 evaluated_constraint &= bdd_eql(
                     props.at(c.lhs.prop)[i].at(c.lhs.value),
                     props.at(c.rhs.prop).at(i + N_SQRT - 1).at(c.rhs.value));
               }
             }
             return evaluated_constraint;
           }}};

  return std::invoke(bind_evaluation.at(c.side.direction), std::cref(props),
                     std::cref(c), split_allowed);
}

static bdd evaluate_constraint(const props_array &props,
                               const course::constraint4 &c) {
  return evaluate_constraint(props, c.first) |
         evaluate_constraint(props, c.second);
}

static std::filesystem::path get_program_dir(const char *program_path) {
  std::filesystem::path p(program_path);
  return p.parent_path();
}

int course::main_logic(int argc, char **argv) {
  static_assert((N > 0 && M > 0) && (M < N), "check your N and M values");

  {
    auto N_SQRT = std::sqrt(N);
    assert(N == N_SQRT * N_SQRT);
  }

  std::vector<general_constraint> constraints;
  constraints.reserve(N + M);

  std::filesystem::path program_dir = get_program_dir(argv[0]);

  {
    std::filesystem::path config = program_dir / "constraints.ini";
    std::ifstream in(config);
    if (!in.is_open()) {
      std::cerr << "cant open a file: " << config.c_str() << "\n";
      return -1;
    }

    std::copy(std::istream_iterator<general_constraint>{in},
              std::istream_iterator<general_constraint>{},
              std::back_insert_iterator{constraints});

    if (in.bad()) {
      std::cerr << "bad config data\n";
      return -1;
    }
  }

  bdd_init(nodenum, cache_size);
  bdd_setvarnum(N_VAR);

  props_array properties;

  // NOTE: Set properties p(k, i, j)
  // NOTE: this part of the code has been provided in task description
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      for (int k = 0; k < M; ++k) {
        bdd &tmp = properties[k][i][j];
        tmp = bddtrue;
        for (int t = 0; t < LOG_N; ++t) {
          auto idx = var_index(i, k, t);
          tmp &= ((j >> t) & 1) ? bdd_ithvar(idx) : bdd_nithvar(idx);
        }
      }
    }
  }

  // NOTE: init the function
  bdd puzzle_to_solve = bddtrue;

  // NOTE: evaluate constraints
  {
    auto evaluate_general_constraint = [&](const auto &item) {
      puzzle_to_solve &= std::visit(
          [&](auto &&rhs) -> bdd {
            return evaluate_constraint(properties, rhs);
          },
          item);
    };

    std::for_each(constraints.begin(), constraints.end(),
                  evaluate_general_constraint);
  }

  // NOTE: lvl 5 constraints
  for (int k = 0; k < M; ++k) {
    for (int j = 0; j < N; ++j) {
      for (int i1 = 0; i1 < N; i1++) {
        for (int i2 = i1 + 1; i2 < N; i2++) {
          puzzle_to_solve &=
              bdd_imp(properties[k][i1][j], !properties[k][i2][j]);
        }
      }
    }
  }

  // NOTE: lvl 6 constraints
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < M; ++j) {
      bdd tmp_false = bddfalse;
      for (int k = 0; k < N; ++k) {
        tmp_false |= properties[j][i][k];
      }
      puzzle_to_solve &= tmp_false;
    }
  }

  double satcount = bdd_satcount(puzzle_to_solve);

  std::cout << "If solution number is greater than 10000 or equal to 0 file "
               "will not be created\n";
  std::cout
      << "To activate split functionality define SPLIT environment variable\n";
  std::cout << "Solution count: " << satcount << '\n';

  if (satcount != 0 && satcount <= 10000) {
    std::filesystem::path output_file(program_dir / "out.txt");
    out.open(output_file);
    if (!out.is_open()) {
      std::cerr << "cant open output file: " << output_file.c_str() << '\n';
      return -1;
    }
    out << satcount << " solutions:\n";
    bdd_allsat(puzzle_to_solve, fun);
  }
  bdd_done();
  return 0;
}

static void print(std::ostream &out) {
  for (unsigned i = 0; i < N; i++) {
    out << i << ": ";
    for (unsigned j = 0; j < M; j++) {
      unsigned J = (i * M + j) * LOG_N;
      unsigned num = 0;
      unsigned power_of_2 = 1;

      for (unsigned k = 0; k < LOG_N; k++) {
        if (var[J + k]) {
          num += power_of_2;
        }
        power_of_2 *= 2;
      }
      out << num << " ";
    }
    out << '\n';
  }
  out << "\n\n";
}

static void build(char *varset, unsigned total_bits) {
  struct State {
    unsigned idx;
    int step;
  };

  std::vector<State> stack;
  stack.reserve(total_bits + 1);
  stack.push_back({0, 0});

  while (!stack.empty()) {
    State &curr = stack.back();
    unsigned i = curr.idx;

    if (i == total_bits) {
      print(out);
      stack.pop_back();
      continue;
    }

    if (varset[i] >= 0) {
      if (curr.step == 0) {
        var[i] = varset[i];
        curr.step = 1; // NOTE: done with branch
        stack.push_back({i + 1, 0});
      } else {
        stack.pop_back();
      }
    }
  }
}

static void fun(char *varset, int size) { build(varset, size); }
