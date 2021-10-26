# Assignment 3 Information

## Changes and implementation

In general an honest attempt was made to implement all the listed firmware changes. Upon rudimentary testing most of these perform as expected ( ignoring the fun new problem listed later in this information packet... ) including for things like datasource loss, data lock and data source destruction.

There are two egregious exceptions to the above:

* One very obvious instance where they do not perform as expected is the bt_connect command. I couldn't figure out a reasonable way to gracefully: accept a name and source -> scan for the name and source ( accounting for all the callbacks ) -> connect to a valid option ( if any ) and return information about this process within an instant function ( the combination of connection callback and an inquiry being entire seconds long makes it really hard to definitely and instantly say whether connection was successful. ) As a result of this the bt_connect response is effectively a lie, it only really tells you whether it could start scanning successfully... bt_status on the other hand will report correctly as there is a connection flag that is set when a connection is successfully made.
* Another obvious omission is the lack of reaction to the "true" value-type of data-sources. Currently data_stat completely ignores that datasets with bt_demo source should be floats. The current way I would implement this is effectively forking data_stat into multiple methods depending on the source of the dataset ( or specifically the value type of the dataset. ) I legitimately did not have the willpower...


## New fun problems!

For some reason changing the configuration broke something in the way input is read through the serial. It now fails to recognize commands at random in a way I'm failing to figure out ...

Example of this fuckery:
```
<< bt_connect ASC_BT_DataSource ASC_BT_DataSourceSvc
>> connection success
<< data_create banana bt_demo
>> command error
<< data_create banana bt_demo
>> data set created
<< data_append banana 20
>> id0
<< ps
>> data_append id0 complete
<< result id0
>> 20 entries recovered
<< data_raw banana
>> command error
<< data_raw banana
>> command error
<< data_raw banana
>> request:banana entries:20
```


## Some general thoughts

* Even this scuffed version of the project ended up taking a surprising amount of time. Providing semaphore handling and the required values for task creation made the implementation fairly longwinded even when "cheating" by effectively hard coding some of the knowledge about the data-sources we have available. 
* It is very likely this type of hard coding will bite me in the ass later if we add an additional arbitrary N sources ( e.g. when implementing WiFi ) but realistically I kind of just started hacking stuff together to make actual progress here. 
* The esp-idf documentation for classic bluetooth was, in my opinion, also surprisingly obtuse - often information was lacking about what result providing an argument to a function would have ... ( inquiry mode for example. )
* This is probably already known but some of the firmware descriptions were a bit vague:
    - From what I could tell sources have their keys tied to them - i.e. noise always has the keys a, b and c and these can never be changed ( at least in the current implementation. ) But some of the text makes it seem as if these can change?
    - Some examples seem contradictory, the data_stat example seems to return values as if it was an instant command but is clearly defined as a background command. For this specific conflict I went with the assumption that the example was "shorthand" for the predefined order of: initiate background task > receive id > check result with id later

# Assignment 2 Information ( Old )

## Changes and new implementation

