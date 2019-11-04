NEXT_LINE = @echo ""
OPT ?= 1
VERBOSE ?= 
COMPILE_ECHO = @echo "Compiling $< to $@"
COMPILE_LINK_ECHO = @echo "Linking $^ to $@"
COMPILE = gcc -std=c99 -O$(OPT) -Wall -pedantic -g $(LINK_FLAGS) $(D_FLAGS) $^ -o $@
COMPILE_O = gcc -std=c99 -O$(OPT) -Wall -g $(D_FLAGS) -c $<
COMPILE_LINK = gcc -std=c99 -O$(OPT) -Wall -pedantic -g $(LINK_FLAGS) -o $@ $^
VALGRIND_ECHO = @echo "Testing $^ with valgrind memory command"
VALGRIND = valgrind --tool=memcheck --leak-check=full --show-reachable=yes --track-origins=yes

SHARED_HEADERS = 
LINK_FLAGS = -lm -lgsl -lgslcblas
D_FLAGS = 

.PHONY: help
help:
	@echo "make options: all, help, clean"

.PHONY: all
all: TigerC.exe TigerS.exe
	@echo "All executables have been compiled."
	$(NEXT_LINE)

.PHONY: clean
clean :
	@echo "Cleaning up temporary files."
	-rm -f *.o
	-rm -f *.exe
	$(NEXT_LINE)

.PHONY: test
test: TigerC.exe TigerS.exe
	
TigerC.exe : TigerC.o 
	$(COMPILE_LINK_ECHO)
	$(COMPILE_LINK)
	$(NEXT_LINE)

TigerS.exe : TigerS.o 
	$(COMPILE_LINK_ECHO)
	$(COMPILE_LINK)
	$(NEXT_LINE)

TigerC.o : TigerC.c $(SHARED_HEADERS)
	$(COMPILE_ECHO)
	$(COMPILE_O)
	$(NEXT_LINE)

TigerS.o : TigerS.c $(SHARED_HEADERS)
	$(COMPILE_ECHO)
	$(COMPILE_O)
	$(NEXT_LINE)
