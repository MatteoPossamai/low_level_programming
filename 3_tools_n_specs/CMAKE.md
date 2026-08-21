# CMAKE

Guide to the usage of CMAKE.

- [Source](https://cliutils.gitlab.io/modern-cmake/)

## How to build

Compiles and puts all the artifacts into `build/`

```shell
# Older versions
~/package $ mkdir build
~/package $ cd build
~/package/build $ cmake ..
~/package/build $ make # or cmake --build .

# Newer versions
~/package $ cmake -S. -Bbuild # S source, B build dir
~/package $ cmake --build build
```

### Flags

- `-v`: verbose
- `-j N`: parallel build on `N` cores

## How to install

Copies the artifacts into the final destination (`CMAKE_INSTALL_PREFIX`).

```shell
# From the build directory (pick one)
~/package/build $ make install
~/package/build $ cmake --build . --target install
~/package/build $ cmake --install . # CMake 3.15+ only

# From the source directory (pick one)
~/package $ make -C build install
~/package $ cmake --build build --target install
~/package $ cmake --install build # CMake 3.15+ only
```

## Picking compiler

Only necessary on the first run:

```shell
> CC=clang CXX=clang++ cmake ..
# OR
> CC=gcc CXX=g++ cmake ..
```

## Generator

Usually the default is `make`. Alternative is `ninja`.
For make to run parallel need to specify `-j 2`.

## Options

- `-DCMAKE_BUILD_TYPE`= either Release, Debug, ...
- `DCMAKE_INSTALL_PREFIX`= The location to install to.
- `DBUILD_SHARED_LIBS`= ON or OFF to control the default for shared libs
- `DBUILD_TESTING`= This is a common name for enabling tests

## DOs and DONTs

### DOs

- Keep `cmake` code clean
- Think in target (folders)
- Use consistent aliasing
- Use lower case function names

### DONTs

- No global functions
- Avoid globbing file names (need recompile `cmake`)

## Files

### Version

Always first line like `cmake_minimum_required(VERSION 3.15)`
Ideal: `cmake_minimum_required(VERSION 3.15...4.4)`

### Project setup

Only top level `cmake` file.

`project(MyProject VERSION 1.0
        DESCRIPTION "Very nice project"
        LANGUAGES CXX)`

### Executables

`add_executable(one two.cpp three.h)` creates a executable named `one`
built with all the `*.cpp` files while headers are ignored but good
for IDE integration. `one`here is also a target.

### Libraries

`add_library(one STATIC two.cpp three.h)`. Can be `STATIC`, `SHARED` or `MODULE`.

### Targets

Can:

- Include directories: `target_include_directories(one PUBLIC include)`
- Set compiler flags for a specific target:

```cmake
target_compile_options(one PRIVATE -Wall -Wextra -Wshadow)
```

`PRIVATE` means the flags apply only when compiling `one`.

- Use libraries:

```shell
add_library(another STATIC another.cpp another.h)
target_link_libraries(another PUBLIC one)
```

### Example

```cmake

cmake_minimum_required(VERSION 3.15...4.4)

project(Calculator LANGUAGES CXX)

add_library(calclib STATIC src/calclib.cpp include/calc/lib.hpp)
target_include_directories(calclib PUBLIC include)
target_compile_features(calclib PUBLIC cxx_std_11)

add_executable(calc apps/calc.cpp)
target_link_libraries(calc PRIVATE calclib)
```

### Variables

`set(MY_VARIABLE "value")` creates a variable accessible with
`${MY_VARIABLE}` and available only in the same file.

For list can use `set(MY_LIST "one;two")` or `set(MY_LIST "one" "two")`.

`set(MY_CACHE_VARIABLE "VALUE" CACHE STRING "Description")` so that can
be passed on the CLI.

Can set Enviromental vars with `set(ENV{variable_name} value)`.

### Properties

```cmake
set_property(TARGET TargetName
             PROPERTY CXX_STANDARD 11)

set_target_properties(TargetName PROPERTIES
                      CXX_STANDARD 11)
```

### Functions and programming

Can have functions and `if` statements and arguments.

## Project structure

Have it as flat are you can, with:

- `lib/` for libraries
- `src/` for source code and internals
- `apps/` for executable
- `tests/` for tests.

Avoid nesting as much as possible.
