/* Decompiler and interpreter for AdventureScape systems
   (C) David Lodge 20th November 2005.

   This is a very quick and dirty code (I know the way I did exits is
   a bit bad) but it runs /Murder at the Abbey/ and /Dungeon Adventure/

   CLI is:
      ascape <datafile> [dump]

   If you specify 'dump' it will dump the database. Otherwise it will interpret
   the game.

   I've tried to remain true to the original program; I added one extra command
   FLAGS as a debugging measure.

   Permission is granted to mangle, alter and use as long as the copyright line
   is left intact */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXIT_NORTH 1
#define EXIT_EAST 2
#define EXIT_WEST 3
#define EXIT_SOUTH 4
#define EXIT_UP 5
#define EXIT_DOWN 6

#define WORD_VERB 1
#define WORD_NOUN 2

#define parseexit(x) (x==1)?"North":(x==2)?"East":(x==3)?"West":(x==4)?"South":(x==5)?"Up":"Down"

typedef struct {
   char name[255];
   char noun[255];
   char description[255];
   int location;
   int score;
   int gettable;
   int wearable;
   int carried;
   int worn;
} objecttype;

typedef struct {
   int condition[10];
   int action[10];
} puzzletype;

typedef struct {
   int room;
   int exit;
   int message;
} blockedexittype;

typedef struct {
   int state;
   int score;
} flagtype;

typedef struct {
   char description[255];
   int n, e, w, s, u, d;
} roomtype;

typedef struct {
   int maxcarry;
   int currentroom;
   int numcarry;
   int moved;
   int won;
   int currverb;
   int currnoun;
   int dead;
} playertype;

typedef struct {
   char name[255];
   int verbs;
   int objects;
   int maxobjects;
   int puzzles;
   int maxpuzzles;
   int flags;
   int blockedexits;
   int rooms;
   int maxphrases;
   int maxscore;
   int messages;
   int winscore;
   int startmessage;
} headertype;

objecttype object[255];
puzzletype puzzle[255];
blockedexittype blockedexit[255];
flagtype flag[255];
roomtype room[255];
playertype player;
headertype header;
char verb[255][20];
char message[255][255];
int numscoreobj=0, numscoreflags=0;
char locationfile[255], messagefile[255];

int read_int_data(FILE *in)
{
   int byte=0, scrap=0;

   scrap=fgetc(in);
   if (scrap != 0x40) { fprintf(stdout,"Cannot find an int type when expecting one!\n"); return -1; }

   byte=fgetc(in)<<24;
   byte+=fgetc(in)<<16;
   byte+=fgetc(in)<<8;
   byte+=fgetc(in);
   return byte;
}

char *read_char_data(FILE *in, char *result)
{
   char strscrap[255];
   int scrap, length, i;
   memcpy(strscrap,"\0",255);

   scrap=fgetc(in);
   if (scrap != 0x00) { fprintf(stdout,"Cannot find a char type when expecting one! %x\n",ftell(in)); return ""; }

   length=fgetc(in);
   if (length == 0)
   {
      result[0]='\0';
      return result;
   }
   fread(strscrap, length, 1, in);

   for (i=0; i<length; i++)
   {
      result[i]=strscrap[length-i-1];
   }
   result[length]='\0';
   return result;
}

int loaddatabase(char *filename, char *initfile)
{
   FILE *infile;
   int scrap, retval, version=1;

   // First thing - is this version 2 or version 1?
   infile=fopen(filename,"rb");
   if (infile == NULL)
   {
      fprintf(stdout, "Could not open data file %s\n", filename);
      return -1;
   }

   fseek(infile,0x28,SEEK_SET);
   scrap=fgetc(infile);
   if (scrap == 0x40) version=2;

   fseek(infile,0,SEEK_SET);
   scrap=fgetc(infile);
   if (scrap == 0x0) version=1;

   fclose(infile);

   if (version == 2)
   {
      printf("Database version 2\n");
      retval=loadv2database(filename, initfile);
   }
   else
   {
      printf("Database version 1\n");
      retval=loadv1database(filename, initfile);
   }
}

