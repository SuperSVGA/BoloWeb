//
// BoloWeb v0.8
// ©1996 Pavani Diwanji <pavani@CS.Stanford.EDU>
//
// BoloWeb is freeware. Providing that you acknowledge my authorship, 
// you may distribute it any way you like, or do anything else with it, 
// including producing derivative works from the source code. 
//

#include <stdio.h>		// for scanf
#include <stdlib.h>		// for strtol
#include <string.h>		// for strlen

#include <Devices.h>
#include <TCPPB.h>
#include <AddressXlation.h>
#include <GetMyIPAddr.h>
#include <ToolUtils.h>
#include <AppleEvents.h>

#include "my_types.h"
#include "BoloWeb.h"
#include "InputParser.h"

// Use the following for debugging purposes when the Bolo Tracker 
// is down/uncontactable or is returning garbage.
//#define USE_INPUT_FILE
// XXX define here what input file to use.

Ptr user_buffer;			  // 8k buf used to receive on the tcp connection -
long user_buffer_len;		  // allocated at init time and reused
local long user_buffer_index; // current index into the user_buffer


local struct TCPiopb cb;
local Ptr stream_rcvBuff=NULL;
local GameInfoList *gamelist = NULL;

local char *BoloTrackerMachine = MattBoloTrackerMachine;
local long BoloTrackerPort = MattBoloTrackerPort;
local ip_addr BoloTrackerIP;

#ifdef USE_INPUT_FILE
FILE *input_fp = NULL;
#endif

local OSErr ContactMattTracker(void); // forward declaration

// Stuart's utility routines that do name resolution i.e. hostname ->ip addr
local pascal void name_callback(struct hostInfo *info, char *finished)
{
	info;
	*finished = TRUE;
}

local ip_addr get_ip_address(char *name)
{
	ip_addr answer = 0;
	//DebugStr("\pget_ip_address");
	// Matt: 128.135.34.9
	//answer = 128L<<24 | 135L<<16 | 34L<<8 | 9L;
	
	if (OpenResolver(NULL) == noErr)
		{
		struct hostInfo x;
		char finished = FALSE;
		StrToAddr(name, &x, name_callback, &finished);
		if (x.rtnCode == cacheFault) while (!finished) continue;
		if (x.rtnCode == noErr) answer = x.addr[0];
		CloseResolver();
		}
	return(answer);
}

// The only exported routines from this file are:
// 		OSErr InitGameList(void);
//		GameInfoList *GetGameList(void);
//		void DisposeGameList(GameInfoList *);
// 		void DisplayGameList(GameInfoList *);
// 		void DebugGameList(GameInfoList *);


export OSErr InitGameList(void)
{
OSErr err = noErr;
BWeb **bweb;

	// Intialize all the globals
	user_buffer_index =  MAX_USER_BUFFER_LENGTH;
	gamelist = NULL; stream_rcvBuff=NULL;

	// Initalize the user buffer
	// This 8k buffer will stick around till the application 
	// goes away and will get reused
	user_buffer = NewPtr(MAX_USER_BUFFER_LENGTH);
	if (!user_buffer) return MemError();
	user_buffer_len = MAX_USER_BUFFER_LENGTH;
	
	// Open TCP driver
	err = OpenDriver("\p.IPP", &cb.ioCRefNum); 
	if (err != noErr) {
		return err;
	}

    bweb = (BWeb **)GetResource('BWeb', 128);
    if (bweb != NULL) {
	  MoveHHi((Handle)bweb);
	  HLock((Handle)bweb);
      if ((*bweb)->name[0] != 0)
		BoloTrackerMachine = ((*bweb)->name);
	  BoloTrackerPort = ((*bweb)->port);
	}
		
	BoloTrackerIP = get_ip_address(BoloTrackerMachine);
	
return err;
}

