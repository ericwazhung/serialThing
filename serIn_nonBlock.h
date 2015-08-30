/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//serIn_nonBlock 0.20
//
// NOTE: IT *IS* possible to run this and serOut on two separate programs
//       (to open the same port from different apps, one Tx and one Rx)
//0.20 Previously: Things like IGNPAR (ignore parity/framing errors) were
//     left to their original values... whatever they may have been
//     Looking into making use of these things....
//0.15-4 Adding SERINNB_GETBYTE_DEVICE_DISCONNECT, etc.
//       (see Usage, below)
///      wasn't there something else, as well...?
//0.15-3 Usage Notes, re: properly exitting...
//0.15-2 BIG CHANGES HERE
//        This is to match the new functionality in serOut_nonBlock,
//          where global-variables aren't expected
//        Since 0.15(-1) have only been used with 'test', lets:
//       BURRY 0.15 and 0.15-1
//0.15-1 Using errno_handleError 0.15
//0.15   Attemping to make "serialThing" -> osccalibrator's calibratorApp
//       use this, instead of having localized code
//       Due to its ability to reconnect, we need to divide _init() a bit.
//       Also TCOFLUSH?! Shouldn't this've been TCIFLUSH?!
//0.10-5 ... making sure all cfmakeraw functionality is POSIX compliant
//0.10-4 ...
//       Yes, Modem-Control Lines was an issue
//       SO IS 'echo' and a few others...
//       TODO: look into cfmakeraw()?
//             (Its functionality should be manually-implemented, now).
//       'sall good a/o serout_nonBlock 0.10-2
//0.10-3 (still working with Debian)
//       Not getting any input, is this a tty issue, with modem-control
//       lines?
//0.10-2 a/o Debian:
//       Haven't actually run into a need to fix this yet, but being
//       proactive. stdIn_nonBlock was rendered nonfunctional due to 'magic
//       number' error-testing, where error-numbers don't match in OSX vs
//       Debian... Surely we'll have the same issue here. So, fixing it.
//       (TODO: UNTESTED. Also, check 'operation timed out' errno?)
//       Modified 'test' to handle /dev/whatever as an argument
//       Pseudo-tested.
//       TODO:
//         Make notes re: BEWARE (per 'man errno'):
// A common mistake is to do
//     if (somecall() == -1) {
//         printf("somecall() failed\n");
//         if (errno == ...) { ... }
//     }

//0.10-1 fix hidden warnings:
//				added include unistd
//				WHOOPS! if(ret = -1) rather than ==
//0.10 stolen from cTools/serIOtest20.c

// Basic Usage:
//
// main.c:
//  char serPortName[100] = "/dev/cu.usbserial";
//  int serPortFileDesc; //will be assigned
//  speed_t baud = 19200;
// main() {
//  serPortFileDesc = serInNB_init(serPortName, baud, TRUE/FALSE);
//  if(serPortFileDesc < 0)
//		return -1;
//  while(1) {
//    int rxTemp = serInNB_getByte(serPortFileDesc);
//    if(rxTemp >= 0)
//      process it...
//  }
// }

// OPTION 
// (OSX only, so-far, enabling it otherwise will have no effect unless the 
// system happens to also use errno=35 for "Resource Temporarily 
// Unavailable" --This is a HOKEY test!)
// Before #including this file, add:
// #define SERINNB_SUPPORT_DISCONNECT 1
// E.G. in serialThing...
// To support getByte's notifying the calling-function that the device was
// disconnected.
// getByte() always returns + values for received values
//           or -1 in other cases, EXCEPT this case, if enabled:
//#define SERINNB_GETBYTE_DEVICE_DISCONNECT	(-2)
// This value is only defined (later) if we think the system supports it.

#ifndef _SERIN_NONBLOCK_H_
#define _SERIN_NONBLOCK_H_
#include <unistd.h>	// for read()
#include <fcntl.h>   // file control, for open/read
#include <termios.h> //for baud-rate
//#include "../../errno_handleError/0.15/errno_handleError.h"
#include "errno_handleError.h"

extern char serPortName[];
//extern speed_t baud;

//int serInPort;

//Generally you'll just use these two...
int serInNB_init(char *serPortName, speed_t baud, int checkFramingErrors);
int serInNB_getByte(int serPortFileDesc);