int loadv2database(char *filename, char *initfile)
{
   FILE *infile, *locfileptr, *messfileptr, *initptr;
   int i, j, scrap;
   int objscore, flagscore;
   int startptr[5];
   int inc[4];

   infile=fopen(filename,"rb");
   initptr=fopen(initfile,"rb");

   // read the header
   header.verbs = read_int_data(infile);
   scrap = read_int_data(infile);
   header.objects = read_int_data(infile);
   header.maxobjects = read_int_data(infile);
   header.puzzles = read_int_data(infile);
   header.maxpuzzles = read_int_data(infile);
   header.flags = read_int_data(infile);
   header.blockedexits = read_int_data(infile);
   scrap = read_int_data(infile);

   strcpy(locationfile, read_char_data(infile, locationfile));
   strcpy(messagefile, read_char_data(infile, messagefile));

   locfileptr=fopen(locationfile,"rb");
   if (locfileptr == NULL)
   {
      printf("Could not open location file %s\n", locationfile);
   }
   scrap = read_int_data(locfileptr);
   header.rooms = read_int_data(locfileptr);

   messfileptr=fopen(messagefile,"rb");
   if (messfileptr == NULL)
   {
      printf("Could not open message file %s\n", messagefile);
   }
   scrap = read_int_data(messfileptr);
   header.messages = read_int_data(messfileptr);

   printf("Header done\n");
   for (i=0; i<5; i++)
   {
      startptr[i]=read_int_data(infile);
   }
   for (i=0; i<4; i++)
   {
      inc[i]=read_int_data(infile);
   }
   header.maxphrases=read_int_data(infile);
   printf("Messages done; objects: %x\n", startptr[0]);

   // Now read objects
   for (i=1; i<=header.objects; i++)
   {
      int baseoffset = startptr[0]+((i-1)*inc[0]);
  
      fseek(infile, baseoffset, SEEK_SET);
      strcpy(object[i].name,read_char_data(infile, object[i].name));
      printf("Name: %s %d\n",object[i].name,header.maxphrases); 
      fseek(infile, baseoffset+header.maxphrases+2, SEEK_SET);
      strcpy(object[i].noun,read_char_data(infile, object[i].noun));
      printf("Noun: %s\n",object[i].noun);
      fseek(infile, baseoffset+header.maxphrases+7, SEEK_SET);
      strcpy(object[i].description,read_char_data(infile, object[i].description));

   }
   printf("Objects done\n");

   // Now read verbs
   for (i=1; i<=header.verbs; i++)
   {
      int baseoffset = startptr[1]+((i-1)*inc[1]);
      fseek(infile,baseoffset, SEEK_SET);
      strcpy(verb[i], read_char_data(infile, verb[i]));
   }
   printf("Verbs done\n");

   // Read rooms now, so we can use the room name later + exits are overwritten by state
   for (i=1; i<=header.rooms; i++)
   {
      fseek(locfileptr, 0x0a+((i-1)*0x10e), SEEK_SET);
      room[i].n=read_int_data(locfileptr);
      room[i].e=read_int_data(locfileptr);
      room[i].w=read_int_data(locfileptr);
      room[i].s=read_int_data(locfileptr);
      room[i].u=read_int_data(locfileptr);
      room[i].d=read_int_data(locfileptr);
      strcpy(room[i].description,read_char_data(locfileptr, room[i].description));
   }
   fclose(locfileptr);

   // Read messages now - so we can get it over with!
   for (i=1; i<=header.messages; i++)
   {
      fseek(messfileptr, 0x0a+(240*(i-1)), SEEK_SET);
      strcpy(message[i],read_char_data(messfileptr,message[i]));
   }
   fclose(messfileptr);

   // Puzzles - this is really screwed up!
   for (i=1; i<=header.puzzles; i++)
   {
      int baseoffset = startptr[2]-header.maxpuzzles;
      int newoffset;
      int n, dummy;

      fseek(infile, baseoffset, SEEK_SET);
      n=0;
      dummy=0;
      while (dummy != i)
      {
         n++;
         dummy=fgetc(infile);
      }
      newoffset=startptr[2]+((n-1)*inc[2])+10;
      fseek(infile, newoffset, SEEK_SET);
      for (j=0; j<10; j++)
      {
         puzzle[i].condition[j]=fgetc(infile);
      }
      for (j=0; j<10; j++)
      {
         puzzle[i].action[j]=fgetc(infile);
      }
   }

   // Blocked exits
   fseek(infile, startptr[3], SEEK_SET);
   for (i=1; i<=header.blockedexits; i++)
   {
      blockedexit[i].room=fgetc(infile);
      blockedexit[i].exit=fgetc(infile);
      blockedexit[i].message=fgetc(infile);
   }

   // Scoring conditions
   fseek(infile, startptr[4], SEEK_SET);
   objscore=fgetc(infile);
   flagscore=fgetc(infile);
   header.winscore=fgetc(infile);
   for (i=1; i<=header.maxobjects; i++)
   {
      scrap=fgetc(infile);
      if (scrap>0)
      {
         numscoreobj++;
         object[i].score=objscore;
      }
   }
   for (i=1; i<=header.flags; i++)
   {
      scrap=fgetc(infile);
      if (scrap>0)
      {
         numscoreflags++;
         flag[i].score=flagscore;
      }
   }
   header.maxscore=(numscoreobj*objscore)+(numscoreflags*flagscore)+header.winscore;

   // Close main datafile
   fclose(infile);

   // Load up the initial state
   strcpy(header.name,read_char_data(initptr,header.name));
   header.startmessage=read_int_data(initptr);
   player.maxcarry=read_int_data(initptr);
   player.currentroom=read_int_data(initptr);
   player.numcarry=read_int_data(initptr);

   for (i=1; i<=header.flags; i++)
   {
      flag[i].state=fgetc(initptr);
   }
   for (i=1; i<=header.objects; i++)
   {
      object[i].location=fgetc(initptr);
      scrap=fgetc(initptr);
      switch (scrap)
      {
         case 0: object[i].gettable=1; object[i].wearable=0; object[i].carried=0; object[i].worn=0; break;
         case 1: object[i].gettable=1; object[i].wearable=1; object[i].carried=0; object[i].worn=0; break;
         case 2: object[i].gettable=0; object[i].wearable=0; object[i].carried=0; object[i].worn=0; break;
         case 3:
         case 5: object[i].gettable=1; object[i].wearable=0; object[i].carried=1; object[i].worn=0; break;
         case 4: object[i].gettable=1; object[i].wearable=1; object[i].carried=1; object[i].worn=1; break;
      }
   }
   for (i=1; i<=header.rooms; i++)
   {
      room[i].n=fgetc(initptr);
      room[i].e=fgetc(initptr);
      room[i].w=fgetc(initptr);
      room[i].s=fgetc(initptr);
      room[i].u=fgetc(initptr);
      room[i].d=fgetc(initptr);
   }

   return 0;
}

