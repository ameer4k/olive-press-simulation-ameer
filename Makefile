# Olive press simulation - ENCS4330 project 1
#
#   make            build the simulator and the OpenMP benchmark
#   make run        GUI with config/default.cfg
#   make headless   same line without a window (good over ssh)
#   make bench      sequential vs OpenMP timing
#   make test       unit tests + headless end to end runs
#   make debug      -O0 build for gdb
#   make clean

CC      ?= gcc
CSTD    := -std=gnu11
WARN    := -Wall -Wextra -Wshadow -Wformat=2 -Wno-unused-parameter
OPT     ?= -O2
CFLAGS  ?= $(OPT) -g
# kept apart from CFLAGS so `make CFLAGS=...` cannot drop the include paths
CPPFLAGS := -Iinclude -Iprocesses -Igraphics -MMD -MP
ALL_CFLAGS = $(CSTD) $(WARN) $(CPPFLAGS) $(CFLAGS)
LDLIBS  := -lglut -lGLU -lGL -lm

BUILD   := build
BIN     := olive_press
BENCH   := omp_bench
CONFIG  ?= config/default.cfg

# Every stage file a teammate adds under processes/ is picked up here and
# overrides the matching weak mock in processes/mock_stages.c.
SRCS    := $(wildcard src/*.c common/*.c processes/*.c graphics/*.c)
OBJS    := $(SRCS:%.c=$(BUILD)/%.o)
CORE    := $(filter-out $(BUILD)/src/% $(BUILD)/graphics/%,$(OBJS))

TESTS   := $(BUILD)/test_ipc $(BUILD)/test_config

.PHONY: all run headless bench test debug clean

all: $(BIN) $(BENCH)

$(BIN): $(OBJS)
	$(CC) $(ALL_CFLAGS) -o $@ $^ $(LDLIBS)

$(BENCH): benchmark/openmp_benchmark.c
	@mkdir -p $(BUILD)
	$(CC) $(ALL_CFLAGS) -fopenmp -o $@ $< -lm

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

$(BUILD)/test_%: tests/test_%.c $(CORE)
	$(CC) $(ALL_CFLAGS) -o $@ $^ -lm

run: $(BIN)
	./$(BIN) $(CONFIG)

headless: $(BIN)
	./$(BIN) $(CONFIG) --headless </dev/null

bench: $(BENCH)
	./$(BENCH)

test: $(BIN) $(TESTS)
	@for t in $(TESTS); do ./$$t || exit 1; done
	@sh tests/run_headless.sh ./$(BIN)

debug:
	$(MAKE) clean
	$(MAKE) OPT=-O0

clean:
	rm -rf $(BUILD) $(BIN) $(BENCH)

-include $(OBJS:.o=.d)
