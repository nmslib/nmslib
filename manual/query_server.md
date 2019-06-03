## Query Server (Linux/Mac)

The query server requires Apache Thrift. We used Apache Thrift 0.11.0.
If a newer version is used, one may need to regenerate both the client and the source code.  
To install Apache Thrift on Mac we can use brew:
```
brew install thrift
```
On Linux, to install a recent version, you need to [build it from source](https://thrift.apache.org/manual/BuildingFromSource).
This may require additional libraries. On Ubuntu they can be installed as follows:
```
sudo apt-get install libboost-dev libboost-test-dev libboost-program-options-dev libboost-system-dev libboost-filesystem-dev libevent-dev automake libtool flex bison pkg-config g++ libssl-dev libboost-thread-dev make
```

After Apache Thrift is installed, you need to build the library itself. Then, change the directory
to [query_server/cpp_client_server](query_server/cpp_client_server) and type ``make`` (the makefile may need to be modified,
if Apache Thrift is installed to a non-standard location).
The query server has a similar set of parameters to the benchmarking utility ``experiment``.  For example,
you can start the server as follows:
```
 ./query_server -i ../../sample_data/final8_10K.txt -s l2 -m sw-graph -c NN=10,efConstruction=200,initIndexAttempts=1 -p 10000
```
There are also three sample clients implemented in [C++](query_server/cpp_client_server), [Python](query_server/python_client/),
and [Java](query_server/java_client/). 
A client reads a string representation of a query object from the standard stream.
The format is the same as the format of objects in a data file. 
Here is an example of searching for ten vectors closest to the first data set vector (stored in row one) of a provided sample data file:
```
export DATA_FILE=../../sample_data/final8_10K.txt
head -1 $DATA_FILE | ./query_client -p 10000 -a localhost  -k 10
```
It is also possible to generate client classes for other languages supported by Thrift from [the interface definition file](query_server/protocol.thrift), e.g., for C#. To this end, one should invoke the thrift compiler as follows:
```
thrift --gen csharp  protocol.thrift
```
For instructions on using generated code, please consult the [Apache Thrift tutorial](https://thrift.apache.org/tutorial/).