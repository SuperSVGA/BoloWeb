//
// BoloWeb v0.8
// ©1996 Pavani Diwanji <pavani@CS.Stanford.EDU>
//
// BoloWeb is freeware. Providing that you acknowledge my authorship, 
// you may distribute it any way you like, or do anything else with it, 
// including producing derivative works from the source code. 
//


// This application is the server application which receives input queries from
// the HTTP server, gets the most current bolo game list from the BoloTracker and 
// and hands back to HTTP server a bolo game list in HTML format.

// Note: This is a background application that gets launched when the first
// query is received from the Mosaic client.
// As per the rules of background applications, the following care is taken:
// - Its compeltely transperent with no ui. The only way to see it when running
// is to use the process manager / taskit.
// - Between events, the sleep/nap time is very high so it does not hog the 
// system resources.
// - Use of stack space is minimal - heap space is used mainly for queries.
// SIZE resource: canBackground and onlyBackground bits are set.


// the application  is divided up into two main files which contain group of routines:
// 			1. Stuart's utility routines that do name resolution
// 			2. Debbugging routines
// 			3. BoloTracker output parsing and building of intermediate rep game list.
// 			4. Routiens that do TCP connectivity to Bolo tracker
// 			5. Apple Event routines that receive the apple events from HTTP server
//			   and send a reply back.

#include <stdio.h>		// for vsprintf
#include <stdarg.h>		// for variable argument lists
#include <string.h>		// for strlen

#include "my_types.h"
#include "BoloWeb.h"
#include "InputParser.h"
#include "html.h"

Boolean quitting = FALSE;

// We go to the tracker only every n sec where n is the update interval
// here it is set to 1 minute.
#define UPDATE_INTERVAL 60 
#define MAX_HTML 65536

// Under normal circustances construct the html: 64k html buffer
local char *html;
local int html_index = 0;
	
unsigned long last_update=0;

#define kGetHTTPLEventClass 'WWW�'
#define kGetHTTPLEventID 'sdoc'

export void do_memcheck(void)
	{
	Ptr p = NewPtr(4);
	if (p) DisposePtr(p);
	}

// stuart's debug utility
export void debug(char *format, ...)
	{
	u_char buffer[256];
	va_list ptr;
	va_start(ptr,format);
	buffer[0] = vsprintf((char *)buffer+1, format, ptr);
	va_end(ptr);
	DebugStr(buffer);
	}


// Mustn't write more than 4K at a time
// index is index into the html buffer
local Boolean WriteHTML(int *index, const char *format, ...)
{
	Boolean result = true;
	va_list ptr;
	va_start(ptr,format);
	if (*index > MAX_HTML - PageTrailerSize - 4096) result = false; // Not enough space in the buffer
	else *index += vsprintf(&html[*index], format, ptr);
	va_end(ptr);
	
	return result;
}

local Boolean WriteTrailerHTML(int *index, const char *format, Str255 lut, Str255 lud)
{
int incr;
	
	if (((*index) + PageTrailerSize) > MAX_HTML)
		return false; // Not enough space in the buffer
	// Else put it in - here we use sprintf as lut and lud are pascal style strings
	if ((incr = sprintf(html+(*index), format, lut, lud)) != EOF)
		(*index) += incr;
	else return false;
		
	return true;
}

// Initalize the html buffer with constant header - so we do not have to
// do it on every call
local Boolean InitHTML()
{
	html_index = 0; 

	PageTrailerSize = strlen(PageTrailer) + 1 + 2*sizeof(Str255); 

	if (WriteHTML(&html_index, HTTPHeader) && WriteHTML(&html_index, PageHeader)) {
		return true;
	}
	return false;
}

// Write the trailer html

