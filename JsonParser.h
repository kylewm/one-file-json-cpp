/*
Copyright © 2013 Kyle Mahan <kyle.mahan@gmail.com>
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.
 */

#ifndef JSONPARSER_H_
#define JSONPARSER_H_

namespace Json
{

  typedef enum
  {
    T_NUMBER, T_STRING, T_BOOLEAN, T_ARRAY, T_OBJECT
  } 
  Type;

  struct Value 
  {
    Value(){}
    virtual ~Value(){}

    virtual Type getType() const = 0;
  };

  struct Number : public Value
  {
    double v;
    Number() : v(0) {}
    Number(double in) : v(in) {}
    Number(const Number& in) : v(in.v) {}
    virtual ~Number(){}

    double value() const { return v; }

    static const Type TYPE = T_NUMBER;
    virtual Type getType() const { return TYPE; }
  };

  struct Boolean : public Value
  {
    bool v;
    Boolean() : v(false) {}
    Boolean(bool in) : v(in) {}
    Boolean(const Boolean& in) : v(in.v) {}
    virtual ~Boolean(){}

    bool value() const { return v; }

    static const Type TYPE = T_BOOLEAN;
    virtual Type getType() const { return TYPE; }
  };

  struct Array : public Value
  {
    typedef std::vector<Value*> VectorType;
    VectorType v;

    Array(){}
    Array(const VectorType& in) : v(in) {}
    Array(const Array& in) : v(in.v) {}
    virtual ~Array(){
      for (size_t ii = 0 ; ii < v.size() ; ii++)
        delete v[ii];
    }

    const VectorType& value() const { return v; }
    size_t size() const { return v.size(); }

    const Value* get(size_t idx) const
    {
      return v[idx];
    }

    template<typename _T>
    bool get(size_t idx, const _T*& out) const
    {
      const Value* e = get(idx);
      if (e->getType() == _T::TYPE)
      {
        out = static_cast<const _T*>(e);
        return true;
      }
      return false;
    }

    static const Type TYPE = T_ARRAY;
    virtual Type getType() const { return TYPE; }
  };

  struct String : public Value
  {
    std::string v;
    String(){}
    String(const char* c): v(c) {}
    String(const std::string& in): v(in) {}
    String(const String& in) : v(in.v){}
    virtual ~String(){}

    const std::string& value() const { return v; }
    static const Type TYPE = T_STRING;
    virtual Type getType() const { return TYPE; }
  };

  struct Object : public Value
  {
    typedef std::map<std::string, Value*> MapType;
    typedef std::set<std::string> KeySet;
    MapType v; 

    Object(){}
    Object(const MapType& in) : v(in) {}
    Object(const Object& in) : v(in.v) {}
    virtual ~Object(){
      for(MapType::iterator it = v.begin() ; it != v.end() ; it++)
        delete it->second;
    }

    const MapType& value() const { return v; }

    KeySet keys() const 
    {
      KeySet keys;
      for(MapType::const_iterator it = v.begin() ; it != v.end() ; it++)
      {
        keys.insert(it->first);
      }
      return keys;
    }

    Value* get(const std::string& key) const
    {
      MapType::const_iterator found = v.find(key);
      if(found != v.end())
      {
        return found->second;
      }
      return NULL;
    }

    template<typename _T>
    bool get(const std::string& key, const _T*& out) const
    {
      const Value* e = get(key);
      if (e && e->getType() == _T::TYPE) {
        out = static_cast<const _T*>(e);
        return true;
      }
      return false;
    }

    static const Type TYPE = T_OBJECT;
    virtual Type getType() const { return TYPE; }
  };



  namespace impl {

    Value* parseGeneric(char*& s);

    inline bool chompComment(char*& s)
    {
      if (*(s) == '/' && *(s + 1) == '/')
      {
        s += 2;
        while (*s != '\n')
          ++s;
        return true;
      }
      return false;
    }

