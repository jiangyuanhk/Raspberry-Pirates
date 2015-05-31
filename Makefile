all:  fileMonitor/fileMonitorTestClient tracker peer

fileMonitor/fileMonitor.o: fileMonitor/fileMonitor.c fileMonitor/fileMonitor.h
	gcc -Wall -pedantic -std=c11 -g -c fileMonitor/fileMonitor.c -o fileMonitor/fileMonitor.o
fileMonitor/fileMonitorTestClient: fileMonitor/fileMonitorTestClient.c fileMonitor/fileMonitor.o 
	gcc -Wall -pedantic -std=c11 -g -pthread fileMonitor/fileMonitorTestClient.c fileMonitor/fileMonitor.o -o fileMonitor/fileMonitorTestClient
tracker: tracker/tracker.h
	gcc -Wall -pedantic -std=c11 -ggdb -D_GNU_SOURCE -pthread -o app_tracker tracker/tracker.c common/pkt.c common/filetable.c common/peertable.c common/utils.c
peer: peer/peer.h fileMonitor/fileMonitor.o
	gcc -Wall -pedantic -std=c11 -ggdb -D_GNU_SOURCE -pthread -o app_peer peer/peer.c common/pkt.c common/filetable.c common/peertable.c common/utils.c peer/peer_helpers.c peer/file_monitor.c fileMonitor/fileMonitor.o

clean:
	rm -rf fileMonitor/*.o
	rm -rf client/app_simple_client
	rm -rf app_tracker
	rm -rf app_peer
