CSE 532 Fall 2013 Lab 3
Gabriel Hope, ghope@wustl.edu
Kevin Kieselbach, kevin.kieselbach@wustl.edu
Ethan Rabb, ethanrabb@go.wustl.edu

Overview of new Director components:

In order to handle multiple plays, our director stores a map from play names to shared pointers to Play objects. Rather than storing a reference to a Play, players store a reference to this map. Each player also stores the string name of the current play it is performing, which it uses whenever it needs to access a Play object. The director also stores the string name of the current play. In order to ensure that the player's play names are safely updated, the director must obtain a lock in order to set the player's play name, which can only be done when the player is not in the middle of performing a play.

In order to enable stopping and termination, we have a stop flag and a terminate flag on the director. The Player threads check these flags in the Play's enter and recite methods and in the Player's act and startPlayer methods. The director notifies condition variables in the Play and Player after setting the flags in case the Player threads are waiting. In the Play's exit method, the last Player thread to exit notifies the director that the Play is over (so the director can inform the producer). The director also keeps track of whether a play is running so that it can signal the producer immediately without waiting for a play to stop. The first Player thread to call the Play's enter method informs the director that the play is now running.

Rather than simply providing a single cue method, the director now has a direct method that runs the ACE reactor event loop. The cue method now takes a play name as an argument and is called when the director receives a request from the producer to start a play.

To implement signal handling, we use the ACE reactor::notify method to schedule an event on the reactor. This event is scheduled before any pending events (other than the event currently being processed). The reactor::notify method allows us to perform system calls outside signal context without requiring a constantly awake sentry thread to check flags. Since the signal handler only has access to global state, we made our director a singleton so the signal handler can reference it. Despite making director a singleton, we still pass references to the director to the Play and Player classes because passing references to constructors is less error prone than relying on a singleton (e.g. we do not need to perform NULL checks when we pass references).

Overview of Producer components:

Our producer design consists of four classes: Producer, Console, Acceptor, and Connection.

Producer contains the core logic that keeps track of which directors are available and which plays those directors have and all communication between the user and the directors goes through the Producer. Producer contains three main data structures: a map from director ID to vector of play names, a map from director ID to a pair of values indicating which of the directors plays has been requested and whether the director has started that play yet, and a map from director ID to the Connection object for the director. Producer has methods that the Acceptor can call to add a director, that a Connection can call to inform the producer about the state of the director, and that the Console can call to request start, stop, or quit operations. The Producer's methods send messages to the directors as appropriate and print the UI to the screen whenever the state of the Producer changes. The Producer also has a produce method that runs the ACE reactor event loop.

The Console takes in command line input from the user, parses it, and calls the appropriate methods on the Producer in response to the command. As far as we can tell, there is no way to reactively read command line input, so the Console is an active object with its own thread. In order to not block, so that the Console can watch for a terminate flag set via a signal, we use the cin.get() method to read one character at a time from the command line. Cin.get() returns immediately with EOF if no input is available. Since we read input a single character at a time, we implement a state machine to parse the input. In fact, we also read socket input one character at a time (since we perform blocking reads), so we use state machines to parse all inputs in our solution.

The Acceptor is an ACE_Event_Handler and encapsulates an ACE_SOCK_Acceptor. It opens the ACE_SOCK_Acceptor and registers itself as a handler for the ACE_SOCK_Acceptor. When a connection attempt is received, it creates a new Connection object for the director and adds the director to the Producer.

The Connection is an ACE_Event_Handler and encapsulates an ACE_SOCK_Stream. It parses input received from the director and calls the appropriate method on the director. The director uses the ACE_SOCK_Stream contained in the Connection to send messages to the director. The Producer contains one Connection object per director.

To implement signal handling, we use the ACE reactor::notify method to schedule an event on the reactor. This event is scheduled before any pending events (other than the event currently being processed). The reactor::notify method allows us to notify condition variables when a signal occurs without performing system calls in signal context or requiring a constantly awake sentry thread to check flags. Since the signal handler only has access to global state, we made our Producer a singleton so the signal handler can reference it. Despite making Producer a singleton, we still pass references to the Producer to the Acceptor, Connection, and Console classes because passing references to constructors is less error prone than relying on a singleton (e.g. we do not need to perform NULL checks when we pass references).

Overview of components from previous labs:

We adopted a Leader-Followers approach in our design. The director parses the script and populates a queue of character parts based upon the script. Players are active objects that grab parts from the character parts queue and perform them using the Play object to synchronize the performance. The character parts must be assigned to players in the order defined in the script, which is also the order that the director inserted the parts into the queue.

