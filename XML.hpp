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

        XMLElement(XMLElementStruct *element) : _element(element)
        {}

    public:
        using XMLElementIterator = std::vector<XMLElement>::iterator;
        using XMLElements = std::vector<XMLElement>;

        XMLElement(std::string tag = "", std::string context = "") :
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
        void AddChild(XMLElement &child)
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

        void SetParent(XMLElement &parent)
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
        // TODO:const return can changed?
        [[nodiscard]] XMLElement FindFirstChildByTagName(const std::string &tagName) const
        {
            // TODO:标准库算法替换
            // performance compare
            for (auto &child : _element->_children)
            {
                if (child.GetTag() == tagName)
                {
                    return child;
                }
            }
            return XMLElement();
        }

        [[nodiscard]] XMLElements FindChildByTagName(const std::string &tagName) const
        {
            XMLElements children;
            for (auto &child : _element->_children)
            {
                if (child.GetTag() == tagName)
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

            XMLElementStruct(std::string tag, std::string context, XMLElementStruct *parent = nullptr) :
                    _tag(std::move(tag)), _context(std::move(context)), _parent(parent)
            {}

            std::map<std::string, std::string> _attributes;
            std::string _tag, _context;
            XMLElements _children;
            XMLElementStruct *_parent;
        };

        XMLElementStruct *_element;
    };


    class XMLParserResult;

    constexpr std::string_view SymbolNotUsedInName = R"(!"#$%&'()*+,/;<=>?@[\]^`{|}~ )";
    constexpr std::string_view Blank = "\t\r\n ";

    class XMLParser
    {
    public:
        enum ParseStatus
        {
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
            std::string_view view = contents;
            return _Parse(view);
        }

        XMLElement ParseString(std::string_view XMLString)
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

        [[nodiscard]] bool _IsNameChar(char c) const
        {
            return SymbolNotUsedInName.find(c) == std::string::npos;
        }

        // TODO:declaration
        // must have version
        // spell sensitive
        // order must be version encoding standalone
        // use " or '
        int _ParseDeclaration(std::string_view &XMLString, int index)
        {
            return index;
        }

        // in XML, \t \r \n space are blank character
        bool _IsBlankChar(char c)
        {
            return Blank.find(c) != std::string::npos;
        }

        std::string _ReadName(std::string_view contents, size_t &index)
        {
            // TODO: index = contents.size()
            auto first = index;
            while (_IsNameChar(contents[index]) && index < contents.size())
            {
                ++index;
            }
            // TODO : if == 0
            return std::string(contents.substr(first, index - first));
        }

        XMLElement _Parse(std::string_view XMLString)
        {
            auto contents = XMLString;
            auto i = contents.find_first_not_of(Blank);

            auto root = XMLElement();
            // not error, only space in file
            if (i == std::string::npos)
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

            //i = _ParseDeclaration(XMLString, i);

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
                        while (_IsBlankChar(contents[i]))
                        {
                            ++i;
                        }
                        // TODO:error process
                        tag = _ReadName(contents, i);
                        // </tag (space)* >
                        while (_IsBlankChar(contents[i]))
                        {
                            ++i;
                        }
                        if (contents[i] != '>')
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
                        if (current.GetTag().empty())
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
                        if (i == std::string::npos)
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


                    auto oldI = i;
                    tag = _ReadName(contents, i);
                    assert(oldI <= i);

                    if(i >= contents.size())
                    {
                        _status = TagBadCloseError;
                        _errorIndex = i;
                        return root;
                    }
                    if(!(_IsBlankChar(contents[i]) || contents[i] == '/' || contents[i] == '>'))
                    {
                        _status = TagSyntaxError;
                        _errorIndex = i;
                        return root;
                    }

                    tagStack.push(tag);
                    auto newNode = XMLElement(tag);

                    // Repeat read Attribute
                    while (_IsBlankChar(contents[i]))
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
                            attributeName = _ReadName(contents, i);
                            // TODO:error process
                            // read (space)*
                            while (contents[i] != '=')
                            {
                                if (contents[i] != ' ')
                                {
                                    _status = AttributeSyntaxError;
                                    _errorIndex = i;
                                    return root;
                                }
                                ++i;
                            }
                            ++i;
                            while (contents[i] != '"' && contents[i] != '\'')
                            {
                                if (contents[i] != ' ')
                                {
                                    _status = AttributeSyntaxError;
                                    _errorIndex = i;
                                    return root;
                                }
                                ++i;
                            }
                            ++i;
                            // Attribute Value
                            // not restrict
                            // TODO: " " can contain ', ' ' can contain "
                            auto firstIndex = i;
                            while (contents[i] != '"' && contents[i] != '\'')
                            {
                                ++i;
                            }
                            attributeValue = contents.substr(firstIndex, i - firstIndex);
                            // judge next char is " or '
                            ++i;

                            //repeat attribute
                            auto attr = newNode.GetAttribute();
                            if (attr.find(attributeName) != attr.end())
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
                    if (contents[i] == '/')
                    {
                        // tag end
                        ++i;
                        if (contents[i] == '>')
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
                    else if (contents[i] == '>')
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
                    auto firstIndex = i;
                    // read until '<' (next tag start)
                    // TODO: escape characters
                    // TODO: all while read, judge i < contents.size() | refactor
                    auto textLast = contents.find('<', i);
                    if(textLast == std::string::npos)
                    {
                        _status = TagNotMatchedError;
                        return root;
                    }
                    auto firstCharIndex = contents.find_first_not_of(Blank, i);
                    // TODO:npos compare
                    // if find and before testLast, then set text, otherwise set text ""
                    if(firstCharIndex != std::string::npos && firstCharIndex < textLast)
                    {
                        current.SetContext(std::string(contents.substr(firstIndex, textLast - firstIndex)));
                    }
                    else
                    {
                        current.SetContext("");
                    }
                    i = textLast;
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

        [[nodiscard]] std::string ErrorInfo() const
        {
            std::string errorName;
            switch (_status)
            {
                case XMLParser::NoError:
                    errorName = "NoError";
                    break;
                case XMLParser::FileOpenFailed:
                    errorName = "FileOpenFailed";
                    break;
                case XMLParser::TagSyntaxError:
                    errorName = "TagSyntaxError";
                    break;
                case XMLParser::TagBadCloseError:
                    errorName = "TagBadCloseError";
                    break;
                case XMLParser::NullTagError:
                    errorName = "NullTagError";
                    break;
                case XMLParser::CommentSyntaxError:
                    errorName = "CommentSyntaxError";
                    break;
                case XMLParser::TagNotMatchedError:
                    errorName = "TagNotMatchedError";
                    break;
                case XMLParser::AttributeSyntaxError:
                    errorName = "AttributeSyntaxError";
                    break;
                case XMLParser::AttributeRepeatError:
                    errorName = "AttributeRepeatError";
                    break;
                case XMLParser::DeclarationSyntaxError:
                    errorName = "DeclarationSyntaxError";
                    break;
                case XMLParser::DeclarationPositionError:
                    errorName = "DeclarationPositionError";
                    break;
            }
            return errorName;
        }

        XMLParser::ParseStatus _status;
        int _errorIndex;
    };

    // TODO: LoadFile judge is xml file??
    class XMLDocument : public XMLElement
    {
    public:
        XMLParserResult LoadFile(const std::string &fileName)
        {
            XMLParser parser;
            auto root = parser.ParseFile(fileName);
            // except children, other member don't changed
            _element->_children = root.Children();
            return XMLParserResult(parser.Status(), parser.ErrorIndex());
        }

        XMLParserResult LoadString(const std::string &str)
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
