# David Lin
# Makefile - adapted from Applied Programming HW (https://github.com/dl3061/AppliedProgrammingHW)
#			 because no sane mind will rewrite a Makefile from scratch. 
# For TigerS and TigerC FTP socket program (https://github.com/dl3061/CMPE-570-Sockets-Project) 

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

MAKE_DIR_ECHO = @echo "mkdir -p $@"
MAKE_DIR = mkdir -p $@

SHARED_HEADERS = TParam.h Tiger.h THelp.h
LINK_FLAGS = -lm -lgsl -lgslcblas -lpthread
D_FLAGS = 

SERVER_OUT_FILE = err_server.txt
CLIENT_OUT_FILE = err_client.txt

SERVER_DIRECTORY = TigerS
CLIENT_DOWNLOADS_DIRRECTORY = Downloads

.PHONY: help
help:
	@echo "make options: all, help, clean"
	@echo "     server (run), client (run)"
	@echo "     test (log ins and uploads/downloads small file 100 times)"
	@echo "     test_login (just logs in 100 times)"
	@echo "     test_ftp (logs in and uploads/downloads files of varying size)"

.PHONY: all
all: TigerC.exe TigerS.exe $(SERVER_DIRECTORY) $(CLIENT_DOWNLOADS_DIRRECTORY)
	@echo "All executables have been compiled."
	$(NEXT_LINE)

.PHONY: clean
clean :
	@echo "Cleaning up temporary files."
	-rm -f *.o
	-rm -f *.exe
	-rm -f $(SERVER_OUT_FILE)
	-rm -f $(CLIENT_OUT_FILE)
	-rm -f $(SERVER_DIRECTORY)/*
	-rm -f $(CLIENT_DOWNLOADS_DIRRECTORY)/*
	$(NEXT_LINE)

.PHONY: server
server: TigerS.exe $(SERVER_DIRECTORY)
	$(NEXT_LINE) >> $(SERVER_OUT_FILE)
	./TigerS.exe 2> $(SERVER_OUT_FILE)

.PHONY: client
client: TigerC.exe $(CLIENT_DOWNLOADS_DIRRECTORY)
	$(NEXT_LINE) >> $(CLIENT_OUT_FILE)
	./TigerC.exe 2> $(CLIENT_OUT_FILE)
	
.PHONY: test_login
test_login: TigerC.exe
	./test_login.sh 2> $(CLIENT_OUT_FILE)
	$(NEXT_LINE)

.PHONY: test
test: TigerC.exe $(CLIENT_DOWNLOADS_DIRRECTORY)
	./test_ftp_simple.sh 2> $(CLIENT_OUT_FILE)
	$(NEXT_LINE)

.PHONY: test_gif
test_gif: TigerC.exe $(CLIENT_DOWNLOADS_DIRRECTORY)
	./test_ftp_gif.sh 2> $(CLIENT_OUT_FILE)
	$(NEXT_LINE)

.PHONY: test_ftp
test_ftp: TigerC.exe $(CLIENT_DOWNLOADS_DIRRECTORY)
	./test_ftp_files.sh 2> $(CLIENT_OUT_FILE)
	$(NEXT_LINE)

.PHONY: test_concurrent
test_concurrent: TigerC.exe
	./test_concurrent_login.sh 2> $(CLIENT_OUT_FILE)
	$(NEXT_LINE)

TigerC.exe : TigerC.o TParam.o THelp.o
	$(COMPILE_LINK_ECHO)
	$(COMPILE_LINK)
	$(NEXT_LINE)

TigerS.exe : TigerS.o TParam.o THelp.o
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

TParam.o : TParam.c $(SHARED_HEADERS)
	$(COMPILE_ECHO)
	$(COMPILE_O)
	$(NEXT_LINE)

THelp.o : THelp.c $(SHARED_HEADERS)
	$(COMPILE_ECHO)
	$(COMPILE_O)
	$(NEXT_LINE)

$(SERVER_DIRECTORY): 
	$(MAKE_DIR_ECHO)
	$(MAKE_DIR)

$(CLIENT_DOWNLOADS_DIRRECTORY): 
	$(MAKE_DIR_ECHO)
	$(MAKE_DIR)
	