    inline bool chompSpace(char*& s)
    {
      if (isspace(*s))
      {
        while (isspace(*s))
          ++s;
        return true;
      }
      return false;
    }

    inline void chomp(char*& s)
    {
      while (chompSpace(s) || chompComment(s))
        ;
    }

    inline std::string parseCharString(char*& s)
    {
      char *prev;

      std::string result; 
      result.reserve(16);

      chomp(s);

      if(*s != '"') {
        fprintf(stderr, "parseFieldName error, expected '\"'. Got: %s\n", s);
        return result;
      }

      prev = s;
      s++;

      do {
        if(*prev != '\\' && *s == '"') {
          s++;
          break;
        }

        result += *s;

        prev = s;
        s++;
      }
      while(*s != 0);

      return result;
    }

    inline Value* parseMap(char*& s)
    {
      Object::MapType result;

      chomp(s);
      if(*s != '{') {
        fprintf(stderr, "Map parse error, expected '{'. Got: %s\n", s);
        return NULL;
      }
      s++;

      while(1)
      {
        std::string field_name;
        Value* field_value;

        chomp(s);
        if(*s == '}')
          break;

        field_name = parseCharString(s);

        chomp(s);
        if(*s != ':') {
          fprintf(stderr, "Map parse error, expected separator ':'. Got: %s\n", s);
          return NULL;
        }
        s++;
        field_value = parseGeneric(s);

        result[field_name] = field_value;

        chomp(s);
        if(*s == ',')
          s++;
        else if(*s != '}')
        {
          fprintf(stderr, "Map parse error, expected ',' or '}'. Got: %s\n", s);
          return NULL;
        }
      }

      chomp(s);
      if(*s != '}') {
        fprintf(stderr, "Map parse error, expected '}'. Got: %s\n", s);
        return NULL;
      }
      s++;

      return new Object(result);
    }

    inline Value* parseList(char*& s)
    {
      Array::VectorType result;

      chomp(s);
      if(*s != '[') {
        fprintf(stderr, "List parse error, expected '['. Got: %s\n", s);
        return NULL;
      }
      s++;

      while(1)
      {
        chomp(s);
        if(*s == ']')
          break;

        Value* value = parseGeneric(s);
        result.push_back(value);

        chomp(s);

        if(*s == ',')
          s++;
        else if(*s != ']')
        {
          fprintf(stderr, "List parse error, expected separator ','. Got: %s\n", s);
          return NULL;
        }
      }

      chomp(s);
      if(*s != ']') {
        fprintf(stderr, "List parse error, expected ']'. Got: %s\n", s);
        return NULL;
      }
      s++;

      return new Array(result);
    }

    inline Value* parseString(char*& s)
    {
      std::string chars = parseCharString(s);
      return new String(chars);
    }

    static Value* parseLiteral(char*& s)
    {
      Value* result = NULL;

      char *tok_start = s;
      char* tok_end;
      char* tok;
      int tok_size;

      chomp(s);

      // s starts with - or a digit
      if(isdigit(*s) || *s == '-') {
        do {
          tok_end = s+1;
          s++;
        } while(isdigit(*s) || *s == '-' || *s == '.');
        tok_size = tok_end - tok_start;
        tok = new char[tok_size + 1];
        memcpy(tok, tok_start, tok_size);
        tok[tok_size] = '\0';
        result = new Number(atof(tok));
        delete[] tok;
      }
      // alpha
      else if(isalpha(*s)) {
        do {
          tok_end = s+1;
          s++;
        } while(isalnum(*s) || *s == '_');
        tok_size = tok_end - tok_start;
        tok = new char[tok_size + 1];
        memcpy(tok, tok_start, tok_size);
        tok[tok_size] = '\0';
        if(strcmp(tok, "true") == 0) {
          result = new Boolean(true);
        }
        else if(strcmp(tok, "false") == 0) {
          result = new Boolean(false);
        }
        else {
          fprintf(stderr, "Unrecognized literal: %s\n", s);
        }
      }
      else {
        fprintf(stderr, "Unrecognized literal: %s\n", s);
      }

      return result;
    }

