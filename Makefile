BIN_DIR := bin

TARGET_EXEC_DAEMON := $(BIN_DIR)/ithemer-daemon
TARGET_EXEC_MENU := $(BIN_DIR)/ithemer-menu
TARGET_EXEC_QUERY := $(BIN_DIR)/ithemer-query

DEBUG_EXEC_MENU := $(BIN_DIR)/ithemer-debug-menu
DEBUG_EXEC_DAEMON := $(BIN_DIR)/ithemer-debug-daemon
DEBUG_EXEC_QUERY := $(BIN_DIR)/ithemer-debug-query

BASE_BUILD_DIR := build
BUILD_DIR_MENU := $(BASE_BUILD_DIR)/obj/menu
BUILD_DIR_DAEMON := $(BASE_BUILD_DIR)/obj/daemon
BUILD_DIR_QUERY := $(BASE_BUILD_DIR)/obj/query

DEBUG_BUILD_DIR_MENU := $(BASE_BUILD_DIR)/obj_debug/menu
DEBUG_BUILD_DIR_DAEMON := $(BASE_BUILD_DIR)/obj_debug/daemon
DEBUG_BUILD_DIR_QUERY := $(BASE_BUILD_DIR)/obj_debug/query

SRC_DIR_MENU := src/menu
SRC_DIR_DAEMON := src/daemon
SRC_DIR_QUERY := src/query

INC_DIR_MENU := include/menu
INC_DIR_DAEMON := include/daemon
INC_DIR_QUERY := include/query

CC := gcc

GLIBFLAGS := $(shell pkg-config --cflags --libs gobject-2.0)
STD_FLAGS := -O2 $(GLIBFLAGS) -Wall -Wextra -pedantic -Wno-unused-parameter $(PROFILING_OPTS) # -lm -pthread -Wconversion
STD_DEBUG_FLAGS := -O0 -g3 $(GLIBFLAGS) -Wall -Wextra -pedantic -Wno-unused-parameter $(PROFILING_OPTS)
MENU_FLAGS := -I$(INC_DIR_MENU) $(STD_FLAGS)
DAEMON_FLAGS := -I$(INC_DIR_DAEMON) $(STD_FLAGS)
QUERY_FLAGS := -I$(INC_DIR_QUERY) $(STD_FLAGS)

# CFLAGS := -O2 $(STD_FLAGS)

DEBUG_FLAGS_MENU := -I$(INC_DIR_MENU) $(STD_DEBUG_FLAGS)
DEBUG_FLAGS_DAEMON := -I$(INC_DIR_DAEMON) $(STD_DEBUG_FLAGS)
DEBUG_FLAGS_QUERY := -I$(INC_DIR_QUERY) $(STD_DEBUG_FLAGS)

LDFLAGS := $(PROFILING_OPTS)

# is there a better way to do this?????????????
SRC_MENU := $(shell ls $(SRC_DIR_MENU) | grep '.c')
OBJ_MENU := $(subst .c,.o,$(SRC_MENU))
OBJ_DEBUG_MENU := $(OBJ_MENU:%=$(DEBUG_BUILD_DIR_MENU)/%)
OBJ_MENU := $(OBJ_MENU:%=$(BUILD_DIR_MENU)/%)

SRC_DAEMON := $(shell ls $(SRC_DIR_DAEMON) | grep '.c')
OBJ_DAEMON := $(subst .c,.o,$(SRC_DAEMON))
OBJ_DEBUG_DAEMON := $(OBJ_DAEMON:%=$(DEBUG_BUILD_DIR_DAEMON)/%)
OBJ_DAEMON := $(OBJ_DAEMON:%=$(BUILD_DIR_DAEMON)/%)

SRC_QUERY := $(shell ls $(SRC_DIR_QUERY) | grep '.c')
OBJ_QUERY := $(subst .c,.o,$(SRC_QUERY))
OBJ_DEBUG_QUERY := $(OBJ_QUERY:%=$(DEBUG_BUILD_DIR_QUERY)/%)
OBJ_QUERY := $(OBJ_QUERY:%=$(BUILD_DIR_QUERY)/%)


