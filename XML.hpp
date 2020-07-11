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
                _element(new XMLElementStruct(std::move(tag), std::move(context)))
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

        // TODO:element not nullptr and parent nullptr, what should I do
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

        [[nodiscard]] XMLElement GetParent() const noexcept
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

        // TODO:重载[]
        XMLElement FindFirstChildByTagName(const std::string& tagName) const
        {
            // TODO:标准库算法替换
            for(auto& child : _element->_children)
            {
                if(child.GetTag() == tagName)
                {
                    return child;
                }
            }
            return XMLElement();
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

            XMLElementStruct(std::string tag, std::string context, XMLElementStruct* parent = nullptr):
                    _tag(std::move(tag)), _context(std::move(context)), _parent(parent)
            {}

            std::map<std::string, std::string> _attributes;
            std::string _tag, _context;
            XMLElements _children;
            XMLElementStruct *_parent;
        };

        XMLElementStruct* _element;
    };


    class XMLParserResult;

    class XMLParser
    {
    public:
        enum ParseStatus {
            NoError = 0,
            FileOpenFailed,
            TagSyntaxError,
            TagBadCloseError,
            NullTagError,
            CommentSyntaxError,
            TagNotMatchedError,
            AttributeSyntaxError,
            AttributeRepeatError,
            DeclarationSyntaxError,
            DeclarationPositionError
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

        int ErrorIndex()
        {
            return _errorIndex;
        }

    private:
        ParseStatus _status = NoError;

        int _errorIndex = -1;

        struct XMLDeclarationInfo
        {
            XMLDeclarationInfo() = default;

            std::string _version, _encoding, _standalone;
            XMLDeclarationInfo(std::string version, std::string encoding, std::string standalone)
            {

            }
        };

        XMLDeclarationInfo _declarationInfo;

        bool _IsNameChar(char c) const
        {
            // TODO:constexpr and Blank
            // TODO:change judge is character or number or - or .
            return std::string(R"(!"#$%&'()*+,/;<=>?@[\]^`{|}~ )").find(c) == std::string::npos;
        }

        // TODO:declaration
        // must have version
        // spell sensitive
        // order must be version encoding standalone
        // use " or '
        int _ParseDeclaration(const std::string &XMLString, int index)
        {
            return index;
        }

        // in XML, \t \r \n space are blank character
        bool _IsBlankChar(char c)
        {
            return std::string("\t\r\n ").find(c) != std::string::npos;
        }

        XMLElement _Parse(const std::string &XMLString)
        {
            auto contents = XMLString;
            auto i = contents.find_first_not_of("\t\r\n ");

            auto root = XMLElement();
            // not error, only space in file
            if(i == std::string::npos)
            {
                _status = NoError;
                return root;
            }
            if (contents[i] != '<')
            {
                _status = TagSyntaxError;
                _errorIndex = i;
                return root;
            }

            i = _ParseDeclaration(XMLString, i);

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
                    // declaration must be first line which not null
                    if (contents[i] == '?')
                    {
                        _status = DeclarationPositionError;
                        return root;
                    }

                    // end tag
                    if (contents[i] == '/')
                    {
                        ++i;
                        // < (space)* /
                        while(_IsBlankChar(contents[i]))
                        {
                            ++i;
                        }
                        // while (contents[i] != '>' && contents[i] != ' ')
                        while (_IsNameChar(contents[i]))
                        {
                            tag.push_back(contents[i]);
                            ++i;
                        }
                        // </tag (space)* >
                        while(_IsBlankChar(contents[i]))
                        {
                            ++i;
                        }
                        if(contents[i] != '>')
                        {
                            _status = TagBadCloseError;
                            return root;
                        }
                        auto stackTop = tagStack.top();
                        if (tag != stackTop)
                        {
                            _status = TagNotMatchedError;
                            _errorIndex = i;
                            return root;
                        }
                        tagStack.pop();
                        ++i;
                        if(current.GetTag().empty())
                        {
                            _status = NullTagError;
                            _errorIndex = i;
                            return root;
                        }
                        current = current.GetParent();
                        continue;
                    }

                    // comment
                    if (contents[i] == '!')
                    {
                        auto comment = contents.find("<!--", i - 1);
                        if (comment == std::string::npos)
                        {
                            _status = CommentSyntaxError;
                            _errorIndex = i;
                            return root;
                        }

                        i = contents.find("-->", comment + 4);
                        if(i == std::string::npos)
                        {
                            break;
                        }
                        else
                        {
                            i = i + 3;
                        }
                        ++i;
                        if (i >= contents.size())
                        {
                            break;
                        }
                        continue;
                    }

                    // new tag start
                    // read start tag name
                    while (contents[i] != '>' && contents[i] != ' ')
                    {
                        if(i > contents.size())
                        {
                            _status = TagBadCloseError;
                            _errorIndex = i;
                            return root;
                        }
                        if (!_IsNameChar(contents[i]))
                        {
                            _status = TagSyntaxError;
                            _errorIndex = i;
                            return root;
                        }
                        tag.push_back(contents[i]);
                        ++i;
                    }
                    tagStack.push(tag);
                    // std::cout << "tag:" << tag << std::endl;
                    auto newNode = XMLElement(tag);

                    // Repeat read Attribute
                    while(_IsBlankChar(contents[i]))
                    {
                        std::string attributeName;
                        std::string attributeValue;
                        // read space between tag and attribute name
                        while (_IsBlankChar(contents[i]))
                        {
                            ++i;
                        }
                        if (contents[i] != '>' && contents[i] != '/')
                        {
                            // Attribute Name
                            // name(space)*=(space)*\"content\"
                            // while (contents[i] != '=' || contents[i] != ' ')
                            while(_IsNameChar(contents[i]))
                            {
                                attributeName.push_back(contents[i]);
                                ++i;
                            }
                            // read (space)*
                            while(contents[i] != '=')
                            {
                                if(contents[i] != ' ')
                                {
                                    _status = AttributeSyntaxError;
                                    _errorIndex = i;
                                    return root;
                                }
                                ++i;
                            }
                            ++i;
                            while(contents[i] != '"' && contents[i] != '\'')
                            {
                                if(contents[i] != ' ')
                                {
                                    _status = AttributeSyntaxError;
                                    _errorIndex = i;
                                    return root;
                                }
                                ++i;
                            }
                            ++i;
                            // TODO:内容有什么限制
                            // Attribute Value
                            while (contents[i] != '"' && contents[i] != '\'')
                            {
                                attributeValue.push_back(contents[i]);
                                ++i;
                            }
                            ++i;

                            //repeat attribute
                            auto attr = newNode.GetAttribute();
                            if(attr.find(attributeName) != attr.end())
                            {
                                _status = AttributeRepeatError;
                                _errorIndex = i;
                                return root;
                            }
                            newNode.AddAttribute(attributeName, attributeValue);
                        }
                        // scanf to tag eng
                        else
                        {
                            // to end
                            break;
                        }
                    }

                    // one tag, not one pair tag
                    if(contents[i] == '/')
                    {
                        // tag end
                        ++i;
                        if(contents[i] == '>')
                        {
                            current.AddChild(newNode);
                            tagStack.pop();
                            ++i;
                        }
                        else
                        {
                            _status = TagBadCloseError;
                            _errorIndex = i;
                            return root;
                        }
                        continue;
                    }

                    // tag end by >
                    if(contents[i] == '>')
                    {
                        current.AddChild(newNode);
                        current = newNode;
                        ++i;
                        continue;
                    }
                    else
                    {
                        // > can't match
                        _status = TagBadCloseError;
                        _errorIndex = i;
                        return root;
                    }

                }
                // content start
                else
                {
                    // read until '<' (next tag start)
                    // TODO: escape characters
                    // TODO: all while read, judge i < contents.size() | refactor
                    // TODO: about white space
                    bool setNull = true;
                    while (contents[i] != '<' && i < contents.size())
                    {
                        // TODO:different breakline symbol
                        // other space character
                        // if only blank character then context set ""
                        // TODO:compare with find_first_of("")
                        if(!_IsBlankChar(contents[i]))
                        {
                            setNull = false;
                        }
                        text.push_back(contents[i]);
                        ++i;
                    }

                    // TODO:compare with text=""; SetContext(text);
                    if(setNull)
                    {
                        current.SetContext("");
                    }
                    else
                    {
                        current.SetContext(text);
                    }

                }
            }

            if (current._element != root._element)
            {
                _status = TagNotMatchedError;
                _errorIndex = i;
                return root;
            }

            root.SetContext("");
            assert(root.GetParent()._element == nullptr);
            assert(root.GetTag().empty());
            assert(root.GetContext().empty());
            assert(root.GetAttribute().empty());
            return root;
        }

    };

    class XMLParserResult
    {
    public:
        XMLParserResult(XMLParser::ParseStatus status, int errorIndex) :
            _status(status), _errorIndex(errorIndex)
        {}
        XMLParser::ParseStatus _status;
        int _errorIndex;
    };

    // TODO: LoadFile judge is xml file??
    class XMLDocument : public XMLElement
    {
    public:
        XMLParserResult LoadFile(const std::string& fileName)
        {
            XMLParser parser;
            auto root = parser.ParseFile(fileName);
            // except children, other member don't changed
            _element->_children = root.Children();
            return XMLParserResult(parser.Status(), parser.ErrorIndex());
        }

        XMLParserResult LoadString(const std::string& str)
        {
            XMLParser parser;
            auto root = parser.ParseString(str);
            _element->_children = root.Children();
            return XMLParserResult(parser.Status(), parser.ErrorIndex());
        }

        void Write(XMLElement root);
    };
}
#endif //CRAFT_XML_HPP
