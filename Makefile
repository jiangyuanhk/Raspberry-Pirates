all: fileMonitor/fileMonitorTestClient tracker peer

fileMonitor/fileMonitor.o: fileMonitor/fileMonitor.c fileMonitor/fileMonitor.h
	gcc -Wall -pedantic -std=c99 -D_POSIX_SOURCE -D_BSD_SOURCE -pthread -g -c fileMonitor/fileMonitor.c -o fileMonitor/fileMonitor.o
fileMonitor/fileMonitorTestClient: fileMonitor/fileMonitorTestClient.c fileMonitor/fileMonitor.o 
	gcc -Wall -pedantic -std=c99 -D_POSIX_SOURCE -D_BSD_SOURCE -g -pthread fileMonitor/fileMonitorTestClient.c fileMonitor/fileMonitor.o -o fileMonitor/fileMonitorTestClient
tracker: tracker/tracker.h
	gcc -Wall -pedantic -std=c99 -ggdb -D_GNU_SOURCE -pthread -o app_tracker tracker/tracker.c common/pkt.c common/filetable.c common/peertable.c common/utils.c
peer: peer/peer.h fileMonitor/fileMonitor.o
	gcc -Wall -pedantic -std=c99 -ggdb -D_GNU_SOURCE -pthread -o app_peer peer/peer.c common/pkt.c common/filetable.c common/peertable.c common/utils.c peer/peer_helpers.c peer/file_monitor.c fileMonitor/fileMonitor.o

clean:
	rm -rf fileMonitor/*.o
	rm -rf app_tracker
	rm -rf app_peer