// Get a new game list by contacting the bolotracker.
// XXX change this to return one list even if the other fails
export GameInfoList *GetGameList(void)
{
OSErr err = noErr;

	// Intialize all the globals - necessary for every new iteration
	user_buffer_index =  MAX_USER_BUFFER_LENGTH;
	gamelist = NULL; stream_rcvBuff=NULL;
	
	// NOTE WE DO NOT NEED TO CONTACT MIKE's TRACKER AS MATT CONTACTS HIM AND
	// INCLUDES HIS GAMES IN THE LIST
	
	// Contact Matt's tracker
	err = ContactMattTracker();
	
	if (err != noErr) {
		/*debug("ContactMattTracker failed");*/
		// Delete any half baked list we formed
		if (gamelist) { DisposeGameList(gamelist); gamelist = NULL; }
		return NULL;
	}

	return gamelist;
}

// Utility routine to delete the existing  game info list.
// This routine is used to get rid of an existing list, when
// tracker is to be contacted for the new list.
export void DisposeGameList(GameInfoList *oldlist)
{
GameInfoList *curr = oldlist;
GameInfoList *temp = NULL;
	while (curr) {
		//DebugStr("\pDisposing...");
		//if (curr->map_name) DisposePtr((Ptr)(curr->map_name));
		temp = curr; 
		curr = curr->next;
		DisposePtr((Ptr)temp);
	} // end of while
	memcheck();
}

// Useful Debugging routines 
// Display the GameInformation Link List
// XXX Change this to log information (query,reply in a compact
// format to a file.
export void DebugGameList(GameInfoList *theGameList)
{
GameInfoList *curr = theGameList;
int i=0;

	debug("Game List:\n");
	while (curr) 
	{	
		debug("%s Game: %s\n", curr->game_type, curr->map_name);
		debug("\t\tPlayers: %d\n", curr->num_players);
		debug("\t\tPillBoxes: %d\n", curr->num_pillboxes);
		debug("\t\tNeutral Bases: %d\n",curr->num_bases);
		debug("\t\tHidden Mines: %s\n", (curr->hidden_mines) ? "Allowed": "not allowed");
		debug("\t\tRobots: %s\n", (curr->robots) ? "Allowed": "not allowed");

		for (i=0; i<curr->num_sites; ++i) {
			debug("\t\tIP Address: %s\n", curr->ip_addr[i]);
			debug("\t\tPort: %ld\n", curr->port[i]);
			debug("\t\tPlace: %s\n", curr->place[i]);
		}
		curr = curr->next;

	}
}

export void DisplayGameList(GameInfoList *)
{
/*
GameInfoList *curr = glist;
int i=0;

	printf("Game List:\n");
	while (curr) 
	{	
		printf("%s Game: %s\n", curr->game_type, curr->map_name);
		printf("\t\tPlayers: %d\n", curr->num_players);
		printf("\t\tPillBoxes: %d\n", curr->num_pillboxes);
		printf("\t\tNeutral Bases: %d\n",curr->num_bases);
		printf("\t\tHidden Mines: %s\n", (curr->hidden_mines) ? "Allowed": "not allowed");
		printf("\t\tRobots: %s\n", (curr->robots) ? "Allowed": "not allowed");

		for (i=0; i<curr->num_sites; ++i) {
			printf("\t\tIP Address: %s\n", curr->ip_addr[i]);
			printf("\t\tPort: %ld\n", curr->port[i]);
			printf("\t\tPlace: %s\n", curr->place[i]);
		}
		curr = curr->next;

	}
*/
}

// XXX eventually change this to exit if error.
local void printError(char *call, OSErr err)
{
call; err;
	// debug("Error occured in %s: %d\n", call, err);
}

local OSErr PBControlSyncYield(ParmBlkPtr p)
	{
	long t = TickCount();
	OSErr err = PBControlAsync((ParmBlkPtr)p);
	if (err) return(err);
	while (p->cntrlParam.ioResult == ioInProgress)
		{
		EventRecord e; EventAvail(everyEvent,&e);
		if (TickCount() - t > 5*60*60) debug("Error: wedged in PBControlSyncYield for 5 min");
		}
	return(p->cntrlParam.ioResult);
	}