# make .d
DEPS := $(OBJ_MENU:.o=.d) $(OBJ_DAEMON:.o=.d) $(OBJ_DEBUG_MENU:.o=.d) $(OBJ_DEBUG_DAEMON:.o=.d)

CPPFLAGS := -MMD -MP

.PHONY: all
all: daemon menu query

.PHONY: daemon
daemon: $(TARGET_EXEC_DAEMON)

$(TARGET_EXEC_DAEMON): $(OBJ_DAEMON)
	mkdir -p $(dir $@)
	$(CXX) $(OBJ_DAEMON) $(GLIBFLAGS) -o $@ $(LDFLAGS)

$(BUILD_DIR_DAEMON)/%.o: $(SRC_DIR_DAEMON)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(DAEMON_FLAGS) -c $< -o $@

.PHONY: menu
menu: $(TARGET_EXEC_MENU)

$(TARGET_EXEC_MENU): $(OBJ_MENU)
	mkdir -p $(dir $@)
	$(CXX) $(OBJ_MENU) $(GLIBFLAGS) -o $@ $(LDFLAGS)

# will the mkdir not get repeated?????
$(BUILD_DIR_MENU)/%.o: $(SRC_DIR_MENU)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(MENU_FLAGS) -c $< -o $@

.PHONY: query
query: $(TARGET_EXEC_QUERY)

$(TARGET_EXEC_QUERY): $(OBJ_QUERY)
	mkdir -p $(dir $@)
	$(CXX) $(OBJ_QUERY) $(GLIBFLAGS) -o $@ $(LDFLAGS)

# C source
$(BUILD_DIR_QUERY)/%.o: $(SRC_DIR_QUERY)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(QUERY_FLAGS) -c $< -o $@


.PHONY: debug
debug: debug-menu debug-daemon debug-query

.PHONY: debug-menu
debug-menu: $(DEBUG_EXEC_MENU)

$(DEBUG_EXEC_MENU): $(OBJ_DEBUG_MENU)
	mkdir -p $(dir $@)
	$(CXX) $(OBJ_DEBUG_MENU) $(GLIBFLAGS) -o $@ $(LDFLAGS)

$(DEBUG_BUILD_DIR_MENU)/%.o: $(SRC_DIR_MENU)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(DEBUG_FLAGS_MENU) -c $< -o $@

.PHONY: debug-daemon
debug-daemon: $(DEBUG_EXEC_DAEMON)

$(DEBUG_EXEC_DAEMON): $(OBJ_DEBUG_DAEMON)
	mkdir -p $(dir $@)
	$(CXX) $(OBJ_DEBUG_DAEMON) $(GLIBFLAGS) -o $@ $(LDFLAGS)

$(DEBUG_BUILD_DIR_DAEMON)/%.o: $(SRC_DIR_DAEMON)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(DEBUG_FLAGS_DAEMON) -c $< -o $@

.PHONY: debug-query
debug-query: $(DEBUG_EXEC_QUERY)

$(DEBUG_EXEC_QUERY): $(OBJ_DEBUG_QUERY)
	mkdir -p $(dir $@)
	$(CXX) $(OBJ_DEBUG_QUERY) $(GLIBFLAGS) -o $@ $(LDFLAGS)

$(DEBUG_BUILD_DIR_QUERY)/%.o: $(SRC_DIR_QUERY)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(DEBUG_FLAGS_QUERY) -c $< -o $@

.PHONY: clean
RM_DIRS := $(BASE_BUILD_DIR) $(BIN_DIR)
RED := \033[0;31m
NC := \033[0m
clean:
	@echo -ne '$(RED)Removing:\n$(NC) $(RM_DIRS:%=%\n)'
	-@rm -r $(RM_DIRS) 2>/dev/null || true

-include $(DEPS)
