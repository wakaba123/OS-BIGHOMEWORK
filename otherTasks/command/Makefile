# commands/Makefile

#ENTRYPOINT	= 0x1000
HD		= ../80m.img

ASM		= nasm
DASM		= objdump
CC		= gcc
LD		= ld
ASMFLAGS	= -I ../include/ -f elf
CFLAGS		= -I ../include/ -c -fno-builtin -fno-stack-protector -Wall -I ../include/sys/
LDFLAGS		= -Ttext 0x1000
DASMFLAGS	= -D
LIB		= ../lib/orangescrt.a
BIN		= echo pwd ls touch rm testa testb attack poc infected

# All Phony Targets
.PHONY : everything final clean realclean disasm all install

# Default starting position
everything : $(BIN)

install : all clean
	cp ../boot/hd*.bin ./ -v
	cp ../kernel.bin ./ -v
	tar vcf inst.tar kernel.bin $(BIN) hd*.bin
	dd if=inst.tar of=$(HD) seek=`echo "obase=10;ibase=16;(\`egrep -e '^ROOT_BASE' ../boot/include/load.inc | sed -e 's/.*0x//g'\`+\`egrep -e '#define[[:space:]]*INSTALL_START_SECT' ../include/sys/config.h | sed -e 's/.*0x//g'\`)*200" | bc` bs=1 count=`ls -l inst.tar | awk -F " " '{print $$5}'` conv=notrunc

all : realclean everything

final : all clean

clean :
	rm -f *.o

realclean :
	rm -f $(BIN) *.o

kernel.bin :
	cp ../kernel.bin ./

start.o : start.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

echo.o: echo.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

echo : echo.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

pwd.o: pwd.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

pwd : pwd.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

ls.o: ls.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

ls : ls.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

touch.o: touch.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

touch : touch.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?


rm.o: rm.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

rm : rm.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

testa.o: testa.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

testa : testa.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

testb.o: testb.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

testb : testb.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

attack.o: attack.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

attack : attack.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

poc.o: poc.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

poc : poc.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?

infected.o: infected.c ../include/type.h ../include/stdio.h
	$(CC) $(CFLAGS) -o $@ $<

infected : infected.o start.o $(LIB)
	$(LD) $(LDFLAGS) -o $@ $?