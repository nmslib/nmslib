# Extending NMSLIB : Adding Spaces and Methods

## Overview

It is possible to add new spaces and search methods.
This is done in three steps, which we only outline here.
A more detailed description can be found in the following subsections.

In the first step, the user writes the code that implements a
functionality of a method or a space.
In the second step, the user writes a special helper file  
containing a method that creates a class or a method.
In this helper file, it is necessary to include
the method/space header. 

Because we tend to give the helper file the same name
as the name of header for a specific method/space,
we should not include method/space headers using quotes (in other words,
use only **angle** brackets).
The code with quoes fails to compile under the Visual Studio. 
Here is an example of a proper include-directive:

```
#include <method/vptree.h>
```

In the third step, the user adds 
the registration code to either the file 
[init_spaces.h](/similarity_search/include/factory/init_spaces.h) (for spaces)
or to the file
[init_methods.h](/similarity_search/include/factory/init_methods.h) (for methods).
This step has two sub-steps. 
First, the user includes the previously created helper file into either
`init_spaces.h` or `init_methods.h`.
Second, the function `initMethods` or `initSpaces` is extended
by adding a macro call that actually registers the space or method in a factory class.

Note that no explicit/manual modification of makefiles (or other configuration files) is required.
However, you have to re-run `cmake` each time a new source file is created (addition
of header files does not require a `cmake` run). 
This is necessary to automatically update makefiles so that they include new source files.

Is is noteworthy that all implementations of methods and spaces
are mostly template classes parameterized by the distance value type.
Recall that the distance function can return an integer (`int`), 
a single-precision (`float`), or a double-precision (`double`) real value.
The user may choose to provide specializations for all possible
distance values or decide to focus, e.g., only on integer-valued distances.

The user can also add new applications, which are meant to be 
a part of the testing framework/library.
However, adding new applications does require minor editing of the meta-makefile `CMakeLists.txt`,
deleting the cmake cache and re-running `cmake` (see ["Building the main library"](/manual/build.md)).
It is also possible to create standalone applications that use the library.

In the following subsections, 
we consider extension tasks in more detail.
For illustrative purposes,
we created a zero-functionality space [DummySpace](/similarity_search/include/space/space_dummy.h), 
method [DummyMethod](/similarity_search/include/method/dummy.h), and application [dummy_app](/similarity_search/apps/dummy_app.cc).
These zero-functionality examples can also be used as starting points to develop fully functional code.

## Benchmarking Workflow

The main benchmarking utility `experiment` parses command line parameters. 
Then, it creates a space and a search method using the space and the method factories.
Both search method and spaces can have parameters,
which are passed to the method/space in an instance
of the class `AnyParams`. 
We consider this in detail in the following sections.

When we create a class representing a search method,
the constructor of the class does not create an index in the memory.
The index is created using either the function `CreateIndex` (from scratch)
or the function `LoadIndex` (from a previously created index image).
The index can be saved to disk using the function `SaveIndex`.
Note, however, that most methods do not support index (de)-serialization.