// The following routines are used for connecting using TCP
// with the Bolo Tracker.
local OSErr OpenStream()
{
OSErr err;

	// debug("Entering from OpenStream"); 
	memcheck();
	// Create a TCP stream
	cb.csCode = TCPCreate;
	stream_rcvBuff = NewPtr(STREAM_BUFFER_LENGTH);
	if (!stream_rcvBuff) {
		debug("stream_rcvBuff failed");
		return MemError();
	}
	// debug("OpenStream NewPtr done"); 
	memcheck();
	cb.csParam.create.rcvBuff = stream_rcvBuff;
	cb.csParam.create.rcvBuffLen = STREAM_BUFFER_LENGTH;
	cb.csParam.create.notifyProc = 0;
	cb.csParam.create.userDataPtr = 0;
	err = PBControlSync((ParmBlkPtr)&cb);
	//  debug("Return from OpenStream"); 
	memcheck();
	return err;
}

local OSErr CloseStream()
{
OSErr err;
	// Release the stream
	cb.csCode = TCPRelease;
	err = PBControlSync((ParmBlkPtr)&cb);
	// Now that stream is released, free the rcv buffer.
	if (err == noErr) { if (stream_rcvBuff) DisposePtr(stream_rcvBuff); stream_rcvBuff = NULL; }
	return err;
}

local OSErr OpenActiveConnection(char *machine, ip_addr ip, long port)
{
OSErr err=-1;
machine;	// unused at the moment
	// Open an active connection
	cb.csCode = TCPActiveOpen;
	cb.csParam.open.ulpTimeoutValue = 0; // use default timeout value = 2 minutes; 
	cb.csParam.open.ulpTimeoutAction = 1; // nonzero means Abort on timeout
	cb.csParam.open.validityFlags = 0xC0; // timeout and action are valid
	cb.csParam.open.remoteHost = ip;
	cb.csParam.open.remotePort = port;
	cb.csParam.open.localPort = 0; 		// assign an unused port
	cb.csParam.open.dontFrag = 0; 
	cb.csParam.open.timeToLive = 0; 	// default=60
	cb.csParam.open.security = 0; 		// no security
	cb.csParam.open.optionCnt = 0;
	cb.csParam.open.userDataPtr = 0;
	if (cb.csParam.open.remoteHost!=0) err = PBControlSyncYield((ParmBlkPtr)&cb);
	return err;
}

local OSErr CloseMyConnection()
{
OSErr err;
	
	// Close the connection
	cb.csCode = TCPClose;
	cb.csParam.close.validityFlags = 0; // use default timeout value; Abort on timeout
	cb.csParam.close.userDataPtr = 0;
	err = PBControlSyncYield((ParmBlkPtr)&cb);
	return err;
}

local Boolean RcvOnConnection()
{
OSErr err;
Boolean ret = true;

	user_buffer_len = MAX_USER_BUFFER_LENGTH;
	cb.csCode = TCPRcv;
	cb.csParam.receive.commandTimeoutValue = 10; // Wait infinitely
	cb.csParam.receive.rcvBuff = user_buffer;
	cb.csParam.receive.rcvBuffLen = user_buffer_len;
	cb.csParam.receive.userDataPtr = 0;
	err = PBControlSyncYield((ParmBlkPtr)&cb);
	user_buffer_len = cb.csParam.receive.rcvBuffLen;
	switch (err) {
		case noErr:
			break;
		case connectionClosing:
		case connectionTerminated:
			printError("Receive", err);
			ret = false;
			break;
		case invalidStreamPtr:
		case invalidLength:
		case invalidBufPtr:
		case commandTimeout:
		case connectionDoesntExist:
		default:
			printError("Receive", err);
			ret = false;
			break;
	}	
	
	return ret;
}

