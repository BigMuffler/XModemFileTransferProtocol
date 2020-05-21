//============================================================================
//
//% Student Name 1: Stefan Ungurean
//% Student 1 #: 301316291
//% Student 1 userid (email): sungurea (stu1@sfu.ca)
//
//% Student Name 2: Jamyl Johnson
//% Student 2 #: 301313059
//% Student 2 userid (email): jamylj (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None') All of TA's, Jakeb Puffer, Tal Kazkov
//
//
// Also, list any resources beyond the course textbooks and the course pages on Piazza
// that you used in making your submission.
//
// Resources:  ___________
//
//%% Instructions:
//% * Put your name(s), student number(s), userid(s) in the above section.
//% * Also enter the above information in other files to submit.
//% * Edit the "Helpers" line and, if necessary, the "Resources" line.
//% * Your group name should be "P2_<userid1>_<userid2>" (eg. P2_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : ReceiverX.cpp
// Version     : September 3rd, 2019
// Description : Starting point for ENSC 351 Project Part 2
// Original portions Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

#include <string.h> // for memset()
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include "myIO.h"
#include "ReceiverX.h"
#include "VNPE.h"

//using namespace std;

ReceiverX::ReceiverX(int d, const char *fname, bool useCrc):PeerX(d, fname, useCrc),
NCGbyte(useCrc ? 'C' : NAK),goodBlk(false),  goodBlk1st(false), syncLoss(true), checkSumCheck(false), crcCheck(false), numLastGoodBlk(0) //writeChunk();
{
}

void ReceiverX::receiveFile()
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	transferringFileD = PE2(myCreat(fileName, mode), fileName);
	enum {START, RECEIVE, GOODBLK, WRITE, CANCEL, EOT1, ERR};
	int state = START;
	bool executing = true;

	// ***** improve this member function *****

	// below is just an example template.  You can follow a
	// 	different structure if you want.

	// inform sender that the receiver is ready and that the
	//		sender can send the first block
	sendByte(NCGbyte);

	while (executing)
	{
	    std::cout << "R" << state << std::endl;

	    switch(state)
	    {
	    case START:
	    {
	        numLastGoodBlk = 0;
	        if(Crcflg)
	        {
	            sendByte('C');
	        }
	        else
	        {
	            sendByte(NAK);
	        }
	        state = RECEIVE;
	    }break;
	    case RECEIVE:
	    {
	        PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
	        if(rcvBlk[0] == EOT)
	        {
	            sendByte(NAK);
	            state = EOT1;
	        }
	        else if(rcvBlk[0] == CAN)
	        {
	            can8();
	            state = CANCEL;
	        }
            else if(rcvBlk[0] == SOH) //GOOD
            {
                getRestBlk();
                state = GOODBLK;
            }
            else
            {
                state = ERR;
            }

	    }break;
	    case GOODBLK:
	    {
	        std::cout << "RB:" << rcvBlk[1] << " NLGB: " << numLastGoodBlk << std::endl;
	        if (Crcflg){
	            if(rcvBlk[1] + rcvBlk[2] != 255)
            {
                sendByte(NAK);
                state = RECEIVE;
                errCnt++;
            }
            else if (rcvBlk[1] == (uint8_t)(numLastGoodBlk))
            {
                sendByte(ACK);

                state = RECEIVE;
            }
            else if (rcvBlk[1] == (uint8_t)(numLastGoodBlk + 1 ))
            {
/*              if (goodBlk1st)
                {*/
                    state = WRITE;
/*              }
                else
                {
                    sendByte(NAK);
                    errCnt++;
                    state = RECEIVE;
                }*/
            }
            else
            {
                std::cout << "Err in GOODBLK" << std::endl;
                state = ERR;
            }
	        }
	        if (Crcflg == false)
	        {
	        if (checkSumCheck == true && crcCheck == false)
	        {
	            state = WRITE;
	        }
	        else
	        {
	            sendByte(NAK);
	            state = RECEIVE;
	            errCnt++;
	        }
	        }



	    }break;
	    case WRITE:
	    {
	        if (Crcflg)
            {
                if (crcCheck)
                {
                   std::cout << "Written CRC Chunk" << std::endl;
                   writeChunk();
                   sendByte(ACK);
                   numLastGoodBlk++;
                }
                else
                {
                    sendByte(NAK);
                    errCnt++;
                }
            }
            else
            {

                if (checkSumCheck)
                {
                    std::cout << "Written CS Chunk" << std::endl;
                    sendByte(ACK);
                    writeChunk();
                    numLastGoodBlk++;
                }
                else
                {
                    sendByte(NAK);
                    errCnt++;
                }
            }
	        state = RECEIVE;
	    }break;
	    case CANCEL:
	    {
	        if (rcvBlk[0] == CAN)
	        {
	            result = "Cancelled";
	            executing = false;
	        }
	        else
	        {
	            state = ERR;
	        }

	    }break;
	    case EOT1:
	    {
	        PE_NOT(myRead(mediumD, rcvBlk, 1), 1);
	        if (rcvBlk[0] == EOT)
	        {
	            sendByte(ACK);
	            result = "DONE";
	            executing = false;
	        }

	    }break;
	    case ERR:
	    {
	        if (errCnt > errB)
	        {
	            can8();
	            result = "ExcessiveNAKS";
	        }
	        else
	        {
	            result = "Unknown";
	        }
	        executing = false;
	    }break;
	    }

	}

	(close(transferringFileD));

	//while(PE_NOT(myRead(mediumD, rcvBlk, 1), 1), (rcvBlk[0] == SOH))
	//{
		//getRestBlk();
		//sendByte(ACK); // assume the expected block was received correctly.
		//writeChunk();


	//};
	// assume EOT was just read in the condition for the while loop
	//sendByte(NAK); // NAK the first EOT
	//PE_NOT(myRead(mediumD, rcvBlk, 1), 1); // presumably read in another EOT

	// check if the file closed properly.  If not, result should be something other than "Done".
	//result = "Done"; //assume the file closed properly.
	//sendByte(ACK); // ACK the second EOT
}

