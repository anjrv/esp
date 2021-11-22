
*** IMPORTANT ***
The network layer implementation here is in its early stages.  It may have bugs!  We may produce updated versions at any time.  You cannot, in your usage of this library, rely on or use internal functionality of the network layer implementation (functions / structs defined in net_layer.h).  The public API of this library is the Network.h header file.  If you integrate this correctly, you should be able to hot-swap updated implementations (net_layer.h/.c) in without requiring changes to your firmware code.

IF you run into unexpected behaviour which you think is a bug with the network layer implementation, look into it!  At the very least, let us know if it behaves in an unusual manner -- this implementation has _not_ been tested exhaustively.
*** ********* ***

This firmware package contains a complete implementation of the network layer of our net stack, and a testing application (app_bounce) to demonstrate aspects of its functionality and usage of the network APIs.

You are encouraged to test out varying combinations of network initializations and observe what happens.  Do the two nodes connect to form a network?  Does the bounce application behave correctly?
 - 1 root note, 1 regular node (*)
 - 2 regular nodes
 - 2 root nodes

(*) Try this first.  This is the 'working' version which behaves as you would expect.  You can compare the results of the other node arrangements to this.

Further applications will be made available as we manage to churn them out!  Please integrate this content in some manner into your firmware such that it can behave as a network node (outside of testing situations, you should never be initializing the network layer as a root node, FYI).