    inline Value* parseGeneric(char*& s)
    {
      chomp(s);
      if(*s == '{') {
        return parseMap(s);
      }
      else if(*s == '[') {
        return parseList(s);
      }
      else if(*s == '\"') {
        return parseString(s);
      }
      else {
        return parseLiteral(s);
      }
    }

    void formatGeneric(const Value* obj, std::stringstream& out);

    inline void formatNumber(const Number* num, std::stringstream& out)
    {
      out << num->value();
    }

    inline std::string escape(std::string const &s)
    {
      std::size_t n = s.length();
      std::string escaped;
      escaped.reserve(n * 2);        // pessimistic preallocation

      for (std::size_t ii = 0; ii < n; ++ii) {
        if (s[ii] == '\\' || s[ii] == '\"')
          escaped += '\\';
        escaped += s[ii];
      }
      return escaped;
    }

    inline void formatString(const std::string& str, std::stringstream& out) 
    {
      out << "\"" << escape(str) << "\"";
    }

    inline void formatString(const String* str, std::stringstream& out)
    {
      formatString(str->value(), out);
    }

    inline void formatBoolean(const Boolean* bo, std::stringstream& out)
    {
      out << bo->value() ? "true" : "false";
    }

    inline void formatArray(const Array* arr, std::stringstream& out)
    {
      out << "[";

      for(size_t ii = 0 ; ii < arr->size() ; ++ii)
      {
        if(ii > 0) out << ", ";
        const Value* entry = arr->get(ii);
        formatGeneric(entry, out);
      }

      out << "]";
    }

    inline void formatObject(const Object* obj, std::stringstream& out)
    {
      out << "{";

      Object::KeySet keys = obj->keys();
      bool first = true;
      for(Object::KeySet::iterator it = keys.begin() ; it != keys.end() ; it++)
      {
        if (!first) out << ", ";
        const std::string& key = *it;
        formatString(key, out);
        out << " : ";
        const Value* entry = obj->get(key);
        formatGeneric(entry, out);
        first = false;
      }

      out << "}";
    }

    inline void formatGeneric(const Value* obj, std::stringstream& out)
    {
      if (!obj)
      {
        out << "null";
        return;
      }

      switch(obj->getType())
      {
      case T_NUMBER:
        formatNumber(static_cast<const Number*>(obj), out);
        break;
      case T_STRING:
        formatString(static_cast<const String*>(obj), out);
        break;
      case T_BOOLEAN:
        formatBoolean(static_cast<const Boolean*>(obj), out);
        break;
      case T_ARRAY:
        formatArray(static_cast<const Array*>(obj), out);
        break;
      case T_OBJECT:
        formatObject(static_cast<const Object*>(obj), out);
        break;
      default:
        out << "???";
      }
    }


  }; // namespace



  /* Read a JSON Value from a string of characters */
  inline Value* read(const char* s)
  {
    char* p = (char*) s;
    return impl::parseGeneric(p);
  }

  /* Read a JSON Value from a string of characters. Returns true if 
     parsing succeeds and top-level Value type matches the type of _T */
  template<typename _T>
  bool read(const char* s, const _T*& out)
  {
    Value* parsed = read(s);
    if (parsed && parsed->getType() == _T::TYPE)
    {
      out = static_cast<const _T*>(parsed);
      return true;
    }
    return false;
  }

  /* Write a JSON Value to a std::stringstream */
  inline void write(const Value* value, std::stringstream& ss) {
    impl::formatGeneric(value, ss);
  }

  /* Write a JSON Value and return a std::string containing the 
     formatted data */
  inline std::string write(const Value* value) {
    std::stringstream ss;
    write(value, ss);
    return ss.str();
  }

};

#endif /* JSONPARSER_H_ */