* The response variant from the first firmware version was relatively easy to implement into this task structure. All that was really required was to shift the app_main process to a main process that could be task( created and then after that the old response function could simply be created as a high priority task.
    - A minor adjustment was made in that serial responses were moved to the commands themselves rather than the response function, this was done because ps demanded multi line response capability and at that point, for consistency, it became better to standardize response output into the commands and subfunctions therein.
* The new commands were generally implemented by giving them a semaphore controlled local linked list structure which matched the required information ( current process state, the factor response, etc. )
* Explicit core pinning was not really used and instead I opted for `tskNO_AFFINITY` which currently uses available cores after the priority available ( This should then provide multicore functionality for our tasks if it is available but simply use single core if it is not. ) Interestingly providing no explicit affinity causes the framework to queue these tasks successfully without an explicit queue structure which means that currently the simplified linked list is sufficient and the scheduler will eventually work through all these tasks.
* Additional note/concern:
    - Currently there is no explicit protection from filling the memory of the device
    - It will allow you to initialize 100 of these factoring functions until it stops being able to allocate memory
    - Similarly the results are stored in a linked list which does not delete nodes as it stands but a delete function can be easily added later if we want to define situations where a result is to be discarded ( e.g. deletion instead of complete* on result request. )

### Command add changes

* Why was the previous implementation weird and used integer arguments instead of stored variables?
    - I can't fucking read
    - It has since been fixed to work as defined
* Why does/did it allocate memory ( malloc(...) ) for the arguments received
    - I mostly made the parsing and storage functions take pointers which they mutated
    - ( parse_int(...) as well as the dictionary query(...) )
    - This was kind of a hacky way to make the return value int an exit status ( initially parse_int actually used atoi but... yeah )
    - In general I had the sense that the function would end up being well defined enough that it could free the two pointers it mallocs

## Quirks

* For the factoring function itself a generic special response was added for input numbers of 1 and 0  ( cannot be factored ) this is also used for undefined behavior when the value to be factored fails to be read properly.
* Generally the newly implemented ps function will list processes from newest to oldest since it simply iterates from the head of the list and new entries are inserted at the head.
* There is no real explicit limit to number of tasks, its not so much as a queue as it is just creating tasks named based on their ID tag. It remains to be really seen how this works out in the long run, currently integer factors don't exactly take very long so testing intended behaviour of a large number of background processes was a little strange.

# Assignment 1 Information ( Old )

## File structure

**In general I went with a variant of the structure suggested in the assignment description.**

* The initial serial communication loop is located in the main app file, serial.c
    - This also contains data structure initialization and a helper function for routing to individual commands
* The commands used in the response loop are located in commands.c
* The data structures used were split into two seperate files ( one for each data structure )
    - stack.c contains an extremely simple stack implementation
    - dict.c contains a slightly more complex dictionary/hashtable integration ( and a helper linked list but that is not globally exposed )
* An additional file called utils.c was added to keep basic helper functions to be used globally. Currently these are:
    - parse_int
    - long_to_string ( This also works on integers so I decided to apply a wider scope )
* All .c files aside from the main serial.c have their corresponding header files for declaration
    - Not all functions are globally exposed such as the overflow linked lists that are exlusively used for the dictionary data structure

## Response sequence and implementation

* Input is read through a modified example of the serial program
    - Main changes made to the reading of the query string is the detection of excess whitespace and initialization of the implemented data structures (dictionary and stack)
    - **NOTE**: Because excess whitespace is accounted for before an attempt at parsing is made the argument and command errors resulting from it are more approximations rather than exact responses. ( E.g. consecutive and trailing whitespace between words will trigger an argument error even if the first command was not valid ) 
* When a valid input query string has been received it is:
    - Sent to a response function which will first parse the query string into a list of pointers using strtok and ' ' as a delimiter.
    - Once the querystring has been split the first stored string is set to all uppercase to fulfill the case insensitive rule. 
    - This modified command string is then sent through a sequence of if statements to check whether it matches any of the available commands.
    - If a matching command is found the split querystring and its number of tokens ( as well as any required data structure ) is forwarded to that command to be executed.
* Currently each of the command functions returns a string which is to be used as a response by the response function that calls the command. Implementation decisions for the commands:
    - The commands validate their own arguments and set their relevant error state, if a command does not require arguments to run such as ping, version, mac or error it will not check if excess arguments have been given. ( This is to fulfill the specification that the error command should always succeed. )
    - Commands that do not take arguments will not trigger an argument error
    - Excess arguments to commands that do take arguments will trigger an argument error

## Other implementation decisions

* Currently leading zeroes are allowed for all number inputs.
* All integer parsing and formatting is done through the use of longs to easier detect overflow problems that would otherwise cause issues.
* Functions that can fail such as parse_int take a pointer as an argument which would store the outcome of the function, the return value is instead an integer indicating whether the operation was successful or not. This allows us to avoid the problem of, for example returning an integer from a parse_int function where 0 is a fail state but also a valid result.
* Argument and command errors do not set the error state, generally error states are reserved for when queries pass "first inspection".
* The initial error state was chosen to be "no history". This is what the error command will return if no other commands have been used.
* The maximum storage space of named variables is limited rather than being dynamically allocating memory infinitely. This was mostly done to be able to choose a faster data structure instead of something that would have to iterate over stored variables. ( Albeit linked lists with a skip list helper were considered )
