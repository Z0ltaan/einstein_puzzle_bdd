# Description

This repository contains a program that solves a modified version of Einstein puzzle. Puzzle is described using BBD (binary decision diagrams) from BuDDy C++ library (for more info look BuDDy directory). The program itself is written in C++.

## Short task description

The task is defined by the following parameters:
• There are N = 9 objects with M = 4 properties. The properties take N distinct values.
• There are individual neighbor relations, a gluing condition, and mandatory numbers of constraints n1, n2, n3, n4.
• Additionally, a delta C is defined (used for calculating the variant).

Let us describe the sequence of actions for the variant without gluing:

1. Determine the neighbors of all objects according to your variant without gluing.
2. Modify constraints of type 3 and 4 from the Einstein puzzle considering the neighbor relations of your variant. Set n3 constraints similar to type 3 relations and n4 constraints similar to type 4 relations according to your assignment.
3. Invent n1 constraints of the first type and n2 constraints of the second type.
4. Build a program to solve your problem using BDD.
5. Find all possible solutions.
6. Invent a physical interpretation for the problem.
7. Write out the interpretation of the constraints in natural language.
8. Leave about 12-16 solutions to the problem. This can be achieved by adding additional constraints of any type or modifying the current ones. If you have fewer solutions, you can remove some of the type 1-2 constraints.
9. Choose one of the solutions and write its interpretation in natural language.

For cases with gluing, the sequence of actions remains the same. The interpretation remains the same for all levels.
To complete the assignment, it is necessary to write a program. The implementation must be scalable, i.e., changing the parameters N, M should not lead to changes in the main part of the program.

Variant parameters: v1 = 17, v2 = 1, v3 = 2, where v1 — specifies neighbor relations, v2 — specifies gluing, v3 — specifies the number of constraints by type. Thus:

1. v1 = 17 specifies the following neighbor relations:

| @ | | |
| | O | |
| @ | | |

2. v2 = 1 specifies gluing along the left-right borders 3. v3 = 2 specifies n1 = 7 constraints of the first type, n2 = 4 constraints of the second type, n3 = 2 of the third type, n4 = 6 of the fourth type

# Build

## Requirements

    1. CMake (>=3.22)
    2. C++ compiler

## Build

To build the program you can use the following example:

```bash
git clone <this_repo_link> &&
cd einstein_puzzle_bdd &&
cd BuDDy &&
unzip buddy.zip &&
cd .. &&
cmake -B build &&
cmake --build build -j9
```

When build ends you will have _build_ directory in root directory of the project.
There will be an executable named _matlog_ as it says in CMakeLists.txt

# Usage

The program requires a file with puzzle constraints (constraints.ini). File must be placed in the same directory as the project.

There are two examples of constraints file. One is for non-split (no gluing) scenario and the other is for split scenario.

## Constraint syntax

### Constraint 1

Constraint format is c1{<prop>, <obj>, <value>}, indexes start w 0
Example: c1{0, 0, 0}

### Constraint 2

Constraint format is c2{{<prop1>, <val1>}, {<prop2>, <val2>}}, indexes start w 0
Example: c2{{2, 1}, {3, 1}}

### Constraint 3

Constraint format is c3{<side>, {<prop1>, <val1>}, {<prop2>, <val2>}}, indexes start w 0
Example: c3{u, {3, 8}, {0, 4}}

### Constraint 4

Constraint format is c4{<c3_1>, <c3_2>}, indexes start w 0
Example: c4{{u, {2, 4}, {2, 0}}, {d, {2, 4}, {2, 0}}}

## Usage example

From project's root directory:

1. Without split:

```bash
./build/matlog
```

2. With split:

```bash
SPLIT="" ./build/matlog
```

If ini file is written correctly then the program will calculate every solution that is possible with current constraints and will write them in separate file.
