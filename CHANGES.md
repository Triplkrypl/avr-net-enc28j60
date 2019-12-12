## 0.1 - 0.2

* Changed **HttpRequest structure** used in **HttpOnIncomingRequest parameter**, some structure **members moved into HttpMessage** which is accessible by pointer in HttpRequest
* Declaration of **HttpParseHeaderValue** has **HttpMessage** as first parameter **instead of HttpRequest**
* Changed behavior **if HttpSendResponse is not called in HttpOnIncomingRequest**, library will **close connection instead** of **send Http 204 response**

## Changes what i made before versioning.

* Fixes:
    * icmp ping with different length works, before library not response with same icmp length as was request but with constant length

* Improvements:
    * tcp can handle multiple established connections
    * synchronous wait for packet if receive different packet than expected will send received packed in to main loop function for handle except drop
    * arp broadcast responses are cached