// This is a simple routine which takes in fixed lenght line_buffer and its
// length and returns a line (upto \n) in line_buffer.
// If enough data is not present in the user_buffer, RcvOnConnection is called
// to receive from the network. If the newline character does not occur and
// the line_buffer fills up, its truncated with a newline at the end.
// This routine skips blank lines.
// This routine returns false only if there is no data in the line buffer
local Boolean get_line(char *line, short *line_len)
{

int i;
Boolean done = false;

	while (!done) {

#ifdef USE_INPUT_FILE
		char c;
		
			for (i=0; i<(*line_len)-1; ++i) {
				c=fgetc(input_fp);
				if ((c!= EOF) && (c!= '\n') && (c!='\r'))
					line[i] = c;
				else break;
			}
			
			// if no data return
			if ((c == EOF) && (i==0)) return false;
#else
		
			for (i=0; i<(*line_len)-1; ++i) {
				if (user_buffer_index >= user_buffer_len) {
					if (RcvOnConnection()==false)
						if (i==0) return false; else break;
					// successful in receiving some data
					user_buffer_index = 0;	
				}
				// If we have reached the newline char, break out of for
				// incr user_buffer_index s that next time we do not read the newline
				if ( (user_buffer[user_buffer_index]=='\n') || ((user_buffer)[user_buffer_index]=='\r') )
					{ user_buffer_index++; break;} 
				else line[i] = user_buffer[user_buffer_index++];
			}
#endif
		// if it is not a blank line we are done
		// else start all over again
		if (i>0) done = true;
	} // end of while
	
	// Put int he newline at the end
	line[i++] = '\n';
	// Fill up the new length
	*line_len = i;
	return true;
}

// ----------------- PARSING RPOUTINES SPECIFIC TO MATT'S BOLO TRACKER -------------------

// Host line for Matt's tracker looks like:
// Host: F010.MIM.UIC.EDU  Players: 1  Bases: 11  Pills: 16
local OSErr parse_host_line(char *buf, long buflen, GameInfoList **glistptr)
{
GameInfoList *gi=NULL;
int args = 0;
const char pbuf [10] = "Players:";
int i=0;

	gi = (GameInfoList *) NewPtrClear(sizeof(GameInfoList) * 2);		// * 2 for debugging
	if (!gi) return MemError(); 

	// Discard any information upto the word Players as we do not care
	for (i= 0; i<buflen-strlen(pbuf); ++i) 
			 if (strncmp(buf+i, pbuf, strlen(pbuf))==0) break;

	if (i==buflen-strlen(pbuf)) { DisposePtr((Ptr)gi); return -1; }  // We could not find players?
	
	if((args = sscanf(buf+i, "Players: %d  Bases: %d  Pills: %d",
		&gi->num_players, &gi->num_bases, &gi->num_pillboxes)) != 3)
		 { DisposePtr((Ptr)gi); return -1; }
	// Initalize information
	gi->map_name[0] = '\0';	gi->game_type[0] = '\0';	
	gi->hidden_mines = false; gi->robots=false;
	gi->passwd[0] = '\0'; gi->vers = (99<<8)|2;
	gi->num_sites = 0; // intialize the number of sites to be zero
					
	// Add it to the head of the list
	gi->next = (*glistptr);
	(*glistptr) = gi;
			
	return noErr;			
}

// Map line for Matt's tracker looks like:
// Map: Everard Island  Game: Tourn.  Mines: Visible  Bots: No  PW: No
local Boolean parse_map_line2(char *buf, long buflen, GameInfoList **glistptr)
{
GameInfoList *gi = (*glistptr); 
int Gameindex = 0;
int i, j;
char game_type[64];
char mine[16], bot[8], pw[8];
int args=0;

	// Make sure the line starts with string "Map:"
	if (!((buf[0]=='M') && (buf[1]=='a') && (buf[2]=='p') && (buf[3] ==':') && (buf[4]==' ')))
		return false; 
	// copy everything between "Map:" and Game: in the map_name
	for (i=5, j=0; i<buflen-4; ++i, j++)
		if (((buf[i]=='G') && (buf[i+1]=='a') && (buf[i+2]=='m') 
			&& (buf[i+3] =='e') && (buf[i+4]==':')))
			{ Gameindex = i; break; }
		else gi->map_name[j] = buf[i];
		
	// Replace the trailing balnk in map_name with null
	if (j>0 && gi->map_name[j-1] == ' ') gi->map_name[j-1] = '\0';
	else gi->map_name[j] = '\0';
	
	// interpret the rest of the information.
	if ((args = sscanf(buf + Gameindex, "Game: %s  Mines: %s  Bots: %s  PW: %s", 
		game_type, mine, bot, pw)) != 4)
		return false;
	// Fix the game type
	if (!strcmp("Tourn.", game_type))
		strcpy(gi->game_type, "Tournament");
	else if (!strcmp("Strict", game_type))
		strcpy(gi->game_type, "Strict Tournament");
	else if (!strcmp("Open", game_type))
		strcpy(gi->game_type, "Open game");
	else strcpy(gi->game_type, game_type);
	
	// put in the mine field
	if (!strcmp("Visible", mine))
		gi->hidden_mines = false;
	else if (!strcmp("Hidden", mine))
		gi->hidden_mines = true;
	// put in Bot field
	if (!strcmp("Yes", bot))
		gi->robots = true;	
	// put in passwd
	if (!strcmp("Yes", pw))
		strcpy(gi->passwd, "Password Required");
	return true;
}