// Convert the game info list into html and put the resultant html into the buffer
local Boolean ConvertToHTML(GameInfoList *gil)
{
GameInfoList *curr = NULL;
Str255 last_update_date;
Str255 last_update_time;
short i=0;

	// HTML content skeleton for every game
	static const char GameHTML [] =
		"<h3><img alt=\"o\" src=\"images/MapIcon.gif\" width=32 height=16> %s \r\n"
		"(v 0.%ld.%ld %s) %s %s </h3> \r\n" 
		"<h4>\r\n"
		"<DD><img alt=\"o\" src=\"images/tankgrn.gif\" width=16 height=12> Players %d\r\n"
		"<DD><img alt=\"o\" src=\"images/Pillbox.gif\" width=16 height=16> PillBoxes %d\r\n"
	   	"<DD><img alt=\"o\" src=\"images/NeutBase.gif\" width=16 height=16> Neutral Bases %d\r\n"
		"<DD><img alt=\"o\" src=\"images/%s\" width=16 height=16> Hidden Mines %s\r\n"
		"<DD><img alt=\"o\" src=\"images/%s\" width=18 height=16> Robots %s\r\n";
				 
	static const char SitesStart [] = "";
	static const char JoinSiteHTML [] = " <DD><img alt=\"o\" src=\"images/Site.gif\" width=12 height=15> <a href=\"%s\">%s</a> (%s) \r\n";
	static const char SiteHTML [] = " <DD><img alt=\"o\" src=\"images/Site.gif\" width=12 height=15>  %s (%s) \r\n";
	static const char SitesEnd [] = "</h4>\r\n";
	static const char GameEnd[] = "<HR>";
	static const char Exclaim [] = "<img alt=\"o\" src=\"images/Exclamation.gif\" width=16 height=16>";
					 
	IUTimeString(last_update, TRUE, last_update_time);
	IUDateString(last_update, TRUE, last_update_date);
	
	// Intialize the buffer
	if (!InitHTML()) 
		{ debug("cannot initialize html"); return false; }
	
	// construct the HTML format for the Game List elements.
	// Use standard 64k html buffer - truncate the html buffer if the all the games 
	// cannot fit in

	// Put the html for all the games in the html buffer
	curr = gil; 
	while (curr) {
		char *mines = (curr->hidden_mines)?"Allowed":"Not Allowed";
		char *mines_icon = (curr->hidden_mines)?"HiddenMine.gif":"NoHiddenMine.gif";
		char *robots = (curr->robots)?"Allowed":"Not Allowed";
		char *robots_icon = (curr->robots)?"Brain.gif":"NoBrain.gif";
		const char *pwd = (curr->passwd[0]!=NULL)?Exclaim:"";
		int old_html_index = html_index;
		
		// Write in basic game information
		if (!WriteHTML(&html_index, GameHTML, 
			curr->map_name, (curr->vers)>>8, (curr->vers)&0xff, curr->game_type, pwd, curr->passwd,
			curr->num_players, curr->num_pillboxes, curr->num_bases, 
			mines_icon, mines, robots_icon, robots))
			{ html_index = old_html_index; break; }
			
		// if any sites listed
		if (curr->num_sites>0) {
			if (!WriteHTML(&html_index, SitesStart))
				{ html_index = old_html_index; break; }

			for (i=0; i<curr->num_sites; ++i) {
			
				if (curr->vers>=WEBAWARE_BOLO_VERSION) {
					if (!WriteHTML(&html_index, JoinSiteHTML, 
						curr->ip_addr[i], // curr->port[i], 
						curr->ip_addr[i], curr->place[i]))
						{ html_index = old_html_index; break; }
				} else {
					if (!WriteHTML(&html_index, SiteHTML, 
						curr->ip_addr[i], curr->place[i]))
						{ html_index = old_html_index; break; }
			    }
			}
			if (!WriteHTML(&html_index, SitesEnd))
				{ html_index = old_html_index; break; }
		}
		
		// put in games end marker
		if (!WriteHTML(&html_index, GameEnd))
			{ html_index = old_html_index; break; }

		curr = curr->next; 
	}
	
	if (!WriteTrailerHTML(&html_index, PageTrailer, last_update_date, last_update_time))
		{ debug("Cannot put the trailer in -error!!"); return false;}
	return true;
}

// ****** Code to cope with required Apple Events ********

local OSErr MoreParameters(const AppleEvent *e)
	{
	DescType returnedType;
	Size actualSize;
	OSErr err;
	
	//debug("in MoreParameters");
	memcheck();
	err = AEGetAttributePtr(e, keyMissedKeywordAttr, typeWildCard,
									&returnedType, NULL, 0, &actualSize);
	if (err != errAEDescNotFound) return (errAEParamMissed); else return(noErr);
	}


local pascal OSErr MyHandleOApp(const AppleEvent *e, AppleEvent *r, long Refcon)
	{
	OSErr err; 
	r;
	Refcon;

	//debug("in MyHandleOApp");
	memcheck();
	err = MoreParameters(e);
	return(err);
	}

local pascal OSErr MyHandleODoc(const AppleEvent *e, AppleEvent *r, long Refcon)
	{
	//FSSpec myFSS;
	AEDescList docList;
	OSErr myErr;
	//long index;
	//long itemsInList;
	//Size actualSize;
	//AEKeyword keywd;
	//DescType returnedType;
	r;
	Refcon;
	e;

	//debug("in MyHandleODoc");
	memcheck();
	myErr = AEGetParamDesc(e, keyDirectObject, typeAEList, &docList);
	if (myErr) return(myErr);

	myErr = MoreParameters(e);
	if (myErr) { AEDisposeDesc(&docList); return(myErr); }

	AEDisposeDesc(&docList);
	return(noErr);
	}

local pascal OSErr MyHandlePDoc(const AppleEvent *e, AppleEvent *r, long Refcon)
	{
	e;
	r;
	Refcon;
	//debug("in MyHandlePDoc");
	memcheck();
	return(errAENoUserInteraction);
	}

