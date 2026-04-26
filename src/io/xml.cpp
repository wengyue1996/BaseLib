#include "io/xml.h"
#include <fstream>
#include <sstream>
#include <cctype>

namespace base {
namespace io {

XmlDocument::Node::Node() {}

XmlDocument::Node::Node(const std::string& name) : m_name(name) {}

XmlDocument::Node::Node(const std::string& name, const std::string& text) : m_name(name), m_text(text) {}

std::string XmlDocument::Node::getName() const {
    return m_name;
}

void XmlDocument::Node::setName(const std::string& name) {
    m_name = name;
}

std::string XmlDocument::Node::getText() const {
    return m_text;
}

void XmlDocument::Node::setText(const std::string& text) {
    m_text = text;
}

std::string XmlDocument::Node::getAttribute(const std::string& name) const {
    auto it = m_attributes.find(name);
    if (it != m_attributes.end()) {
        return it->second;
    }
    return "";
}

void XmlDocument::Node::setAttribute(const std::string& name, const std::string& value) {
    m_attributes[name] = value;
}

bool XmlDocument::Node::hasAttribute(const std::string& name) const {
    return m_attributes.find(name) != m_attributes.end();
}

void XmlDocument::Node::removeAttribute(const std::string& name) {
    m_attributes.erase(name);
}

XmlDocument::Node XmlDocument::Node::addChild(const std::string& name) {
    m_children.push_back(Node(name));
    return m_children.back();
}

XmlDocument::Node XmlDocument::Node::addChild(const std::string& name, const std::string& text) {
    m_children.push_back(Node(name, text));
    return m_children.back();
}

std::vector<XmlDocument::Node> XmlDocument::Node::getChildren(const std::string& name) const {
    std::vector<Node> result;
    for (const auto& child : m_children) {
        if (name.empty() || child.getName() == name) {
            result.push_back(child);
        }
    }
    return result;
}

bool XmlDocument::Node::hasChildren() const {
    return !m_children.empty();
}

void XmlDocument::Node::removeChild(const Node& child) {
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        if (it->getName() == child.getName()) {
            m_children.erase(it);
            return;
        }
    }
}

void XmlDocument::Node::clearChildren() {
    m_children.clear();
}

static std::string encodeXmlText(const std::string& str) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find('&', pos)) != std::string::npos) {
        result.replace(pos, 1, "&amp;");
        pos += 5;
    }
    pos = 0;
    while ((pos = result.find('<', pos)) != std::string::npos) {
        result.replace(pos, 1, "&lt;");
        pos += 4;
    }
    pos = 0;
    while ((pos = result.find('>', pos)) != std::string::npos) {
        result.replace(pos, 1, "&gt;");
        pos += 4;
    }
    return result;
}

std::string XmlDocument::Node::toString(int indent) const {
    std::stringstream ss;
    for (int i = 0; i < indent; ++i) ss << "  ";
    ss << "<" << m_name;
    for (const auto& attr : m_attributes) {
        ss << " " << attr.first << "=\"" << attr.second << "\"";
    }
    if (m_text.empty() && m_children.empty()) {
        ss << " />";
    } else if (!m_text.empty() && m_children.empty()) {
        ss << ">" << encodeXmlText(m_text) << "</" << m_name << ">";
    } else {
        ss << ">";
        if (!m_text.empty()) {
            ss << encodeXmlText(m_text);
        }
        for (const auto& child : m_children) {
            ss << "\n" << child.toString(indent + 1);
        }
        if (!m_children.empty()) {
            ss << "\n";
            for (int i = 0; i < indent; ++i) ss << "  ";
        }
        ss << "</" << m_name << ">";
    }
    return ss.str();
}

XmlDocument::XmlDocument() {}

XmlDocument::XmlDocument(const std::string& rootName) : m_root(Node(rootName)) {}

XmlDocument XmlDocument::load(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return XmlDocument();
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return parse(buffer.str());
}

