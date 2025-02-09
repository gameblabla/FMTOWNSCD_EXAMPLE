CC = i386-pc-run386-gcc
AS = i386-pc-run386-as
CFLAGS = -O2 -Wall -Icommon
ASFLAGS = -march=i386
SRC_DIRS = common src

# Find all C and assembly source files
SRCS = $(wildcard $(patsubst %,%/*.c,$(SRC_DIRS))) $(wildcard $(patsubst %,%/*.s,$(SRC_DIRS)))

# Generate object file names
OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.s=.o)

TARGET = FROG

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	rm -f CD/$(TARGET).EXP
	elf2exp $(TARGET) CD/$(TARGET).EXP
	rm -f $(TARGET)

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Assemble assembly files
%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
