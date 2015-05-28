





#include <stdio.h>

int p2pcommuniate_recvFilePiece(int sockfd, char* fileName, char* sourceIP, unsigned long timeStamp,
    int pieceID, int PIECE_LEN, char* buffer, ) {
    // Wait for ready signal from uploader
    recv(sockfd, buffer, 6, 0);
    if (strcmp(buffer, "READY") != 0)
    {
        return -1;
    }

    // Cue uploader to send
    send(sockfd, "SEND", 5, 0);

    // Send request details
    send(sockfd, &timeStamp, sizeof(time_t), 0);
    send(sockfd, &pieceID, sizeof(unsigned int), 0);
    send(sockfd, &pieceSize, sizeof(int), 0);

    // Receive file piece to buffer
    int bytesReceived = recv(sockfd, buffer, PIECE_LEN, 0);

    // Send SUCCESS/FAILURE
    if (bytesReceived == PIECE_LEN) {
        send(sockfd, "SUCCESS", 8, 0);
        /* KEEP FOR RELEASE */
        printf("\n%s: received piece %d of file %s from %s\n", __func__, pieceID, fileName, sourceIP);
        return 1;
    } else {
        send(sockfd, "FAILURE", 8, 0);
        return -1;
    }
}





/* Send file piece requested by peer, according to our communication
 protocol. Returns 1 on success, -1 on failure */
int sendFilePiece(int sockfd, char* fileName, char* downloaderIP)
{
    // Notify downloader that I'm ready to send
    send(sockfd, "READY", 6, 0);

    // Receive notification about whether to send or stop
    char sig[5];
    recv(sockfd, sig, 5, 0);
    if (strcmp(sig, "SEND") != 0)
    {
        printf("P2PUPLOAD: Upload complete.\n");
        return -1;
    }

    time_t* timeStamp = (time_t*) malloc(sizeof(time_t));
    recv(sockfd, timeStamp, sizeof(time_t), 0);

    unsigned int pieceId;
    recv(sockfd, &pieceId, sizeof(unsigned int), 0);

    int pieceSize;
    recv(sockfd, &pieceSize, sizeof(int), 0);

    // Check to see if file with requested name and timestamp exists
    int prefixLen = strlen(pathPrefix);
    int fileNameLen = strlen(fileName);
    char fullFileName[prefixLen + fileNameLen + 2];
    memcpy(fullFileName, pathPrefix, prefixLen);
    memcpy(&(fullFileName[prefixLen]), "/", 1);
    memcpy(&(fullFileName[prefixLen + 1]), fileName, fileNameLen);
    memcpy(&(fullFileName[prefixLen + fileNameLen + 1]), "\0", 1);

    FILE* filePointer = fopen(fullFileName, "r");
    if (filePointer == NULL)
    {
        printf("P2PUPLOAD: Failed to open file %s. Trying again...\n", fileName);
        sleep(FILE_COLLISION_WAIT);
        filePointer = fopen(fullFileName, "r");
        if (filePointer == NULL)
        {
            printf("P2PUPLOAD: Failed to open file. Exiting.\n");
            return -1;
        }
    }

    struct stat fileStat;
    stat(fullFileName, &fileStat);

    if (difftime(*timeStamp, fileStat.st_mtime) != 0)
    {
        perror("P2PUPLOAD: Current file is different version than one requested");
        return -1;
    }

    // Send over file piece
    char* buffer = (char*) malloc(pieceSize);
    for (int i = 1; i < pieceId; i++)
    {
        fseek(filePointer, FILE_PIECE_SIZE, SEEK_CUR);
    }
    fread(buffer, pieceSize, 1, filePointer);
    fclose(filePointer);

    send(sockfd, buffer, pieceSize, 0);

    // Receive success/failure message
    char res[8];
    recv(sockfd, res, 8, 0);
    if (strcmp(res, "SUCCESS") == 0)
    {
        /* KEEP FOR RELEASE */
        printf("P2PUPLOAD: Sent piece %d of file %s, size %d, to ", pieceId,
            fileName, pieceSize);
        printHostName(downloaderIP);
        printf("\n");

        return 1;
    } else
    {
        return -1;
    }
}