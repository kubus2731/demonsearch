CC       := cc
TARGET   := demonsearch
SRC      := $(wildcard src/*.c)
OBJ      := $(SRC:.c=.o)
DEP      := $(SRC:.c=.d)

CPPFLAGS := -Iinclude
CFLAGS   := -std=gnu11 -Wall -Wextra -Wpedantic -O2
LDFLAGS  :=
LDLIBS   :=

.PHONY: all clean debug release help

all: release

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

release: $(TARGET)

$(TARGET): $(OBJ)
	@echo "  [LD]  $@"
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

src/%.o: src/%.c
	@echo "  [CC]  $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -c -o $@ $<

clean:
	@echo "  [CLEAN]"
	@rm -f $(TARGET) $(OBJ) $(DEP)

help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build release binary (default)"
	@echo "  debug   - Build with debug symbols"
	@echo "  clean   - Remove generated files"
	@echo "  help    - Show this message"

-include $(DEP)
