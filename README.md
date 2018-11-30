# AVR Net enc28j60

Library written in C for AVR microcontroller. This code is based on other public repository witch i do not remember where it is.
If someone will recognize code with is not mine and will know repository i will glad add reference here.

## Changes what i made.

* Fixes:
    * icmp ping with different length works, before library not responce with same icmp length as was request but with constant length

* Improvements:
    * tcp can handle multiple established connections
    * synchronous wait for packet if receive different packet than expected will send received packed in to main loop function for handle except drop
