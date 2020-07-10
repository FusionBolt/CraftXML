//
// Created by fusionbolt on 2020/7/5.
//

#ifndef CRAFT_XML_HPP
#define CRAFT_XML_HPP

#include <string>
#include <stack>
#include <iostream>
#include <map>
#include <utility>
#include <vector>
#include <fstream>
#include <sstream>
#include <cassert>

namespace Craft
{
    class XMLParser;

    class XMLElement
    {
    protected:
        class XMLElementStruct;
        friend class XMLParser;
        XMLElement(XMLElementStruct* element):_element(element){}

    public:
        using XMLElementIterator = std::vector<XMLElement>::iterator;
        using XMLElements = std::vector<XMLElement>;

        XMLElement(std::string tag = "", std::string context = ""):
                _element(new XMLElementStruct(std::move(tag), std::move(context), nullptr))
        {}

        virtual ~XMLElement() = default;

        XMLElementIterator begin() noexcept
        {
            return _element->_children.begin();
        }

        XMLElementIterator end() noexcept
        {
            return _element->_children.end();
        }

        [[nodiscard]] std::string GetTag() const
        {
            return _element->_tag;
        }

        [[nodiscard]] std::string GetContext() const
        {
            return _element->_context;
        }

        [[nodiscard]] std::map<std::string, std::string> GetAttribute() const
        {
            return _element->_attributes;
        }

        [[nodiscard]] bool HasChild() const noexcept
        {
            return !_element->_children.empty();
        }

        void AddChild(XMLElement& child)
        {
            assert(child._element != nullptr);
            child.SetParent(*this);
        }

        void AddAttribute(const std::string &name, const std::string &value)
        {
            _element->_attributes[name] = value;
        }

        [[nodiscard]] std::vector<XMLElement> Children() const
        {
            return _element->_children;
        }

        void SetParent(XMLElement& parent)
        {
            assert(parent._element != nullptr);
            _element->_parent = parent._element;
            parent._element->_children.push_back(*this);
        }

        XMLElement GetParent() const noexcept
        {
            return XMLElement(_element->_parent);
        }

        void SetContext(std::string context)
        {
            _element->_context = std::move(context);
        }

        [[nodiscard]] XMLElement FirstChild() const
        {
            return _element->_children[0];
        }

        [[nodiscard]] XMLElements FindChildByTagName(const std::string& tagName) const
        {
            XMLElements children;
            for(auto& child : _element->_children)
            {
                if(child.GetTag() == tagName)
                {
                    children.push_back(child);
                }
            }
            return children;
        }

        [[nodiscard]] std::string StringValue() const
        {
            return _element->_context;
        }

        void output() const noexcept
        {
            for (auto &c : _element->_children)
            {
                std::cout << "<" << c._element->_tag << ">" << std::endl;
                std::cout << c._element->_context << std::endl;
                c.output();
                std::cout << "<\\" << c._element->_tag << ">" << std::endl;
            }
        }

    protected:
        struct XMLElementStruct
        {
            XMLElementStruct() = default;

            XMLElementStruct(std::string tag, std::string context, XMLElementStruct* parent):
                    _tag(std::move(tag)), _context(std::move(context)), _parent(parent)
            {}

            std::map<std::string, std::string> _attributes;
            std::string _tag, _context;
            XMLElements _children;
            XMLElementStruct *_parent;
        };

        XMLElementStruct* _element;
    };

    class XMLParser
    {
    public:
        enum ParseStatus {
            NoError = 0,
            FileOpenFailed,
            TagSyntaxError,
            TagBadCloseError,
            NullTagError,
            CommentError,
            TagNestedError,
            AttributeSyntaxError,
        };

        XMLParser() = default;

