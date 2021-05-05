object = scanner.o
cc = gcc
#scanner : scanner.o
#	cc scanner -o scanner.o

scanner : $(object)
	cc -o scanner $(object) -lpthread
scanner.o : scanner.c scanner.h
	cc -c -g scanner.c
.PHONY :clean
clean :
	-rm scanner $(object)
	sudo rm -rf ./scanlog