int loadv1database(char *filename, char *initfile)
{
   FILE *infile, *initptr, *messptr, *locptr, *messptrptr, *locptrptr;
   int i, j, scrap;
   int objscore, flagscore;
   int disc=0;
   char messagefile[255], locationfile[255];
   char mangle[255];

   infile=fopen(filename,"rb");
   initptr=fopen(initfile,"rb");

   // Check whether it the messages/locs are read from disc
   scrap=fgetc(infile);
   if (scrap == 0)
   {
      disc=1;
      rewind(infile);
      strcpy(locationfile, read_char_data(infile, locationfile));
      strcpy(messagefile, read_char_data(infile, messagefile));
      messptr=fopen(messagefile,"rb");
      locptr=fopen(locationfile,"rb");
      strcpy(mangle,"I.");
      strcat(mangle,messagefile);
      messptrptr=fopen(mangle,"rb");
      strcpy(mangle,"I.");
      strcat(mangle,locationfile);
      locptrptr=fopen(mangle,"rb");
   }
   else
   {
      rewind(infile);
   }

   // read the header
   header.rooms = read_int_data(infile);
   player.currentroom = read_int_data(infile);
   header.verbs = read_int_data(infile);
   header.objects = read_int_data(infile);
   player.maxcarry = read_int_data(infile);
   header.flags = read_int_data(infile);
   header.messages = read_int_data(infile);
   header.puzzles = read_int_data(infile);

   if (!disc)
   {
      // Now read objects
      for (i=1; i<=header.objects; i++)
      {
         object[i].location=fgetc(infile);
      }

      for (i=1; i<=header.objects; i++)
      {
         //object[i].state=fgetc(infile);
         scrap=fgetc(infile);
      }
   }

   numscoreobj=read_int_data(infile);
   objscore=read_int_data(infile);
   if (numscoreobj > 0)
   {
      for (i=1; i<=numscoreobj; i++)
      {
         scrap=fgetc(infile);
         if (scrap>0)
         {
            object[scrap].score=objscore;
         }
      }
   }
   numscoreflags=read_int_data(infile);
   flagscore=read_int_data(infile);
   if (numscoreflags > 0)
   {
      for (i=1; i<=numscoreflags; i++)
      {
         scrap=fgetc(infile);
         if (scrap>0)
         {
            flag[scrap].score=flagscore;
         }
      }
   }
   header.winscore=fgetc(infile);
   if (header.winscore == 0x40)
   {
      // It uses an integer rather than a byte for the winning score
      fseek(infile, -1, SEEK_CUR);
      header.winscore=read_int_data(infile);
   }
   header.maxscore=(numscoreobj*objscore)+(numscoreflags*flagscore)+header.winscore;

   // Now read verbs
   for (i=1; i<=header.verbs; i++)
   {
      strcpy(verb[i], read_char_data(infile, verb[i]));
   }

   for (i=1; i<=header.objects; i++)
   {
      strcpy(object[i].name, read_char_data(infile,object[i].name));
   }

   for (i=1; i<=header.objects; i++)
   {
      strcpy(object[i].noun, read_char_data(infile,object[i].noun));
   }

   if (disc)
   {
      strcpy(header.name,read_char_data(infile,header.name));
      strcpy(message[0],read_char_data(infile,message[0]));
      header.startmessage=0;
   }

   // Read rooms now, so we can use the room name later + exits are overwritten by state
   for (i=1; i<=header.rooms; i++)
   {
      if (disc)
      {
         scrap=(i-1)*5;
         fseek(locptrptr, scrap, SEEK_SET);
         scrap=read_int_data(locptrptr);
         fseek(locptr, scrap, SEEK_SET);
         strcpy(room[i].description,read_char_data(locptr, room[i].description));
      }
      else
      {
         strcpy(room[i].description,read_char_data(infile, room[i].description));
         room[i].n=fgetc(infile);
         room[i].e=fgetc(infile);
         room[i].w=fgetc(infile);
         room[i].s=fgetc(infile);
         room[i].u=fgetc(infile);
         room[i].d=fgetc(infile);
      }
   }

   if (!disc) strcpy(header.name,read_char_data(infile,header.name));

   // Read messages now - so we can get it over with!
   for (i=0; i<=header.messages; i++)
   {
      if (disc)
      {
         if (i<header.messages)
         {
            scrap=i*5;
            fseek(messptrptr, scrap, SEEK_SET);
            scrap=read_int_data(messptrptr);
            fseek(messptr, scrap, SEEK_SET);
            strcpy(message[i+1],read_char_data(messptr,message[i+1]));
         }
      }
      else
      {
         strcpy(message[i],read_char_data(infile,message[i]));
      }
   }

   for (i=1; i<=header.objects; i++)
   {
      strcpy(object[i].description, read_char_data(infile,object[i].description));
   }
   // Puzzles - this is really screwed up!
   for (i=1; i<=header.puzzles; i++)
   {
      for (j=0; j<10; j++)
      {
         puzzle[i].condition[j]=fgetc(infile);
      }
      for (j=0; j<10; j++)
      {
         puzzle[i].action[j]=fgetc(infile);
      }
   }

   if (disc)
   {
      // Blocked exits
      for (i=1; i<=header.blockedexits; i++)
      {
         blockedexit[i].room=fgetc(infile);
         blockedexit[i].exit=fgetc(infile);
         blockedexit[i].message=fgetc(infile);
      }
   }

   // Close main datafile
   fclose(infile);

   // Load up the initial state
   player.currentroom=read_int_data(initptr);
   player.numcarry=read_int_data(initptr);

   for (i=1; i<=header.objects; i++)
   {
      object[i].location=fgetc(initptr);
   }
   for (i=1; i<=header.objects; i++)
   {
      scrap=fgetc(initptr);
      switch (scrap)
      {
         case 0: object[i].gettable=1; object[i].wearable=0; object[i].carried=0; object[i].worn=0; break;
         case 1: object[i].gettable=1; object[i].wearable=1; object[i].carried=0; object[i].worn=0; break;
         case 2: object[i].gettable=0; object[i].wearable=0; object[i].carried=0; object[i].worn=0; break;
         case 3:
         case 5: object[i].gettable=1; object[i].wearable=0; object[i].carried=1; object[i].worn=0; break;
         case 4: object[i].gettable=1; object[i].wearable=1; object[i].carried=1; object[i].worn=1; break;
      }
   }
   if (header.flags > 0)
   {
      for (i=1; i<=header.flags; i++)
      {
         flag[i].state=fgetc(initptr);
      }
   }
   else
   { // BBC Basic always seems to execute a loop even when the start is greater than the end!
      scrap=fgetc(initptr);
   }
   for (i=1; i<=header.rooms; i++)
   {
      room[i].n=fgetc(initptr);
      room[i].e=fgetc(initptr);
      room[i].w=fgetc(initptr);
      room[i].s=fgetc(initptr);
      room[i].u=fgetc(initptr);
      room[i].d=fgetc(initptr);
   }

   fclose(initptr);

   return 0;
}