/* Only called after an SOH character has been received.
The function tries
to receive the remaining characters to form a complete
block.  The member
variable goodBlk1st will be made true if this is the first
time that the block was received in "good" condition.
*/
void ReceiverX::getRestBlk()
{
	// ********* this function must be improved ***********
    goodBlk1st = goodBlk = true;
    checkSumCheck = crcCheck = false;
    uint8_t checksum = 0;

    if(Crcflg)
    {
        PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CRC, REST_BLK_SZ_CRC, 0, 0), REST_BLK_SZ_CRC);
    }
    else
    {
        PE_NOT(myReadcond(mediumD, &rcvBlk[1], REST_BLK_SZ_CS, REST_BLK_SZ_CS, 0, 0), REST_BLK_SZ_CS);
    }

    if(rcvBlk[1] + rcvBlk[2] != 255)
    {
        goodBlk1st = goodBlk = false;
    }

    if (rcvBlk[1] == numLastGoodBlk - 1)
    {
        goodBlk1st = false;
    }
    else if ((rcvBlk[1] != numLastGoodBlk)  && (rcvBlk[1] != numLastGoodBlk + 1))
    {
        goodBlk1st = goodBlk = false;
    }

    if (Crcflg)
    {
        uint16_t myCrc;
        crc16ns(&myCrc, &rcvBlk[3]);

        if ((uint16_t)(myCrc << 8 | myCrc >> 8) == (uint16_t)(rcvBlk[PAST_CHUNK] << 8 | rcvBlk[PAST_CHUNK+1]))      //(!(rcvBlk[131] == (MyCrc>>8)) && (rcvBlk[132] == MyCrc))
        {
            crcCheck = true;
        }
        else
        {
            crcCheck = goodBlk1st = goodBlk = false;
        }
    }
    else
    {
        for(int i = 0; i < CHUNK_SZ; i++)
        {
            checksum += rcvBlk[DATA_POS+i];
        }

        if (checksum == rcvBlk[PAST_CHUNK])
        {
            checkSumCheck = true;
        }
        else
        {
            checkSumCheck = goodBlk1st = goodBlk = false;
        }
    }


}

//Write chunk (data) in a received block to disk
void ReceiverX::writeChunk()
{
	PE_NOT(write(transferringFileD, &rcvBlk[DATA_POS], CHUNK_SZ), CHUNK_SZ);
}

//Send 8 CAN characters in a row to the XMODEM sender, to inform it of
//	the cancelling of a file transfer
void ReceiverX::can8()
{
	// no need to space CAN chars coming from receiver in time
    char buffer[CAN_LEN];
    memset( buffer, CAN, CAN_LEN);
    PE_NOT(myWrite(mediumD, buffer, CAN_LEN), CAN_LEN);
}