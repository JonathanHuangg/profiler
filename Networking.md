Computer Networking - A Top-Down Approach

# Computer Networks and the Internet

## Basics
### Network Edge
Hosts - end systems. Can be clients and servers
Access network - connects end system to the first router

#### DSL
A single DSL (digital subscriber line) line can carry different frequencies and is based on the local telephone infrastructure. However, it is designed for short distances between the home and CO (central office).

Devices like a home PC use a DSL modem that connects to the phone line. The phone line feeds into a DSLAM (in the CO) which routes phones to the telephone network and devices to the internet. 

#### Cable
Cable Internet Access - uses TV infrastructure. Fiber optics link neighborhoods together typically 500-5000. A coaxial cable connects hundreds of homes together linking to a fiber node. Fiber nodes connect with a fiber cable to the cable head end which connects to the internet. 

Requires cable modems connecting to home PC via ethernet. cable modem termination system (CMTS) - analogous to DSLAM - turns the analog from cable modems into digital. 

Cable modems divide HFC network into downstream + upstream channel. 

New idea: fiber to the home (Fiber Broadband) which connects an optical fiber path from CO directly to the home.

Optical CO to the Home is typically done using direct fiber - one fiber leaves the CO and is split into customer fibers close to the home.

#### Optical distribution networks 
1)AONs - switched ethernet
2) PONs - each home has optical network terminator (ONT) connected by optical fiber to a neighborhood splitter. Splitter combines multiple homes which feeds into a optical line terminator (OLT). The OLT converts optical to electrical signals and connects to the internet via a telco router. At home, routers connect to the ONT to access internet. 

Routers -> ONT -> Optical Splitters -> OLT (central office) -> Internet

#### Ethernet
Computers connect to an ethernet switch which feeds into an institution router which goes to the ISP. 

Wireless LAN - devices connect to an access point that feeds to the enterprise network (using ethernet) which then connects to internet. 

#### Physical Mediums
HFC uses fiber + coaxial cable.
DSL and ethernet use copper wire.
mobile access networks use radio spectrum. 

2 Categories of Physical Media -  Guided and Unguided
1) Guided media - waves are guided along a solid medium (fiber-optic cable, twisted copper wires, coaxial)
2) Unguided media - wireless LAN or digital satellite

#### Twisted-Pair Copper Wire
Least expensive and most common. Twisted to reduce electrical interference from similar pairs close by. 

A number of pairs can be bundled together. 

The data rates depend on thickness of wire and distance between transmitter and receiver. 

Used for high-speed LAN networking and residential internet access. 

#### Coaxial Cable
Also 2 copper conductors but they are concentric rather than parallel. 

Used typically in cable television. 

#### Fiber Optics
Thin, flexible medium that conducts pulses of light where each pulse is a bit. 

High bit rates (tens to hundreds gigabits/s)

Immune to electromagnetic interference

Low single attenuation up to 100 km

Hard to tap

Typically found in long distances like overseas links. Long distance telephone networks also use fiber optics. Also used in the backbone of the internet.

Is quite expensive with transmitters, receivers, and switches making LAN more difficult. 

#### Terrestrial Radio Channels
1) Short Distance (1-2 meters) 
2) Local areas (10-100s meters)
3) Large areas (10s of kilometers)

#### Satellite Radio Channels

A satellite links 2 Earth-based receivers (ground stations)
Geostationary satellites - remain the same spot on Earth
    Large propagation delay but can operate at speeds of hundreds of Mbps

LEO satellites - rotate around the earth and communicate to each other as well as ground stations

### Network Core
The mesh of packet switches that connect the internet's end systems

2 Types of packet switches 1) routers 2) link-layer switches

#### Store and Forward Transmission
Packet switch must receive the entire packet before it can begin to transmit the first bit of the packet onto the outbound link. 

For a single switch, it will take N time to receive, and sending out takes N time taking 2N time.

With multiple switches, it's pipelined so with 3 switches, it only takes 4N time. 

More mathematically, given L bits, and R rate. With one packet and 1 switch, it will take 2L/R seconds. 

With 3 packets, it will take 4L/R.

Given N links, the time is N * L/R

#### Queuing Delays and Packet Loss
Packet switch has multiple links attached. For each link, there is an output buffer from router to link. If the buffer is full, drop the arriving packet or already-queued packet (packet loss)

