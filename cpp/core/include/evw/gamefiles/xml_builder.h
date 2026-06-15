// Small shared helper for building indented XML, mirroring the tag/value helpers
// used across CodeWalker's MetaXmlBase. Header-only to keep it dependency-free.
#pragma once

#include <cstdio>
#include <sstream>
#include <string>

namespace evw::gamefiles::xml
{
    inline std::string escape(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            switch (c)
            {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c; break;
            }
        }
        return out;
    }

    inline std::string indent(int level) { return std::string(static_cast<size_t>(level) * 2, ' '); }

    inline std::string fmtFloat(float f)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(f));
        return buf;
    }

    // A tiny fluent writer over an ostringstream.
    class Writer
    {
    public:
        void header() { os_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"; }

        void open(int level, const std::string& tag)
        {
            os_ << indent(level) << "<" << tag << ">\n";
        }
        void openAttrs(int level, const std::string& tag, const std::string& attrs)
        {
            os_ << indent(level) << "<" << tag << " " << attrs << ">\n";
        }
        void close(int level, const std::string& tag)
        {
            os_ << indent(level) << "</" << tag << ">\n";
        }
        // Self-closing element with raw attribute text.
        void selfClose(int level, const std::string& tag, const std::string& attrs)
        {
            os_ << indent(level) << "<" << tag << " " << attrs << " />\n";
        }
        void valueTag(int level, const std::string& tag, const std::string& value)
        {
            os_ << indent(level) << "<" << tag << " value=\"" << value << "\" />\n";
        }
        void stringTag(int level, const std::string& tag, const std::string& text)
        {
            os_ << indent(level) << "<" << tag << ">" << escape(text) << "</" << tag << ">\n";
        }
        void comment(int level, const std::string& text)
        {
            os_ << indent(level) << "<!-- " << text << " -->\n";
        }
        void raw(const std::string& text) { os_ << text; }

        std::string str() const { return os_.str(); }

    private:
        std::ostringstream os_;
    };
}