local pascal OSErr MyHandleQuit(const AppleEvent *e, AppleEvent *r, long Refcon)
	{
	OSErr err;
	r;
	Refcon;
	//debug("in MyHandleQuit");
	memcheck();
	err  = MoreParameters(e);
	if (err) return(err);
	quitting = TRUE;
	return(noErr);
	}

// This routine receives the Apple event URL, gets the most current Bolo List from
// the BoloTracker and presents it in HTML format.	
local pascal OSErr MyHandleHTTP(const AppleEvent *event, AppleEvent *reply, long Refcon)
	{
	OSErr err = noErr;
	unsigned long current_time;
	GameInfoList *new_gamelist = NULL;
	
	event;
	Refcon;

	// make sure we do not contact the bolo tracker on every request.
	// Only every UPDATE_INTERVAL sec we send out for update...
	//debug("Starting MyHandleHTTP");
	memcheck();
	
	GetDateTime(&current_time);
	if (current_time-last_update>=UPDATE_INTERVAL){
		
		//debug("Before GetGameList");
		memcheck();
			// get a new game list
		new_gamelist = GetGameList();
		
		//debug("After building game list");
		memcheck();
		
		// If there is an error contacting the bolo tracker 
		// display the old html with corresponding timestamp.
		// If we do manage to get a new game info list,
		// then construct the new html and dispose the game list, once we have
		// html output
		if (new_gamelist!=NULL) {
			last_update = current_time;
			
			// Put the new games information in HTML format
			ConvertToHTML(new_gamelist);
			//debug("After converting new game list");
			memcheck();
			
			if (new_gamelist) {
				DisposeGameList(new_gamelist); new_gamelist = NULL;
				//debug("After disposing new game list");
				memcheck();
			}
		}
	}

	// Give the resultant html back to HTTP server.
	if (!html_index) err = AEPutParamPtr(reply, keyDirectObject, typeChar, Errorhtml, strlen(Errorhtml));
	else err = AEPutParamPtr(reply, keyDirectObject, typeChar, html, html_index);

	//debug("After returning new html reply");
	memcheck();
	if (err != noErr) return err;

	return(noErr);
}
	

 void main(void)
{
	EventRecord e;
	GameInfoList *new_gamelist = NULL;
	
	//debug("Starting up");
	//debug("Starting up; hs; g;");
	
	// Initialize the Quickdraw globals - needed by Apple Events Manager
	// Really?
	InitGraf(&qd.thePort);
	
	SetApplLimit(GetApplLimit() - 0x8000);		// Add 32K to the stack
	MaxApplZone();
	// debug("After MaxApplZone"); 
	memcheck();
	
	MoreMasters();
	// debug("After MoreMasters"); 
	memcheck();
		
	// Flush any events already in queue
	FlushEvents(everyEvent, 0);
	
	// printf("Starting up\n");
	AEInstallEventHandler(kCoreEventClass,   kAEOpenApplication, MyHandleOApp, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass,   kAEOpenDocuments,   MyHandleODoc, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass,   kAEPrintDocuments,  MyHandlePDoc, 0, FALSE);
	AEInstallEventHandler(kCoreEventClass,   kAEQuitApplication, MyHandleQuit, 0, FALSE);

	AEInstallEventHandler(kGetHTTPLEventClass, kGetHTTPLEventID, MyHandleHTTP, 0, FALSE);

	// debug("After AEInstallEventHandler"); 
	memcheck();
	
	html = NewPtr(MAX_HTML);
	if (!html) { debug("failed to initialize - out of memory?"); return; }
	// debug("After NewPtr"); 
	memcheck();

	// Initalize the last_update with current time - UPDATE_INTERVAL
	GetDateTime(&last_update); last_update = last_update - UPDATE_INTERVAL;
	// debug("After GetDateTime"); 
	memcheck();

	// Initalise the input parser
	if (InitGameList() != noErr) { debug("failed to initialize - out of memory?"); return; }
	
	// debug("Entering event loop"); 
	memcheck();
	
	// Loop around and process all the events that come by until the application 
	// received a quit event.
	do	
	{
		// Loop until there is an event
		// the large parameter 0xffffffff tells the system I do not need to receive
		// the null events
		if (WaitNextEvent(everyEvent, &e, 1000, NULL))
			{
			switch (e.what)
				{
				case mouseDown: case mouseUp: break;
				case keyDown: case keyUp: case autoKey: break;
				case updateEvt: BeginUpdate((WindowPtr)e.message); EndUpdate((WindowPtr)e.message); break;
				case activateEvt: break;
				case osEvt: break;	// Ignore; may be suspend, resume, convert clip, etc.
				case kHighLevelEvent: AEProcessAppleEvent(&e); break;
				default: debug("Got unexpected event %d", e.what); break;
				}
			}
	} while (!quitting);
	
	return;
}
