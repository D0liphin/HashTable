# ****************************** CHANGE THESE ******************************* #
# the file containing the entry point for this program
ENTRY_POINT=./src/main.cpp
# All the tests that should be run -- by default runs all source files in 
# `./test`. Note that this filters out non-source files later in the Makefile 
RUN_TESTS=$(wildcard ./tests/*)
# all directories that might contain header files
INCLUDE=. ./include
# all directories that might contain source files
SRC=. ./src ./include
# all extensions that should be considered as C++ source files
SRC_EXTS=cpp cxx cc
# the C++ standard to use
STD=c++11
# *************************************************************************** #

# some ANSI escape codes
GREEN=\033[0;32m
RED=\033[0;31m
BOLD=\033[1m
RESET=\033[0m

# the compiler to be used
CC=g++
# flags for compiling translation units
CFLAGS=-std=$(STD) -Wall -Wextra -g $(foreach dir, $(INCLUDE),-I $(dir))
# flags for linking
LFLAGS=
# where all generated files are stored
TARGET=./target
# name of the built executable
EXEC=main

# 1 arg: convert this source file name into an object file name
define make_o_files
$(foreach ext,$(SRC_EXTS),$(patsubst ./%.$(ext),$(TARGET)/%.$(ext).o,\
$(filter %.$(ext),$(1))))
endef

# list of all source files with directory information
CPP_FILES=$(foreach dir,$(SRC),\
				$(foreach ext,$(SRC_EXTS),\
					$(wildcard $(dir)/*.$(ext))))

# list of all .o files (inside target) 
O_FILES=$(call make_o_files,$(CPP_FILES))

# list of all .d files
D_FILES=$(patsubst %.o,%.d,$(O_FILES))
-include $(D_FILES)

# filter test files to only include valid source files
TESTS=$(foreach ext,$(SRC_EXTS),$(filter %.$(ext),$(RUN_TESTS)))

all: build

# remove target
clean:
	@rm -rf ./target
	@echo "$(GREEN)***$(RESET) All cleaned up, boss! (￣ー￣)ゞ $(GREEN)***\
	$(RESET)"

# build the project, creating ./target/main
build: $(TARGET)/main

# continuation of the above, but making sure we don't relink each time
$(TARGET)/main: $(O_FILES) Makefile
	@mkdir -p $(TARGET)
# only compile if there is actually something to compile
ifdef CPP_FILES
	$(CC) $(LFLAGS) -o $(TARGET)/$(EXEC) $(O_FILES)
	@echo "$(GREEN)***$(RESET) done! \(^-^)/ $(GREEN)***$(RESET)"
else
	@echo "$(RED)Error$(RESET): no source files found -- nothing to build!\
	 (-_-)"
endif

# variables for testing (basically just exclude the specified entry point
# from compilation)
NO_ENTRY_POINT_CPP_FILES=$(filter-out $(ENTRY_POINT),$(CPP_FILES))
NO_ENTRY_POINT_O_FILES=$(call make_o_files,$(NO_ENTRY_POINT_CPP_FILES))
TEST_CPP_FILES=$(NO_ENTRY_POINT_CPP_FILES) $(TESTS)
TEST_O_FILES=$(call make_o_files,$(TEST_CPP_FILES))
TEST_D_FILES=$(patsubst %.o,%.d,$(TEST_O_FILES))
-include $(TEST_D_FILES)

# (1) convert a source file name (or names) to an executable file path
define create_exec_files 
$(foreach ext,$(SRC_EXTS),$(patsubst .%.$(ext),%,$(filter %.$(ext),$(1))))
endef

# run all tests
run-tests:
	@$(foreach test,$(call create_exec_files,$(TESTS)),\
	echo "\n$(BOLD)olibuild: running test "$(test)"$(RESET)" ;\
	echo "======================================================" ;\
	$(TARGET)$(test) ;\
	echo "======================================================" ;)\
	echo ""

# build all tests
build-tests: $(TEST_O_FILES) Makefile
ifneq ($(TESTS),)
	@$(foreach test,$(TESTS),\
		echo "$(BOLD)olibuild: building test:" $(test) "$(RESET)"; \
		g++ -o $(TARGET)$(call create_exec_files,$(test))\
		$(call make_o_files,$(test)) $(NO_ENTRY_POINT_O_FILES) ; ) echo ""
else
	@echo "$(RED)Error$(RESET): no test files found -- nothing to test! (-_-)"
endif
	
# build each .o file from the appropriate source file
# Since .o files contain the source file information after stripping $(TARGET) 
# and .o, we can use this to rely on the appropriate source file immediately
$(TARGET)/%.o: ./%
	@mkdir -p $(dir ./$@)
	$(CC) $(CFLAGS) -MMD -MP -c -o ./$@ $<

# runs the main program
run:
	@./target/main

# print the various files to be created and the files from which they will be
# built (I don't know why i need to offset O_FILES and CPP_FILES)
print-src: 
	@echo "o files      ="$(O_FILES)
	@echo "source files ="$(CPP_FILES)
	@echo "d files      = "$(D_FILES)
	@echo "tests        = "$(TESTS)

# initialise a recommended directory structure for an olibuild project
MAIN=./src/main.cpp
init:
	@mkdir -p ./src
	@sudo touch $(MAIN)
	@sudo chmod a+rw $(MAIN)
	@echo "#include <iostream>" >> $(MAIN)
	@echo "" >> $(MAIN)
	@echo "int main() {" >> $(MAIN)
	@echo "	std::cout << \"Hello, world!\" << std::endl;" >> $(MAIN)
	@echo "	return 0;" >> $(MAIN)
	@echo "}" >> $(MAIN)

	@mkdir -p ./include
	@mkdir -p ./tests
	@echo "$(GREEN)$(BOLD)*** $(RESET)Initialised olibuild project! \\(^-^)/ \
	$(GREEN)$(BOLD)*** $(RESET)"

# prints help about the usage of this Makefile
help:
	@echo ""
	@echo "Simple C++ build utility"
	@echo ""
	@echo "USAGE:"
	@echo "    make [COMMAND]"
	@echo ""
	@echo "COMMAND:"
	@echo "    init         Create the recommended project structure"
	@echo "    build        Compile all source files into ./target/main"
	@echo "    clean        Remove the target directory"
	@echo "    run          Shorthand for ./target/main"
	@echo "    help         Display this message"
	@echo "    print-src    Print files being used by olibuild"
	@echo "    build-tests  Compiles all tests (for now, this will always relink)"
	@echo "    run-tests    Runs all built tests"
	@echo ""
	@echo "NOTE: if all your tests are in ./tests you can clean just the binaries"
	@echo "      generated from your tests with `sudo rm -rf ./target/tests`"
	@echo ""