XmlDocument XmlDocument::parse(const std::string& xmlStr) {
    XmlDocument doc;
    size_t pos = 0;
    doc.skipWhitespace(xmlStr, pos);
    if (pos < xmlStr.size() && xmlStr[pos] == '<') {
        doc.parseElement(xmlStr, pos, doc.m_root);
    }
    return doc;
}

bool XmlDocument::save(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    file << toString();
    file.close();
    return true;
}

std::string XmlDocument::toString() const {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" + m_root.toString(0);
}

XmlDocument::Node XmlDocument::getRoot() const {
    return m_root;
}

void XmlDocument::setRoot(const Node& root) {
    m_root = root;
}

bool XmlDocument::validate() const {
    return !m_root.getName().empty();
}

static std::string decodeXmlText(const std::string& str) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find("&amp;", pos)) != std::string::npos) {
        result.replace(pos, 5, "&");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find("&lt;", pos)) != std::string::npos) {
        result.replace(pos, 4, "<");
        pos += 1;
    }
    pos = 0;
    while ((pos = result.find("&gt;", pos)) != std::string::npos) {
        result.replace(pos, 4, ">");
        pos += 1;
    }
    return result;
}

void XmlDocument::parseElement(const std::string& xmlStr, size_t& pos, Node& node) {
    skipWhitespace(xmlStr, pos);
    if (pos < xmlStr.size() && xmlStr[pos] == '<') {
        ++pos;
        std::string name;
        while (pos < xmlStr.size() && xmlStr[pos] != ' ' && xmlStr[pos] != '>' && xmlStr[pos] != '/') {
            name += xmlStr[pos++];
        }
        node.setName(name);
        skipWhitespace(xmlStr, pos);
        if (pos < xmlStr.size() && xmlStr[pos] != '>' && xmlStr[pos] != '/') {
            parseAttributes(xmlStr, pos, node);
        }
        skipWhitespace(xmlStr, pos);
        if (pos < xmlStr.size() && xmlStr[pos] == '/') {
            ++pos;
            if (pos < xmlStr.size() && xmlStr[pos] == '>') ++pos;
            return;
        }
        if (pos < xmlStr.size() && xmlStr[pos] == '>') {
            ++pos;
            std::string text;
            while (pos < xmlStr.size() && xmlStr[pos] != '<') {
                text += xmlStr[pos++];
            }
            node.setText(decodeXmlText(text));
            if (pos < xmlStr.size() && xmlStr[pos] == '<') {
                parseElement(xmlStr, pos, node);
            }
        }
        while (pos < xmlStr.size() && xmlStr[pos] != '>') ++pos;
        if (pos < xmlStr.size()) ++pos;
    }
}

void XmlDocument::parseAttributes(const std::string& xmlStr, size_t& pos, Node& node) {
    while (pos < xmlStr.size() && xmlStr[pos] != '>' && xmlStr[pos] != '/') {
        skipWhitespace(xmlStr, pos);
        std::string attrName;
        while (pos < xmlStr.size() && xmlStr[pos] != '=' && xmlStr[pos] != '>' && xmlStr[pos] != '/' && !std::isspace(xmlStr[pos])) {
            attrName += xmlStr[pos++];
        }
        skipWhitespace(xmlStr, pos);
        if (pos < xmlStr.size() && xmlStr[pos] == '=') {
            ++pos;
            skipWhitespace(xmlStr, pos);
            if (pos < xmlStr.size() && (xmlStr[pos] == '"' || xmlStr[pos] == '\'')) {
                char quote = xmlStr[pos++];
                std::string attrValue;
                while (pos < xmlStr.size() && xmlStr[pos] != quote) {
                    attrValue += xmlStr[pos++];
                }
                if (pos < xmlStr.size()) ++pos;
                node.setAttribute(attrName, attrValue);
            }
        }
        skipWhitespace(xmlStr, pos);
    }
}

void XmlDocument::skipWhitespace(const std::string& xmlStr, size_t& pos) {
    while (pos < xmlStr.size() && std::isspace(xmlStr[pos])) ++pos;
}

} // namespace io
} // namespace base