// Version line looks like:
// Version: 0.99.5  Tracked-For: 1 min.  Player-List:
// Or alternatively:
// Version: <Unknown>  Tracked-For: 3 min.    Player-List:
// If the version # is unknown, we take it to be 0.996
local Boolean parse_version_line(char *buf, long buflen, GameInfoList **glistptr)
{
int major, minor;
GameInfoList *gi = (*glistptr); 
int args = 0;
buflen;

	// scan the version
	if ((args = sscanf(buf, "Version: 0.%d.%d", &major, &minor)) != 2) {
		// see if the version is unknown; 
		if (!strncmp(buf, "Version: <Unknown>", strlen("Version: <Unknown>")) )
			{ major = 99; minor = 6; }
		else return false;
	}
	gi->vers = (major<<8)|minor;
	return true;
}

// Matts tracker ip line format is like:
//    structlab.vuse.vanderbilt.edu ........ Vanderbilt University
//    141.212.58.209 ....................... University of Michigan
//    EV29-dynamic-133.Stanford.EDU {505} .. SRI International
local Boolean parse_ip_line2(char *buf, long buflen, GameInfoList **glistptr)
{
GameInfoList *gi = (*glistptr);
char *bufptr;
int i, j;

	// skip the leading white space
	for (i=0; i<buflen && buf[i]==' '; i++) continue;
	if (i==buflen) return false;
	
	// If the first character is - or = then it is not the ip line but its the seperator line.
	if ((buf[i]=='-') || (buf[i] =='=')) return false;
	
	// next field is ip addr
	j=0;
	while (i<buflen && buf[i]!=' ' && buf[i]!='\t') gi->ip_addr[gi->num_sites][j++] = buf[i++];
	gi->ip_addr[gi->num_sites][j] = '\0';
	if (i==buflen) return false; // its not ok to get just ipaddr, w/o place
	
	i++;
	// Next thing after the space could be { or a '.'
	// If it is curly brace then Next field is port#;
	if (buf[i] == '{') {
		gi->port[gi->num_sites] = strtol(buf+i+1, &bufptr, 0);
		// read off the curly brace
		if (*bufptr++ != '}')  return false; // half baked port# - err!
		// the next character after } should be a blank - so skip it.
		*bufptr++;
	}
	else {
		// Put the defalut port # in as none is available.
		gi->port[gi->num_sites] = DEFAULT_PORT;
		bufptr = buf+i;
	}
	
	// skip the dots
	while (bufptr && (*bufptr)=='.') bufptr++;
	if (!bufptr) return false; // its not ok to get just ipaddr, w/o place

	// skip the space
	for (; bufptr && *bufptr==' '; bufptr++) continue;
	if (!bufptr) return false;
		
	// put the rest of the string into the place field.
	j=0;
	while (bufptr && (*bufptr != '\n')) gi->place[gi->num_sites][j++] = *bufptr++;
	gi->place[gi->num_sites][j] = '\0';
	
	// increment the num_sites
	gi->num_sites++;
	
	return true;
}

