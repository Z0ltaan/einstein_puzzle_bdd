#include "main_logic.hpp"
#include "cli.hpp"
#include "constraints.hpp"
#include "props_array.hpp"
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
#include <stdexcept>
#include <string>
#include <vector>

static course::config config{false, 9, 4, 1000000, 10000, ""};

class solution_writer {
public:
  solution_writer() = delete;
  solution_writer(const solution_writer &rhs) = delete;
  solution_writer(solution_writer &&rhs) = delete;
  solution_writer &operator=(const solution_writer &rhs) = delete;
  solution_writer &operator=(solution_writer &&rhs) = delete;

  solution_writer(const std::string &output_path, size_t object_count,
                  size_t properties_count)
      : object_count_(object_count), properties_count_(properties_count),
        var_(object_count * properties_count *
             std::ceil(std::log2(object_count))),
        out_(output_path) {

    out_.open(output_path);
    if (!out_.is_open()) {
      std::string error_message_template("cant open output file: ");
      throw std::runtime_error(error_message_template.append(output_path));
    }
  }

  double write_solutions(bdd &puzzle_to_solve) {
    double satcount = bdd_satcount(puzzle_to_solve);
    if (satcount != 0 && satcount <= 10000) {
      out_ << satcount << " solutions:\n";
      solution_writer::current_instance_ = this;
      bdd_allsat(puzzle_to_solve, static_callback_writer);
    }
    return satcount;
  }

private:
  size_t object_count_;
  size_t properties_count_;
  std::vector<char> var_;
  std::ofstream out_;

  inline static solution_writer *current_instance_ = nullptr;

  static void static_callback_writer(char *varset, int size) {
    if (solution_writer::current_instance_) {
      solution_writer::current_instance_->build(varset, size);
    }
  }

  void build(char *varset, unsigned total_bits) {
    struct state_t {
      size_t idx;
      enum { NOT_VISITED, VISITED } step;
    };

    std::vector<state_t> stack;
    stack.reserve(total_bits + 1);
    stack.push_back({0, state_t::NOT_VISITED});

    while (!stack.empty()) {
      state_t &curr = stack.back();
      unsigned i = curr.idx;

      if (i == total_bits) {
        print(out_);
        stack.pop_back();
        continue;
      }

      if (varset[i] >= 0) {
        if (curr.step == 0) {
          var_[i] = varset[i];
          curr.step = state_t::VISITED;
          stack.push_back({i + 1, state_t::NOT_VISITED});
        } else {
          stack.pop_back();
        }
      }
    }
  }

  void print(std::ostream &out) {
    size_t log = std::ceil(std::log2(object_count_));
    const unsigned shift_base = 1;
    for (unsigned i = 0; i < object_count_; i++) {
      out << i << ": ";
      for (unsigned j = 0; j < properties_count_; j++) {
        unsigned J = (i * properties_count_ + j) * log;
        unsigned object_value = 0;
        for (unsigned k = 0; k < log; k++) {
          if (var_[J + k]) {
            object_value += shift_base << k;
          }
        }
        out << object_value << " ";
      }
      out << '\n';
    }
    out << "\n\n";
  }
};

static int var_index(int i, int k, int t) {
  const int LOG_N = std::ceil(std::log2(::config.object_count));
  return i * ::config.properties_count * LOG_N + k * LOG_N + t;
}

static bdd bdd_eql(const bdd &a, const bdd &b) { return (a & b) | (!a & !b); }

static bdd evaluate_constraint(const course::props_array &props,
                               const course::constraint1 &c) {
  return props.at(c.prop).at(c.obj).at(c.value);
}

