#include "gui-windows.h"

void logToFile(char * message, int src){
	char copy[1024] = {0};
	strcpy(copy, message);
	FILE *file = fopen("log.txt", "ab+");

	if (file == NULL){
		perror("\n\nFailed to open to log file. Reason");
		exit(-1);
	}

	if (previousState != src){
		if(src == 0){
			char copy[] = "SERVER:\n";
			fwrite(copy, 1, strlen(copy), file);
		}else{
			char copy[] = "CLIENT:\n";
			fwrite(copy, 1, strlen(copy), file);
		}
	}

	if (fwrite(copy, 1, strlen(copy), file) != strlen(copy)) {
        write(2, "There was an error writing to testfile.txt\n", 43);
        exit(-1);
    }

	fwrite("\n", 1, strlen("\n"), file);

	previousState = src;
	fclose(file);
}

void sendMessage(char * message){
	send(client_socket, message, strlen(message), 0);
	logToFile(message, 1);
	return;
}

int waitUntilRecv(char * buff, int buffSize){
	memset(buff, 0, buffSize);
	int bytes = recv(client_socket, buff, buffSize, 0);
	if (bytes <= 0){
		char err[] = "\nSomething went seriously wrong :(\nReason: Probably the client has disconnected...\n";
		printf("%s", err);
		logToFile(err, 1);
		menu();
	}
	logToFile(buff, 0);
	return bytes;
}

void serverStart(int port){
	memset(&server_addr, '0', sizeof(server_addr));
	memset(sendBuff, 0, sizeof(sendBuff));
	memset(recvBuff, 0, sizeof(sendBuff));

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	// setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	if (server_socket < 0){
		perror("\n\nCan't open socket! Reason");
		exit(-1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	int res = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

	if (res < 0){
		strcpy(message, strerror(errno));
		sprintf(message, "Can't bind socket! Reason%s", message);
		printf("%s", message);
		logToFile(message, 1);
		getchar();
		menu();
	}

	sprintf(message, "Now accepting connections in 127.0.0.1:%d\n", port);
	printf("%s", message);
	logToFile(message, 1);
	listen(server_socket, 5);

	clilen = sizeof(client_addr);
	client_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&clilen);

	struct sockaddr_in *pV4Addr = (struct sockaddr_in *)&client_addr;
	struct in_addr ipAddr = pV4Addr->sin_addr;
	char str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN);

	printf("Client %s has connected!\n", str);

	readLoop();
}

void readLoop(){
	while (1){
		waitUntilRecv(recvBuff, 1024);

		// HANDLE THE WRITE! COMMAND
		if (memcmp(recvBuff, "WRITE!", strlen("WRITE!")) == 0){
			logToFile("Started erasing the PDS16", 1);
			erasePDS();
			logToFile("Successfully erased the PDS16", 1);
			sendMessage("OK!");
			char line[512] = {0};

			while (1){
				int n = waitUntilRecv(line, 512);
				char temp[512];
				strcpy(temp, line);

				if (memcmp(line, "END!", strlen("END!")) != 0){
					for (int i = 0; i < n - 4; i++){
						if (line[i] != '\n'){
							line[i] = getVal(line[i]);
						}
					}
					if (line[0] == 0 && line[1] == 0){
						sendMessage("OK!");
					}else{
						int memLoc = getAddressFromLine(line);
						sendMessage("OK!");
						sprintf(message, "Successfully wrote %s at offset 0x%04x", temp, memLoc);
						programRam(line, memLoc);
						logToFile(message, 1);
					}
				}else{
					sendMessage("OK!");
					logToFile("Done flashing RAM", 1);
					readLoop();
				}
			}
		}

		if (memcmp(recvBuff, "GET REGS!", strlen("GET REGS!")) == 0){
			sendMessage("OK!");
			sprintf(message, "%05hu:%05hu:%05hu:%05hu:%05hu:%05hu:%05hu:%05hu", readFromRegister(0), readFromRegister(1), readFromRegister(2),
																				readFromRegister(3), readFromRegister(4), readFromRegister(5),
																				readFromRegister(6), readFromRegister(7));
			sendMessage(message);
		}

		if (memcmp(recvBuff, "SET REGS!", strlen("SET REGS!")) == 0){
			int registerID;
			int value;
			if(sscanf(recvBuff, "SET REGS! %d %i", &registerID, &value) == 2){
				writeToRegister(registerID, value);
				printRegisters();
				sendMessage("OK!");
			}
		}

		if (memcmp(recvBuff, "STEP!", strlen("STEP!")) == 0){
			int instruction = (readFromRam(readFromRegister(7))<<8)+readFromRam(readFromRegister(7)+1);
            decodeOp(instruction);
			sendMessage("OK!");
		}

		if (memcmp(recvBuff, "GET IO!", strlen("GET IO!")) == 0){
			sendMessage("OK!");
			sprintf(message, "%05hu:%05hu:%05hu:%05hu:%05hu:%05hu", handleIO(1, false, 0), handleIO(2, false, 0), handleIO(3, false, 0), handleIO(4, false, 0), handleIO(5, false, 0), handleIO(6, false, 0));
			sendMessage(message);
		}

		if (memcmp(recvBuff, "SET IO!", strlen("SET IO!")) == 0){
			int IOIdentifier;
			int value;
			if(sscanf(recvBuff, "SET IO! %d %i", &IOIdentifier, &value) == 2){
				handleIO(IOIdentifier, true, value);
				sendMessage("OK!");
			}
		}

		if (memcmp(recvBuff, "SET BREAK!", strlen("SET BREAK!")) == 0){
			int BreakID;
			int value;
			if(sscanf(recvBuff, "SET BREAK! %d %i", &BreakID, &value) == 2){
				breakpointManager(BreakID, value, true);
				sendMessage("OK!");
			}
		}

		if (memcmp(recvBuff, "DEL BREAK!", strlen("DEL BREAK!")) == 0){
			int BreakID;
			int value;
			if(sscanf(recvBuff, "DEL BREAK! %d %i", &BreakID, &value) == 2){
				breakpointManager(BreakID, value, false);
				sendMessage("OK!");
			}
		}

		if (memcmp(recvBuff, "GET MEM!", strlen("GET MEM!")) == 0){
			int start;
			int end;
			char memory[110] = {0};
			if(sscanf(recvBuff, "GET MEM! %i %i", &start, &end) == 2){
				int a = 0;
				for (int i = start; i <= end; i++){
					a += sprintf(&memory[a], "%03i", readFromRam(i));
				}
				sendMessage("OK!");
				sendMessage(memory);
			}
		}

		if (memcmp(recvBuff, "AUTO!", strlen("AUTO!")) == 0){
			// sendMessage("OK!");
			// toEnd = false;
			// if ((cpid = fork()) == 0){
			// 	runG();
			// }else{
			// 	killThreadG();
			// }
			// sendMessage("OK!");
		}


		/*
				pthread_create(&tids[1], NULL, killThread, NULL);
                pthread_create(&tids[2], NULL, run, NULL);
                pthread_join(tids[1], NULL);
                pthread_join(tids[2], NULL);
		*/
	}
}