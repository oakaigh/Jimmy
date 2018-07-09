
#include "stdafx.h"

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <Ws2tcpip.h>
#include "windivert.h"
#include <thread>

#define MAXBUF  0xFFFF
#pragma comment(lib, "WinDivert.lib")
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

#define JIMMY_SRC 0
#define JIMMY_DST 1

typedef struct
{
	WINDIVERT_IPHDR  ip;
	WINDIVERT_TCPHDR tcp;
} PACKET, *PPACKET;

typedef struct
{
	PACKET header;
	PVOID data;
} DATAPACKET, *PDATAPACKET;


HANDLE handleDst;
HANDLE handleSrc;
UINT16 tar_port;


string readParam(const char* in, int opt) {

	string dstAddrLang = "ip.DstAddr=";
	string srcAddrLang = "ip.SrcAddr=";
	string dstPortLang = "tcp.DstPort=";
	string srcPortLang = "tcp.SrcPort=";

	string tarPortMatch = "";
	struct sockaddr_in sa;
	char* strdupe = _strdup(in);
	char* delimiter = _strdup("-");

	if (strstr(strdupe, delimiter) != NULL) {
		tarPortMatch = strtok(strdupe, delimiter);
		tar_port = htons(atoi(strtok(NULL, delimiter)));
		if (opt == JIMMY_DST)
			return dstPortLang + tarPortMatch;
		else if (opt == JIMMY_SRC)
			return srcPortLang + tarPortMatch;
		else
			return "";
	}
	else if (inet_pton(AF_INET, strdupe, &(sa.sin_addr)) == 1) {
		if (opt == JIMMY_DST)
			return dstAddrLang + string(strdupe);
		else if (opt == JIMMY_SRC)
			return srcAddrLang + string(strdupe);
		else
			return "";
	}
	return "";
}

void intercpt(int opt, HANDLE handel) {

	PDATAPACKET packet;
	UINT packet_len;
	WINDIVERT_ADDRESS windivt_addr;

	bool control = true;

	while (control) {

		packet = (PDATAPACKET)malloc(sizeof(DATAPACKET));
		memset(packet, 0, sizeof(DATAPACKET));

		WinDivertRecv(handel, packet, sizeof(DATAPACKET), &windivt_addr, &packet_len);

		printf("=");

		if (opt == JIMMY_DST) {
			packet->header.tcp.DstPort = tar_port;
			printf("<%s:%u>", "DST", ntohs(tar_port));
			printf("[to %u]", ntohs(packet->header.tcp.DstPort));
		}
		else if (opt == JIMMY_SRC) {
			packet->header.tcp.SrcPort = tar_port;
			printf("<%s:%u>", "SRC", ntohs(tar_port));
			printf("[to %u]", ntohs(packet->header.tcp.SrcPort));
		}
		printf("@");
		WinDivertHelperCalcChecksums((PVOID)packet, sizeof(DATAPACKET), &windivt_addr, 0);
		WinDivertSend(handel, packet, sizeof(DATAPACKET), &windivt_addr, NULL);
		printf("-");

		//control = false;
	}

}

int main(int argc, char* argv[]) {

	string truncPolicDst = "";
	string truncPolicSrc = "";

	int i = 0;
	while (i < argc) {
		char* token = _strdup(argv[i]);
		truncPolicDst += readParam(token, JIMMY_DST) + ((i == argc - 1) || (i == 0) ? "" : " and ");
		truncPolicSrc += readParam(token, JIMMY_SRC) + ((i == argc - 1) || (i == 0) ? "" : " and ");
		i++;
	}

	printf("%s", truncPolicDst.c_str());
	printf("%s", truncPolicSrc.c_str());
	printf(" to %u\n", htons(tar_port));
	handleDst = WinDivertOpen(truncPolicDst.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);
	handleSrc = WinDivertOpen(truncPolicSrc.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);

	if (handleDst == INVALID_HANDLE_VALUE)
		printf("\t ERROR: %i", GetLastError());

	thread asyncDst(intercpt, JIMMY_DST, handleDst);
	thread asyncSrc(intercpt, JIMMY_SRC, handleSrc);

	asyncDst.join();
	asyncSrc.join();

}
