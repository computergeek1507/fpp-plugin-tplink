#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Just enough JSON to read ids out of a Kasa get_sysinfo reply. Nothing here
// depends on FPP.
namespace tplink {
namespace json {

struct Value {
    enum class Type { Null, Boolean, Number, String, Array, Object };

    Type type = Type::Null;
    std::string text;               // a String's contents, or a Number's or Boolean's literal
    std::vector<std::string> keys;  // an Object's keys, in step with `values`
    std::vector<Value> values;      // an Array's elements, or an Object's values

    // An Object's member, or null. On a repeated key the last one wins, as in jsoncpp.
    Value const* member(char const* key) const;
    // An Array's element, or null.
    Value const* element(size_t index) const;
    // A String's contents; empty for any other type.
    std::string asString() const;
};

// Reads the first JSON value in `text`. As with jsoncpp's defaults, anything
// after it is ignored.
bool parse(std::string const& text, Value& out);

}  // namespace json
}  // namespace tplink