        XMLElement ParseFile(const std::string &fileName)
        {
            std::ifstream file(fileName, std::ios::in);
            if (!file.is_open())
            {
                _status = FileOpenFailed;
                return XMLElement();
            }
            std::string contents((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
            return _Parse(contents);
        }

        XMLElement ParseString(const std::string &XMLString)
        {
            return _Parse(XMLString);
        }

        ParseStatus Status()
        {
            return _status;
        }

    private:
        ParseStatus _status = NoError;

        XMLElement _Parse(const std::string &XMLString)
        {
            auto contents = XMLString;
            auto i = contents.find_first_not_of(' ');

            //TODO: XML declaration Parse
            auto root = XMLElement();
            if (contents[i] != '<')
            {
                _status = TagSyntaxError;
                return root;
            }
            std::stack<std::string> tagStack;
            auto current = root;
            while (i < contents.size())
            {
                std::string tag;
                std::string text;
                // tag start
                if (contents[i] == '<')
                {
                    ++i;
                    // end tag
                    if (contents[i] == '/')
                    {
                        ++i;
                        while (contents[i] != '>')
                        {
                            tag.push_back(contents[i]);
                            ++i;
                        }
                        auto stackTop = tagStack.top();
                        // std::cout << "last:" << tag << " stackTop:" << stackTop << std::endl;
                        if (tag != stackTop)
                        {
                            _status = TagNestedError;
                            // Boom("</tag> error", i);
                            return root;
                        }
                        tagStack.pop();
                        ++i;
                        if(current.GetTag().empty())
                        {
                            _status = NullTagError;
                            return root;
                        }
                        if (current.HasChild())
                        {
                            // TODO:Bug ,not leaf node also can have context
                            // if not leaf node, then clear context
                            // std::cout << "current text:" << current->GetContext() << ":end" << std::endl;
                            current.SetContext("");
                        }
                        current = current.GetParent();
                        continue;
                    }

                    // comment
                    if (contents[i] == '!')
                    {
                        auto comment = contents.find("<!--", i - 1) + 4;
                        if (comment == std::string::npos)
                        {
                            _status = CommentError;
                            //Boom("comment syntax error", i);
                            return root;
                        }
                        i = contents.find("-->", comment) + 3;
                        // if nops
                        ++i;
                        if (i >= contents.size())
                        {
                            break;
                        }
                    }

                    // read start tag name
                    while (contents[i] != '>' && contents[i] != ' ')
                    {
                        if(i > contents.size())
                        {
                            _status = TagBadCloseError;
                            return root;
                        }
                        //TODO: tag name can't have symbol
                        if (contents[i] == '<')
                        {
                            _status = TagSyntaxError;
                            // Boom("syntax error, double <", i);
                            return root;
                        }
                        tag.push_back(contents[i]);
                        ++i;
                    }

                    tagStack.push(tag);
                    // std::cout << "tag:" << tag << std::endl;
                    auto newNode = XMLElement(tag);
                    // TODO: Attribute can have space
                    // TODO: Attribute is unique
                    // Repeat read Attribute
                    while(contents[i] == ' ')
                    {
                        std::string attributeName;
                        std::string attributeValue;
                        // read space between tag and attribute name
                        while (contents[i] == ' ')
                        {
                            ++i;
                        }
                        if (contents[i] != '>')
                        {
                            // Attribute Name
                            while (contents[i] != '=')
                            {
                                attributeName.push_back(contents[i]);
                                ++i;
                            }
                            ++i;

                            // Attribute Value
                            if (contents[i] == '"')
                            {
                                ++i;
                                while (contents[i] != '"')
                                {
                                    attributeValue.push_back(contents[i]);
                                    ++i;
                                }
                                ++i;
                            }
                            else
                            {
                                _status = AttributeSyntaxError;
                                // Boom("attribute syntax error");
                                return root;
                            }
                            // TODO:repeat attribute is error
                            newNode.AddAttribute(attributeName, attributeValue);
                        }
                        else
                        {
                            _status = AttributeSyntaxError;
                            return root;
                        }
                    }

                    // tag end by >
                    if(contents[i] == '>')
                    {
                        current.AddChild(newNode);
                        current = newNode;
                        ++i;
                    }
                    else
                    {
                        // > can't match
                        _status = TagBadCloseError;
                        return root;
                    }

                }
                    // content start
                else
                {
                    // read until '<' (tag start)
                    // TODO: if contents contain '<'
                    // TODO: all while read, judge i < contents.size() | refactor
                    while (contents[i] != '<' && i < contents.size())
                    {
                        text.push_back(contents[i]);
                        ++i;
                    }
//                    if(text.empty())
//                    {
//                        _status = NullTagError;
//                        //Boom("null tag");
//                        return root;
//                    }
                    std::cout << "text:" << text << std::endl;
                    current.SetContext(text);
                }
            }

            if (current._element != root._element)
            {
                _status = TagNestedError;
                // Boom("tag level mismatch");
                return root;
            }

            // TODO: 转换为布尔值判断，是否为空
            root.SetContext("");
            assert(root.GetParent()._element == nullptr);
            assert(root.GetTag().empty());
            assert(root.GetContext().empty());
            assert(root.GetAttribute().empty());
            return root;
        }

    };

    class XMLDocument : public XMLElement
    {
    public:
        XMLParser::ParseStatus LoadFile(const std::string& fileName)
        {
            XMLParser parser;
            auto root = parser.ParseFile(fileName);
            // except children, other member don't changed
            _element->_children = root.Children();
            return parser.Status();
        }

        XMLParser::ParseStatus LoadString(const std::string& str)
        {
            XMLParser parser;
            auto root = parser.ParseString(str);
            _element->_children = root.Children();
            return parser.Status();
        }

        void Write(XMLElement root);
    };
}
#endif //CRAFT_XML_HPP
