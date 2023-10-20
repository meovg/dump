OBJDIR = obj
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)%.c=$(OBJDIR)%.o)
INCLUDES = $(wildcard *.h)

TARGET = lc3asm

CC =gcc
CFLAGS =-Wall -std=gnu11 -g -I. -O0
DEBUG_CFLAGS = -fsanitize=undefined,leak,address

error:
	@echo Please choose 1 out of these options: win, win-clean, linux, linux-clean, linux-debug
	@exit 2

win: winmkdir $(TARGET)

linux: linuxmkdir $(TARGET)

linux-debug: linuxmkdir $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(DEBUG_CFLAGS) -o $(TARGET)

winmkdir:
	if not exist $(OBJDIR) mkdir $(OBJDIR)

linuxmkdir:
	if [ ! -d "$(OBJDIR)" ]; then mkdir $(OBJDIR); fi

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/*.c
	$(CC) -c $(SRCDIR)/$*.c $(CFLAGS) -o $@

win-clean:
	@echo cleaning up executables...
	del $(OBJDIR)\*.o *.exe *~
	rmdir $(OBJDIR)

linux-clean:
	@echo cleaning up executables...
	rm -f $(TARGET)
	rm -f $(OBJDIR)/*.o *~
	rmdir $(OBJDIR)