// Parse the input available from the bolo tracker and build up an internal 
// GameInformation Linked List.
// = ==================================================================== =
//
// Host: F010.MIM.UIC.EDU  Players: 1  Bases: 11  Pills: 16
// Map: Everard Island  Game: Tourn.  Mines: Visible  Bots: No  PW: No
// Version: 0.99.5  Tracked-For: 1 min.  Player-List:
//    F010.MIM.UIC.EDU ..................... University of Illinois at Chicago
// 
// Host: structlab.vuse.vanderbilt.edu  Players: 3  Bases: 16  Pills: 16
// Map: Kinda like chicken  Game: Tourn.  Mines: Visible  Bots: No  PW: Yes
// Version: 0.99.5  Tracked-For: 1 min.  Player-List:
//    structlab.vuse.vanderbilt.edu ........ Vanderbilt University
//    141.212.58.209 ....................... University of Michigan
//    155.52.20.29 ......................... Dana-Farber Cancer Institute
//
// - -------------------------------------------------------------------- -
//
// Host: Mulligan-Robert.tuliptree.indiana.edu  Players: 2  Bases: 1  Pills: 2
// Map: Islasa  Game: Tourn.  Mines: Hidden  Bots: Yes  PW: No
// Version: 0.99.2  Tracked-For: 21 min.    Player-List:
//   Mulligan-Robert.tuliptree.indiana.edu  Indiana University Computing Services

local OSErr MattParser(GameInfoList **glistptr)
{
Boolean cont = false;
char buf[LINE_BUFFER_LENGTH];
short len;

	len=LINE_BUFFER_LENGTH; cont = get_line(buf, &len);
	while (cont) {
		if ((buf[0]=='H') && (buf[1]=='o') && (buf[2]=='s') && (buf[3] =='t') && (buf[4] ==':'))  {
			if (parse_host_line(buf, len, glistptr) != noErr) return -1;

			len=LINE_BUFFER_LENGTH; cont = get_line(buf, &len);
			if (!cont || !parse_map_line2(buf, len, glistptr)) return -1;
						
			len=LINE_BUFFER_LENGTH; cont = get_line(buf, &len);
			if (!cont || !parse_version_line(buf, len, glistptr)) return -1;
			
			// Everything after a game line but before a map line is 
			len=LINE_BUFFER_LENGTH; cont = get_line(buf, &len);
			while(cont && 
				!((buf[0]=='H') && (buf[1]=='o') && (buf[2]=='s') && (buf[3] =='t') && (buf[4] ==':'))) {
				if (!parse_ip_line2(buf, len, glistptr)) break; // if its not host line and its not ip_line break
															 // out of ip-while.
				len=LINE_BUFFER_LENGTH;
				cont = get_line(buf, &len);
			}
		}
		else {len=LINE_BUFFER_LENGTH; cont = get_line(buf, &len);}
	} // end of while;
	DisplayGameList(*glistptr);
	return noErr;
}

local OSErr ContactMattTracker(void)
{
OSErr err;

#ifdef USE_INPUT_FILE
	input_fp = fopen(MattTestFile, "r");
	if (!input_fp) {
		return fnOpnErr;
	}
#else
	// Create a TCP stream
	err = OpenStream ();
	if (err != noErr) {
		printError("OpenStream", err);
		return err;
	}
	
	// debug("After open stream"); 
	memcheck();
	// Open a Connection
	err = OpenActiveConnection(BoloTrackerMachine, BoloTrackerIP, BoloTrackerPort);
	if (err != noErr) {
		CloseStream();
		printError("OpenConnection", err);
		return err;
	}
	// debug("After open connection"); 
	memcheck();
#endif
	// Let the parser drive the receiving routine
	// as need be.
	err = MattParser(&gamelist);

#ifdef USE_INPUT_FILE
	fclose(input_fp);
#else
	if (err != noErr) {
		CloseMyConnection();
		CloseStream();
		printError("parser", err);
		return err;
	}
	
	// Close the connection
	err = CloseMyConnection();
	if (err != noErr) {
		CloseStream();
		printError("CloseMyConnection", err);
		return err;
	}
	
	// Close stream
	err = CloseStream();
	if (err != noErr) {
		printError("CloseStream", err);
		return err;
	}		
	
	// Should never Close the TCP driver
	// err = CloseDriver(cb.ioCRefNum);
	// printError("CloseDriver", err);
#endif

	return noErr;
}