void printexit(int roomnum, int roomexit, int exit)
{
   int i,blocked;
   char exitname[8];

   strcpy(exitname,parseexit(exit));

   if (roomexit > 0)
   {
      blocked=0;
      if (roomexit == 255)
      {
         for (i=1; i<=header.blockedexits; i++)
         {
            if (blockedexit[i].room == roomnum && blockedexit[i].exit == exit)
            {
               blocked=1;
               printf(" %s is a blocked exit with a message of ",exitname);
               printf("\"%s\"\n", message[blockedexit[i].message]);
            }
         }
      }
      if (blocked==0)
      {
         printf(" %s leads to %d \"%s\"\n",exitname,roomexit,room[roomexit].description);
      }
   }
}

void printdatabase(void)
{
   int i;
   // Prints the whole database

   // First off the header
   printf("Game: %s\n", header.name);
   printf("Numbers of objects:\n");
   printf(" Verbs: %d\n", header.verbs);
   printf(" Objects: %d\n", header.objects);
   printf(" Maximum Objects: %d\n", header.maxobjects);
   printf(" Puzzles: %d\n", header.puzzles);
   printf(" Maximum Puzzles: %d\n", header.maxpuzzles);
   printf(" Flags: %d\n", header.flags);
   printf(" Blocked Exits: %d\n", header.blockedexits);
   printf(" Rooms: %d\n", header.rooms);
   printf(" Maximum phrases: %d\n", header.maxphrases);
   printf(" Messages: %d\n", header.messages);
   printf("Maxmimum score: %d\n", header.maxscore);
   printf("Score for winning: %d\n", header.winscore);
   printf("Starting Message: %s\n", message[header.startmessage]);

   printf("\n");
   for (i=1; i<=header.rooms; i++)
   {
      printf("Room %d: %s\n", i, room[i].description);
      printexit(i, room[i].n, 1);
      printexit(i, room[i].e, 2);
      printexit(i, room[i].w, 3);
      printexit(i, room[i].s, 4);
      printexit(i, room[i].u, 5);
      printexit(i, room[i].d, 6);
   }

   printf("\n");
   for (i=1; i<=header.objects; i++)
   {
      printf("Object %d: %s\n", i, object[i].name);
      printf(" has a noun of %s\n", object[i].noun);
      printf(" a description of %s\n", object[i].description);
      printf(" and a score of %d\n", object[i].score);
      printf(" it starts in %s\n", room[object[i].location].description);
      printf(" it can be: ");
      if (object[i].gettable) printf("picked up ");
      if (object[i].wearable) printf("worn ");
      printf("\n");
   }

   printf("\n");
   for (i=1; i<=header.flags; i++)
   {
      if (flag[i].score > 0)
      {
         printf("Flag %d has a score of %d\n", i, flag[i].score);
      }
   }

   printf("\n");
   for (i=1; i<=header.verbs; i++)
   {
      printf("Verb %d: %s\n", i, verb[i]);
   }

   printf("\n");
   for (i=1; i<=header.messages; i++)
   {
      printf("Message %d: \"%s\"\n", i, message[i]);
   }

   printf("\n");
   for (i=1; i<=header.puzzles; i++)
   {
      printf("Puzzle %d\n",i);
      printf("IF {\n");
      if (puzzle[i].condition[0] != 0) printf(" PLAYERIN %s\n",room[puzzle[i].condition[0]].description);
      if (puzzle[i].condition[1] != 0) printf(" VERB %s\n",verb[puzzle[i].condition[1]]);
      if (puzzle[i].condition[2] != 0) printf(" OBJECT %s\n",object[puzzle[i].condition[2]].name);
      if (puzzle[i].condition[3] != 0) printf(" FLAG %d\n",puzzle[i].condition[3]);
      if (puzzle[i].condition[4] != 0) printf(" FLAGSET %d\n",puzzle[i].condition[4]);
      if (puzzle[i].condition[5] != 0) printf(" 2FLAG %d\n",puzzle[i].condition[5]);
      if (puzzle[i].condition[6] != 0) printf(" 2FLAGSET %d\n",puzzle[i].condition[6]);
      if (puzzle[i].condition[7] != 0) printf(" CARRYOBJ %s\n",object[puzzle[i].condition[7]].name);
      if (puzzle[i].condition[8] != 0) printf(" 2CARRYOBJ %s\n",object[puzzle[i].condition[8]].name);
      if (puzzle[i].condition[9] != 0) printf(" OBJWORN %s\n",object[puzzle[i].condition[9]].name);
      printf("}\nTHEN {\n");
      if (puzzle[i].action[0] != 0) printf(" SETFLAG %d\n",puzzle[i].action[0]);
      if (puzzle[i].action[1] != 0) printf(" PRINTMESS %s\n",message[puzzle[i].action[1]]);
      if (puzzle[i].action[2] != 0) printf(" ALTEREXIT %s\n",parseexit(puzzle[i].action[2]));
      if (puzzle[i].action[3] != 0) printf(" NEWLOC %s\n",room[puzzle[i].action[3]].description);
      if (puzzle[i].action[4] != 0) printf(" ALTEROBJ %s\n",object[puzzle[i].action[4]].name);
      if (puzzle[i].action[5] != 0)
      {
         switch(puzzle[i].action[5])
         {
            case 1:  printf(" NEWOBJ MAKE_GETTABLE\n"); break;
            case 2:  printf(" NEWOBJ DISAPPEAR\n"); break;
            case 3:  printf(" NEWOBJ MOVE_OBJECT\n"); break;
            case 4:  printf(" NEWOBJ MAKE_GET_WEAR\n"); break;
            case 5:  printf(" NEWOBJ APPEAR\n"); break;
            default: printf(" NEWOBJ UNKNOWN %d\n", puzzle[i].action[5]); break;
         }
      }
      if (puzzle[i].action[6] != 0) printf(" NEWOBJLOC %s\n",room[puzzle[i].action[6]].description);
      if (puzzle[i].action[7] != 0)
      {
         if (puzzle[i].action[7] == 255)
         {
            printf(" MOVEPLAYER %s\n",room[puzzle[i].action[8]].description);
         }
         else
         {
            printf(" 2ALTEROBJ %s\n",object[puzzle[i].action[7]].name);
         }
      }
      if (puzzle[i].action[8] != 0)
      {
         if (puzzle[i].action[7] != 255)
         {
            switch(puzzle[i].action[8])
            {
               case 1:  printf(" 2NEWOBJ MAKE_GETTABLE\n"); break;
               case 2:  printf(" 2NEWOBJ DISAPPEAR\n"); break;
               case 3:  printf(" 2NEWOBJ MOVE_OBJECT\n"); break;
               case 4:  printf(" 2NEWOBJ MAKE_GET_WEAR\n"); break;
               case 5:  printf(" 2NEWOBJ APPEAR\n"); break;
               default: printf(" 2NEWOBJ UNKNOWN %d\n", puzzle[i].action[8]); break;
            }
         }

      }
      if (puzzle[i].action[9] != 0) printf(" NEWOBJLOC %s\n",room[puzzle[i].action[9]].description);
      printf("}\n");
   }
}

