#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "csp2.h"

int SetDTR(int on);
int main(int argc, char* argv[]) {
	int nRetStatus, AsciiMode, RTC;
	int PacketLength, BarcodesRead;
	char Packet[64], aBuffer[256], TimeStamp[32];
	int i, k;

	/* Check to see if we have any command line arguments. */
	if ((argc == 1) || (strcmp( argv[1], "/dev/ttyUSB")) <= 0) {
		printf("Add additional arguments: %s /dev/ttyUSB0\n", argv[0]);
		return 1;
	}

	/* Enable debug logging */
	csp2SetDebugMode(PARAM_ON);
	int
			fd =
					open(argv[1], O_RDWR /*| O_NOCTTY | O_FSYNC | O_NDELAY*//* | O_NONBLOCK*/);
	if (fd < 0) {
		perror(argv[1]);
		printf("\n");
		return -1;
	} else
		fcntl(fd, F_SETFL, 0);

	if ((nRetStatus = csp2Init(fd)) != STATUS_OK) {
		/* error condition, alert the user of the problem */
		printf("Comm port not initialized.\nError Status:%d\n", nRetStatus);
		return 1;
	}

	nRetStatus = csp2ReadData(); /* Read barcodes */
	if (nRetStatus < 0) {
		/* error condition, alert the user of the problem */
		printf("Error reading barcode info.\nError Status: %d", nRetStatus);
	}

	else {
		/* save number of barcodes read into DLL */
		BarcodesRead = nRetStatus;
		//printf("Read %d packets\n", BarcodesRead);

		/* Get Device Id */
		nRetStatus = csp2GetDeviceId(aBuffer, 8);
		printf("{\n\"DeviceID\": \"");
		for (k = 0; k < nRetStatus; k++)
			printf(" %02X", (unsigned char) aBuffer[k]);

		/* Get Device Software Revision */
		nRetStatus = csp2GetSwVersion(aBuffer, 9);
		printf("\",\n\"DeviceSW\" : \"%s\",\n", aBuffer);

		/* Get packet type (ASCII/Binary) */
		AsciiMode = csp2GetASCIIMode();
		//printf("ASCII Mode = %d",AsciiMode);

		/* Get TimeStamp setting (on/off) */
		RTC = csp2GetRTCMode();
		//printf(", RTC Mode = %d\n",RTC);
		printf("\"data\": [\n");
		for (i = 0; i < BarcodesRead; i++) {
			PacketLength = csp2GetPacket(Packet, i, 63); /* Read packets */
			if ((PacketLength > 0) && (AsciiMode == PARAM_ON)) /* normal packet processing */
			{
				/* convert ascii mode code type to string */
				nRetStatus = csp2GetCodeType((long) Packet[1], aBuffer, 30);
				//				printf("CodeConversion returns: %d\n",nRetStatus);
				strcat(aBuffer, "\", ");

				if (RTC == PARAM_ON) /* convert timestamp if necessary */
				{
					/* convert binary timestamp to string */
					csp2TimeStamp2Str(
							(unsigned char *) &Packet[PacketLength - 4],
							TimeStamp, 30);
					/* append barcode (less timestamp) to codetype */
					/* offset: timestamp=4, len=1, codetype = 1 */
					const char code[] = "\"code\": \"";
					strncat(aBuffer, code, sizeof(code));
					strncat(aBuffer, Packet + 2, PacketLength - 6);

					/* append timestamp */
					const char time[] = "\", \"time\": \"";
					strncat(aBuffer, time, sizeof(time));
					strcat(aBuffer, TimeStamp);

					const char endline[] = "\"},";
					const char endline1[] = "\"}";
					if (i == BarcodesRead - 1)
						strncat(aBuffer, endline1, sizeof(endline1));
					else
						strncat(aBuffer, endline, sizeof(endline));
				}

				/* if no RTC just append barcode to codetype (no timestamp) */
				else
					strncat(aBuffer, Packet + 2, PacketLength - 2);

				printf("%s\n", aBuffer);
			} else {
				/* Process Binary Packet Mode */
			}

		} /* end of read packets loop */
		printf("]\n}\n");
	}
	return 0;
}

