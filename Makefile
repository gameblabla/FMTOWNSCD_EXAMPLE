CC = i386-pc-run386-gcc
CFLAGS = -O2 -Wall -Icommon

SRC_DIRS = common src
SRCS = $(wildcard $(patsubst %,%/*.c,$(SRC_DIRS)))
OBJS = $(SRCS:.c=.o)

TARGET = FROG

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	rm CD/$(TARGET).EXP
	elf2exp $(TARGET) CD/$(TARGET).EXP
	rm $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
