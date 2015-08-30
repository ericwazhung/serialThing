/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#define TRUE 1
#define FALSE 0

//Setting this true *attempts* to add support for USB->RS232 converter
//disconnect/reconnect
//This isn't implemented anywhere but OSX, so leaving it TRUE here just
//enables it if we're compiling on OSX (or one weird case, which is noted
//in warnings)
#define SERINNB_SUPPORT_DISCONNECT	TRUE

//TODO: Maybe make serIn-Flush on init optional (to intentionally view the
//existing buffered data...?)

//Haven't been versioning...
// New Latest - Adding AutoBaud
//    This is *simplistic*
//    For automatically switching between baud rates when,
//    e.g. running a project that may require switching of clocks:
//    i.e. 16MHz (9600bps) or 8MHz (4800bps)
// Think there were a couple other Latests in between
// Latest - Adding Baud Rate...

// NOTE: TODO: See stdinNonBlock for some notes/fixes
//         clearerr(stdin) != errno = 0 !!!
// 
// 20- from 15... looking into tablet-reading
//  (binary)
// Really, it doesn't make sense to keep this as serIOtest
//  it's becoming pretty app-specific
// 15 USED WITH HPGL threeAxisPoxner94...

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h> //file control for open/read
#include <termios.h> // for baud-rate
// thanks: http://timmurphy.org/2009/08/04/baud-rate-and-other-serial-comm-settings-in-c/
#include <string.h>//Needed for strerror() and more
#include <ctype.h> //isgraph()
#include <time.h>
#include "autoBaud.h"


char serPortName[300] = "/dev/cu.usbserial";
speed_t baud = 9600;

//void updateSpinner(void);

int serInPort;
int serOutPort;

int printAll = 0; //FALSE;
int kb;

#define BUFFLEN 100

int done = 0;
int useAutoBaud = 0;

//#include "../../../__std_wrappers/serIn_nonBlock/0.20/serIn_nonBlock.h"
//#include "../../../__std_wrappers/serOut_nonBlock/0.15/serOut_nonBlock.h"
//#include "../../../__std_wrappers/stdin_nonBlock/0.10/stdin_nonBlock.h"
#include "serIn_nonBlock.h"
#include "serOut_nonBlock.h"
#include "stdin_nonBlock.h"

//Override the default
#define SPINTIME 1
#include "../../../__std_wrappers/spinner/0.10/spinner.h"


void displayUsage(uint8_t argInvalid)
{
	if(argInvalid)
		printf("invalid argument.\n\n");

	printf("Usage: \n"
			 "  (currently works with /dev/cu.usbserial, by default)\n"
			 "  (Also, currently has no support for sending data via the keyboard! Use screen for now...)\n"
			 "  !!! CAN use another terminal to e.g.:\n"
			 "  'echo [-n] \"sendThisText\" >> /dev/cu.usbserial'\n"
			 "  (Interesting, since the port is opened here...)\n"
			 "\n"
			 " -?  print this message\n"
			 " -h  print Hex/Binary\n"
			 " -U  Transmit 'U's quickly (for calibrating an OSCCAL value)\n"
          "     (This used to be default, but is no longer)\n"
			 "     (ALSO: Latest updates may have broken functionality with\n"
			 "      osccalibrator!!! Since usleep has been increased dramatically)\n"
			 " -u  don't print received 'U's\n"
          " -s  don't show the spinner\n"
	       " -r  reconnect when the USB->serial port is disconnected.\n"
			 "     (Only known to function on MacOS 10.5.8)\n"
			 " -t  show time-stamps\n"
			 " -c  show byte-count (cycles at 0xff->0x00)\n"
			 " -n  don't flush the serial-input buffer before starting\n"
			 " -d=<device-node> e.g. '-d=/dev/cu.usbserial'\n"
			 " -b=<baud> Set baud rate (integer, e.g. 9600, 19200)\n"
          " -a  use 'autoBaud' for detecting if a crystal was switched\n"
          "     This is not highly-sophisticated. Requires a compatible data-source.\n"
			 " -f  ignore framing/parity-errors\n"
			 "     errored-bytes will come through as 0x00==\\0\n"
		"     DEFAULT is NOT -f, framing/parity errors will come through\n"
		"     as a sequence of three bytes: 0xff, 0x00, and the likely-\n"
		"     in-error received character\n"
			 "\n"
	" Note, this was compiled in osccalibrator/0.10ncf/calibratorApp/\n");
}

volatile int deviceConfigured = FALSE;

