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
include CMakeFiles/test_cargstest.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/test_cargstest.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/test_cargstest.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_cargstest.dir/flags.make

CMakeFiles/test_cargstest.dir/test/cargstest.c.o: CMakeFiles/test_cargstest.dir/flags.make
CMakeFiles/test_cargstest.dir/test/cargstest.c.o: /home/tanmeet/Desktop/libvnc/libvncserver-nautilus/test/cargstest.c
CMakeFiles/test_cargstest.dir/test/cargstest.c.o: CMakeFiles/test_cargstest.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tanmeet/Desktop/cs446/nautilus/libvnc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/test_cargstest.dir/test/cargstest.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/test_cargstest.dir/test/cargstest.c.o -MF CMakeFiles/test_cargstest.dir/test/cargstest.c.o.d -o CMakeFiles/test_cargstest.dir/test/cargstest.c.o -c /home/tanmeet/Desktop/libvnc/libvncserver-nautilus/test/cargstest.c

CMakeFiles/test_cargstest.dir/test/cargstest.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/test_cargstest.dir/test/cargstest.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tanmeet/Desktop/libvnc/libvncserver-nautilus/test/cargstest.c > CMakeFiles/test_cargstest.dir/test/cargstest.c.i

CMakeFiles/test_cargstest.dir/test/cargstest.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/test_cargstest.dir/test/cargstest.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tanmeet/Desktop/libvnc/libvncserver-nautilus/test/cargstest.c -o CMakeFiles/test_cargstest.dir/test/cargstest.c.s

# Object files for target test_cargstest
test_cargstest_OBJECTS = \
"CMakeFiles/test_cargstest.dir/test/cargstest.c.o"

# External object files for target test_cargstest
test_cargstest_EXTERNAL_OBJECTS =

test/cargstest: CMakeFiles/test_cargstest.dir/test/cargstest.c.o
test/cargstest: CMakeFiles/test_cargstest.dir/build.make
test/cargstest: libvncserver.so.0.9.14
test/cargstest: libvncclient.so.0.9.14
test/cargstest: /usr/lib/x86_64-linux-gnu/libpng.so
test/cargstest: /usr/lib/x86_64-linux-gnu/libz.so
test/cargstest: /usr/lib/x86_64-linux-gnu/libjpeg.so
test/cargstest: /usr/lib/x86_64-linux-gnu/libssl.so
test/cargstest: /usr/lib/x86_64-linux-gnu/libcrypto.so
test/cargstest: /usr/lib/x86_64-linux-gnu/libgnutls.so
test/cargstest: /usr/lib/x86_64-linux-gnu/libssl.so
test/cargstest: /usr/lib/x86_64-linux-gnu/libcrypto.so
test/cargstest: /usr/lib/x86_64-linux-gnu/libgnutls.so
test/cargstest: CMakeFiles/test_cargstest.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tanmeet/Desktop/cs446/nautilus/libvnc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable test/cargstest"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_cargstest.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_cargstest.dir/build: test/cargstest
.PHONY : CMakeFiles/test_cargstest.dir/build

CMakeFiles/test_cargstest.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_cargstest.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_cargstest.dir/clean

CMakeFiles/test_cargstest.dir/depend:
	cd /home/tanmeet/Desktop/cs446/nautilus/libvnc/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tanmeet/Desktop/libvnc/libvncserver-nautilus /home/tanmeet/Desktop/libvnc/libvncserver-nautilus /home/tanmeet/Desktop/cs446/nautilus/libvnc/build /home/tanmeet/Desktop/cs446/nautilus/libvnc/build /home/tanmeet/Desktop/cs446/nautilus/libvnc/build/CMakeFiles/test_cargstest.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/test_cargstest.dir/depend