void start(void)
{
   printf("%s\n\n", header.name);
   printf("%s\n\n", message[header.startmessage]);
   printf("For help type HELP.\n\n");
}

void displayroom(int roomnum)
{
   int i=0;

   printf("You are %s\n\n",room[roomnum].description);

   printf("Visible exits: ");
   if (room[roomnum].n) printf("north ");
   if (room[roomnum].e) printf("east ");
   if (room[roomnum].w) printf("west ");
   if (room[roomnum].s) printf("south ");
   if (room[roomnum].u) printf("up ");
   if (room[roomnum].d) printf("down ");
   printf("\n\n");

   for (i=1; i <= header.objects; i++)
   {
      if (object[i].location == roomnum)
      {
         printf("You see %s\n",object[i].name);
      }
   }
   printf("\n");
}

void getinput(char *verb, char *noun)
{
   char *space;
   char command[255];
   int i;

   memset(verb,'\0',255);
   memset(noun,'\0',255);
   do
   {
      printf("What now? ");
      fgets(command, 250, stdin);
   } while (strlen(command) < 2);

   for (i=0; i<strlen(command); i++)
   {
      command[i]=toupper(command[i]);
   }

   space=strchr(command, ' ');
   if (space == NULL)
   {
      strncpy(verb,command,strlen(command)-1);
   }
   else
   {
      strncpy(verb,command,space-command);
      strncpy(noun,space + 1,strlen(space + 1)-1);
   }
}

