/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//serOut_nonBlock 0.15-2
//
// NOTE: IT *IS* possible to run this and serOut on two separate programs
//       (to open the same port from different apps, one Tx and one Rx)


//0.15-2 void function with return-value!
//       errno_handleError() isn't optimizing out the 'return <val>'
//0.15-1 adding note to Usage, for return on init-failure
//0.15 Requirement of serPort[] and baud to be defined in main causes
//     conflict with serIn_nonBlock
//     Trying to bring these together in 'serialThing'
//0.10-2 continued...
//       adding 'cfmakeraw()' functionality here per serin_nonBlock
//       discoveries. 'sall good a/o serin_nonBlock 0.10-4
//0.10-1 Had to modify serIn_nonBlock for debian, so checking it with this
//       Modifying 'test' to accept an argument for the device...
//       This appears to be working, but serIN doesn't seem to be...
//       (Tested with a 'scope)
//0.10 stolen from cTools/serIOtest20.c
//     structure stolen from ../serIn_nonBlock 0.10
// Basic Usage:
//
// main.c:
//  char serPortName[100] = "/dev/cu.usbserial";
//  int  serPortFileDescriptor; //will be assigned.
//  speed_t baud = 19200;
// main() {
//  serPortFileDescriptor = serOutNB_init(serPortName, baud);
//  if(serPortFileDescriptor < 0)
//		return 1;
//  serOutNB_sendByte(serPortFileDescriptor, 'a');
//  while(1) {
//  }
// }


#ifndef _SEROUT_NONBLOCK_H_
#define _SEROUT_NONBLOCK_H_
#include <unistd.h>  // for write()
#include <fcntl.h>   // file control, for open/read
#include <termios.h> //for baud-rate
//#include "../../errno_handleError/0.15/errno_handleError.h"
#include "errno_handleError.h"

//extern char serPort[];
//extern speed_t baud;

//int serOutPort;

//Returns nonnegative file-descriptor for the serial port
// unless there's an error (then returns -1)
#define ERRHANDLE_RETURN_NEG1	(-1)
int serOutNB_init(char *portName, speed_t baud)
{
	int serOutPort = open(portName, O_WRONLY, 0);
	errno_handleError("Serial Out Port didn't open.", 
			ERRHANDLE_RETURN_NEG1);

	//The following were originally for serOutPort
	// but I'm assuming they'll work here, and there as well...

	//Set the baud rate and other settings...
	// (need this be done to both files? probably not...)
	struct termios settings;
	tcgetattr(serOutPort, &settings);
	errno_handleError("Unable to get port attributes.", 
			ERRHANDLE_RETURN_NEG1);

	cfsetspeed(&settings, baud);
	errno_handleError("Unable to set baud rate.", 
			ERRHANDLE_RETURN_NEG1);

	//Was commented-out in OSX, but mighta just been luck...
	// e.g. what if we had unicode functioning...?
	//Also, see below...
	//settings.c_oflag &= ~OPOST; //Raw Output (necessary?)

//cfmakeraw()  sets  the terminal to something like the "raw" mode of the
//old Version 7 terminal driver: input is available character by  charac‐
//ter,  echoing is disabled, and all special processing of terminal input
//and output characters is disabled.  The terminal attributes are set  as
//follows:
#define termios_p (&(settings))
//    termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
//                    | INLCR | IGNCR | ICRNL | IXON);
    termios_p->c_oflag &= ~OPOST;
//    termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios_p->c_cflag &= ~(CSIZE | PARENB);
    termios_p->c_cflag |= CS8;
//cfmakeraw() and cfsetspeed() are  nonstandard,  but  available  on  the
//BSDs.
//Feature Test Macro Requirements for glibc (see feature_test_macros(7)):
//    cfsetspeed(), cfmakeraw(): _BSD_SOURCE
//TODO: Look into Feature Test Macros...
// It seems cfmakeraw IS available in linux, but only if _BSD_SOURCE is set
// via FTMs... too confusing right now.
// cfmakeraw(&settings);



	// Apply the settings
	tcsetattr(serOutPort, TCSANOW, &settings);
	errno_handleError("Unable to set port attributes", 
			ERRHANDLE_RETURN_NEG1);

	tcflush(serOutPort, TCOFLUSH);
	errno_handleError("tcflush() failed.", 
			ERRHANDLE_RETURN_NEG1);


	return serOutPort;
}

//Return value is only because errno_handleError compiles with a "return
//<value>" call...
// (It wouldn't be the case if optimized...)
int serOutNB_sendByte(int serOutPort, char txChar)
{
	write(serOutPort, (void*)(&txChar), 1);
	errno_handleError("Unable to sendByte.",
			ERRHANDLE_DONT_RETURN);
}


#endif

/* mehPL:
 *    I would love to believe in a world where licensing shouldn't be
 *    necessary; where people would respect others' work and wishes, 
 *    and give credit where it's due. 
 *    A world where those who find people's work useful would at least 
 *    send positive vibes--if not an email.
 *    A world where we wouldn't have to think about the potential
 *    legal-loopholes that others may take advantage of.
 *
 *    Until that world exists:
 *
 *    This software and associated hardware design is free to use,
 *    modify, and even redistribute, etc. with only a few exceptions
 *    I've thought-up as-yet (this list may be appended-to, hopefully it
 *    doesn't have to be):
 * 
 *    1) Please do not change/remove this licensing info.
 *    2) Please do not change/remove others' credit/licensing/copyright 
 *         info, where noted. 
 *    3) If you find yourself profiting from my work, please send me a
 *         beer, a trinket, or cash is always handy as well.
 *         (Please be considerate. E.G. if you've reposted my work on a
 *          revenue-making (ad-based) website, please think of the
 *          years and years of hard work that went into this!)
 *    4) If you *intend* to profit from my work, you must get my
 *         permission, first. 
 *    5) No permission is given for my work to be used in Military, NSA,
 *         or other creepy-ass purposes. No exceptions. And if there's 
 *         any question in your mind as to whether your project qualifies
 *         under this category, you must get my explicit permission.
 *
 *    The open-sourced project this originated from is ~98% the work of
 *    the original author, except where otherwise noted.
 *    That includes the "commonCode" and makefiles.
 *    Thanks, of course, should be given to those who worked on the tools
 *    I've used: avr-dude, avr-gcc, gnu-make, vim, usb-tiny, and 
 *    I'm certain many others. 
 *    And, as well, to the countless coders who've taken time to post
 *    solutions to issues I couldn't solve, all over the internets.
 *
 *
 *    I'd love to hear of how this is being used, suggestions for
 *    improvements, etc!
 *         
 *    The creator of the original code and original hardware can be
 *    contacted at:
 *
 *        EricWazHung At Gmail Dotcom
 *
 *    This code's origin (and latest versions) can be found at:
 *
 *        https://code.google.com/u/ericwazhung/
 *
 *    The site associated with the original open-sourced project is at:
 *
 *        https://sites.google.com/site/geekattempts/
 *
 *    If any of that ever changes, I will be sure to note it here, 
 *    and add a link at the pages above.
 *
 * This license added to the original file located at:
 * /home/meh/_commonCode/osccalibrator/0.10ncf/serialThingPortable/serOut_nonBlock.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