#define ERRHANDLE_RETURN_NEG1	(-1)
/*
int handleError_wrapper(char *string, int doExit)
{
	errno_handleError(string, doExit);

	return 0;
}
*/

// openPort is mostly for internal purposes
//Generally this'll be called with an argument of FALSE, unless e.g. using
//reconnection-scheme in serialThing
//Will return -1 if there's an error and ignoreError is FALSE
//Otherwise, will return the serialPortFileDescriptor
int serInNB_openPort(char *serPortName, int ignoreError)
{
	//open() returns a file descriptor, a small, nonnegative integer 
	int serInPort = open(serPortName, O_RDONLY | O_NONBLOCK, 0);

	if(!ignoreError)
		errno_handleError("Serial In Port didn't open.", 
				ERRHANDLE_RETURN_NEG1);

	//This'll return a nonnegative integer unless there's an error
	return serInPort;
}

int serInNB_configurePort(int serPortFileDesc, speed_t baud, int dontFlush,
																	int checkFramingErrors);

#define FALSE 0

//Returns a fileDescriptor, or -1 if there's an error.
int serInNB_init(char* serPortName, speed_t baud, int checkFramingErrors)
{
	int fileDesc;
	//serInPort = open(serPortName, O_RDONLY | O_NONBLOCK, 0);

	//errno_handleError("Serial In Port didn't open.", 1);

	if(0 > (fileDesc = serInNB_openPort(serPortName, FALSE)))
		return -1;

	if(serInNB_configurePort(fileDesc, baud, FALSE, checkFramingErrors))
		return -1;

	return fileDesc;
}


//Returns 0 if everything goes smoothly...
int serInNB_configurePort(int fileDesc, speed_t baud, int dontFlush,
																	int checkFramingErrors)
{
	//The following were originally for serOutPort
	// but I'm assuming they'll work here, and there as well...

	//Set the baud rate and other settings...
	// (need this be done to both files? probably not...)
	struct termios settings;
	tcgetattr(fileDesc, &settings);
	errno_handleError("Unable to get port attributes.", ERRHANDLE_RETURN_NEG1);

	cfsetspeed(&settings, baud);
	errno_handleError("Unable to set baud rate.", ERRHANDLE_RETURN_NEG1);
/* Debian, after some experiments:
$ sudo stty -F /dev/ttyUSB0 -a
speed 19200 baud; rows 0; columns 0; line = 0;
intr = ^C; quit = ^\; erase = ^?; kill = ^U; eof = ^D; eol = <undef>;
eol2 = <undef>; swtch = <undef>; start = ^Q; stop = ^S; susp = ^Z; rprnt =
^R;
werase = ^W; lnext = ^V; flush = ^O; min = 1; time = 0;
-parenb -parodd cs8 hupcl -cstopb cread clocal -crtscts
-ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr icrnl ixon
-ixoff
-iuclc -ixany -imaxbel -iutf8
opost -olcuc -ocrnl onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0
ff0
isig icanon iexten echo echoe echok -echonl -noflsh -xcase -tostop -echoprt
echoctl echoke

NOTE: echo!
*/


	//TODO: Use cfmakeraw, as well?
/* Redundant with below
	//A/O Debian: 'Ignore Modem Control Lines'
	settings.c_cflag |= CLOCAL;


	//Disable: 'Echo input characters', 'Canonical Mode', Signal generation
	settings.c_lflag &= ~(	  ECHO 			//Echo input characters.
									| ICANON
	// Input is made available line by line.
	// Line  editing is enabled
	//In  noncanonical  mode input is available immediately
									| ISIG
	//When  any  of  the  characters  INTR,  QUIT,  SUSP, or DSUSP are
	//received, generate the corresponding signal.
									| IEXTEN
	// implementation-defined input processing.  This  flag,  as
	//well  as ICANON must be enabled for the special characters EOL2,
	//LNEXT, REPRINT, WERASE to be interpreted, and for the IUCLC flag
	//to be effective.
	);
*/

//cfmakeraw()  sets  the terminal to something like the "raw" mode of the
//old Version 7 terminal driver: input is available character by  charac‐
//ter,  echoing is disabled, and all special processing of terminal input
//and output characters is disabled.  The terminal attributes are set  as
//follows:
#define termios_p (&(settings))
//NOTE: These have all been verified to be POSIX compliant...
    termios_p->c_iflag &= ~(IGNBRK | BRKINT | ISTRIP // | PARMRK
                    | INLCR | IGNCR | ICRNL | IXON);

	 //NEW a/o v0.20: DON'T ignore Parity/Framing errors:
	 //PARMRK: If IGNPAR is not set, prefix a character with a parity error
	 //or framing  error  with  \377  \0.  If neither IGNPAR nor PARMRK is
	 //set, read a character with a parity error or  framing	error  as \0.
	 //I'm looking forward to \377 \0... so PARMRK should be set and IGNPAR
	 //should not be.
	 if(checkFramingErrors)
	 {
	 	termios_p->c_iflag |= PARMRK;
	 	termios_p->c_iflag &= ~(IGNPAR);
    }
	 else
	 {
	 	termios_p->c_iflag &= ~(PARMRK | IGNPAR);
	 }

//    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios_p->c_cflag &= ~(CSIZE | PARENB);
    termios_p->c_cflag |= CS8;
//cfmakeraw() and cfsetspeed() are  nonstandard,  but  available  on  the
//BSDs.
//Feature Test Macro Requirements for glibc (see feature_test_macros(7)):
//    cfsetspeed(), cfmakeraw(): _BSD_SOURCE
//TODO: Look into Feature Test Macros...
// It seems cfmakeraw IS available in linux, but only if _BSD_SOURCE is set
// via FTMs... too confusing right now.
//	cfmakeraw(&settings);

	//settings.c_oflag &= ~OPOST; //Raw Output (necessary?)
	// Apply the settings from above immediately
	tcsetattr(fileDesc, TCSANOW, &settings);
	errno_handleError("Unable to set port attributes", ERRHANDLE_RETURN_NEG1);
/*
	tcsetattr(fileDesc, CLOCAL, &settings);
	errno_handleError("Unable to set 'Ignore Modem Control Lines' CLOCAL",
			ERRHANDLE_RETURN_NEG1);
*/
//'man termios': TODO:
//Note  that  tcsetattr() returns success if any of the requested changes
//could be successfully carried out.   Therefore,  when  making multiple
//changes  it may be necessary to follow this call with a further call to
//tcgetattr() to check that all changes have been performed successfully.

// WTF?
//	tcflush(fileDesc, TCOFLUSH);

	if(!dontFlush)
	{
		tcflush(fileDesc, TCIFLUSH);
		errno_handleError("tcflush() failed.", ERRHANDLE_RETURN_NEG1);
	}

	return 0;
}

