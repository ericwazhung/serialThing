/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//autoBaud.h
//
// autoBaud is a feature for serialThing (osccalTest)
//  that allows for switching baud-rates while running
//  e.g. if a project requires switching crystals for debugging...
//  i.e. 16MHz = 9600bps, 8MHz = 4800bps
//
// Upon boot, the autoBaud-capable device will send a baud-identifier
// serialThing will recognize this and switch baud rates accordingly


#ifndef __AUTOBAUD_H__
#define __AUTOBAUD_H__

//
// Let's handle three plausible baud rates, just for thoroughness
//
// Let's see if there's a data-stream that could be considered acceptable,
// framing-wise, by all baud-rates...
// (detecting, and more importantly handling, of framing-errors appears
//  complicated, and plausibly OS-specific...?)
//
// 4800   ¯¯¯¯¯¯¯¯¯___.___._______|
//                  s   0   
//
// 9600   ¯¯¯¯¯¯¯¯¯_._._._._¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
//                 s 0 1 2 3 4 5 6 7 s
//
// 19200  ¯¯¯¯¯¯¯¯¯_________¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
//                 s01234567s

// THERE SEEMS TO BE A DIFFICULTY:
//
// 19200 wants 9 bits low...
// (why not 8?)
//
// Let's try that again...
//

// 4800   ¯¯¯¯¯¯¯¯¯___.___.¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯¯¯¯¯¯¯¯¯
//                  s   0   1   2   3   4   5   6   7   s
//
// 9600   ¯¯¯¯¯¯¯¯¯_._._._.¯'¯'¯'¯'¯'¯'¯¯¯¯¯¯¯¯¯¯
//                 s 0 1 2 3 4 5 6 7 s
//
// 19200  ¯¯¯¯¯¯¯¯¯________¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
//                 s01234567s
//
//I think that should do it.

// SO:
//
// The device should transmit....
// ?
//
// 0x01
//
// Then...
//  
// Don't we have a shitton of cases... ?
//
// Device -> serialThing         tx=0x01, received
// 4800      4800          1:1            0x01
// 4800      9600          1:2            0x03
// 4800      19200         1:4            0x0... NO.... Framing Error
// 9600      4800          2:1                   and multiple bytes...
// 9600      9600          1:1
// 9600      19200         1:2
// 19200     4800          4:1
// 19200     9600          2:1
// 19200     19200         1:1
//
// 0x7f
// 4800 -> 19200 1:4 0x01
// 19200 -> 4800 4:1 ... Invalid start-bit
//
// THUS: For simpleAutoBaud, three cases isn't really possible.
// Which is fine, because, usually I'd be switching between TWO crystals
// (at least in the case of sdramThing3.5-11)


// Let's ignore the 19200 case

// 4800   ¯¯¯¯¯¯¯¯¯___.___.¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯'¯¯¯¯¯¯¯¯¯¯¯
//                  s   0   1   2   3   4   5   6   7   s
//
// 9600   ¯¯¯¯¯¯¯¯¯_._._._.¯'¯'¯'¯'¯'¯'¯¯¯¯¯¯¯¯¯¯
//                 s 0 1 2 3 4 5 6 7 s
//
//
// Transmit 0x7f:
//
// 4800 -> 9600 1:2 0x7f -> 0x...
//
// GAH!
// Does it start with the LSB? Am thinkink so... let's check with PUAT
//
// PUAT suggests LSB is transmitted first:
//
//                 LSB                         MSB
//            start                                stop
// bits:        v   0   1   2   3   4   5   6   7   v 
//  1 .....___     ___ ___ ___ ___ ___ ___ ___ ___ ___ .......
//  0         \___/___X___X___X___X___X___X___X___/
//            ^ ^   ^   ^   ^   ^   ^   ^   ^   ^   ^
//            | |   |   |   |   |   |   |   |   |   |
//            | |   \---+-Data Bits Sampled-+---/   |
//            | |                                   |
//            | \--Start Tested             Stop Bit Sampled
//            |
//            \--Start Detected


// So...
// Not transmitting 0x7f, but 0x01, duh.
//
// 4800 -> 9600 1:2 0x01 -> 0x07
// 9600 -> 4800 2:1 0x01 -> Just A Start Bit. (0x
// NO....
// "active" bits are *LOW*
//
// TRANSMITTING: 0xfe
// 4800 -> 9600 1:2 0xfe -> 0xf8
// 9600 -> 4800 2:1 0xfe -> Just A Start Bit. (0xff)
// No Framing Errors.

//Still seems somewhat plausible that the right value (0xfc?)
// could work with 19200 (three bauds)
// But I don't *need* that right now.

#define AUTOBAUD_SYNC_CHAR 0xfe

//These are for the receiver to detect which type of switchover is
// necessary
#define AUTOBAUD_Tx96_Rx48_CHAR  0xff
#define AUTOBAUD_Tx48_Rx96_CHAR  0xf8


