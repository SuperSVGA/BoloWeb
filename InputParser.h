//
// BoloWeb v0.8
// ©1996 Pavani Diwanji <pavani@CS.Stanford.EDU>
//
// BoloWeb is freeware. Providing that you acknowledge my authorship, 
// you may distribute it any way you like, or do anything else with it, 
// including producing derivative works from the source code. 
//

#define LINE_BUFFER_LENGTH 128
#define MAX_USER_BUFFER_LENGTH 8192
#define STREAM_BUFFER_LENGTH 8192
#define MAX_PLAYERS 16
#define WEBAWARE_BOLO_VERSION (99<<8)|5
#define MAP_SIZE 64
#define DEFAULT_PORT 50000

typedef struct
	{
	long port;
	char name[];
	} BWeb;

// Default tracker in case the resource is not specified.
#define MattBoloTrackerMachine "bolo.usu.edu"
// Old tracker was MattBoloTrackerMachine "noproblem.uchicago.edu"

// Default tracker port in case the resource is not specified
#define MattBoloTrackerPort 50000

#define MattTestFile "test"

// The following is the internal data structure used to represent
// information about all ongoing bolo games.
// Intermediate representation is used so that in future, we can parse the
// output of different/nearest tracking programs to construct this rep.

typedef struct GameInfoList GameInfoList;
struct GameInfoList{
	char map_name[MAP_SIZE];
	short num_players;
	short num_pillboxes;
	short num_bases;
	char game_type[64];
	Boolean hidden_mines; // Hidden mines allowed?
	Boolean robots;		  // Robots/cybogs allowed?
	short num_sites;
	char ip_addr[MAX_PLAYERS][LINE_BUFFER_LENGTH];
	long port[MAX_PLAYERS];
	char place[MAX_PLAYERS][LINE_BUFFER_LENGTH];
	char passwd[64];
	long vers;
	GameInfoList *next;
};

// The exported routines from InputParser are:
import	GameInfoList *GetGameList(void);
import	void DisposeGameList(GameInfoList *);
import	void DisplayGameList(GameInfoList *);
import	void DebugGameList(GameInfoList *);
import  OSErr InitGameList(void);