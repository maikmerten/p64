###########################################################
# The makefile for the P64 program.
# There should be no changes for most UNIX compilers.
###########################################################

PFLAGS = -O3 -pg -g
DEFS = system.h globals.h marker.h
BASELINE =  p64.o codec.o huffman.o io.o aandct.o chendct.o lexer.o marker.o me.o mem.o stat.o stream.o transform.o y4m_input.o vidinput.o

.c.o:
	$(CC) $(PFLAGS) -I. -c $*.c

.c.ln:
	lint -c $*.c 

all: p64

clean:
	rm *.o p64

p64: $(BASELINE)
	$(CC) $(PFLAGS) $(BASELINE) -lm -o p64

p64.o: p64.c $(DEFS) vidinput.h ogg/os_types.h
codec.o: codec.c $(DEFS)
marker.o: marker.c $(DEFS) marker.h
huffman.o: huffman.c $(DEFS) huffman.h
init.o: init.c $(DEFS) ctables.h
io.o: io.c $(DEFS) vidinput.h ogg/os_types.h
aandct.o: aandct.c $(DEFS)
chendct.o: chendct.c $(DEFS)
lexer.o: lexer.c
mem.o: mem.c 
mem.math.o: mem.math.c 
me.o: me.c 
stat.o: stat.c 
stream.o: stream.c $(DEFS)
transform.o: transform.c $(DEFS) dct.h
vidinput.o: vidinput.c vidinput.h ogg/os_types.h
y4m_input.o: y4m_input.c vidinput.h ogg/os_types.h

lcheck: p64.ln codec.ln huffman.ln io.ln aandct.ln chendct.ln lexer.ln marker.ln me.ln mem.ln stat.ln stream.ln transform.ln
	lint  p64.ln codec.ln huffman.ln io.ln aandct.ln chendct.ln lexer.ln marker.ln me.ln mem.ln stat.ln stream.ln transform.ln


#
# Should you wish to modify the huffman tables, use these commands.
#
#
#ctables.h: tables.h htable
#	rm -f ctables.h
#	htable <tables.h >ctables.h
#
# Make sure that you move the first comment from the start to 
# within the braces %{ /* */ %} in htable.lex, lexer.l,
# otherwise it will give you a error (definitions too long). 
# Caution: the htable name may conflict with /usr/etc/htable
# on some machines.
#
#
#htable:htable.lex
#	lex htable.lex
#	mv lex.yy.c htable.c
#	cc htable.c -o htable -ll
#
#  Caution: Sometimes -ll is required.
#
#lexer.c:lexer.l
#	lex lexer.l
#	mv lex.yy.c lexer.c
#
