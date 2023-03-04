TARGET_EXEC := ithemer
DEBUG_EXEC := ithemer-debug
TEST_EXEC := ithemer-tests
TEST_DEBUG_EXEC := ithemer-debug-tests

BASE_BUILD_DIR := build
BUILD_DIR := $(BASE_BUILD_DIR)/obj
DEBUG_BUILD_DIR := $(BASE_BUILD_DIR)/obj_debug
SRC_DIR := src
INC_DIR := include
TEST_BUILD_DIR := $(BASE_BUILD_DIR)/tests
TEST_SRC_DIR := $(SRC_DIR)/tests
TEST_INC_DIR := $(INC_DIR)/tests
TEST_DEBUG_BUILD_DIR := $(BASE_BUILD_DIR)/tests_debug

CC := gcc

GLIBFLAGS := $(shell pkg-config --cflags --libs gobject-2.0)
STD_FLAGS := -I$(INC_DIR) $(GLIBFLAGS) -Wall -Wextra -pedantic -Wno-unused-parameter $(PROFILING_OPTS) # -lm -pthread -Wconversion
CFLAGS := -O2 $(STD_FLAGS)
DEBUG_FLAGS := -O0 -g3 $(STD_FLAGS) #-ggdb3
LDFLAGS := $(PROFILING_OPTS)

# get .c files, remove original path and turn into .o
SRCS_ALL := $(shell ls $(SRC_DIR) | grep '.c')

# gets files common between tests and src to avoid conflicts
REPETITIONS := $(shell ./Scripts/get_file_repetitions.sh $(SRC_DIR) $(TEST_SRC_DIR))
OBJ_WITHOUT_REPETITIONS := $(subst .c,.o,$(filter-out $(REPETITIONS), $(SRCS_ALL)))

SRCS_DEBUG := $(SRCS_ALL)

SRCS_TEST := $(shell ls $(TEST_SRC_DIR) | grep '.c')

SRCS_TEST_DEBUG := $(SRCS_TEST)

OBJS_ALL := $(subst .c,.o,$(SRCS_ALL))
OBJS_ALL := $(OBJS_ALL:%=$(BUILD_DIR)/%)

OBJS_DEBUG := $(subst .c,.o,$(SRCS_DEBUG))
OBJS_DEBUG := $(OBJS_DEBUG:%=$(DEBUG_BUILD_DIR)/%)

OBJS_TEST := $(subst .c,.o,$(SRCS_TEST))
OBJS_TEST := $(OBJS_TEST:%=$(TEST_BUILD_DIR)/%) $(OBJ_WITHOUT_REPETITIONS:%=$(BUILD_DIR)/%) # $(filter-out $(REPETITIONS), $(OBJS_ALL)) #$(TEST_BUILD_DIR)/main.o $(TEST_BUILD_DIR)/test_header.o

OBJS_DEBUG_TEST := $(subst .c,.o,$(SRCS_TEST_DEBUG))
OBJS_DEBUG_TEST := $(OBJS_DEBUG_TEST:%=$(TEST_DEBUG_BUILD_DIR)/%) $(OBJ_WITHOUT_REPETITIONS:%=$(DEBUG_BUILD_DIR)/%)


# make .d
DEPS := $(OBJS_ALL:.o=.d) $(OBJS_DEBUG:.o=.d) $(OBJS_TEST:.o=.d)

CPPFLAGS := -MMD -MP

# all??
.PHONY: all
all: $(TARGET_EXEC)

$(TARGET_EXEC): $(OBJS_ALL)
	$(CXX) $(OBJS_ALL) $(GLIBFLAGS) -o $@ $(LDFLAGS)

# C source
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: debug
debug: $(DEBUG_EXEC)

$(DEBUG_EXEC): $(OBJS_DEBUG)
	$(CXX) $(OBJS_DEBUG) $(GLIBFLAGS) -o $@ $(LDFLAGS)

$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(DEBUG_FLAGS) -c $< -o $@

.PHONY: tests
tests: $(TEST_EXEC) all

$(TEST_EXEC): $(OBJS_TEST)
	$(CXX) $(OBJS_TEST) $(GLIBFLAGS) -o $@ $(LDFLAGS)

$(TEST_BUILD_DIR)/%.o: $(TEST_SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -I$(TEST_INC_DIR) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: tests_debug
tests_debug: $(TEST_DEBUG_EXEC) debug

$(TEST_DEBUG_EXEC): $(OBJS_DEBUG_TEST)
	$(CXX) $(OBJS_DEBUG_TEST) $(GLIBFLAGS) -o $@ $(LDFLAGS)

$(TEST_DEBUG_BUILD_DIR)/%.o: $(TEST_SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -I$(TEST_INC_DIR) $(CPPFLAGS) $(DEBUG_FLAGS) -c $< -o $@

.PHONY: clean
RM_DIRS := $(BASE_BUILD_DIR) $(TARGET_EXEC) $(DEBUG_EXEC) $(TEST_EXEC) $(TEST_DEBUG_EXEC)
RED := \033[0;31m
NC := \033[0m
clean:
	@echo -ne '$(RED)Removing:\n$(NC) $(RM_DIRS:%=%\n)'
	-@rm -r $(RM_DIRS) 2>/dev/null || true

-include $(DEPS)
