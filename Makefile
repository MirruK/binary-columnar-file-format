all : # The canonical default target.


BUILD ?= debug
build_dir := ${CURDIR}/build/${BUILD}
exes := # Executables to build

# ==== Begin define executable test-bincoff
exes += test-bincoff
objects.test-bincoff = bincoff.o sized_bincoff_buffer.o tests.o
# ==== End define executable test-bincoff

# ==== Begin define executable test-sized-bincoff-buffer
exes += test-sized-bincoff-buffer
objects.test-sized-bincoff-buffer = sized_bincoff_buffer.o test_sized_bincoff_buffer.o
# ==== End define executable test-sized-bincoff-buffer


# ==== Begin define executable cli
exes += bincoff-cli
objects.bincoff-cli = bincoff.o sized_bincoff_buffer.o main.o
# ==== End define executable cli

# ==== Begin define executable debug-bincoff
exes += debug-bincoff
objects.debug-bincoff = bincoff.o sized_bincoff_buffer.o debug.o
# ==== End define executable debug-bincoff


# ==== Begin rest of boilerplate.
SHELL := /bin/bash
COMPILER=clang

CC.gcc := /bin/gcc
LD.gcc := /bin/gcc


CC.clang := /bin/clang
LD.clang := /bin/clang


CC := ${CC.${COMPILER}}
LD := ${LD.${COMPILER}}


CFLAGS.gcc.debug := -Og -fstack-protector-all
CFLAGS.gcc.release := -O3 -march=native -Ivendor/criterion-2.4.3/include -DNDEBUG
CFLAGS.gcc := -Wall -Wextra -Werror -g -fmessage-length=0 ${CFLAGS.gcc.${BUILD}}

CFLAGS.clang.debug := -O0 -fstack-protector-all -Ivendor/criterion-2.4.3/include
CFLAGS.clang.release := -O3 -Ivendor/criterion-2.4.3/include -DNDEBUG
CFLAGS.clang := -Wall -Wextra -Werror -g -fmessage-length=0 ${CFLAGS.clang.${BUILD}}

CFLAGS := ${CFLAGS.${COMPILER}}

LDFLAGS.debug :=
LDFLAGS.release :=

# Add flags for linking and resolving path to vendored criterion library
LDFLAGS.${BUILD} += -Lvendor/criterion-2.4.3/lib/ -Wl,-rpath,'$$ORIGIN/../../vendor/criterion-2.4.3/lib'
LDFLAGS := ${LDFLAGS.${BUILD}}

LDLIBS.test-bincoff := -lcriterion
LDLIBS.test-sized-bincoff-buffer := -lcriterion
LDLIBS.bincoff-cli :=
LDLIBS.debug-bincoff :=

COMPILE.C = ${CC} -c $(abspath $<) -o $@ ${CFLAGS}

all : ${exes:%=${build_dir}/%} # Build all exectuables.

.SECONDEXPANSION:
# Build all executables, exes (e.g. test-bincoff and main) => build/test-bincoff and build/main

${exes:%=${build_dir}/%}: ${build_dir}/% : $$(addprefix ${build_dir}/,$${objects.$$*}) | ${build_dir}
	$(strip ${LD} -o $@ $(LDFLAGS) $(filter-out Makefile,$^) ${LDLIBS.$*})

# Run an executable. E.g. make test-bincoff
${exes:%=run_%} : run_% : ${build_dir}/%
	@echo "---- running $< ----"
	time $<

# Create the build directory on demand.
${build_dir} :
	mkdir -p $@

# Compile a C source into .o.
${build_dir}/%.o : src/%.c Makefile | ${build_dir}
	$(strip ${COMPILE.C})

clean :
	rm -rf ${build_dir}

.PHONY : clean all $(exes:%=run_%)
