namespace java edu.cmu.lti.oaqa.similarity
namespace cpp similarity

struct ReplyEntry {
  1: required i32 id ;   // an unique integer object ID
  2: required double dist ; // the distance to the object from the query
  3: optional string externId; // external (not necessarily unique) ID
  4: optional binary obj ;  // an optional binary/string representation of the answer object
}

typedef list<ReplyEntry> ReplyEntryList
typedef list<list<ReplyEntry>> ReplyEntryListBatch

exception QueryException {
    1: string message;
}

service QueryService {
  /*
   * This function allows one to set query-time parameters.
   * Currently, this has a global effect for all subsequent queries.
   * Therefore, this call shouldn't overlap in time with search.
   * Note that the call *IS* thread-safe, however, different
   * threads concurrently calling this method will merely override 
   * same global variables. As a result, we will see the settings
   * related to the last call of this function.
   */
  void setQueryTimeParams(1: required string queryTimeParams)
  throws (1: QueryException err),
  ReplyEntryList knnQuery(1: required i32 k,           // k as in k-NN
                          2: required binary queryObj, // a binary/string representation of a query object 
                          3: required bool retExternId,// if true, we will return an external ID
                          4: required bool retObj)     // if true, we will return a string representation of each answer object
  throws (1: QueryException err),
  ReplyEntryList rangeQuery(1: required double r,      // a range value in the range search
                          2: required binary queryObj, // a binary/string representation of a query object 
                          3: required bool retExternId,// if true, we will return an external ID
                          4: required bool retObj)     // if true, we will return a string representation of each answer object
  throws (1: QueryException err),
  ReplyEntryListBatch knnQueryBatch(1: required i32 k,           // k as in k-NN
                                    2: required list<binary> queryObj, // a list of binary/string representation of a query object
                                    3: required bool retExternId, // if true, we will return an external ID
                                    4: required bool retObj, // if true, we will return a string representation of each answer object
                                    5: required i32 numThreads) // number of threads for batched operations
  throws (1: QueryException err),

  /*
   * Compute the distance between two objects represented as binaries. 
   * This function is intended to be used for debugging purproses.
   */
  double getDistance(1: required binary obj1,
                     2: required binary obj2)
  throws (1: QueryException err)
}
