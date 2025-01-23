CC = gcc
CCFLAGS = -Wall

TARGET = imcsh
SRCS = imcsh.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
    $(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to build object files
%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

clean:
    rm -f $(OBJS) $(TARGET)

# Rule to run the shell
run: $(TARGET)
    ./$(TARGET)

# Phony targets
.PHONY: all clean run