Our leader election protocol is simple: player threads that do not currently have a part race to grab the lock on the parts queue (which is thread-safe). The first thread to grab the lock on the parts queue is the leader.

The work distribution protocol is also simple: the leader takes the first part from the queue and assigns it to itself. It then abdicates the throne and starts to perform the part. The remaining player threads that do not have parts repeat the leader election protocol.

This approach can alternatively be thought of as a greedy scheduling approach where threads grab work from a global work queue in a greedy manner. Since the parts are stored in the appropriate order in the queue and we ensure that there are at least the minimum number of required players, the greedy schedule is guaranteed to successfully complete the play.

After obtaining a character part (which contains a character name, a part file containing the lines, and a scene fragment identifier), a player thread assumes the name of the character, reads in the characters lines from the file into a map, and tries to enter the play. The enter attempt will block until the current scene fragment is the appropriate scene fragment for the character part. The player will then recite all of the character's lines using the Play object to synchronize the recitation. Then the player exits the play. When all players have exited, the scene fragment counter is incremented and players for the next scene fragment begin to enter. After a player exits, the player participates in the leader election process again (i.e. it tries to grab another part).

The termination protocol is also simple: when a player is the leader and finds the parts queue to be empty, the player thread ends. After starting the players, the director waits for all players to finish, so when all players finish, the program exits.

We defined a DialogueLine struct to encapsulate each line of text, the speaker of the line, and the line number.  We used C++ streams and utility methods/operators that operate on those streams to safely and easily read from the files.  We defined a trim method to remove whitespace from the front of lines to ensure that lines consisting of only whitespace are treated as empty lines.  

We use a map from line numbers to DialogueLines to store the lines for each player.  The map sorts the lines as they are inserted, so if the lines were scrambled in the input file players will still recite their lines in order. This prevents a player from self-deadlocking (i.e. waiting on one of their own lines).

We added additional instance variables to the Play class other than the ones specified in the instructions. First, the Play keeps track of the current speaker which allows it to print the speaker when the speaker changes. Second, the Play has an additional count, numPlayersTimedOut, and a boolean flag, makingProgress, which are used to implement a deadlock free and semantically correct recite method (see below). NumPlayersTimedOut keeps track of how many players timed out during their wait on the condition variable. MakingProgress keeps track of if any player has been able to successfully recite a line. Using onStage, numPlayersTimedOut, and makingProgress, we can ensure correct semantics when recovering from an deadlock situation.

The naive implementation of the recite method simply uses a condition variable to wait until the counter equals the line number before printing the line, updating the iterator and counter, and notifying the other threads. However, there are three possible deadlock scenarios that must be avoided: 

(1) A player is waiting on one of his/her own lines because the lines are out of order. In this case the solution is trivial: sort the lines like we do with the map data structure. 

(2) A player is waiting for a line number that is less than the current value of counter. In this case, another player has already read a line with that number and has incremented the counter (duplicate line number), so that line number will never occur again (our map data structure prevents a single player from having two lines with the same number). Once again the solution is simple: wait for the counter to be greater than or equal to the player's line number instead of exactly equal to. After exiting the wait, we can print an error message and update the player's line iterator for the greater than case.

(3) If no player has a certain line number, then all players will be waiting indefinitely for a line that does not exist. This could be caused by a missing or badly formatted line in the character parts (someone forgot their lines, tsk, tsk). Without having a global view of all the lines of all the players, the Play's recite method cannot know with certainty if a line has actually been lost or if a player is just slow to call recite on the line. Therefore, we assume that if a line has not been recited for a given value of the counter after one second, we will timeout, assume the line is lost, and skip it. We use condition_variable::wait_for to implement waiting with a timeout.

On the surface, deadlock case (3) does not seem too hard to solve. The challenge is to ensure that we report the error and update the counter exactly once per lost line. Since all player threads wake from the wait_for in the event of the timeout, we must ensure that one and only one player thread updates counter. We cannot use a simple flag because we need to reset the synchronization mechanism before the next line has a chance to timeout but after all threads have passed through the synchronization mechanism.

In the previous lab, we attempted to solve deadlock case (3) by having the first player who timed out increment the line counter, and the last player who timed out reset numPlayersTimedOut. While this solution worked most of the time, it contained a subtle race condition where after timing out, a player could lap the other timed out players and thereby skew our accounting mechanism. We developed a more complex solution for Lab 2 that solves deadlock case (3) 100% of the time. See extra credit for description.

Player is exception safe since it will join with its thread in its destructor if its thread is still joinable.

For out input we make the simplifying assumption that character names and file names do not contain whitespace.

