all: sim_main

# Automatically detect whether the core is C or C++
# Must have either sim_core.c or sim_core.cpp - NOT both
SRC_CORE = $(wildcard core_api.c core_api.cpp)
SRC_GIVEN = main.c sim_api.c
EXTRA_DEPS = sim_api.h core_api.h

OBJ_GIVEN = $(patsubst %.c,%.o,$(SRC_GIVEN))
OBJ_CORE = core_api.o
OBJ = $(OBJ_GIVEN) $(OBJ_CORE)

#$(info OBJ=$(OBJ))

CFLAGS = -std=c99 -g

$(OBJ_GIVEN): %.o: %.c
	gcc -c $(CFLAGS) -o $@ $^

ifeq ($(SRC_CORE),core_api.c)
sim_main: $(OBJ)
	gcc -o $@ $(OBJ)

sim_core.o: sim_core.c
	gcc -c $(CFLAGS) -o $@ $^

else
sim_main: $(OBJ)
	g++ -o $@ $(OBJ)

sim_core.o: sim_core.cpp
	g++ -c -o $@ $^
endif

.PHONY: clean
clean:
	rm -f sim_main $(OBJ_GIVEN) $(OBJ_CORE)
