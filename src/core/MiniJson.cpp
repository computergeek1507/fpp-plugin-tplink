#include "MiniJson.h"

#include <cctype>
#include <cstdint>
#include <cstring>
#include <utility>

namespace tplink {
namespace json {

Value const* Value::member(char const* key) const {
    if (type != Type::Object) {
        return nullptr;
    }
    for (size_t i = keys.size(); i-- > 0;) {
        if (keys[i] == key) {
            return &values[i];
        }
    }
    return nullptr;
}

Value const* Value::element(size_t index) const {
    return type == Type::Array && index < values.size() ? &values[index] : nullptr;
}

std::string Value::asString() const {
    return type == Type::String ? text : std::string();
}

namespace {

class Reader {
public:
    explicit Reader(std::string const& text) : m_text(text) {}

    bool read(Value& out) {
        skipSpace();
        return readValue(out, 0);
    }

private:
    static constexpr int kMaxDepth = 64;

    bool atEnd() const { return m_pos >= m_text.size(); }
    char peek() const { return atEnd() ? '\0' : m_text[m_pos]; }

    void skipSpace() {
        while (!atEnd()) {
            const char c = m_text[m_pos];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                return;
            }
            ++m_pos;
        }
    }

    bool consume(char c) {
        if (atEnd() || m_text[m_pos] != c) {
            return false;
        }
        ++m_pos;
        return true;
    }

    bool consumeWord(char const* word) {
        const size_t length = std::strlen(word);
        if (m_text.compare(m_pos, length, word) != 0) {
            return false;
        }
        m_pos += length;
        return true;
    }

    bool readValue(Value& out, int depth) {
        if (depth > kMaxDepth || atEnd()) {
            return false;
        }
        switch (m_text[m_pos]) {
        case '{':
            return readObject(out, depth);
        case '[':
            return readArray(out, depth);
        case '"':
            out.type = Value::Type::String;
            return readString(out.text);
        case 't':
            out.type = Value::Type::Boolean;
            out.text = "true";
            return consumeWord("true");
        case 'f':
            out.type = Value::Type::Boolean;
            out.text = "false";
            return consumeWord("false");
        case 'n':
            out.type = Value::Type::Null;
            return consumeWord("null");
        default:
            return readNumber(out);
        }
    }

    bool readObject(Value& out, int depth) {
        out.type = Value::Type::Object;
        ++m_pos;
        skipSpace();
        if (consume('}')) {
            return true;
        }
        for (;;) {
            skipSpace();
            std::string key;
            if (peek() != '"' || !readString(key)) {
                return false;
            }
            skipSpace();
            if (!consume(':')) {
                return false;
            }
            skipSpace();
            Value value;
            if (!readValue(value, depth + 1)) {
                return false;
            }
            out.keys.push_back(std::move(key));
            out.values.push_back(std::move(value));
            skipSpace();
            if (consume('}')) {
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }
    }

    bool readArray(Value& out, int depth) {
        out.type = Value::Type::Array;
        ++m_pos;
        skipSpace();
        if (consume(']')) {
            return true;
        }
        for (;;) {
            skipSpace();
            Value value;
            if (!readValue(value, depth + 1)) {
                return false;
            }
            out.values.push_back(std::move(value));
            skipSpace();
            if (consume(']')) {
                return true;
            }
            if (!consume(',')) {
                return false;
            }
        }
    }

    bool readNumber(Value& out) {
        const size_t start = m_pos;
        size_t digits = 0;
        while (!atEnd()) {
            const char c = m_text[m_pos];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                ++digits;
            } else if (c != '-' && c != '+' && c != '.' && c != 'e' && c != 'E') {
                break;
            }
            ++m_pos;
        }
        if (digits == 0) {
            return false;
        }
        out.type = Value::Type::Number;
        out.text = m_text.substr(start, m_pos - start);
        return true;
    }

    bool readHex4(uint32_t& value) {
        if (m_text.size() - m_pos < 4) {
            return false;
        }
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = m_text[m_pos++];
            value <<= 4;
            if (c >= '0' && c <= '9') {
                value |= static_cast<uint32_t>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                value |= static_cast<uint32_t>(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                value |= static_cast<uint32_t>(c - 'A' + 10);
            } else {
                return false;
            }
        }
        return true;
    }

    static void appendUtf8(std::string& out, uint32_t cp) {
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    bool readEscape(std::string& out) {
        if (atEnd()) {
            return false;
        }
        const char escaped = m_text[m_pos++];
        switch (escaped) {
        case '"':
        case '\\':
        case '/':
            out.push_back(escaped);
            return true;
        case 'b':
            out.push_back('\b');
            return true;
        case 'f':
            out.push_back('\f');
            return true;
        case 'n':
            out.push_back('\n');
            return true;
        case 'r':
            out.push_back('\r');
            return true;
        case 't':
            out.push_back('\t');
            return true;
        case 'u': {
            uint32_t cp = 0;
            if (!readHex4(cp)) {
                return false;
            }
            if (cp >= 0xD800 && cp <= 0xDBFF && m_text.compare(m_pos, 2, "\\u") == 0) {
                const size_t save = m_pos;
                m_pos += 2;
                uint32_t low = 0;
                if (readHex4(low) && low >= 0xDC00 && low <= 0xDFFF) {
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                } else {
                    m_pos = save;
                }
            }
            appendUtf8(out, cp);
            return true;
        }
        default:
            return false;
        }
    }

    bool readString(std::string& out) {
        ++m_pos;  // the opening quote
        out.clear();
        while (!atEnd()) {
            const char c = m_text[m_pos++];
            if (c == '"') {
                return true;
            }
            if (c != '\\') {
                out.push_back(c);
            } else if (!readEscape(out)) {
                return false;
            }
        }
        return false;
    }

    std::string const& m_text;
    size_t m_pos = 0;
};

}  // namespace

bool parse(std::string const& text, Value& out) {
    Reader reader(text);
    return reader.read(out);
}

}  // namespace json
}  // namespace tplink
