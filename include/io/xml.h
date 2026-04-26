#ifndef XML_H
#define XML_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace base {
namespace io {

class XmlDocument {
public:
    class Node {
    public:
        Node();
        Node(const std::string& name);
        Node(const std::string& name, const std::string& text);

        std::string getName() const;
        void setName(const std::string& name);

        std::string getText() const;
        void setText(const std::string& text);

        std::string getAttribute(const std::string& name) const;
        void setAttribute(const std::string& name, const std::string& value);
        bool hasAttribute(const std::string& name) const;
        void removeAttribute(const std::string& name);

        Node addChild(const std::string& name);
        Node addChild(const std::string& name, const std::string& text);
        std::vector<Node> getChildren(const std::string& name = "") const;
        bool hasChildren() const;
        void removeChild(const Node& child);
        void clearChildren();

        std::string toString(int indent = 0) const;

    private:
        std::string m_name;
        std::string m_text;
        std::map<std::string, std::string> m_attributes;
        std::vector<Node> m_children;
    };

    static XmlDocument load(const std::string& filePath);
    static XmlDocument parse(const std::string& xmlStr);

    XmlDocument();
    XmlDocument(const std::string& rootName);

    bool save(const std::string& filePath) const;
    std::string toString() const;

    Node getRoot() const;
    void setRoot(const Node& root);

    bool validate() const;

private:
    Node m_root;

    void parseElement(const std::string& xmlStr, size_t& pos, Node& node);
    void parseAttributes(const std::string& xmlStr, size_t& pos, Node& node);
    std::string encodeXml(const std::string& str) const;
    std::string decodeXml(const std::string& str) const;
    void skipWhitespace(const std::string& xmlStr, size_t& pos);
};

} // namespace io
} // namespace base

#endif // XML_H