#### Forwarding Tables and Routing Protocols
The destination IP is always in the packet header. 

Each router has a forwarding table that maps destination addresses to outbound links.

Different routing protocols set the forwarding tables. 

#### Circuit Switching
Packet switching moves network through link and switches, circuit switching is the other way. 

In circuit switching, communication resources are reserved for the duration of the communication session. 

Example: Traditional telephone network. Before the call, a connection must be established with a given transmission rate. 

#### Multiplexing in Circuit Switched Networks
Circuit in a link is implemented with frequency-division multiplexing (FDM) or time-division multiplexing (TDM). 

1) frequency-division multiplexing (FDM) - frequency is divided up so routers all get a portion of the bandwidth. ie: radio station uses shares frequency spectrum between 88 MHz and 108 Mhz where each station has a frequency band.

2) TDM - time is divided into frames of fixed duration. When network has a connection across the link, the network dedicates a time slot in every frame to this connection. In other words, each circuit continuously gets a fraction of the bandwidth over brief intervals of time. If only one router is active, the rest of the frames are empty.

Circuit switching can be considered wasteful as they are idle during silent periods. When a person stops talking, the frequency bands or time slots are still active. 

#### Network of Networks
Tier1 - backbone
Regional ISPs or IXP (datacenters)
Access ISPs

Content-provider networks - undefined atm

#### Delays in Packet Switched Networks
1) Nodal processing delay - time it takes to examine packet header and where to direct to
2) queuing delay - packets wait to be transmitted onto the link. based on number of earlier-arriving packets. Use statistical measures like avg, variance, probability delay exceeds a specified value
Given L bits, R is bits/sec transmission, a (packets/s) is the average of rate of packet arrival to the queue. 

(La)/r is the traffic intensity. If (La)/r > 1, average rate of which bits arrive exceeds leaving so the queue will increase without bound. Queueing delay will thus approach infinity. 

If (La)/4 <= 1, if packets are periodic - no queing delay because they are processed during the down time. If in bursts, there is still significant queing delay. 

As traffic intensity approaches 1, average queuing delay increases rapidly. 

3) transmission delay - The amount of time it takes for the router to push out the packet. L/R
4) Propagation delay - time from A - B on the physical medium

#### End system delays
An end system that wants to send packet may purposefully delay transmission as part of its protocol.

Media packetization delay - in voice-over-IP (VoIP) applications. The sending side fills packet with encoded digitized speech before passing packet to the internet. 

#### Throughput
The rate of which a host sends/receives a file. 

### Protocol Layers and Service Models
#### Network Protocols
Application Layer Protocols - HTTP and SMTP. Typically implemented in software in end systems. 
Transport layer protocols - also implemented in software

Some potential problems with layering
1) Duplicate layer functionality - many protocol stacks have error recovery on per-link basis and end-to end basis
2) a functionality at one layer may need information (ie: timestmap) that is present in another layer which violates goal of separation of layer.

#### Layer Overview
1) Application Layer 
a) HTTP Protocol - web document request and transfer
2) SMTP - transfer of email messages
3) FTP - transfer of files between 2 end systems
4) DNS - domain name system. Translates something like www.abc.com to 32 bit network address

#### Transport Layer
Transports application layer messages between endpoints. 
1) TCP/UDP. 

TCP - connection based with guaranteed delivery with segments and congestion control
UDP - connectionless service with no reliability, flow control, and congestion control

#### Network Layer
Also known as the IP layer Moving network-layer packets (datagrams) from one host to another. It is the actual service of delivering from transport layer. 

Has the IP protocol with fields in datagrams. Also has routing protocols.

#### Link Layer
Moves packet from host/router to the next node in the route. 

Also has protocols for reliable delivery.

TCP is reliable delivery from one end system to another. 
Link layer protocol is reliable delivery from one node to another. 

Example protocols - ethernet, WiFi, cable access

#### Physical Layer
Moves the individual bits within one frame from one node to the next. The actual transmission medium (twisted-pair copper wire, single node fiber optics)

Ethernet has many physical layer protocols, one for copper, one for coaxial, etc. 

