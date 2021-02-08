//
// BoloWeb v0.8
// ©1996 Pavani Diwanji <pavani@CS.Stanford.EDU>
//
// BoloWeb is freeware. Providing that you acknowledge my authorship, 
// you may distribute it any way you like, or do anything else with it, 
// including producing derivative works from the source code. 
//

#include "my_types.h"
#include "html.h"

export const char HTTPHeader[] = "HTTP/1.0 200 OK\r\nServer: MacHTTP\r\nMIME-Version: 1.0\r\nContent-type: text/html\r\n\r\n";
// Web page title.
export const char PageHeader[] =
	"<HTML>\r\n<HEAD>\r\n<TITLE>BoloWeb</TITLE>\r\n</HEAD>\r\n<BODY>\r\n"
	"<img alt=\"Bolo Title\" src=\"images/BoloWeb.gif\" width=208 height=104 align=left>\r\n"
	"<img alt=\"Double Pill Take\" src=\"images/battle.gif\" width=200 height=104 align=right>\r\n"
	"<br clear=left><br clear=right>\r\n"
	"<h1><a href=\"http://sai.Stanford.EDU/BoloWeb.html\">\r\nBoloWeb</a></h1>\r\n";
// Web page trailer
export const char PageTrailer[] = 
	"<p align=center>"
	"<img src=\"http://www-cs-students.stanford.edu/cgi-bin/count.cgi?df=BoloWeb.dat\">\r\n"
	"<p><hr>"
	"<ADDRESS> \r\n<b> Bolo Author:</b>\r\n"
	"<a href=\"http://ResComp.Stanford.EDU/~cheshire/bio.html\">\r\nStuart Cheshire</a>"
	"<p>\r\nThe <b>Bolo Web</b> by:\r\n"
	"<a href=\"http://sai.Stanford.EDU/pavani-bio.html\">\r\nPavani Diwanji </a>"
	"<p>\r\n Bolo games information provided by <b>Matt Slot's</b> and <b>Mike Ellis's</b> Bolo Trackers\r\n"
	"<p>\r\n Last update received from the Bolo Trackers on %#s at %#s (Pacific)\r\n"
	"<p>\r\n<b> Check out: </b>"
	"<a href=\"http://bolo.ncsa.uiuc.edu/\">\r\nBolo Home Page </a>"
	"<p>\r\n</ADDRESS>\r\n</BODY>\r\n\r\n</HTML>";
	
export int PageTrailerSize;

// Under any error circumstance return the following.
export const char Errorhtml[] =
	"HTTP/1.0 200 OK\r\nServer: MacHTTP\r\nMIME-Version: 1.0\r\nContent-type: text/html\r\n\r\n"
	"<HTML>\r\n<HEAD>\r\n<TITLE>BoloWeb</TITLE>\r\n</HEAD>\r\n<BODY>\r\n"
	"<img alt=\"Bolo Title\" src=\"images/BoloWeb.gif\" width=208 height=104 align=left>\r\n"
	"<img alt=\"Double Pill Take\" src=\"images/battle.gif\" width=200 height=104 align=right>\r\n"
	"<h1><a href=\"http://sai.Stanford.EDU/BoloWeb.html\">\r\nBoloWeb</a></h1>\r\n"
	"<HR>"
//	"<img alt=\"Oops\" src=\"images/sorry.gif\">\r\n"
	"<b> BoloWeb failed to get or generate bolo games information - please try again later</b>"
	"<HR>"
	"<ADDRESS> \r\n<b> Bolo Author:</b>\r\n"
	"<a href=\"http://ResComp.Stanford.EDU/~cheshire/bio.html\">\r\nStuart Cheshire</a>"
	"<p>\r\nThe <b>Bolo Web</b> by:\r\n"
	"<a href=\"http://sai.Stanford.EDU/pavani-bio.html\">\r\nPavani Diwanji </a>"
	"<p>\r\n Bolo games information provided by <b>Matt Slot's</b> and <b>Mike Ellis's</b> Bolo Trackers\r\n"
	"<p>\r\n<b> Check out: </b>"
	"<a href=\"http://ResComp.Stanford.EDU/~cheshire/Bolo.html\">\r\nBolo Web Pages</a>"
	"<p> \r\n</ADDRESS>\r\n</BODY>\r\n\r\n</HTML>";