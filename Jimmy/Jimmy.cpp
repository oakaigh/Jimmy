
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

HANDLE handleDst;
HANDLE handleSrc;
UINT16 tar_port;
UINT16 tar_port_origin;

bool verbose = false;

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
		char* tarPortMatchStr = _strdup(strtok(strdupe, delimiter));
		char* tarPortStr = _strdup(strtok(NULL, delimiter));
		tar_port_origin = htons(atoi(tarPortMatchStr));
		tar_port = htons(atoi(tarPortStr));
		tarPortMatch = tarPortMatchStr;

		if (opt == JIMMY_DST)
			return dstPortLang + tarPortMatch;
		else if (opt == JIMMY_SRC)
			return srcPortLang + string(tarPortStr);
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

	WINDIVERT_ADDRESS windivt_addr;

	char packet[MAXBUF];
	UINT packetLen;
	PWINDIVERT_IPHDR  ip_header;
	PWINDIVERT_TCPHDR tcp_header;
	bool control = true;

	while (control) {

		WinDivertRecv(handel, packet, sizeof(packet), &windivt_addr, &packetLen);
		WinDivertHelperParsePacket(packet, packetLen, &ip_header, NULL, NULL, NULL, &tcp_header, NULL , NULL, NULL);

		if (opt == JIMMY_DST) {
			tcp_header->DstPort = tar_port;
			if (verbose) {
				printf("<%s:%u>", "DST", ntohs(tar_port));
				printf("[to %u]", ntohs(tcp_header->DstPort));
			}
		}
		else if (opt == JIMMY_SRC) {
			tcp_header->SrcPort = tar_port_origin;
			if (verbose) {
				printf("!");
				printf("<%s:%u>", "SRC", ntohs(tar_port_origin));
				printf("[to %u]\n", ntohs(tcp_header->SrcPort));
			}
		}
		WinDivertHelperCalcChecksums((PVOID)packet, packetLen, &windivt_addr, 0);
		WinDivertSend(handel, packet, packetLen, &windivt_addr, NULL);

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

	printf("Welcome!\n");
	printf("eLitet Insiders Packet forwarding console.\n");
	printf("%s\n", truncPolicDst.c_str());
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