int checkcondition(int puzzlenum)
{
   if (puzzle[puzzlenum].condition[0]!=player.currentroom && puzzle[puzzlenum].condition[0] > 0) return 0;
   if (puzzle[puzzlenum].condition[1]!=player.currverb && puzzle[puzzlenum].condition[1] > 0) return 0;
   if (puzzle[puzzlenum].condition[2]!=player.currnoun && puzzle[puzzlenum].condition[2] > 0) return 0;
   if (puzzle[puzzlenum].condition[3] > 0 && flag[puzzle[puzzlenum].condition[3]].state != puzzle[puzzlenum].condition[4]) return 0;
   if (puzzle[puzzlenum].condition[5] > 0 && flag[puzzle[puzzlenum].condition[5]].state != puzzle[puzzlenum].condition[6]) return 0;
   if (puzzle[puzzlenum].condition[7] > 0 && !object[puzzle[puzzlenum].condition[7]].carried) return 0;
   if (puzzle[puzzlenum].condition[8] > 0 && !object[puzzle[puzzlenum].condition[8]].carried) return 0;
   if (puzzle[puzzlenum].condition[9] > 0 && !object[puzzle[puzzlenum].condition[9]].worn) return 0;
   return puzzlenum;
}

void performaction(int puzzlenum)
{
   if (puzzle[puzzlenum].action[0] == 255) player.dead = 1;
   else if (puzzle[puzzlenum].action[0] == 254) player.won = 1;
   else if (puzzle[puzzlenum].action[0] > 0)
   {
      flag[puzzle[puzzlenum].action[0]].state=!flag[puzzle[puzzlenum].action[0]].state;
   }
   if (puzzle[puzzlenum].action[1] > 0) printf("%s\n\n",message[puzzle[puzzlenum].action[1]]);
   switch(puzzle[puzzlenum].action[2])
   {
      case 1: room[player.currentroom].n=puzzle[puzzlenum].action[3]; break;
      case 2: room[player.currentroom].e=puzzle[puzzlenum].action[3]; break;
      case 3: room[player.currentroom].w=puzzle[puzzlenum].action[3]; break;
      case 4: room[player.currentroom].s=puzzle[puzzlenum].action[3]; break;
      case 5: room[player.currentroom].u=puzzle[puzzlenum].action[3]; break;
      case 6: room[player.currentroom].d=puzzle[puzzlenum].action[3]; break;
   }
   if (puzzle[puzzlenum].action[2] > 0) player.moved=1;
   if (puzzle[puzzlenum].action[4] > 0)
   {
      switch(puzzle[puzzlenum].action[5])
      {
         case 1: object[puzzle[puzzlenum].action[4]].gettable=1; break;
         case 2: if (object[puzzle[puzzlenum].action[4]].carried) player.numcarry--;
                 object[puzzle[puzzlenum].action[4]].carried=0; object[puzzle[puzzlenum].action[4]].location=0; break;
         case 3: object[puzzle[puzzlenum].action[4]].location=puzzle[puzzlenum].action[6]; break;
         case 4: object[puzzle[puzzlenum].action[4]].gettable=1; object[puzzle[puzzlenum].action[4]].wearable=1; break;
         case 5: object[puzzle[puzzlenum].action[4]].location=player.currentroom; break;
      }
   }
   if (puzzle[puzzlenum].action[7] == 255)
   {
      player.currentroom=puzzle[puzzlenum].action[8];
      player.moved=1;
   }
   else if (puzzle[puzzlenum].action[7] > 0)
   {
      switch(puzzle[puzzlenum].action[8])
      {
         case 1: object[puzzle[puzzlenum].action[7]].gettable=1; break;
         case 2: object[puzzle[puzzlenum].action[7]].carried=0; object[puzzle[puzzlenum].action[7]].location=0; break;
         case 3: object[puzzle[puzzlenum].action[7]].location=puzzle[puzzlenum].action[9]; break;
         case 4: object[puzzle[puzzlenum].action[7]].gettable=1; object[puzzle[puzzlenum].action[7]].wearable=1; break;
         case 5: object[puzzle[puzzlenum].action[7]].location=player.currentroom; break;
      }
   }
}