Depending on parameters passed to the benchmarking utility, two test scenarios are possible.
In the first scenario, the user specifies separate data and test files.
In the second scenario, a test file is created by bootstrapping:
The data set is randomly divided into training and a test set.
Then,
we call the function [RunAll](/similarity_search/include/experiments.h#L63) 
and subsequently [Execute](/similarity_search/include/experiments.h#L108) for all possible test sets.

The function `Execute` is a main workhorse, which creates queries, runs searches,
produces gold standard data, and collects execution statistics.
There are two types of queries: nearest-neighbor and range queries,
which are represented by (template) classes `RangeQuery` and `KNNQuery`.
Both classes inherit from the class `Query`.
Similar to spaces, these template classes are parameterized by the type of the distance value.

Both types of queries are similar in that they implement the `Radius` function
and the functions `CheckAndAddToResult`. 
In the case of the range query, the radius of a query is constant.
However, in the case of the nearest-neighbor query,
the radius typically decreases as we compare the query
with previously unseen data objects (by calling the function `CheckAndAddToResult`).
In both cases, the value of the function `Radius` can be used to prune unpromising
partitions and data points.

This commonality between the `RangeQuery` and `KNNQuery`
allows us in many cases to carry out a nearest-neighbor query 
using an algorithm designed to answer range queries.
Thus, only a single implementation of a search method--that answers queries of both types--can be used in many cases.

A query object proxies distance computations during the testing phase.
Namely, the distance function is accessible through the function
`IndexTimeDistance`, which is defined in the class `Space`.
During the testing phase, a search method can compute a distance
only by accessing functions `Distance`, 
`DistanceObjLeft` (for left queries) and 
`DistanceObjRight` for right queries,
which are member functions of the class `Query`.
The function `Distance` accepts two parameters (i.e., object pointers) and 
can be used to compare two arbitrary objects.
The functions `DistanceObjLeft` and `DistanceObjRight` are used 
to compare data objects with the query.
Note that it is a query object  memorizes the number of distance computations.

## Creating a Space

A space is a collection of data objects.
In our library, objects are represented by instances
of the class `Object`.
The functionality of this class is limited to
creating new objects and/or their copies as well providing
access to the raw (i.e., unstructured) representation of the data
(through functions `data` and `datalength`).
We would re-iterate that currently (though this may change in the future releases),
`Object` is a very basic class that only keeps a blob of data and blob's size.
For example, the `Object` can store an array of single-precision floating point
numbers, but it has no function to obtain the number of elements.
These are the spaces that are responsible for reading objects from files,
interpreting the structure of the data blobs (stored in the `Object`), 
and computing a distance between two objects.


For dense vector spaces the easiest way to create a new space,
is to create a functor (function object class) that computes a distance.
Then, this function should be used to instantiate a template
[VectorSpaceGen](/similarity_search/include/space/space_vector_gen.h).
A sample implementation of this approach can be found
in [sample_standalone_app1.cc](/sample_standalone_app/sample_standalone_app1.cc#L114).
However, as we explain below, **additional work** is needed if the space should 
work correctly with all projection methods  or any other methods that rely on projections (e.g., OMEDRANK).

To further illustrate the process of developing a new space,
we created a sample zero-functionality space `DummySpace`.
It is represented by 
the header file 
[space\_dummy.h](/similarity_search/include/space/space_dummy.h)
and the source file
[space_dummy.cc](/similarity_search/src/space/space_dummy.cc).
The user is encouraged to study these files and read the comments.
Here we focus only on the main aspects of creating a new space.

The sample files include a template class `DummySpace`, 
which is declared and defined in the namespace `similarity`.
It is a direct ancestor of the class `Space`.

It is possible to provide the complete implementation of the `DummySpace`
in the header file. However, this would make compilation slower.
Instead, we recommend to use the mechanism of explicit template instantiation.
To this end, the user should instantiate the template in the source file
for all possible combination of parameters.
In our case, the **source** file 
[space_dummy.cc](/similarity_search/src/space/space_dummy.cc#L90)
contains the following lines:
```
template class SpaceDummy<int>;
template class SpaceDummy<float>;
template class SpaceDummy<double>;
```

Most importantly, the user needs to implement the function `HiddenDistance`,
which computes the distance between objects,
and the function `CreateObjFromStr` that creates a data point object from an instance
of a C++ class `string`.
For simplicity--even though this is not the most efficient approach--all our spaces create
objects from textual representations. However, this is not a principal limitation,
because a C++ string can hold binary data as well.
Perhaps, the next most important function is `ReadNextObjStr`, 
which reads a string representation of the next object from a file. 
A file is represented by a reference to a subclass of the class `DataFileInputState`.

Compared to some older releases, the current `Space` API is substantially more complex.
This is necessary to standardize reading/writing of generic objects.
In turn, this has been essential to implementing a generic query server.
The query server accepts data points in the same format as they are stored in a data file.
The above mentioned function `CreateObjFromStr` is used for de-serialization
of both the data points stored in a file and query data points passed to the query server. 

Additional complexity arises from the need to update space parameters after a space object is created.  
This permits a more complex storage model where, e.g., parameters are stored
in a special dedicated header file, while data points are stored elsewhere,
e.g., split among several data files. 
To support such functionality, we have a function that opens a data file (`OpenReadFileHeader`)
and creates a state object (sub-classed from `DataFileInputState`),
which keeps the current file(s) state as well as all space-related parameters. 
When we read data points using the function `ReadNextObjStr`, 
the state object is updated.
The function `ReadNextObjStr` may also read an optional external identifier for an object.
When it produces a non-empty identifier it is memorized by the query server and is further
used for query processing (see [building and using the query server](/manual/query_server.md)).
After all data points are read, this state object is supposed to be passed to the `Space` object
in the following fashion:

```
unique_ptr<DataFileInputState> 
inpState(space->ReadDataset(dataSet, externIds, fileName, maxNumRec));
space->UpdateParamsFromFile(*inpState);
```

For a more advanced implementation of the space-related functions,
please, see the file
[space_vector.cc](/similarity_search/src/space/space_vector.cc).

In addition to regular data reading/writing functionality, a space may reimplement functions
`ReadObjectVectorFromBinData` and `WriteObjectVectorBinData`. 
The default implementations of these functions save/load the array of `Object` instances
in the simple binary format. One drawback of this implementation is that it does not store
external IDs, because  no standard space currently uses them.

Remember that the function `HiddenDistance` should not be directly accessible 
by classes that are not friends of the `Space`.
As explained in the previous section,
during the indexing phase, 
`HiddenDistance` is accessible through the function
`Space::IndexTimeDistance`.
During the testing phase, a search method can compute a distance
only by accessing functions `Distance`, `DistanceObjLeft`, or
`DistanceObjRight`, which are member functions of the `Query`.
This solution is not perfect, but we are not going to change this for now. 

Should we implement a vector space that works properly with projection methods
and classic random projections, we need to define functions `GetElemQty` and `CreateDenseVectFromObj`. 
In the case of a **dense** vector space, `GetElemQty`
should return the number of vector elements stored in the object.
For **sparse** vector spaces, it should return zero. The function `CreateDenseVectFromObj`
extracts elements stored in a vector. 
For **dense** vector spaces,
it merely copies vector elements to a buffer. 
For **sparse** space vector spaces,
it should do some kind of basic dimensionality reduction. 
Currently, we do it via the hashing trick.


Importantly, we need to "tell" the library about the space,
by registering the space in the space factory.
At runtime, the space is created through a helper function.
In our case, it is called `CreateDummy`.
The function, accepts only one parameter,
which is a reference to an object of the type `AllParams`:

```
template <typename dist_t>
Space<dist_t>* CreateDummy(const AnyParams& AllParams) {
  AnyParamManager pmgr(AllParams);

  int param1, param2;

  pmgr.GetParamRequired("param1",  param1);
  pmgr.GetParamRequired("param2",  param2);

  pmgr.CheckUnused();

  return new SpaceDummy<dist_t>(param1, param2);
```

To extract parameters, the user needs an instance of the class `AnyParamManager` (see the above example).
In most cases, it is sufficient to call two functions: `GetParamOptional` and
`GetParamRequired`.
To verify that no extra parameters are added, it is recommended to call the function `CheckUnused`
(it fires an exception if some parameters are unclaimed).
This may also help to identify situations where the user misspells 
a parameter's name.

Parameter values specified in the commands line are interpreted as strings.
The `GetParam*` functions can convert these string values
to integer or floating-point numbers if necessary.
A conversion occurs, if the type of a receiving variable (passed as a second parameter
to the functions `GetParam*`) is different from a string.
It is possible to use boolean variables as parameters.
In that, in the command line, one has to specify 1 (for `true`) or 0 (for `false`).
Note that the function `GetParamRequired` raises an exception, 
if the request parameter was not supplied in the command line.

The function `CreateDummy` is registered in the space factory using a special macro.
This macro should be used for all possible values of the distance function,
for which our space is defined. For example, if the space is defined
only for integer-valued distance function, this macro should be used only once.
However, in our case the space `CreateDummy` is defined for integers,
single- and double-precision floating pointer numbers. Thus, we use this macro
three times as follows:

```
REGISTER_SPACE_CREATOR(int,    SPACE_DUMMY,  CreateDummy)
REGISTER_SPACE_CREATOR(float,  SPACE_DUMMY,  CreateDummy)
REGISTER_SPACE_CREATOR(double, SPACE_DUMMY,  CreateDummy)
```

This macro should be placed into the function `initSpaces` in the 
file [init_spaces.h](/similarity_search/include/factory/init_spaces.h#L40).
Last, but not least we need to add the include-directive
for the helper function, which creates
the class, to the file `init_spaces.h` as follows:

```
#include "factory/space/space_dummy.h"
```

To conlcude, we recommend to make a `Space` object is non-copyable. 
This can be done by using our macro `DISABLE_COPY_AND_ASSIGN`. 
 
 
## Creating a Search Method

To illustrate the basics of developing a new search method,
we created a sample zero-functionality method `DummyMethod`.
It is represented by 
the header file 
[dummy.h](/similarity_search/include/method/dummy.h)
and the source file
[dummy.cc](/similarity_search/src/method/dummy.cc).
The user is encouraged to study these files and read the comments.
Here we would omit certain details.

Similar to the space and query classes, a search method is implemented using
a template class, which is parameterized by the distance function value.
Note again that the constructor of the class does not create an index in the memory.
The index is created using either the function `CreateIndex` (from scratch)
or the function `LoadIndex` (from a previously created index image).
The index can be saved to disk using the function `SaveIndex`.
It does not have to be a comprehensive index that contains a copy of the data set.
Instead, it is sufficient to memorize only the index structure itself (because
the data set is always loaded separately).
Also note that most methods do not support index (de)-serialization.

The constructor receives a reference to a space object as well as  a reference to an array of data objects. 
In some cases, e.g., when we wrap existing methods such as the multiprobe LSH,
we create a copy of the data set (simply because is was easier to write the wrapper this way).
The framework can be informed about such a situation via the virtual function `DuplicateData`.
If this function returns true, the framework "knows" that the data was duplicated.
Thus, it can correct an estimate for the memory required by the method. 

The function `CreateIndex` receives a parameter object.
In our example, the parameter object is used to retrieve the single index-time parameter: `doSeqSearch`.
When this parameter value is true, our dummy method carries out a sequential search.
Otherwise, it does nothing useful.
Again, it is recommended to call the function `CheckUnused` to ensure
that the user did not enter parameters with incorrect names.
It is also recommended to call the function `ResetQueryTimeParams` (`this` pointer needs to
be specified explicitly here) to reset query-time parameters after the index is created (or loaded from disk).

Unlike index-time parameters, query-time parameters can be changed without rebuilding the index
by invoking the function `SetQueryTimeParams`.
The function `SetQueryTimeParams` accepts a constant reference to a parameter object.
The programmer, in turn, creates a parameter manager object to extract actual parameter values.
To this end, two functions are used: `GetParamRequired` and `GetParamOptional`.
Note that the latter function must be supplied with a mandatory **default** value for the parameter.
Thus, the parameter value is properly reset to its default value when the user does not specify the parameter value
explicitly (e.g., the parameter specification is omitted when the user invokes the benchmarking utility `experiment`)!

There are two search functions each of which receives two parameters.
The first parameter is a pointer to a query (either a range or a _k_-NN query).
The second parameter is currently unused.
Note again that during the search phase, a search method can
compute a distance only by accessing functions `Distance`, `DistanceObjLeft`, or
`DistanceObjRight`, which are member functions of a query object.
The function `IndexTimeDistance` **should not be used** in a function `Search`,
but it can be used in the function `CreateIndex`. 
If the user attempts to invoke `IndexTimeDistance` during the test phase,
**the program will terminate**.


Finally, we need to "tell" the library about the method,
by registering the method in the method factory,
similarly to registering a space.
At runtime, the method is created through a helper function,
which accepts several parameters.
One parameter is a reference to an object of the type `AllParams`.
In our case, the function name is `CreateDummy`:

```
#include <method/dummy.h>

namespace similarity {
template <typename dist_t>
Index<dist_t>* CreateDummy(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {
    return new DummyMethod<dist_t>(space, DataObjects);
}
```

There is an include-directive preceding
the creation function, which uses angle brackets.
As explained previously, if you opt to using quotes (in the include-directive),
the code may not compile under the Visual Studio.

Again, similarly to the case of the space, 
the method-creating function `CreateDummy` needs
to be registered in the method factory in two steps.
First, we need to include `dummy.h` into the file
[init\_methods.h](/similarity_search/include/factory/init_methods.h#L55) as follows:

```
#include "factory/method/dummy.h"
```

Then, this file is further modified by adding the following lines to the function `initMethods}:

```
REGISTER_METHOD_CREATOR(float,  METH_DUMMY, CreateDummy)
REGISTER_METHOD_CREATOR(double, METH_DUMMY, CreateDummy)
REGISTER_METHOD_CREATOR(int,    METH_DUMMY, CreateDummy)
```

If we want our method to work only with integer-valued distances,
we only need the following line:

```
REGISTER_METHOD_CREATOR(int,    METH_DUMMY, CreateDummy)
```

When adding the method, please, consider expanding
the test utility `test_integr`.
This is especially important if for some combination of parameters the method is expected
to return all answers (and will have a perfect recall). Then, if we break the code in the future,
this will be detected by `test_integr`.

To create a test case, the user needs to add one or more test cases 
to the file
[test\_integr.cc](/similarity_search/test/test_integr.cc#L65). 
A test case is an instance of the class `MethodTestCase`. 
It encodes the range of plausible values
for the following performance parameters: the recall,
the number of points closer to the query than the nearest returned point,
and the improvement in the number of distance computations.

## Creating an Application on Linux (inside the framework)

Imagine, we need to add an additional benchmarking/testing/etc utility
that is built as a part of NMSLIB.
First, we would create a hello-world source file 
[dummy_app.cc](/similarity_search/apps/dummy_app.cc):

```
#include <iostream>

using namespace std;
int main(void) {
  cout << "Hello world!" << endl;
}
```

Then we need to modify 
[the meta-makefile](/similarity_search/apps/CMakeLists.txt) and 
re-run `cmake` as described in ["Building the main library"](/manual/build.md).
