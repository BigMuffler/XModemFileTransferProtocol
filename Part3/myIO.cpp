//============================================================================
//
//% Student Name 1: Stefan Ungurean
//% Student 1 #: 301316291
//% Student 1  (email): sungurea (stu1@sfu.ca)
//
//% Student Name 2: Jamyl Johnson
//% Student 2 #: 301313059
//% Student 2 userid (email): jamylj (stu2@sfu.ca)
//
//% Below, edit to list any people who helped you with the code in this file,
//%      or put 'None' if nobody helped (the two of) you.
//
//
// Helpers: _everybody helped us/me with the assignment (list names or put 'None')__
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
//% * Your group name should be "P3_<userid1>_<userid2>" (eg. P3_stu1_stu2)
//% * Form groups as described at:  https://courses.cs.sfu.ca/docs/students
//% * Submit files to courses.cs.sfu.ca
//
// File Name   : myIO.cpp
// Version     : September, 2019
// Description : Wrapper I/O functions for ENSC-351
// Copyright (c) 2019 Craig Scratchley  (wcs AT sfu DOT ca)
//============================================================================

/* Wrapper functions for ENSC-351, Simon Fraser University, By
 *  - Craig Scratchley
 *
 * These functions may be re-implemented later in the course.
 */

#include <unistd.h>			// for read/write/close
#include <fcntl.h>			// for open/creat
#include <sys/socket.h> 		// for socketpair
#include "SocketReadcond.h"
#include <condition_variable>
#include <mutex>
#include <vector>
#include <iostream>

//define Global

std::mutex vecMut;
//int writtenBytes;
//int readBytes;
bool release = false;

struct socketPairing
{
   std::mutex m;
   std::condition_variable cv;
   std::condition_variable bbcv;
   bool Socket = true;
   int myPair = 0;
   int bigBuf = 0;

};

std::vector<socketPairing*> spVec;

int mySocketpair( int domain, int type, int protocol, int des[2] )
{
	int returnVal = socketpair(domain, type, protocol, des);
	std::lock_guard<std::mutex> lck (vecMut);

	//Sets the size of the socket pairing vector to be the largest descriptor
	int size = des[0] > des[1] ? des[0]:des[1];
	if((unsigned)size + 1 > spVec.size()){spVec.resize(size+1);}

	//Create the new socketParing objects
	spVec[des[0]] = new socketPairing;
	spVec[des[1]] = new socketPairing;

	//Links descriptors to each other
	spVec[des[0]]->myPair = des[1];
	spVec[des[1]]->myPair = des[0];

	return returnVal; //removes mutex lock upon leaving
}

int myOpen(const char *pathname, int flags, mode_t mode)
{
//	std::lock_guard<std::mutex> lck (vecMut);
//	int des = open(pathname, flags, mode);
//	if((unsigned)des <= spVec.size()){spVec.resize(des+1);}
//	spVec[des] = new socketPairing;
//	spVec[des]->Socket = false;
//	return des;
	return open(pathname, flags, mode);
}

int myCreat(const char *pathname, mode_t mode)
{

//	std::lock_guard<std::mutex> lck (vecMut);
//	int des = creat(pathname, mode);
//	if((unsigned)des <= spVec.size()){spVec.resize(des+1);}
//	spVec[des] = new socketPairing;
//	spVec[des]->Socket = false;
//	return des;
	return creat(pathname, mode);
}


ssize_t myWrite( int fildes, const void* buf, size_t nbyte )
{
	//std::unique_lock<std::mutex> lck (spVec[(spVec[fildes]->myPair)] -> m);


    //int mpbb = spVec[(spVec[fildes]->myPair)]->bigBuf;

    //if (spVec[fildes] == nullptr )
    if (((unsigned)fildes >= spVec.size()) || !(spVec.at(fildes)))
    {
        return write(fildes, buf, nbyte );
    }
    std::lock_guard<std::mutex> lck (spVec[fildes]->m);
    int writtenBytes = 0;

    //std:: cout << "Buff: " << spVec[fildes]-> bigBuf << std::endl;

	writtenBytes = write(fildes, buf, nbyte ); //write bites to variable

	spVec[fildes]->bigBuf += writtenBytes; //increment bytes

	//std::cout << "Increment buff by " << writtenBytes << " bytes" << std::endl;

    //std:: cout << "Buff: " << spVec[fildes]->bigBuf << std::endl;

    //std :: cout << "before bbcv Notify"<< std::endl;

    if (spVec[fildes]->bigBuf >= 0) //check if buffer is larger than 0
    {
    	//std :: cout << "bbcv Notify start"<< std::endl;
    	//release = true;
    	spVec[fildes]->bbcv.notify_all(); //notify statement in myreadcond
    	//std :: cout << "bbcv Notify end"<< std::endl;
    }
//    else if (spVec[fildes]->bigBuf < 0)
//    {
//    	release = true;
//    	spVec[fildes]->bbcv.notify_all(); //notify statement in myreadcond
//    }

	return writtenBytes;
}