void checkpuzzles(int report)
{
   int i=1;
   int doaction=0;

   i=1;
   do
   {
      doaction=checkcondition(i);
      i++;
   } while (i <= header.puzzles && doaction==0);

   if (doaction > 0)
   {
      performaction(doaction);
   }
   else if (!report)
   {
      if (!player.moved) printf("You cannot do that.\n\n");
   }
}

void moveplayer(int newloc, int direction)
{
   int i;

   switch(newloc)
   {
      case 0:
         printf("You cannot move that way.\n\n");
         break;
      case 255:
         // It's a blocked exit
         for (i=1; i<header.blockedexits; i++)
         {
            if (blockedexit[i].room == player.currentroom && blockedexit[i].exit == direction)
            {
               printf("%s\n\n", message[blockedexit[i].message]);
            }
         }
         break;
      default:
         player.currentroom=newloc;
         player.moved=1;
         checkpuzzles(1);
         break;
   }
}

void showinventory(void)
{
   int i, wearing=0;

   if (player.numcarry == 0)
   {
      printf("You are carrying nothing.\n\n");
   }
   else
   {
      printf("You are carrying:\n");
      for (i=1; i<=header.objects; i++)
      {
         if (object[i].carried)
         {
            printf("   %s\n",object[i].name);
         }
      }
      for (i=1; i<=header.objects; i++)
      {
         if (object[i].worn)
         {
            wearing=1;
         }
      }
      if (wearing)
      {
         printf("You are wearing:\n");
         for (i=1; i<=header.objects; i++)
         {
            if (object[i].worn)
            {
               printf("   %s\n",object[i].name);
            }
         }
      }
   }
}

void unknownword(char *word, int type)
{
   if (type == WORD_VERB)
   {
      printf("I don't know how to %s.\n",word);
   }
   else
   {
      printf("I don't know what %s is.\n",word);
   }
}

void savegame(void)
{
}

void loadgame(void)
{
}

void showhelp(void)
{
   printf("I understand one or two word instructions such as CLIMB TREE or QUIT.");
   printf(" Some of the words I know are GET, DROP, INV (to list objects carried), LOOK, EXAMINE and QUIT.\n\n");
   printf("To move around you can type GO NORTH, GO DOWN etc or just N,E,W,S,U and D.");
   printf(" Some other commands can be abbreviated too, eg L for LOOK.\n\n");
   printf("SAVE and LOAD are used to save your game and reload it.");
   printf(" Other words you will have to discover for yourself.\n\n");
}

void examine(int noun)
{
   if (object[noun].worn || object[noun].carried || object[noun].location == player.currentroom)
   {
      if (strlen(object[noun].description) == 0)
      {
         printf("You see nothing special.\n\n");
      }
      else
      {
         printf("%s\n\n",object[noun].description);
      }
   }
   else
   {
      printf("You can't see %s.\n\n", object[noun].name);
   }
}

void wear(int noun)
{
   if (object[noun].carried && object[noun].wearable)
   {
      printf("You put on the %s.\n\n", object[noun].name);
      object[noun].worn=1;
   }
   else if (!object[noun].carried)
   {
      printf("You are not carrying %s.\n\n", object[noun].name);
   }
   else
   {
      printf("You can't wear that!\n\n");
   }
}

void get(int noun)
{
   if (object[noun].carried)
   {
      printf("You are already carrying %s.\n\n", object[noun].name);
   }
   else if (object[noun].location != player.currentroom)
   {
      printf("I can't see %s.\n\n", object[noun].name);
   }
   else if (player.numcarry == player.maxcarry)
   {
      printf("Your hands are full\n\n");
   }
   else if (!object[noun].gettable)
   {
      printf("You can't.\n\n");
   }
   else
   {
      printf("You get %s.\n\n",object[noun].name);
      object[noun].location=0;
      object[noun].carried=1;
      player.numcarry++;
   }
}