### Deeper Dive on Concepts
1) Host vs End to End
2) Different access technologies. Classify as home access, enterprise access, wide-area wireless access.
3) Is HFC transmission dedicated or shared among users? Are collisions possible?
4) What are advantages from circuit switch over packet switch? What advantage is TDM > FDM in circuit switch. 

## Application Layer
### Principles of Network Applicatons
Web apps - 2 programs that communicate
1) Browser program running in host device
2) Web server program running in web server host. 

Netflix runs on the phone. Netflix server program running on Netflix server host. 

Goal is to write software that runs on multiple end systems. 

### Application Architectures
1) Client-server architecture - server services requests from clients. ie: web server servicing browswer requests
    Server has a fixed, well known address because the service is always on

    In cases where a server cannot keep up with requests, data center has large number hosts

2) P2P architecture - minimal reliance on dedicated servers in data centers
    Self-scalability

### Processes Communicating
Programs are represented as processes and they communicate with each other. 

Processes use sockets as the interface with the rest of the OS in order to communicate across internet. 

Socket communication is done via API

Application side controls
1) choice of transport protocol
2) changing parameters for transport-layer parameters

IP address identifies the host. The port number identifies the receiving process.

Choosing the right Transport service
1) reliability
2) throughput
3) timing
4) security

TCP - connection exists between 2 sockets. Starts with a handshake. Reliable data transfer. Congestion control via throttling. 

UDP - no handshake, unreliable data transfer. Messages may arrive out of order

Remember: no encryption. That relies on transport layer security (TLS)

### Application Layer Protocols
Application-layer protocols defines how processes pass messages

- type of message exchanged
- syntax of message types
- semantics of the fields
- rules for determining how a process sends messages

Example: HTTP is public domain. Skype uses proprietary application layer protocols. 

Network Application vs Application Layer Protocol
Web is the application. It has protocols like HTML. 

### HTTP overview
Web page - consists of objects
Object - a file

HTTP uses TCP as underlying transport protocol. The server does not store any state information of the client (stateless protocol). Once HTTP sends out the message from the app, it goes into TCP's hands so no longer Application layer. 

Nonpersistent connection walk through. Assume you want to send 10 images + 1 file and they are all on the same file. 

1) HTTP makes a TCP connection to the server.
2) HTTP client sends HTTP request message to the server
3) HTTP server receives the request message, retrieves the object, and sends the response back
4) HTTP tells TCP to close the TCP connection (TCP doesn't terminate until message is received)
5) HTTP client receives message. TCP terminates. 
6) Do this n more times for all the images and files. 

Quick overview on TCP 3-way handshake
1) client sends hello to server
2) server acknowledge
3) client acknowledges back to server with the HTTP request message

Persistent connection - no need for multiple connection establishments. 

### Cookies 
Because HTTP is stateless, use cookies for identity. 
a) cookie header line in HTTP response message
b) cookie header line in HTTP request message
c) cookie file kept on user end system
d) backend database of website

Walkthrough
1) Go on Amazon for the first time
2) Amazon server creates id in database indexed by identificaiton number
3) Amazon responds to browser request with HTTP response set-cookie: header with id
4) Browser gets the HTTP response with the header, saves that id number
5) Amazon now knows exactly where that cookie id visits on their site

### Web Caching/Proxy Server
Network entity that satisfies HTTP requests on behalf of origin web server. 

Has its own disk storage and keeps copies of recently requested objects in this storage. 

1) Browser has TCP connection to web cache
2) Cache checks if it has copy of object, if yes, return immediately
3) If no, open TCP connection to origin and get it
4) cache keeps that new item

Case Study With Caches:

Suppose average object size is 1 Mbits. Mbps access link (LAN to internet) is 15 Mbps. Request rate to servers is 15 requests per second. Suppose HTTP request messages are negligibly small. Suppose it takes 2 seconds from HTTP request to receiving the message. Assume 100 Mbps LAN. 

Traffic intensity within LAN:
(15 requests/sec * (1 Mbits/request))/100 Mbps = 0.15. 

Traffic intensity from Lan to Internet
(15 requests/sec * (1 Mbits/request))/15 = 1

You could upgrade the access link from 15 mbps to 100 mbps. That's costly. Instal web cache. Let the hit rate be 0.4. This means 60% of traffic goes through the link now. Since you are bypassing the internet delay of getting from web server, cache can be even faster than using plain, fast internet. 

