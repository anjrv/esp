#ifndef VERSION_H_
#define VERSION_H_

#define DEVELOPER_ID "s114"
#define MAJOR 5 
#define MINOR 2 
#define REVISION 0

/**
 * VERSION HISTORY
 * 
 * 1.0.0 - Initial assignment setup
 * 
 * 1.1.0 - Routing if statement table
 *         Command functions
 * 
 * 1.1.1 - Split helper function
 * 
 * 1.1.2 - Integer parsing function
 * 
 * 1.2.0 - Stack structure added
 * 
 * 1.3.0 - Dictionary/Hashtable structure added
 * 
 * 1.3.1 - Integer parsing changed to mutate pointer
 *         and return void to allow parallel use
 *         and memory management
 * 
 * 1.3.2 - Dictionary query method changed to mutate pointer
 *         and return an integer TRUE/FALSE to more easily
 *         manage whether key value pairs exist or not
 * 
 * 2.0.0 - Assignment 2 setup, main response loop tasks added
 * 
 * 2.1.0 - Int return values standardized to something more
 *         C like (0 normal exit, alternatively error state)
 * 
 * 2.2.0 - Factoring functions and tasks added along with
 *         a storage implementation.
 * 
 * 2.3.0 - Semaphore added for factoring helpers
 * 
 * 2.3.1 - More documentation
 * 
 * 2.3.2 - Regex check to block invalid result command requests
 * 
 * 2.4.0 - Functionality of add changed to what it was actually supposed to be
 *         ( I Can't Fucking Read )
 * 
 * 3.0.0 - Assignment 3 setup, implementing bluetooth sdkconfig
 * 
 * 3.1.0 - Including the bluetooth demo command to check connectivity
 * 
 * 3.2.0 - bt_connect, bt_status and bt_close commands
 *         ( NOTE: currently connect only really checks whether the drivers allow it to scan )
 * 
 * 3.2.1 - Modification to serial function to prevent it adding a null terminator
 *         even if one was previously present ( e.g. in the case of an snprintf string )
 * 
 * 3.3.0 - Refactoring the functions contained in factors into more generic variants now found in tasks.c
 * 
 * 3.4.0 - Noise source generation added
 * 
 * 3.5.0 - Initial support for the creation of datasets ( these functions are generally in bt_tasks )
 * 
 * 3.5.1 - Generic variant of data_info and data_destroy added
 *
 * 3.5.2 - Initial variant of data_append added
 *         ( NOTE: currently only supports noise source )
 * 
 * 3.5.3 - Basic data_append state checks ( name, source, num )
 * 
 * 3.5.4 - Some semaphore refactoring for tasks
 * 
 * 3.6.0 - data_raw added
 * 
 * 3.7.0 - data_stat added ...
 * 
 * 4.0.0 - Assignment 4 setup, initialize and configure WiFI / disable bluetooth
 * 
 * 4.1.0 - WiFi files added with placeholder functions and "globals"
 * 
 * 4.2.0 - net_locate, net_reset and net_table added
 * 
 * 4.3.0 - net_status added
 * 
 * 4.3.1 - documentation additions
 * 
 * 5.0.0 - Initial node code implemented into project ...
 *         Most of the old WiFi code discarded due to conflicting definitions
 * 
 * 5.1.0 - Node code adjusted: New implementation of net_table and random 0-10 ms delay on sends
 * 
 * 5.2.0 - Collatz demo added as an available application
 *         serial.c adjusted to initialize required dependencies
 */

#endif
