# Source Code for Bachelor Thesis Project

This directory contains the C++ source code associated with the thesis. 

## Files:

- `main.cpp`: The primary executable source file.
- `BoostGraph.hpp`: The header file containing necessary Boost Graph library functions.

## Compilation:

To compile the solver, you will need the Boost library. You can compile the solver using the following command:

```
g++ -I/path_to_boost -o main main.cpp
```


By default, you would replace `/path_to_boost` with `/usr/local/boost_1_83_0` if you have installed the Boost library in its default location. Make sure you have the correct path to the Boost library on your system.

## Dependencies:

- [Boost Library](https://www.boost.org/users/download/): A comprehensive C++ library used for this project. 

Make sure to provide the correct path to your Boost library when compiling, as mentioned above. Version used in the project is 1.83.0.

## Tuning for Experiments:

Inside the `main.cpp` file, there are several flags and parameters located at the beginning of the file. These can be fine-tuned if necessary for further experimentation.