//We need to make sure, e.g. 9600->4800, that each byte is sent with
//sufficient delay that two don't appear as one at the receiving-end....
//Is there a somewhat standardized means to handle this.......?
// can't use another serial-transmission of 0xff
//   because that sends a second start-bit.
// could use _delay_us()
//   but need to account for the likely case where
//   _delay_us(N) is *shorter* than Nus
//   due to a faster crystal.
//   ONE serial packet at 4800bps is 2.08ms
//   so, use e.g. _delay_ms(5)
#define AUTOBAUD_TRANSMIT_MS  5

//At the receiving end, this could be doubled...
// and allow for a little slop (due to e.g. loop-times in main())
#define AUTOBAUD_TIMEOUT_MS   (AUTOBAUD_TRANSMIT_MS*3)

//Let's transmit several sync "packets"...
#define AUTOBAUD_TX_COUNT  96

//And, let's allow for changing baud-rates at the receiver
// and still expect to receive a bunch of *correct* packets...
// e.g. let's detect 1/3rd of TX_COUNT
// then switch (which takes some time)
// then look for another 1/3rd of TX_COUNT
//THIS IS NOT EXACTLY IMPLEMENTED AS DESCRIBED...
// Forgot to consider that in one case 96 would be transmitted but only 48
// would be received...
// However, looking for another 1/3rd of TX_COUNT *after* switching is a
// bit overkill... And it works.
#define AUTOBAUD_DETECT_COUNT (AUTOBAUD_TX_COUNT/3)



#ifdef __AVR_ARCH__
//For now, we're assuming that autoBaud is used as a transmitter at the AVR
// and a receiver at serialThing...
#include <util/delay.h>


//This is for the device, upon boot, to send to serialThing
void autoBaud_boot(uint8_t puatNum)
{
   uint8_t i;

   for(i=0; i<AUTOBAUD_TX_COUNT; i++)
   {
      //TODO: There's no standardized interface for puat vs usart_uart...
      //      vs usi_uart... etc...
      //
      // sdramThing3.5 uses puat, as do most projects currently under
      // development.
      puat_sendByteBlocking(puatNum, AUTOBAUD_SYNC_CHAR);
      //Could, probably, just as easily use tcnts...
      // since puat uses the tcnter
      // OTOH, if this ever gets changed-over to stdio, etc...
      // then maybe that doesn't make sense...
      _delay_ms(AUTOBAUD_TRANSMIT_MS);
   }

   //The buffer seemed to fill-up with *later* data *after* this, while
   // the Serial Port (via serialThing) was still running at the old
   // baud-rate. This should help that...
   _delay_ms(1000);
}
#else // NOT __AVR_ARCH__

//This is for the "receiver"...
// e.g. serialThing running on a PC...


//As has been somewhat standardized, inChar is typically a uint8_t
// but, let's allow this to be called when there's no received character
// which is usually indicated by a negative value.
//
//RETURNS:
//  
// There're two cases when baud-rate changing is necessary
//  *AND DETECTABLE*
//
// device  serialThing   inChar
// 4800 -> 9600          AUTOBAUD_Tx48_Rx96_CHAR
// 9600 -> 4800          AUTOBAUD_Tx96_Rx48_CHAR
// Might as well return this value, when that's the case...
//  NOTE: This just handles detection of the detectable characters
//        occurring back-to-back...
//        It does *not* take-into-account which baud-rate is currently set.
// And return FALSE otherwise
// (THIS WILL ONLY RETURN A NON-FALSE VALUE ONCE, eh...?)
uint8_t autoBaud_update(int16_t inChar)
{
   static int syncCharCount = 0;
   static uint8_t lastInChar = 0x00;

   if(inChar < 0)
      return FALSE;

   //Possibly the beginning of a sync...
   if(lastInChar != inChar)
   {
      lastInChar = inChar;
      syncCharCount = 0;

      return FALSE;
   }

   //Now, we've received two identical characters, back-to-back
   // (ignoring non-receptions)

   syncCharCount++;

   if(syncCharCount >= AUTOBAUD_DETECT_COUNT)
   {
      //inChar and lastInChar are identical...
      if(   (inChar == AUTOBAUD_Tx96_Rx48_CHAR)
         || (inChar == AUTOBAUD_Tx48_Rx96_CHAR)
        )
      {
         return inChar;
      }
      else
         return FALSE;
      //There's really no need to reset anything, right...?
      // because this is obviously not a sync character
      // so it should be ignored... and will keep getting to the above
      // else-case (returning FALSE, appropriately)
      // until inChar changes, which will reset everything appropriately.

   }

   //Otherwise, syncCharCount < AUTOBAUD_DETECT_COUNT
   return FALSE;
}


#endif //Architecture...


#endif // __AUTOBAUD_H__

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
 * /home/meh/_commonCode/osccalibrator/0.10ncf/serialThingPortable/autoBaud.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
