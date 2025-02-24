# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/augusto/projects/MultiObjective-Tour-Planning

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/augusto/projects/MultiObjective-Tour-Planning/build

# Include any dependencies generated for this target.
include CMakeFiles/tourist_lib.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/tourist_lib.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/tourist_lib.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/tourist_lib.dir/flags.make

CMakeFiles/tourist_lib.dir/src/models.cpp.o: CMakeFiles/tourist_lib.dir/flags.make
CMakeFiles/tourist_lib.dir/src/models.cpp.o: /home/augusto/projects/MultiObjective-Tour-Planning/src/models.cpp
CMakeFiles/tourist_lib.dir/src/models.cpp.o: CMakeFiles/tourist_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/augusto/projects/MultiObjective-Tour-Planning/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/tourist_lib.dir/src/models.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/tourist_lib.dir/src/models.cpp.o -MF CMakeFiles/tourist_lib.dir/src/models.cpp.o.d -o CMakeFiles/tourist_lib.dir/src/models.cpp.o -c /home/augusto/projects/MultiObjective-Tour-Planning/src/models.cpp

CMakeFiles/tourist_lib.dir/src/models.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/tourist_lib.dir/src/models.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/augusto/projects/MultiObjective-Tour-Planning/src/models.cpp > CMakeFiles/tourist_lib.dir/src/models.cpp.i

CMakeFiles/tourist_lib.dir/src/models.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/tourist_lib.dir/src/models.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/augusto/projects/MultiObjective-Tour-Planning/src/models.cpp -o CMakeFiles/tourist_lib.dir/src/models.cpp.s

CMakeFiles/tourist_lib.dir/src/utils.cpp.o: CMakeFiles/tourist_lib.dir/flags.make
CMakeFiles/tourist_lib.dir/src/utils.cpp.o: /home/augusto/projects/MultiObjective-Tour-Planning/src/utils.cpp
CMakeFiles/tourist_lib.dir/src/utils.cpp.o: CMakeFiles/tourist_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/augusto/projects/MultiObjective-Tour-Planning/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/tourist_lib.dir/src/utils.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/tourist_lib.dir/src/utils.cpp.o -MF CMakeFiles/tourist_lib.dir/src/utils.cpp.o.d -o CMakeFiles/tourist_lib.dir/src/utils.cpp.o -c /home/augusto/projects/MultiObjective-Tour-Planning/src/utils.cpp

CMakeFiles/tourist_lib.dir/src/utils.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/tourist_lib.dir/src/utils.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/augusto/projects/MultiObjective-Tour-Planning/src/utils.cpp > CMakeFiles/tourist_lib.dir/src/utils.cpp.i

CMakeFiles/tourist_lib.dir/src/utils.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/tourist_lib.dir/src/utils.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/augusto/projects/MultiObjective-Tour-Planning/src/utils.cpp -o CMakeFiles/tourist_lib.dir/src/utils.cpp.s

CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o: CMakeFiles/tourist_lib.dir/flags.make
CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o: /home/augusto/projects/MultiObjective-Tour-Planning/src/nsga2.cpp
CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o: CMakeFiles/tourist_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/augusto/projects/MultiObjective-Tour-Planning/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o -MF CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o.d -o CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o -c /home/augusto/projects/MultiObjective-Tour-Planning/src/nsga2.cpp

CMakeFiles/tourist_lib.dir/src/nsga2.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/tourist_lib.dir/src/nsga2.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/augusto/projects/MultiObjective-Tour-Planning/src/nsga2.cpp > CMakeFiles/tourist_lib.dir/src/nsga2.cpp.i

CMakeFiles/tourist_lib.dir/src/nsga2.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/tourist_lib.dir/src/nsga2.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/augusto/projects/MultiObjective-Tour-Planning/src/nsga2.cpp -o CMakeFiles/tourist_lib.dir/src/nsga2.cpp.s

# Object files for target tourist_lib
tourist_lib_OBJECTS = \
"CMakeFiles/tourist_lib.dir/src/models.cpp.o" \
"CMakeFiles/tourist_lib.dir/src/utils.cpp.o" \
"CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o"

# External object files for target tourist_lib
tourist_lib_EXTERNAL_OBJECTS =

libtourist_lib.a: CMakeFiles/tourist_lib.dir/src/models.cpp.o
libtourist_lib.a: CMakeFiles/tourist_lib.dir/src/utils.cpp.o
libtourist_lib.a: CMakeFiles/tourist_lib.dir/src/nsga2.cpp.o
libtourist_lib.a: CMakeFiles/tourist_lib.dir/build.make
libtourist_lib.a: CMakeFiles/tourist_lib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/augusto/projects/MultiObjective-Tour-Planning/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX static library libtourist_lib.a"
	$(CMAKE_COMMAND) -P CMakeFiles/tourist_lib.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tourist_lib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/tourist_lib.dir/build: libtourist_lib.a
.PHONY : CMakeFiles/tourist_lib.dir/build

CMakeFiles/tourist_lib.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/tourist_lib.dir/cmake_clean.cmake
.PHONY : CMakeFiles/tourist_lib.dir/clean

CMakeFiles/tourist_lib.dir/depend:
	cd /home/augusto/projects/MultiObjective-Tour-Planning/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/augusto/projects/MultiObjective-Tour-Planning /home/augusto/projects/MultiObjective-Tour-Planning /home/augusto/projects/MultiObjective-Tour-Planning/build /home/augusto/projects/MultiObjective-Tour-Planning/build /home/augusto/projects/MultiObjective-Tour-Planning/build/CMakeFiles/tourist_lib.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/tourist_lib.dir/depend

