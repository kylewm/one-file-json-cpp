JsonParser
==========

author: Kyle Mahan

date: 2013-Jan-11

A quick-and-dirty parser for a JSON-formatted string. It has been
lightly tested (parsing more so than formatting). This example
demonstrates how to read a top-level Object, grab a list from one of
the properties, and traverse the list of numbers.


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