int main(int argc, char *argv[])
{

	uint8_t dontFlush = FALSE;
	uint8_t printHexBin = FALSE;
	uint8_t printReceived_U = TRUE;
	uint8_t printSpinner = TRUE;
	uint8_t reconnect = FALSE;
	uint8_t timeStamp = FALSE;
	uint8_t transmitU = FALSE;
	uint8_t printByteCount = FALSE;
	unsigned int byteCount = 0;
	int ignoreFramingErrors = FALSE;

	uint8_t argNum = 0;

	for(argNum = 1; argNum < argc; argNum++)
	{
		if(argv[argNum][0] == '-')
		{
			if(argv[argNum][1] == '?')
			{
				displayUsage(FALSE);
				return 0;
			}
			else if(argv[argNum][1] == 'f')
			{
				ignoreFramingErrors=TRUE;
			}
			else if(argv[argNum][1] == 'h')
			{
				printf("Since -h could be expected to be 'help'...\n");
				displayUsage(FALSE);
				printf("-------\n\n");
				printHexBin = TRUE;
			}
			else if(argv[argNum][1] == 'U')
			{
			  transmitU = TRUE;
			  printf("FUNCTIONALITY WITH OSCCALIBRATOR MAY WELL BE BROKEN\n");
			}
			else if(argv[argNum][1] == 'u')
				printReceived_U = FALSE;
			else if(argv[argNum][1] == 's')
				printSpinner = FALSE;
			else if(argv[argNum][1] == 'r')
				reconnect = TRUE;
			else if(argv[argNum][1] == 't')
				timeStamp = TRUE;
			else if(argv[argNum][1] == 'c')
				printByteCount = TRUE;
			else if(argv[argNum][1] == 'n')
				dontFlush=TRUE;
         else if(argv[argNum][1] == 'a')
            useAutoBaud = TRUE;
			else if((argv[argNum][1] == 'b') && (argv[argNum][2] == '='))
			{
				if(1!=sscanf(&(argv[argNum][3]), "%lu", &baud))
				{
					printf("Invalid Baud Rate\n");
					return 1;
				}
				
			}
			else if((argv[argNum][1] == 'd') && (argv[argNum][2] == '='))
			{
				//strncpy doesn't guarantee to nul-terminate, if the number of
				//bytes copied == the size-given...
				strncpy(serPortName, &(argv[argNum][3]), sizeof(serPortName)-1);
				//so null-terminate it.
				serPortName[sizeof(serPortName)-1] = '\0';
			}
			else
			{
				//Invalid Argument...
				displayUsage(TRUE);
				return 1;
			}
		}
		else
		{
			//Invalid Argument...
			displayUsage(TRUE);
			return 1;
		}

	}

  int quit = FALSE;

  do //reconnect and plausibly autoBaud
  {

	printf("Opening Serial Port '%s'...\n", serPortName);

	//Because we're allowing reconnect, we can't use serInNB_init()
	// Instead use _openPort and _configure, as well as a discrete call to
	// handleError();
	do
	{
		usleep(100000);
		serInPort = serInNB_openPort(serPortName, TRUE); 
		//Try opening the port, and ignore the error

	} while(reconnect && (serInPort < 0));


	if(reconnect && errno)
		errno = 0;

	errno_handleError("Serial In Port didn't open.", 1);


	printf("Setting Baud Rate to %lu\n", baud);
	serInNB_configurePort(serInPort, baud, dontFlush, !ignoreFramingErrors);




	serOutPort = serOutNB_init(serPortName, baud);
	if(serOutPort < 0)
		return 1;


	// Set STDIN non-blocking... (still requires Return)
	if(stdinNB_init())
		return 1;


	deviceConfigured = TRUE;

	printf("Awaiting Data on %s\n", serPortName);

	int spinnerTimer = 0;


	time_t lastPrintTime;
	time(&lastPrintTime);

	while(!quit && deviceConfigured)
	{

		static uint32_t idleCounter = 0;

		if(transmitU)
		{
			//We need at least 8ms idle before sending a byte...
			// the loop runs once every 1ms
			if(idleCounter > 2) //10)
			{
				uint8_t dataToSend = 0x55; //'U'
				serOutNB_sendByte(serOutPort, (char)(dataToSend));
	
				//printf("Transmitted 0x55 = 'U'\n");
				idleCounter = 0;
			}
		}

		//counter++;

		//if(counter % (960*5) == 0)
		//	printf("Buncha 'U's sent, should be ~5sec\n");

		errno_handleError(" Unhandled Error.", 0);
		spinnerTimer++;


		int kbChar = stdinNB_getChar();

      //The old version which was used forever didn't have keyboard->serial
      //output... now kbCommandMode is always disabled... maybe it'll be
      //reimplemented with escape-codes or something.
      int kbCommandMode = FALSE; 

      if(kbChar == -1)
      {}
      else if(kbCommandMode)
      {
		   switch(kbChar)
		   {
		   	case -1:
		   		break;
		   	case 'q':
		   		quit = 1;
		   		break;
		   	default:
		   		clearerr(stdin);
		   		break;
		   }
      }
      else  //output keyboard data... (after return)
      {
          serOutNB_sendByte(serOutPort, (char)(kbChar));
         printf("Sent: '%c'\n", (char)(kbChar));
      }







		int rxTemp = serInNB_getByte(serInPort); //serGetByte();

      if(useAutoBaud)
      {
         if(autoBaud_update(rxTemp))
         {
            if(rxTemp == AUTOBAUD_Tx96_Rx48_CHAR)
            {
               baud = 9600;
            }
            else if(rxTemp == AUTOBAUD_Tx48_Rx96_CHAR)
            {
               baud = 4800;
            }
            //Since autoBaud_update() ONLY returns a value when a change is
            //detected, we can move the common stuff here...
            reconnect = TRUE;
            printf("*** Switching Baud Rate To: %d ***\n", (int)baud);
            break;
         }
      }

		if(rxTemp >= 0)
		{
			if(printReceived_U || (rxTemp != 'U'))
			{

				if(timeStamp)
				{
					time_t now;
					time(&now);

					if(difftime(now, lastPrintTime) >= 1)
					{
						time(&lastPrintTime);
						char timeString[30];

						strncpy(timeString, ctime(&now), 30);

						uint8_t i;
						for(i=0; i<30; i++)
						{
							if((timeString[i] == '\n') || (timeString[i] == '\r'))
								timeString[i] = '\0';
						}

						printf("\n%s: ", timeString);
					}
				}


				if(printByteCount)
					printf("[0x%02"PRIx8"] ", (uint8_t)(byteCount));

				byteCount++;

				if(printHexBin)
				{
					printf("Received: ");

					int asdf;
					for(asdf=7; asdf>=0; asdf--)
					{
						printf("%d", ((1<<asdf)&rxTemp) ? 1 : 0);
					}
					printf(
						" = 0x%02x = '%c'\n", (unsigned char)rxTemp,
						isgraph((char)rxTemp) ? (char)rxTemp : '	');
				}
				else
				{
					if(( isprint((char)rxTemp)) 
						|| (rxTemp == '\n')
						|| (rxTemp == '\r'))
						printf("%c", (char)rxTemp);
               //backspace
               else if( rxTemp == 0x08 )
                  printf("%c", 0x08);
					else
						printf("?");
					fflush(stdout);
				}
			}

			spinnerTimer = 0;
		}
#ifdef SERINNB_GETBYTE_DEVICE_DISCONNECT
#warning "SERINNB_GETBYTE_DEVICE_DISCONNECT has NOT BEEN TESTED in this new implementation"
		else if( rxTemp == SERINNB_GETBYTE_DEVICE_DISCONNECT )
			deviceConfigured = FALSE;
#endif

		if(printSpinner)
		{
			//Wait for ten seconds after the last reception, then start
			//spinning every second...
			// This value is highly dependent on usleep values, elsewhere.
			if(spinnerTimer > (100)) //1920)
			{
				//spinnerTimer = 0;
				if((spinnerTimer % 10) == 0)
					spinner_update(); //updateSpinner();
			}
		}

		//Only sleep if no data was received
		if((rxTemp < 0))
		{
			//No need to run faster than data can be received...
			// 9600bps -> 960Bps = 1041.6us
			//usleep(1000);
			//Is there any reason not to sleep longer...?
			usleep(100000);
			//usleep(1000000);
			idleCounter++;
		}
	}


	//Not *necessary* according to manpage...
   // BUT: with reconnect and plausibly autoBaud, this is a good ideer
	
   //The serial port didn't seem to close/reopen with autoBaud switching
   //until printf was added here... then it started working...
   // plausibly there's a need for a brief pause... *before* close(?!)
   //printf("Closing Serial Port\n");
   close(serInPort);
	close(serOutPort);
   usleep(100000);
  } while(reconnect && !quit);

  return 0;
}

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
 * /home/meh/_commonCode/osccalibrator/0.10ncf/serialThingPortable/main.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
