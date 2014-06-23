Ascape
------
(C) David Lodge November 2005

Basic player for the Adventurescape games. Including win32 based executable.

The Adventurescape system was given away on a cover disc for A&B Computing in
1985.

There were the following games released using it:
Game				               Database	Works
------------------------------------------------------
2002: A Space Oddity		      1		   yes
The Island			            1		   yes
Jungle Quest			         1.1		yes
The Pobjoy Special		      1.1		yes 
The Silicon Jungle		      1.1		yes
Live Aid Magical Mystery Tour	1.1		yes
The Snow Queen			         1.1		yes
Anthrosin			            1.1		yes
Creepy Adventure		         1.1		yes
Lost in Xanadu			         1.1		yes 
Amnesia!			               1.5		yes 
Murder at the Abbey		      2  		yes
Dungeon Adventure		         2 		   yes
First Contact                 2        no
The Cube of Zoth		         ?		   no
Cordelia                      ?        ?
The Ultimate Prize		      HATRACK  no
Pirate's Peril			         HATRACK  no
Dreamtime			            HATRACK  no
The Taroda Scheme		         HATRACK	no
Stranded!			            HATRACK  no
Stranded!			            BEEBRACK	no
Rise in Crime			         HATRACK	no

Ascape can be used to dump the dictionary or to play the game. Currently this
can only be specified on the command line.

The command line is:
	ascape <game data> <game init> [dump]

If you specify 'dump' then the database will be dumped to standard out and will
not be interpreted.

The version 2 games use a separate messages and locations file which are
referenced within the main <game data> file, these should be placed in the
current directory.

For the curious the only difference between version 1 of the database and
version 1.1 is that an integer is used to saved the score for winning, rather
than a byte.

Version 1.5 is similar to 1.1 but things are mangled around a bit so that the
room data and messages can be read directly from disc.

As I don't know the licence details on the games; finding them is left as an
exercise to the reader. (hint: Stairway to Hell and The BBC Lives). Once found
the necessary files can be extracted from the disc images (I used DFS explorer).

The required files are:
Version 1,1.1	Data and Init
Version 1.5	Data, Init, Messages, Locations, Message pointer, Locations pointer
Version 2.0	Data, Init, Messages and Locations

The Messages and Locations files are pointed to in the data file (all you need
to do is remove the $. from the front.) With version 1.5 the pointer files are
preceded with an "I."

Amendments
Version 2.1, 2008-10-16
2.1	Some clean up on puzzles; they will be properly run after GET or DROP.
	Altered it to redisplay the room if an exit is added.

Bugs and Stuff to be done
-------------------------
* Saving and Loading games isn't supported yet (should be easy all the work is
already done!)
* Get the other database versions working
  (version 1.1 complete 27/11/05)
  (version 1.5 complete 28/11/05)
* Work out the Heyley software game format
* More debug codes built in
* Replace the output and input with a more abstract way, thus allowing easy
GUIfication
* Do real CLI work instead of guessing - including error checking
* Split the source into separate files:
  * main (initalise the structures and load database)
  * database (load and print database)
  * interact (interact: print and input)
  * engine (the nuts and bolts of the engine)
* Remove local data from the structures
* Dual play through all games to ensure lack of bugs
