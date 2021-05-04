object = scanner.o
cc = gcc
#scanner : scanner.o
#	cc scanner -o scanner.o
scanner : $(object)
	cc -o scanner $(object)
scanner.o : scanner.c
	cc -c -g scanner.c
.PHONY : clean
clean :
	-rm scanner $(object)