void drop(int noun)
{
   if (!object[noun].carried)
   {
      printf("You are not carrying %s.\n\n",object[noun].name);
   }
   else
   {
      printf("You drop %s.\n\n",object[noun].name);
      object[noun].location=player.currentroom;
      object[noun].carried=0;
      player.numcarry--;
   }
}

void score(void)
{
   int i,score=0;

   for (i=1; i<=header.objects; i++)
   {
      if (object[i].carried)
      {
         score+=object[i].score;
      }
   }
   for (i=1; i<=header.flags; i++)
   {
      if (flag[i].state)
      {
         score+=flag[i].score;
      }
   }
   if (player.won)
   {
      score+=header.winscore;
   }
   printf("You have score %d points out of %d.\n\n",score, header.maxscore);
}

int parse(char *currverb, char *noun)
{
   int retval=0,i,objnumber=0,verbnumber=0;
   char scrap[10], shortverb[5], shortnoun[5];
   memset(scrap,0,10);
   memset(shortverb,0,5);
   strncpy(shortverb,currverb,4);
   memset(shortnoun,0,5);
   strncpy(shortnoun,noun,3);
   player.currnoun=0;
   player.currverb=0;

   // if there's a noun, check that it exists
   for (i=1;i<=header.objects; i++)
   {
      if (!strcmp(shortnoun,object[i].noun))
      {
         objnumber=i;
      }
   }
   if (objnumber==0 && strlen(shortnoun)!=0)
   {
      unknownword(noun,WORD_NOUN);
      return retval;
   }
   player.currnoun=objnumber;

   // first check for builtins
   if (strlen(shortverb) == 1)
   {
      switch(shortverb[0])
      {
         case 'N':
            moveplayer(room[player.currentroom].n,1);
            break;
         case 'E':
            moveplayer(room[player.currentroom].e,2);
            break;
         case 'W':
            moveplayer(room[player.currentroom].w,3);
            break;
         case 'S':
            moveplayer(room[player.currentroom].s,4);
            break;
         case 'U':
            moveplayer(room[player.currentroom].u,5);
            break;
         case 'D' :
            moveplayer(room[player.currentroom].d,6);
            break;
         case 'I' :
            showinventory();
            break;
         case 'L' :
            player.moved=1;
            break;
         case 'Q' :
            retval=1;
            break;
         default :
            unknownword(shortverb, WORD_VERB);
      }
   }
   else if (!strcmp(shortverb,"LOOK"))
   {
      player.moved=1;
   }
   else if (!strcmp(shortverb,"INV"))
   {
      showinventory();
   }
   else if (!strcmp(shortverb,"HELP"))
   {
      showhelp();
   }
   else if (!strcmp(shortverb, "QUIT"))
   {
      retval=1;
   }
   else if (!strcmp(shortverb, "SAVE"))
   {
      savegame();
   }
   else if (!strcmp(shortverb, "LOAD"))
   {
      loadgame();
   }
   else if (!strcmp(shortverb, "GO"))
   {
      if (strlen(noun) > 0)
      {
         strncpy(scrap,noun,1);
         parse(scrap,"");
      }
   }
   else if (!strcmp(shortverb, "EXAM"))
   {
      examine(objnumber);
   }
   else if (!strcmp(shortverb, "WEAR"))
   {
      wear(objnumber);
   }
   else if (!strcmp(shortverb, "GET") || !strcmp(shortverb,"TAKE"))
   {
      get(objnumber);
      checkpuzzles(1);
   }
   else if (!strcmp(shortverb, "DROP") || !strcmp(shortverb,"THRO"))
   {
      drop(objnumber);
      checkpuzzles(1);
   }
   else if (!strcmp(shortverb, "SCOR"))
   {
      score();
   }
   else if (!strcmp(shortverb, "FLAG"))
   {
      for (i=1; i<header.flags;i++)
      {
         printf("Flag %d: %d\n",i, flag[i].state);
      }
   }
   else
   {
      // other verbs check here
      verbnumber=0;
      for (i=1;i<=header.verbs;i++)
      {
         if (!strcmp(shortverb,verb[i]))
         {
            verbnumber=i;
         }
      }
      if (verbnumber)
      {
         player.currverb=verbnumber;
         checkpuzzles(0);
      }
      else
      {
         unknownword(currverb,WORD_VERB);
         return retval;
      }
   }

   return retval;
}

int main(int argc, char **argv)
{
   int result=0, finished=0;
   char verb[255], noun[255];

   result=loaddatabase(argv[1], argv[2]);
   player.moved=1;
   player.won=0;

   // Start the game!
   if (argc == 4 )
   {
      printdatabase();
   }
   else
   {
      printf("Starting\n");
      start();

      while (!finished)
      {
         if (player.moved) displayroom(player.currentroom);
         player.moved=0;

         getinput(verb,noun);
         finished=parse(verb,noun);

         if (player.dead || player.won)
         {
            score();
            finished=1;
         }
      }
   }
   return result;
}