static bdd evaluate_constraint(const course::props_array &props,
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

static bdd evaluate_constraint(const course::props_array &props,
                               const course::constraint3 &c) {
  const static std::map<
      course::constraint_bind::bind_direction,
      std::function<bdd(const course::props_array &,
                        const course::constraint3 &c, bool split_allowed)>>
      bind_evaluation = {
          {course::constraint_bind::bind_direction::UPPER_LEFT,
           [](const course::props_array &props, const course::constraint3 &c,
              bool split_allowed) -> bdd {
             bdd evaluated_constraint = bddtrue;
             const size_t N_SQRT =
                 static_cast<size_t>(std::sqrt(::config.object_count));
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
           [](const course::props_array &props, const course::constraint3 &c,
              bool split_allowed) -> bdd {
             bdd evaluated_constraint = bddtrue;
             const size_t N_SQRT =
                 static_cast<size_t>(std::sqrt(::config.object_count));
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
                     std::cref(c), ::config.split_allowed);
}

static bdd evaluate_constraint(const course::props_array &props,
                               const course::constraint4 &c) {
  return evaluate_constraint(props, c.first) |
         evaluate_constraint(props, c.second);
}

static void evaluate_lvl5_constraint(bdd &function,
                                     const course::props_array &properties) {
  for (int k = 0; k < ::config.properties_count; ++k) {
    for (int j = 0; j < ::config.object_count; ++j) {
      for (int i1 = 0; i1 < ::config.object_count; i1++) {
        for (int i2 = i1 + 1; i2 < ::config.object_count; i2++) {
          function &= bdd_imp(properties[k][i1][j], !properties[k][i2][j]);
        }
      }
    }
  }
}

static void evaluate_lvl6_constraint(bdd &function,
                                     const course::props_array &properties) {
  for (int i = 0; i < ::config.object_count; ++i) {
    for (int j = 0; j < ::config.properties_count; ++j) {
      bdd tmp_false = bddfalse;
      for (int k = 0; k < ::config.object_count; ++k) {
        tmp_false |= properties[j][i][k];
      }
      function &= tmp_false;
    }
  }
}

static void init_properties_array(course::props_array &properties) {
  const size_t LOG_N = std::ceil(std::log2(::config.object_count));

  // NOTE: Set properties p(k, i, j)
  for (int i = 0; i < ::config.object_count; ++i) {
    for (int j = 0; j < ::config.object_count; ++j) {
      for (int k = 0; k < ::config.properties_count; ++k) {
        bdd &tmp = properties[k][i][j];
        tmp = bddtrue;
        for (int t = 0; t < LOG_N; ++t) {
          auto idx = var_index(i, k, t);
          tmp &= ((j >> t) & 1) ? bdd_ithvar(idx) : bdd_nithvar(idx);
        }
      }
    }
  }
}

static std::filesystem::path get_program_dir(const char *program_path) {
  std::filesystem::path p(program_path);
  return p.parent_path();
}

static bool is_square(size_t original_value, size_t square_root) {
  return original_value == square_root * square_root;
}

static bool ensure_correct_einstein_puzzle_parameters(size_t object_count,
                                                      size_t properties_count) {
  return (object_count > 0 && properties_count > 0) &&
         (properties_count < object_count);
}

int course::main_logic(int argc, char **argv) {
  try {
    handle_command_line_args(argc, argv, ::config);

    size_t object_count = ::config.object_count,
           properties_count = ::config.properties_count;

    if (not(is_square(object_count, std::sqrt(object_count)) &&
            ensure_correct_einstein_puzzle_parameters(object_count,
                                                      properties_count))) {
      throw std::runtime_error(
          "incorrect einstein puzzle parameters; object count should be "
          "representable as a square, object_count > properties_count, both "
          "are positive");
    }

    std::vector<general_constraint> constraints;
    constraints.reserve(object_count + properties_count);

    {
      std::filesystem::path constraints_config_path = get_program_dir(argv[0]);

      if (::config.constraint_config_path != "") {
        constraints_config_path =
            std::filesystem::path(::config.constraint_config_path).parent_path();
      } else {
        ::config.constraint_config_path = constraints_config_path;
      }

      std::filesystem::path constraint_file =
          constraints_config_path / "constraints.ini";
      std::ifstream in(constraint_file);
      if (!in.is_open()) {
        std::string error_message_template = "cant open a file: ";
        throw std::runtime_error(
            error_message_template.append(constraint_file));
      }

      std::copy(std::istream_iterator<general_constraint>{in},
                std::istream_iterator<general_constraint>{},
                std::back_insert_iterator{constraints});

      if (!in.eof() && !in) {
        throw std::runtime_error("bad config data");
      }
    }

    bdd_init(::config.node_number, ::config.cache_size);
    bdd_setvarnum(object_count * properties_count *
                  std::ceil(std::log2(object_count)));

    props_array properties =
        course::create_sized_props_array(object_count, properties_count);
    init_properties_array(properties);

    bdd puzzle_to_solve = bddtrue;

    // NOTE: evaluate constraints from file
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

    evaluate_lvl5_constraint(puzzle_to_solve, properties);
    evaluate_lvl6_constraint(puzzle_to_solve, properties);

    std::cout << "If solution number is greater than 10000 or equal to 0 file "
                 "will not be created\n";

    {
      std::filesystem::path output_file(::config.constraint_config_path / "out.txt");
      solution_writer output_writer(output_file, object_count,
                                    properties_count);
      double solutions_written = output_writer.write_solutions(puzzle_to_solve);
      std::cout << "Solution count: " << solutions_written << '\n';
    }

    bdd_done();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return -1;
  }
  return 0;
}
