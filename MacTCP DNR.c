// ©1996 Stuart Cheshire <cheshire@CS.Stanford.EDU>

#include <MacTCPCommonTypes.h>
#include <AddressXlation.h>
#include <Traps.h>
#include <GestaltEqu.h>
#include <Folders.h>

#define OPENRESOLVER	(1L)
#define CLOSERESOLVER	(2L)
#define STRTOADDR		(3L)
#define	ADDRTOSTR		(4L)
#define	ENUMCACHE		(5L)
#define ADDRTONAME		(6L)
#define	HINFO			(7L)
#define MXINFO			(8L)

#define local static
#define export

typedef OSErr RESOLVER(long command, ...);
#include "my_types.h"
#include "BoloWeb.h"

RESOLVER *dnr = 0L;
Handle codeHndl = 0L;

local Boolean TrapAvailable(unsigned long trap)
	{
	TrapType tType = (trap & 0x800 ? ToolTrap : OSTrap);
	if (trap & 0x800)				// if it is a ToolBox Trap
		{
		unsigned long n = 0x400;	// number of toolbox traps
		if (NGetTrapAddress(_InitGraf, ToolTrap) == NGetTrapAddress(0xAA6E, ToolTrap))
			n = 0x200;
		if ((trap &= 0x7FF) >= n) trap = _Unimplemented;
		}
	return(NGetTrapAddress(trap, tType) != NGetTrapAddress(_Unimplemented, ToolTrap));
	}

local short GetCPanelFolder(void)
	{
	long feature;
	if (!TrapAvailable(_Gestalt) || Gestalt(gestaltFindFolderAttr, &feature) != noErr)
		{ SysEnvRec info; SysEnvirons(1, &info); return(info.sysVRefNum); }
	else
		{
		short wdRef, vRefNum = 0;
		long dirID = 0;
		if (FindFolder(kOnSystemDisk, kControlPanelFolderType, kDontCreateFolder,
			&vRefNum, &dirID) != noErr) return(0);
		if (OpenWD(vRefNum, dirID, 'dnrp', &wdRef) == noErr) return(wdRef); else return(0);
		}
	}

local short OpenOurRF(short *wdRef)	// find MacTCP in Sys Folder and open it
	{
	HParamBlockRec fi;
	Str255 filename;
	
	*wdRef = GetCPanelFolder();
	
	fi.fileParam.ioCompletion = 0L;
	fi.fileParam.ioNamePtr    = (StringPtr)&filename;
	fi.fileParam.ioVRefNum    = *wdRef;
	fi.fileParam.ioDirID      = 0;
	fi.fileParam.ioFDirIndex  = 1;
	
	while (PBHGetFInfo(&fi, false) == noErr)
		{
		if (fi.fileParam.ioFlFndrInfo.fdType    == 'cdev' &&
			fi.fileParam.ioFlFndrInfo.fdCreator == 'mtcp')
				return(OpenRFPerm(filename, *wdRef, fsRdPerm));
				
		if (fi.fileParam.ioFlFndrInfo.fdType    == 'cdev' &&
			fi.fileParam.ioFlFndrInfo.fdCreator == 'ztcp')
				return(OpenRFPerm(filename, *wdRef, fsRdPerm));
				
		fi.fileParam.ioFDirIndex++;
		fi.fileParam.ioDirID = 0;
		}
	return(-1);
	}

export OSErr OpenResolver(char *fileName)
	{
	short refnum, wdRef;
	OSErr retcode;
	long feature = 0;
	
	// debug("Entering OpenResolver"); 
	memcheck();
	if (dnr != 0L) return(noErr);				// resolver already loaded in
	
	// debug("Trying OpenOurRF");
	refnum = OpenOurRF(&wdRef);					// ignore failures since may be in System file
	// debug("After OpenOurRF");
	memcheck();
	codeHndl = GetIndResource('dnrp', 1);		// get first dnrp resource -- the resolver
	if (codeHndl == 0L) return(ResError()); 	// can't open DNR
	// debug("After GetIndResource");
	memcheck();
	DetachResource(codeHndl);					// detatch resource before closing resource file
	// debug("After DetachResource");
	memcheck();
	if (refnum != -1)							// if we opened res file, close it now
		{
		SysEnvRec info;
		SysEnvirons(1, &info);
		if (wdRef != info.sysVRefNum) CloseWD(wdRef);	// don't kill System Folder
		CloseResFile(refnum);
		}
	HLock(codeHndl);							// lock handle so it does not move
	dnr = (RESOLVER *) *codeHndl;				// and dereference it

	retcode = dnr(OPENRESOLVER, fileName);	// call open resolver
	// debug("After dnr(OPENRESOLVER, fileName)");
	memcheck();
	if (retcode != noErr) { HUnlock(codeHndl); DisposHandle(codeHndl); dnr = 0L; }

	// check if the extended time manager is available
	if (TrapAvailable(_Gestalt) && Gestalt(gestaltTimeMgrVersion, &feature) == noErr)
		retcode = (feature != gestaltExtendedTimeMgr && feature != gestaltRevisedTimeMgr);
		
	// debug("Leaving OpenResolver");
	memcheck();
	return(retcode);
	}

export OSErr CloseResolver(void)
	{
	if (dnr == 0L) return(notOpenErr);		// resolver not loaded error
	dnr(CLOSERESOLVER);
	HUnlock(codeHndl);
	DisposHandle(codeHndl);
	dnr = 0L;
	return(noErr);
	}

export OSErr StrToAddr(char *hostName, struct hostInfo *rtnStruct,
									ResultProcPtr resultproc, char *userDataPtr)
	{
	if (dnr == 0L) return(notOpenErr);		// resolver not loaded error
	return(dnr(STRTOADDR, hostName, rtnStruct, resultproc, userDataPtr));
	}
	
export OSErr AddrToStr(unsigned long addr, char *addrStr)
	{
	if (dnr == 0L) return(notOpenErr);		// resolver not loaded error
	dnr(ADDRTOSTR, addr, addrStr);
	return(noErr);
	}
	
export OSErr EnumCache(EnumResultProcPtr resultproc, char *userDataPtr)
	{
	if (dnr == 0L) return(notOpenErr);		// resolver not loaded error
	return(dnr(ENUMCACHE, resultproc, userDataPtr));
	}
	
	
export OSErr AddrToName(unsigned long addr, struct hostInfo *rtnStruct,
									ResultProcPtr resultproc, char *userDataPtr)
	{
	if (dnr == 0L) return(notOpenErr);		// resolver not loaded error
	return(dnr(ADDRTONAME, addr, rtnStruct, resultproc, userDataPtr));
	}
