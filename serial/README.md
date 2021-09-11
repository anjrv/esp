# File structure

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

# Response sequence and implementation

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

# Other implementation decisions

* Currently leading zeroes are allowed for all number inputs.
* All integer parsing and formatting is done through the use of longs to easier detect overflow problems that would otherwise cause issues.
* Functions that can fail such as parse_int take a pointer as an argument which would store the outcome of the function, the return value is instead an integer indicating whether the operation was successful or not. This allows us to avoid the problem of, for example returning an integer from a parse_int function where 0 is a fail state but also a valid result.
* Argument and command errors do not set the error state, generally error states are reserved for when queries pass "first inspection".
* The initial error state was chosen to be "no history". This is what the error command will return if no other commands have been used.
* The maximum storage space of named variables is limited rather than being dynamically allocating memory infinitely. This was mostly done to be able to choose a faster data structure instead of something that would have to iterate over stored variables. ( Albeit linked lists with a skip list helper were considered )
