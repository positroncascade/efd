# Build Guide

# Main Instructions

## Prerequisites

* For running the build:
    1. GCC 4.8.0 >=; Clang 3.5 >= (recommended);
    1. CMake: version 2.8.8 >=;
    2. Any building tool able to use CMake generated scripts.
* For compiling:
    1. PreCiCe;
    2. PETSc: version 3.3, with enabled MPI support.
    3. Eigen: version 3.2 >=
    4. Boost: version 1.57 >=; with locale, filesystem, program_options, regex, system,
       thread (required for PreCiCe)
    5. Python libraries: version 2.7
    6. LibXml2
    7. MPI
    8. HDF5

## Building instructions

The common procedure for CMake is used, therefore for more details go to the
CMake user guide.

### Steps

1.  Execute configuration CMake command.

    For example:

        cmake -G "Unix Makefiles"
              -DCMAKE_BUILD_TYPE=Release         # Release or Debug
              -DCMAKE_INSTALL_PREFIX=../install # Installation Path
              ../

2.  Execute building command.

    For example:

        make

### Dependency lookup

The build system will try to find dependencies by itself, for more information
go to the CMake manual (`find_path` for includes; `find_library` for libraries).

> **NOTE:**
> MPI implementation must be the same that was used to compile PETSc.

> **NOTE:**
> Python libraries must be the same that was used to compile PreCICE.

#### Configuration of the dependency lookup

If the dependency are not visible throughout environment, or properly install in
the system, then use the instructions below before CMake configuration for a
corresponding dependency.

All the dependencies are configured in the same fashion.

For each dependency there are variable names.
Use this variable names in one or both ways to control the search of a
dependency on the system:

+ as environment variables;
+ as CMake configuration variables.

The lookup of the following dependencies could be configured:

*   **PETSc**

    + `PETSC_DIR` --- directory in which PETSc resides.
    + *(optional)* `PETSC_ARCH` --- build architecture.

*   **MPI**

    + `MPI_HOME` --- directory, where MPI resides.

*   **PreCICE**

    + *(optional)* `PRECICE_INCLUDE_DIRS` --- directories, where PreCICE's includes
      resides.
    + *(optional)* `PRECICE_LIBRARY_DIRS` --- directories, where PreCICE's libraries
      resides.
    + *(optional)* `PRECICE_DIR` --- directory, where PreCICE resides.

    Using variables above the order of search is described below, it moves to
    the next step until the check of some conditions succeed.

    Lookup for includes in values of the variables in the order:

    1. `PRECICE_INCLUDE_DIRS`
    2. `PRECICE_DIR`

    Lookup for libraries in values of the variables in the order:

    1. `PRECICE_LIBRARY_DIRS`
    2. `PRECICE_DIR`

*   **Eigen**

    + `EIGEN_INCLUDE_DIRS` --- directories, where Eigen's includes resides.

*   **Boost**

    + `BOOST_ROOT` (or `BOOSTROOT`) --- preferred installation prefix.
    + `BOOST_INCLUDEDIR` --- preferred include directory e.g. '<prefix>/include'.
    + `BOOST_LIBRARYDIR` --- preferred library directory e.g. '<prefix>/lib'.
    + `Boost_NO_SYSTEM_PATHS` --- set to 'ON' to disable searching in locations
      not specified by these hint variables. Default is 'OFF'.
    + `Boost_ADDITIONAL_VERSIONS` --- list of Boost versions not known to this
      module (Boost install locations may contain the version).

*   **Python libraries**

    + `PYTHON_LIBRARY` --- path to the python library.
    + `PYTHON_INCLUDE_DIR` --- path to where Python.h is found.

# Optional Instructions

## Prerequisites

In addition to main prerequisites (mentioned above) it is required to have the
following ones:

* For running the build:
    1. Unix Makefiles build system is recommended;
    2. Python: version 2.7 >=.

## Building instructions

Here a few wrappers are used to simplify, speed up CMake usage.
It is mostly effective for development, when the above commands have to be
executed many times.
These wrappers are tiny and implicitly run routine commands.
To explore what is inside it is offered to look in the code which is supposed
to be self-explanatory, the usage examples are presented below.

### Steps

1.  Execute CMake configuration with `configure` command.
    - By default, generate Unix Makefiles scripts.
    - By default, use 'build' directory in the `configure` script directory as a
      build directory.
    - By default, configure 'install' directory in the `configure` script
      directory as an install directory.

    For example:

        ./configure

    or

        ./configure -G "Ninja"

    or

        ./configure -G "Ninja"
                    -c Release           # Release or Debug
                    -prefix ../install  # Installation path
                    ANY_CMAKE_FLAG=VALUE # Pass flags to CMake,
                    ...                  # will be prefixed by '-D'
                    ...                  # automatically

2. Execute building with `compile` command.
    - By default, use Unix Makefiles.

    For example:

        ./compile

    or to use `ninja` as a build tool

        ./compile ninja

     or set the `V` variable to `1`, what, in case of the Unix Makefiles,
     makes output verbose:

        ./compile V=1 # Make Unix Makefiles verbose

     or pass any flags to the `compile` command that must be passed to the low-level
     build system command:

        ./compile -MyFlagToBuildSystem # Any flags will be passed
                  ...                  # to build system command