Web servers also use conditional GET which allows cache to verify if request message is up to date. The request message needs to 1) be a GET message 2) includes an If-Modified-Since header. 

1) Proxy sends request message to web server
GET ....
HOST: ....

2) Webserver responds
HTTP/1.1 200 OK
DATE ...
Server ...
Last-Modified ...
Content-Type: ...
Data data data

3) Cache forwards and stores the object. When another browser requests the same object, server could have modified. Make an up to date check with conditional GET. 

GET ...
HOST ...
If-modified-since: ...

if not modified:
HTTP/1.1 304 Not Modified
Date ...
Server: ...
empty entity body

### HTTP/1.1 Gets Replaced
Goal is to reduce perceived latency by enabling request and response multiplexing over single TCP connection. 

HTTP 1.1 used persistent TCP connections. Only 1 TCP connection per web page. However, sending all the objects for a web page has a Head of Line blocking problem.

Head of Line blocking: one large file bottlenecks everything else

Traditional solution was to open parallel TCP connections so small objects are rendered much faster. This also had the side benefit because TCP congestion control gives every TCP connection some bandwidth. More TCP connections = more bandwidth for the same browser. 

### HTTP/2 
Break each message into small frames and interleave request and response on the same TCP connection. 

Allows for message prioritization for developers to customize priority of requests to optimize application performance. 

### HTTP/3
QUIC is a new transport protocol implemented in application layer over UDP. Has 
1) message interleaving
2) per-stream flow control
3) low-latency connection establishment

### Email
Broken up to a mail server and individual user agents that retrieve from the mail server.

Based on SMTP

### SMTP
Transfers messages from senders' mail servers to the recipients' mail servers. Legacy technology.

1) A invokes her agent for e-mail, provides B's email address, composes message, and agent sends it
2) A's agent sends the message to her mail server, put into message queue
3) SMTP, running on Alice's mail server, sees the msg in message queue. Opens TCP connection the SMTP on B's mail server.
4) Handshake
5) SMTP client sends A's message to B's mail server
6) B's mail server puts into B's mailbox. 

#### DNS
Give hostnames to represnt IP addresses. DNS is 1) a distributed database implemented in a hierarchy of DNS servers 2) application-layer protocol that allows hosts to query the distributed database.

Used by other app-layer protocols like HTTP and SMTP to translate hostnames to IP addresses. 

DNS servers usually have the host -> ip address so the cost is not too bad.

Other important servers

1) Host aliasing - hosts with complicated hostnames have many aliasing. DNS can track that.
2) Mail server aliases - an address can be xyz@yahoo.com but the real hostname might be relay1.west-coast.yahoo.com
3) Load distribution - DNS does load distribution 

DNS is broken up to hierarchies
Root - com DNS servers, org DNS servers, etc -> facebook.com, pbs.org, etc

#### Peer to Peer File Distribution
How bittorrent works

Each torrent has an infrastructure node called tracker. When a peer joins, it adds itself to tracker and periodically informs tracker it is still in the torrent.

When an another peer joins, it gets the list of IP addresses of a subset of peers and the new peer establishes TCP connections with them. 

the new peer then gets list of chunks that neighbors have and request chunks from those lists. To decide, get the rarest first.

To send chunks, the peer sends chunks that send at the highest rate. Also, every x amount of time, send chunks to a random neighbor. 

#### Video Streaming and CDN
Videos can be bit compressed. 

HTTP streaming - video is stored at an HTTP server as an ordinary file with specific URL. Users can get it via a GET request. Later down the line, there has been Dynamic Adaptive Streaming over HTTP (DASH). With this, each video version is stored in the HTTP server with a different url. 

CDNs solve the problem of a single data center sending the same video out wasting bandwidth to users that are far away. 

CDNs have servers in multiple locations, store copy of videos, and directs user their best locations. 

CDNs can be private CDNs (google distributing youtube) or third party. 

CDNs have 2 server placement philosophies

1) Enter deep - go deep into the networks of ISPs and get close to end users which reduce delay and increase throughput. Because it is more distributed, harder to maintain.

2) Bring home - build large cluster among a small number of sites. Lower maintenance and management at the expense of higher delay. 