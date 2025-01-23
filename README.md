DataSpinner - a framework to compare different protocols for distribution of market data.

## Market data
It is a set of structures and related protocols designed to maximize throughput and minimize latency of data. Data structures have to be flexible and easy to adapt of changing requirements, also they have to be as fast as possible. We will review data structures later.

## Protocols 
There are just two actually - TCP and UDP. But there are variations in a way they are used. For example there are several middle ware solution, designed for robotics and industrial applications. They allow communications with some guarantied Quality of Service (QoS) over UDP channels which looks particularly interesting in building distributed scalable systems – a bigger systems like exchanges, order management and pricing distributions or smaller ones that need to grow.
A few more words about protocols - TCP is point to point protocol (p2p), if we decided to build infrastructure on p2p it has to be graph and it will get complicated and hard to maintain fast. UDP on other hand allow building BUS kind of systems, where new nodes can be easily added, it is much easier to maintain and extend, it is scalable. If UDP-based protocols are so good and if they can be made reliable as in one of DDS implementation - why not use them everywhere? This is the question. Maybe we can't answer it in this little research project but we can try to approach it and give rough estimate by comparing throughput&latency between two nodes on network. We will leave whole network (graph of nodes or nodes on the bus) aside for now.

## Data structures.
Finance infrastrucures are built on FIX protocol, it is simple - collection of tag/value pairs. The power of FIX it is flexible, probably we can build anything out of it. Market data protocols looks much like FIX, it's just they are binary (mostly) and pairs tag/values are specialized for types and sizes and they are grouped by instrument or symbol and they have to be fast. Usually there is reference data - static data and snapshot - current state of the symbol and delta - changes that are emitted upstream to the clients or consumer services, they are eventually merged into snapshots. In this study we will focus on deltas because they are mainly being distributed. So how can we represent delta - it should have only numerical data and maybe enums, no strings because string are stored in reference and static snapshots. Also they will have indexes, which will describe which part of snapshot particular delta fields changes. We will limit our self with just int32_t and double for values and uint16_t for field index. So each delta can be:
```
struct double_data
{
    uint16_t index;
    double value;
};

struct int_data
{
    uint16_t index;
    int32_t value;
};

and delta can be:

struct delta
{
    std::vector<double_data> dbldata;
    std::vector<int_data> intdata;
};
```
These structures have a bit problem - first of all we want to avoid heap as much as possible, so should be std::array, second these are array of structures (AOS), they are from OOP days. Now we have many confirmations that structures of arrays (SOR) are better. How better - in our case not much since index and value are always used together and sit close in memory. We benchmarked AOS and SOR in these applications (see tests folder, BM_ref.cpp) and SOR 5-8% faster than AOS on merge operations (serialize in/out are same). We benchmarked also other possible solution - like limit doubles to 4 digits after commas and pack them next to uint16_t in one uint64_t - in unions, these are slower. So we will use following structures (SOR) to represent delta:

```
template<class T, size_t N>
struct SOR_Data
{
    std::array<uint16_t, N> index;
    std::array <T, N> value;
};

struct delta
{
    SOR_Data<double, 18> dbldata;
    SOR_Data<int32_t, 6> intdata;
};
```
We use 18 doubles and 6 int in one delta, if business logic require more they have to be split. To hide developer from need to count fields we use builder classes, that are streaming above delta, it is easier to show how builders are used:
```
template<class MDELTA, class PUBLISHER>
void send_generated_delta(const std::vector<std::string>& symbols, PUBLISHER& pub, uint16_t num)
{
    MdDeltaBuilder<MDELTA, PUBLISHER> bld(pub);
    for (const auto& s : symbols)
    {
        bld.begin(s);
        for (int i = 1; i <= num; i++) {
            bld.add_intdata(i, i * 10);
        }
        for (int i = 1; i <= num; i++) {
            bld.add_dbldata(i, i * 100.1);
        }
        bld.end();
    }
};
```
The function above will send N number of deltas (whatever is needed to pack delta-structures) over provided PUBLISHER class that knows how to send data over TCP, UDP or some version of DDS. So this API eliminate need to create deltas or split them (builder internally will decide when to send delta, we just add data to it - as much as needed), just a convenience. There is also serialize class to stream structures into array of bytes and back. We used our own implementation for this (avoided protobuf or any other serialize solutions again for performance reason), see BuffMsgArchive.

## The design of the application.
At this point we described data structures that will be used to compare protocols. Now the question is what do we compare against - as a benchmark we choose TCP-based protocol because it is easier to reason about and because it is fast (more about it later). We selected libevent - very well written and widely used C-based API that abstracts asynchronous functionality - we rely upon libevent to choose epoll2 or select on particular platform and even to buffer-in receiving socket channel. We will maintain outbound queue our self to handle slow client connections.

## Server application structure. 
As simple as possible. 2 threads - one receiving, another sending. Server is busy generating ticks in this fashion:
```
void task_publisher(){
  while (1) {
    spnr::make_one_delta<spnr::MdDelta, DeltaDispatcher>(symbol, delta_dispatcher);
    if(cfg.want_slow_server()){
        this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
  }
}
```
## Client application 
It also has 2 threads - one processing ticks, another sends requests and heartbeats (HB) back. Same structure for UDP connection, except there are no buffers (at a moment), we send/receive data as they come without buffering just to see how fast can we do it. Notice it will result in packet loss. In our protocol deltas are sent with header as any other type of data and header maintains session index number and type of message, also length. Maybe we should present header here:
```
struct MsgHeader
{
    uint8_t     pfix;
    uint8_t     mtype;
    uint16_t    len;
    uint32_t    session_idx;
};

union UMsgHeader
{
    MsgHeader   h;
    uint64_t    d;
};
```
So it is 64 bytes header for each message. Also we have variety of messages - login, reference, text, intvec etc. And we have admin client and admin server port to test all those messages and serializations. But main study is about deltas and how fast can we send them around.

So here are results on 1G network with very basic Windows 11 and Ubuntu 24 machines:
```
TCP      throughput=340.00 Kt/s latency=18,000 usec
UDP(raw) throughput=100.00 Kt/s latency=200 usec
FastDDS  throughput=15 Kt/s     latency=30,000 usec
```
Notice that UDP numbers above are deceiving, the software doesn't implement re-transmissions or any buffering and packet at these speeds do get lost. TCP shows reliable data transmission and FastDDS also keeps packets in line without any gaps.

At this moment we didn't try to introduce buffering and re-transmissions in UDP and used default Quality of Service (QoS) options for FastDDS application. The IDL for DDS looks similar to SOR that we used in TCP and UDP:
```
struct IntData
{
    unsigned short index[6];
    long value[6];
};

struct DblData
{
    unsigned short index[18];
    double value[18];
};

struct StampData
{
    unsigned short index[2];
    unsigned long long value[2];
};

struct MdDelta
{
    unsigned long       session_idx;
    unsigned long long  wire_tstamp;
    string              symbol;
    IntData             int_data;
    DblData             dbl_data;
    StampData           stamps;
};
```