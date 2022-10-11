# N Queens Count Solutions

## Goal

Goal of this project is to explore how fast the solutions of [N-Queens problem](https://en.wikipedia.org/wiki/Eight_queens_puzzle#Counting_solutions_for_other_sizes_n)
can be counted using some widespread desktop computer (without GPUs or any other co-processors).
Since the results for backtracking algorithms are not very inspiring, I tried a different approach, where results are gathered from several smaller instances of this problem.

Here are the results I've got on 2013's 4-core CPU:

| Size | Time |
|-----:|-----:|
|  20  | 2m53s|
|  21  |28m12s|
|  22  | 3h51m|

## Quick start

Best place to start reading the sources is quadrants1.h

To compile main program, either use cmake or employ compiler directly:
```
gcc -std=c++20 -O3 main.cpp
```

Right now (oct 2022) only GCC works properly.

To run the program, specify the number of threads (4 by default):
```
nqueens 2
```

To divide work into several parts, after the number of threads give the number of parts followed by zero-based part number. For example, for the last piece of 4-part work:
```
nqueens 2 4 3
```

Other configuration parameters (including the board size) should be set inside the sources. They are initially set to some conservative values. CPU instruction set
may be configured in CMakeLists.txt or in compiler command line:
```
gcc -std=c++20 -march=haswell -mtune=haswell -O3 main.cpp
```

To allow prefetch instruction, edit prefetch.h. To allow BMI2 instructions, edit bmintrin.h. Board size and many performance tuning parameters are in solcounter.h.
It also contains a table of parameters for each board size to give a hint what each parameter should look like. Parameter "matchGroupSize" is not in the table and
likely should be changed for too old (<=SSE4) or too new (>=AVX512) processors. Be careful with board size around 24 and above: it may eat up too much memory.

## Algorithms

There are sevaral ways to count the solutions:

* Since N-queens-solution-count is an instance of [generalized exact cover problem](https://en.wikipedia.org/wiki/Exact_cover#Generalizations), it may be solved by
  [Knuth's Algorithm X](https://en.wikipedia.org/wiki/Knuth%27s_Algorithm_X) (aka Dancing Links). Though manipulating links looks like a relatively slow task, this
  algorithm is pretty fast because it chooses optimal row or column on each step.

* Other idea is to represent the chessboard as four bitsets (for free/occupied rows, columns, and two sets of diagonals) and use bitwise operations to juggle these
  bitsets. This approach is very simple and very popular (probably since 64-bit computers became widespread). Usually these algorithms process the board row by row,
  sequentially, that is sub-optimal compared to Algorithm X. But bitwise operations are very fast, so these algorithms may perform slightly better.

* Of course two previous approaches may work together. Bitwise operations **and** optimal row or column on each step. This is more difficult to implement and probably
  requires more advanced bitwise operations like popcount and countr_zero. Performance could be even better but not by much.

* All of the above algorithms are variants of backtracking approach. And to get better results we could try something completely different. Let's split the chessboard
  into four quarters, prepare partial solution for each quarter, and then join parts together while counting every successfull attempt. This idea is not new but I have
  found only one related [link](http://deepgreen.game.coocan.jp/NQueens/nqueen_index.htm) on the Internet. Performance is noticeably better (for the price of large memory usage).

## C++

C++20 contains many features useful for this project, especially bit manipulation functions that make sources much more portable. The downside is that not many compilers
fit the task yet. Right now (oct 2022) only GCC (version 11+) can compile this project.
