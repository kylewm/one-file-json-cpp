JsonParser
==========

A quick-and-dirty one-file reader/writer for a JSON-formatted string. 
It has been lightly tested (parsing more so than formatting). This
example demonstrates how to read a top-level Object, grab a list from
one of the properties, and traverse the list of numbers.

Attempts to adhere to the JSON standard with the addition of C++-style
comments (prefixed with //). JsonParser was originally written to read
configuration files -- while there should not be any impediments to
performance, it has not been tested with performance in mind.

```cpp
#include "JsonParser.h"

Json::Object* root
if (!Json::read(string, root)) {
  // inform error
}

Json::Array* list;
if (!root->get(propertyName, list)) {
  // inform error
}

for (size_t ii = 0 ; ii < list->size() ; ++ii) {
  Json::Number* val;
  if(!list->get(ii, val)) {
    // inform error
  }
  cout << val->value() << endl;
}

delete root;
```