int serInNB_getByte(int fileDesc)
{
	//a/o v0.20:
	// This *might* be possible
	// (*probably* should be, in fact)
	// When there's a parity/framing error, we should receive *three* bytes:
	// \377 \0 and the received (likely damaged) character
	// It's entirely plausible that a device might send such a sequence
	// normally.
	// So, to attempt to detect whether the sequence is transmitted,
	// or the result of a framing-error, we can keep track of how many calls
	// to this function have resulted in a no-"reception" since the last
	// reception...
	// There're a lot of assumptions, here...
	// First, that this is called often-enough *from the start* *and
	// continuously* that the receive-buffer hasn't had a chance to contain
  	// *any* data
	// That this function is called faster than reception of individual
	// bytes can occur...
	// Probably more I hadn't thought of. 
	// In fact, let's just *not* for now... maybe later.
	// Another possibility: If \377 (0xff) was received, then *immediately*
	// recurse to check for \0, and then for the character
	// (This'd probably require some sort of *buffer* *here*
	//  such that it could return the 0xff (and 0x00?) that *didn't*
	//  correspond to a framing-error, within following calls)
	//static int noDataCount = 0;


   unsigned char rxTemp = '\0';
   int ret;

   ret = read(fileDesc, &rxTemp, 1);

   // Byte Received...
   if(ret == 1)
   {
		//Old Message... not sure whether it's still relevent.
      //Apparently Error 35 "Resource temporarily unavailable"
      //is returned even when bytes are received?!

   //This error (35) is for MacOS (10.5.8)
   //"Resource Temporarily Unavailable" isn't an error...
   //Debian replies: errno=11: 'Resource temporarily unavailable'
//This is defined in 'man errno.h' as:
//  EAGAIN          Resource temporarily unavailable
// (and was verified as '11' in 'gcc -E -dM ...'
#ifndef EAGAIN
	#define EAGAIN	35
   #error "Not gonna let you get away with just a warning..."
   #error " 'EAGAIN' is not defined, so we're defaulting to #35"
   #error "  which is the value on MacOS 10.5.8"
   #error "These errors can be turned into warnings, if you're cautious"
#endif
  		if(errno == EAGAIN)
			errno = 0;
#if((EAGAIN == 35) && SERINNB_SUPPORT_DISCONNECT)
// getByte() always returns + values for received values
//           or -1 in other cases, EXCEPT this case, if enabled:
#define SERINNB_GETBYTE_DEVICE_DISCONNECT	(-2)
// This value is only defined (here) if we think the system supports it.
#warning "This is a hokey test to check whether we're on MacOS..."
#warning " Adding support for disconnection-notice via errno=6"
#warning " Of course, that may not have the same meaning on a different system"
#warning "FURTHER: This is UNTESTED, as the new implementation"
		//See notes re: disconnection at the bottom of the file...
		else if(errno == 6)
		{
			//"Device Not Configured" Error, returned when the cu.usbserial
			// is unplugged...
			handleError("USB Serial Converter Unplugged:", 0);
			//deviceConfigured = FALSE;
			return SERINNB_GETBYTE_DEVICE_DISCONNECT;
			//This INTERFERES with the return of the received byte!
			// (shouldn't even get here, really... right?)
		}
#endif
      else
         errno_handleError("serGetByte: Byte received but:",
					ERRHANDLE_DONT_RETURN);

//    if(errno != 0)
//    {
//       printf("serGetByte: Byte received, but errno = %d\n", errno);
//       errno = 0;
//    }

      return (int)rxTemp;
   }
   // No byte received...
   else if(ret == -1)
   {
      //Since it's non-blocking, it will return "Operation Timed Out"
      // error 60 if nothing came through. Clear it, it's not an error
      // Wait... now it's 35 "Resource temporarily unavailable"
      //if(errno == 35 || errno == 60)
      //    errno=0;
      if( (errno == EAGAIN)
#if (EAGAIN == 35)
			#error "Assuming errno=60 is 'Operation Timed Out' (OSX 10.5.8)"
			 || (errno == 60)
#else
	#warning "'Operation Timed Out' is not being handled"
#endif
		  )
         errno = 0;
#if((EAGAIN == 35) && SERINNB_SUPPORT_DISCONNECT)
#warning "This is a hokey test to check whether we're on MacOS..."
#warning " Adding support for disconnection-notice via errno=6"
#warning " Of course, that may not have the same meaning on a different system"
#warning "FURTHER: This is UNTESTED, as the new implementation"
		//See notes re: disconnection at the bottom of the file...
		else if(errno == 6)
		{
			//"Device Not Configured" Error, returned when the cu.usbserial
			// is unplugged...
			handleError("USB Serial Converter Unplugged:", 0);
			//deviceConfigured = FALSE;
			return SERINNB_GETBYTE_DEVICE_DISCONNECT;
		}
#endif
      else
         errno_handleError("serGetByte: No byte received but:",
					ERRHANDLE_DONT_RETURN);

      return -1;
   }
   // This shouldn't happen...
   else 
	//read returned 0... e.g. EOF, or upon disconnect of a USB->Serial
	//converter (Debian)
   {
		typeof(errno) errTemp = errno;
      printf("serGetByte: Unexpected: read() returned: %d\n", ret);
      errno_handleError(" with", ERRHANDLE_DONT_RETURN);
		printf("   (just in case, errno was: '%d')\n", errTemp);
      return -1;
   }

}


//Notes Re: Disconnection-detection (USB->Serial converter unplugged)
//
//This was used in serialThing on MacOS, pretty reliably.
//
//Not sure why, off hand, it would be useful... Maybe in the case where the
// driver screwed up...? (the PL2303 driver_s_ were a bit hokey)
//
//In MacOS: (10.5.8ppc)
//   errno is set to 6 "Device Not Configured"
//   (Does read() return a value...?)
//
//In Debian:
//   read() returns 0
//   errno isn't set...
//   So, it appears as though the device returned EOF...(?)
//   Also, the device gets a new number when reattached
//      e.g. /dev/ttyUSB0 -> /dev/ttyUSB1
//
//SO: Reconnection doesn't seem to be an option in Debian, for now.

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
 * /home/meh/_commonCode/osccalibrator/0.10ncf/serialThingPortable/serIn_nonBlock.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
