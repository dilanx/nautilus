# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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
CMAKE_SOURCE_DIR = /home/tanmeet/Desktop/libvnc/libvncserver-nautilus

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tanmeet/Desktop/cs446/nautilus/libvnc/build

# Include any dependencies generated for this target.
include CMakeFiles/examples_simple15.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/examples_simple15.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/examples_simple15.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/examples_simple15.dir/flags.make

CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o: CMakeFiles/examples_simple15.dir/flags.make
CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o: /home/tanmeet/Desktop/libvnc/libvncserver-nautilus/examples/server/simple15.c
CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o: CMakeFiles/examples_simple15.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tanmeet/Desktop/cs446/nautilus/libvnc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o -MF CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o.d -o CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o -c /home/tanmeet/Desktop/libvnc/libvncserver-nautilus/examples/server/simple15.c

CMakeFiles/examples_simple15.dir/examples/server/simple15.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/examples_simple15.dir/examples/server/simple15.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tanmeet/Desktop/libvnc/libvncserver-nautilus/examples/server/simple15.c > CMakeFiles/examples_simple15.dir/examples/server/simple15.c.i

CMakeFiles/examples_simple15.dir/examples/server/simple15.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/examples_simple15.dir/examples/server/simple15.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tanmeet/Desktop/libvnc/libvncserver-nautilus/examples/server/simple15.c -o CMakeFiles/examples_simple15.dir/examples/server/simple15.c.s

# Object files for target examples_simple15
examples_simple15_OBJECTS = \
"CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o"

# External object files for target examples_simple15
examples_simple15_EXTERNAL_OBJECTS =

examples/server/simple15: CMakeFiles/examples_simple15.dir/examples/server/simple15.c.o
examples/server/simple15: CMakeFiles/examples_simple15.dir/build.make
examples/server/simple15: libvncserver.so.0.9.14
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libz.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libjpeg.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libpng.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libz.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libjpeg.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libpng.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libssl.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libcrypto.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libgnutls.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libssl.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libcrypto.so
examples/server/simple15: /usr/lib/x86_64-linux-gnu/libgnutls.so
examples/server/simple15: CMakeFiles/examples_simple15.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tanmeet/Desktop/cs446/nautilus/libvnc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable examples/server/simple15"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/examples_simple15.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/examples_simple15.dir/build: examples/server/simple15
.PHONY : CMakeFiles/examples_simple15.dir/build

CMakeFiles/examples_simple15.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/examples_simple15.dir/cmake_clean.cmake
.PHONY : CMakeFiles/examples_simple15.dir/clean

CMakeFiles/examples_simple15.dir/depend:
	cd /home/tanmeet/Desktop/cs446/nautilus/libvnc/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tanmeet/Desktop/libvnc/libvncserver-nautilus /home/tanmeet/Desktop/libvnc/libvncserver-nautilus /home/tanmeet/Desktop/cs446/nautilus/libvnc/build /home/tanmeet/Desktop/cs446/nautilus/libvnc/build /home/tanmeet/Desktop/cs446/nautilus/libvnc/build/CMakeFiles/examples_simple15.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/examples_simple15.dir/depend

