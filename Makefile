all:  fileMonitor/fileMonitorTestClient

fileMonitor/fileMonitor.o: fileMonitor/fileMonitor.c fileMonitor/fileMonitor.h
	gcc -Wall -pedantic -std=c11 -g -c fileMonitor/fileMonitor.c -o fileMonitor/fileMonitor.o
fileMonitor/fileMonitorTestClient: fileMonitor/fileMonitorTestClient.c fileMonitor/fileMonitor.o 
	gcc -Wall -pedantic -std=c11 -g -pthread fileMonitor/fileMonitorTestClient.c fileMonitor/fileMonitor.o -o fileMonitor/fileMonitorTestClient 

clean:
	rm -rf fileMonitor/*.o
	rm -rf client/app_simple_client