Wrapper Fascades:

We used the C++11 thread, mutex, and condition_variable classes to implement concurrency and synchronization without relying on platform specific APIs. We used the unique_lock and lock_guard classes with the Play's mutex to ensure the mutex is released by the end of each synchronized method of Play, which provides exception safety (and protects against programmer oversight). 

Insights and Observations:

We observed that it is not necessarily possible to deterministically avoid deadlock when threads have incomplete information about the state of other threads. We cannot know for certainty if a condition will ever be satisfied unless we know the state of all threads in the system. Thus in practice, it might be necessary to use timeouts to avoid deadlock, which is based on the assumption that if a condition is not satisfied after some amount of time, it likely will never be satisfied.

We also observed that design patterns are used most effectively when they are adapted to fit the problem at hand. We opted for a simple version of the Leader-Followers pattern in which the leader election protocol and work distribution mechanism is simply greedy grabbing of work from a queue. Rather than trying to force a complicated election protocol and distribution mechanism onto our design, we chose ones that fit naturally into our design. The result was an elegantly simple solution.

Unpacking instructions:

1) Unzip the zip file (right click menu).
2) Create two new Win32 Console apps in Visual Studio.
3) Follow the instructions at http://www.cse.wustl.edu/~cdgill/courses/cse532/networked_concurrency.html to set up each project to use ACE.
4) Place the files from the Director folder into one of the project's NAME/NAME folder.
5) Include header and source files in the project
	a) right click on "Header Files"/"Source Files", select "Add", select "Existing Item"
	b) source files: CharacterPartQueue.cpp, DialogueLine.cpp, Director.cpp Lab3-Director.cpp, Play.cpp, Player.cpp, utils.cpp
	c) header files: CharacterPartQueue.h, DialogueLine.h, Director.h, Play.h, Player.h, utils.h
6) Build the Director program (F7)
7) Place the files from the Producer folder into the other project's NAME/NAME folder.
8) Include header and source files in the project
	a) right click on "Header Files"/"Source Files", select "Add", select "Existing Item"
	b) source files: Acceptor.cpp, Connection.cpp, Console.cpp, Producer.cpp, Lab3-Producer.cpp
	c) header files: Acceptor.h, Connection.h, Console.h, Producer.h, utils.h
9) Build the Producer program (F7)
10) Open a copy of cmd.exe for the Producer and each instance of Director desired. Navigate to the project's NAME/debug directory (the outer of the two debug directories) which will be different for the Producer and Directors.
11) Run the Producer program by calling the program's name
12) Run each Director program by calling the program's name followed by the port, IP address, minimum number of threads, and a list of script file names (note: all play files must be in the NAME/debug directory or full paths must be specified)

Testing:

Since we reused most of the file I/O code from Labs 0, 1, and 2 we did not have to do too much testing with file I/O. We quickly verified that it handled blank lines and missing lines, etc. We also determined that our code correctly handled badly formatted scipt files and ignored all unreadable files without crashing. Our focus was testing the concurrency and synchronization code. We first tested with correct input and found that a large number of lines were being skipped even though they were available. We quickly determined that this bug was caused by not resetting the line counter between scene fragments.

We tested for the deadlock conditions described above by arranging lines out of order, adding duplicate lines, and removing lines. We found that our new deadlock avoidance technique worked perfectly even with a large number of out of order and missing lines. Our program never deadlocked in any of our trials.

We tested the Producer-Director interactions with one, two, and multiple Directors connected to the Producer. We tested malformed console input with the Producer, start, stop, and quit commands, ctrl-C termination in the producer and directors, repeatedly running the same play, and switching back and forth between different plays.

Extra Credit:

The details of the deadlock avoidance mechanism are included in the code comments. At a high level, when players wait on the line and scene counters, we include a timeout. In the event of the timeout, the first thread to timeout reports that progress is no longer being made. When all players on stage have timed out, we know that the line is lost, so we skip the line and indicate that progress is once again being made. To avoid the lapping problem we had previously, after skipping the line, the players once again barrier synchronize. Our new mechanism also takes into account the possibility that one player has a really long monologue, in which case other players will time out while waiting even though no line has been lost. Every time a player successfully recites a line, it reports that the play is making progress. When progress is being made, the timed out players, which are waiting for the remaining players to timeout before they skip the line, will stop waiting for players to timeout and resume waiting on their next line.

We tested for the deadlock conditions described above by arranging lines out of order, adding duplicate lines, and removing lines. We found that our new deadlock avoidance technique worked perfectly even with a large number of out of order and missing lines. Our program never deadlocked in any of our trials. 