int myClose( int fd )
	{
		std::lock_guard<std::mutex> lck (vecMut);

		 //if (spVec[fd] == nullptr)
		 if (((unsigned)fd >= spVec.size()) || !(spVec.at(fd)))
		 {
			 return close(fd);
		 }
		 else
		 {

			 //lock mutexes !!!!!!!!!!!!!
			 //std::lock_guard<std::mutex> lck (spVec[fd] -> m);
			 //std::lock_guard<std::mutex> lck1 (spVec[(spVec[fd]->myPair)] -> m);

			 if (spVec[fd]->bigBuf > 0)  //if (spVec[fd]->bigBuf <= 0)
			 {
				 //spVec[fd]->bigBuf = 0;
				 spVec[fd] -> cv.notify_all();
			 }
			 if (spVec[fd]->bigBuf <= 0)
			 {
				 //spVec[fd]->bigBuf = 0;
				 release = true;
				 spVec[fd] -> bbcv.notify_all();
			 }

			 if(spVec[(spVec[fd]->myPair)]->bigBuf > 0) //if(spVec[(spVec[fd]->myPair)]->bigBuf > 0)
			 {
				 spVec[(spVec[fd]->myPair)]->bigBuf = 0;
				 spVec[(spVec[fd]->myPair)]->cv.notify_all();
			 }

			 if(spVec[(spVec[fd]->myPair)]->bigBuf < 0)
			 {
				 spVec[(spVec[fd]->myPair)]->bigBuf = 0;
				 //release = true;
				 spVec[(spVec[fd]->myPair)]->bbcv.notify_all();
			 }


			 return close(fd);
		 }


		//return close(fd);

}




int myTcdrain(int des)
{ //is also included for purposes of the course.
    //Block calling thread until all previous characters have been sent

    //std::cout << "In Tcdrain" << std::endl;

    std::unique_lock<std::mutex> ulk (spVec[des]->m);

    // craig stuff data_cond.wait(lk,[this]{return !data_queue.empty();});
    //std::cout << "Before cv wait" << std::endl;

    spVec[des]->cv.wait(ulk, [des] {return (spVec[des]->bigBuf <= 0 );}); //<=

	return 0;
}

int myReadcond(int des, void * buf, int n, int min, int time, int timeout)
{
	//std::lock_guard<std::mutex> lck (spVec[des]->m); spVec[(spVec[des]->myPair)]
    std::unique_lock<std::mutex> lck (spVec[(spVec[des]->myPair)]-> m); //, std::defer_lock);
    int readBytes = 0;
    //int tempBuf = 0;
    //std::cout << "In Read" << std::endl;

    if (spVec[(spVec[des]->myPair)]->bigBuf < min)
    {
        //tempBuf = spVec[des]->bigBuf;

    	spVec[(spVec[des]->myPair)]->bigBuf -= min;

    	spVec[(spVec[des]->myPair)]->cv.notify_one();

        //std::cout << "START of waiting in min --->>>>>>>> bigBuf >= min, wait complete" << std::endl;

        spVec[(spVec[des]->myPair)]->bbcv.wait(lck, [des] {return (spVec[(spVec[des]->myPair)]->bigBuf >= 0|| release);}); //wait until buffer is larger than minimum

        //std::cout << "END of waiting in min --->>>>>>>> bigBuf >= min, wait complete" << std::endl;

        spVec[(spVec[des]->myPair)]->bigBuf += min;
    }


    readBytes = wcsReadcond(des, buf, n, min, time, timeout) ;

    //std:: cout << "Buff: " << spVec[(spVec[des]->myPair)]->bigBuf << std :: endl;
    spVec[(spVec[des]->myPair)]->bigBuf -= readBytes;
	//std::cout << "Decrement buff by" << readBytes << " bytes" << std::endl;
	//std:: cout << "Buff: " << spVec[(spVec[des]->myPair)]->bigBuf << std :: endl;


    if (spVec[(spVec[des]->myPair)]->bigBuf == 0)
    {
    	spVec[(spVec[des]->myPair)]->cv.notify_one();
    }

    return readBytes;

    //return wcsReadcond(des, buf, n, min, time, timeout) ;
}
ssize_t myRead( int des, void * buf, size_t nbyte ) // Thread safe
{

    //if (spVec[des] == nullptr )
	if (((unsigned)des >= spVec.size()) || !(spVec.at(des)))
    {
        return read(des, buf, nbyte);
    }

    return myReadcond(des, buf, nbyte, 1, 0, 0) ;
}




