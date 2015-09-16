namespace java edu.cmu.lti.oaqa.similarity
namespace cpp similarity

struct ReplyEntry {
  1: required string id ;   // an object ID
  2: required double dist ; // the distance to the object from the query
  3: optional string obj ;  // an optional string representation of the answer object
}

typedef list<ReplyEntry> ReplyEntryList

exception QueryException {
    1: string message;
}

service QueryService {
  ReplyEntryList knnQuery(1: required i32 k,           // k as in k-NN
                          2: required string queryObj, // a string representation of a query object 
                          3: required bool retObj)     // if true, we will return a string representation of each answer object
  throws (1: QueryException err)
}
