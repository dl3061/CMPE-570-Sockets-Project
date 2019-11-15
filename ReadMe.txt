David Lin
Sockets Programming Project

To the server, run
- make server
This will compile the server if necessary, generate the 
server file directory if necessary, and run the program.
Errors will be logged to err_server.txt

To run the client, run
- make client
This will compile the client if necessary, generate the 
downloads file directory if necessary, and run the program.
Errors will be logged to err_client.txt


Four tests are available:
- make test (log ins and uploads/downloads small file 100 times)
- make test_login (log ins and exits 100 times)
- make test_gif (log ins and uploads/downloads binary file 100 times)
- make test_ftp (log ins and uploads/downloads file of varying size 100 times)

Additional information available with 
- make help.

To clean the file directories, object files, executable and log files, run
- make clean

Please be very very careful when redirecting stdout to a log file. 
The Makefile rules will by default only redirect stderr. 

Verbose is enabled by default when compiled. This can be disabled
in the Tiger.h file.