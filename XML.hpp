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

    class XMLNode
    {
    protected:
        class XMLNodeStruct;

        friend class XMLParser;

        XMLNode(std::shared_ptr<XMLNodeStruct> node) : _node(std::move(node))
        {}

    public:
        enum NodeType {
            NodeDocument,
            NodeElement,
            NodeData,
            NodeCData,
            NodeCommet, // TODO: save text and set nottype
            NodeDoctype, // TODO: do it
            NodeDeclaration, // TODO: do it
            NodePI // process instruction TODO: do it
        };

        using XMLNodeIterator = std::vector<XMLNode>::iterator;
        using XMLNodes = std::vector<XMLNode>;

        XMLNode(std::string tag = "", std::string context = "", NodeType type = NodeElement) :
                _node(new XMLNodeStruct(std::move(tag), std::move(context), type))
        {}

        XMLNode(NodeType type) : XMLNode("", "", type)
        {

        }

        virtual ~XMLNode() = default;

        XMLNodeIterator begin() noexcept
        {
            return _node->_children.begin();
        }

        XMLNodeIterator end() noexcept
        {
            return _node->_children.end();
        }

        [[nodiscard]] std::string GetTag() const
        {
            return _node->_tag;
        }

        [[nodiscard]] std::string GetContext() const
        {
            return _node->_context;
        }

        [[nodiscard]] std::map<std::string, std::string> GetAttribute() const
        {
            return _node->_attributes;
        }

        [[nodiscard]] bool HasChild() const noexcept
        {
            return !_node->_children.empty();
        }

        // TODO:element not nullptr and parent nullptr, what should I do
        void AddChild(XMLNode &child)
        {
            assert(child._node != nullptr);
            child.SetParent(*this);
        }

        void AddAttribute(const std::string &name, const std::string &value)
        {
            _node->_attributes[name] = value;
        }

        [[nodiscard]] std::vector<XMLNode> Children() const
        {
            return _node->_children;
        }

        void SetParent(XMLNode &parent)
        {
            assert(parent._node != nullptr);
            _node->_parent = parent._node;
            parent._node->_children.push_back(*this);
        }

        [[nodiscard]] XMLNode GetParent() const noexcept
        {
            return XMLNode(_node->_parent.lock());
        }

        void SetContext(std::string context)
        {
            _node->_context = std::move(context);
        }

        [[nodiscard]] XMLNode FirstChild() const
        {
            return _node->_children[0];
        }

        // TODO:重载[]
        // TODO:const return can changed?
        [[nodiscard]] XMLNode FindFirstChildByTagName(const std::string &tagName) const
        {
            // TODO:标准库算法替换
            // performance compare
            for (auto &child : _node->_children)
            {
                if (child.GetTag() == tagName)
                {
                    return child;
                }
            }
            return XMLNode();
        }

        [[nodiscard]] XMLNodes FindChildByTagName(const std::string &tagName) const
        {
            XMLNodes children;
            for (auto &child : _node->_children)
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
            return _node->_context;
        }

        void output() const noexcept
        {
            for (auto &c : _node->_children)
            {
                std::cout << "<" << c._node->_tag << ">" << std::endl;
                std::cout << c._node->_context << std::endl;
                c.output();
                std::cout << "<\\" << c._node->_tag << ">" << std::endl;
            }
        }

    protected:
        struct XMLNodeStruct
        {
            XMLNodeStruct() = default;

            XMLNodeStruct(std::string tag, std::string context, NodeType type, std::shared_ptr<XMLNodeStruct> parent = nullptr) :
                    _tag(std::move(tag)), _context(std::move(context)), _type(type), _parent(parent)
            {}

            std::map<std::string, std::string> _attributes;
            std::string _tag, _context;
            XMLNodes _children;
            NodeType _type;
            // std::shared_ptr<XMLNodeStruct> _parent;
            std::weak_ptr<XMLNodeStruct> _parent;
        };

        std::shared_ptr<XMLNodeStruct> _node;
    };


    class XMLParserResult;



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
            DeclarationPositionError,
            CDATASyntaxError,
        };
        XMLParser() = default;

        XMLNode ParseFile(const std::string &fileName)
        {
            std::ifstream file(fileName, std::ios::in);
            if (!file.is_open())
            {
                _status = FileOpenFailed;
                return XMLNode();
            }
            std::string contents((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
            std::string_view view = contents;
            return _Parse(view);
        }

        XMLNode ParseString(std::string_view XMLString)
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
        constexpr static std::string_view SymbolNotUsedInName = R"(!"#$%&'()*+,/;<=>?@[\]^`{|}~ )";
        constexpr static std::string_view Blank = "\t\r\n ";
        constexpr static std::string_view CDATA = "[CDATA[";

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
        void _ParseDeclaration(std::string_view &XMLString, size_t &index)
        {

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
            index = contents.find_first_of(SymbolNotUsedInName, index);
            if (index != std::string::npos)
            {
                return std::string(contents.substr(first, index - first));
            }
            else
            {
                return "";
            }
        }

        void _ReadBlank(std::string_view contents, size_t &index)
        {
            index = contents.find_first_not_of(Blank, index);
        }

        std::string _ReadAttributeValue(std::string_view contents, size_t &index)
        {
            auto firstQuotation = contents[index];
            if (firstQuotation != '"' && firstQuotation != '\'')
            {
                _status = AttributeSyntaxError;
                _errorIndex = index;
                return "";
            }
            ++index;
            auto valueFirstIndex = index;
            auto valueLastIndex = contents.find_first_of(firstQuotation, valueFirstIndex);
            if (valueLastIndex == std::string::npos)
            {
                _status = AttributeSyntaxError;
                _errorIndex = index;
                return "";
            }
            index = valueLastIndex + 1;
            return std::string(contents.substr(valueFirstIndex, valueLastIndex - valueFirstIndex));
        }

        //
        // TODO: comment in many place
        // <!  incoming index is point to '!'
        void _ReadComment(std::string_view contents, size_t &index)
        {
            auto comment = contents.find("<!--", index - 1);
            if (comment == std::string::npos)
            {
                _status = CommentSyntaxError;
                _errorIndex = index;
                return;
            }
            index = contents.find("-->", comment + 4);
            if (index == std::string::npos)
            {
                return;
            }
            index += 4;
        }

        // TODO: escape characters
        // TODO: all while read, judge i < contents.size() | refactor
        // TODO: a bug, end of the text, if had Blank, will dump, add test example and fix
        // if end tag, then read blank
        // TODO: CDATA and Comment may be in Content
        std::string _ReadElementContent(std::string_view contents, size_t &index)
        {
            auto firstIndex = index;
            auto textLast = contents.find('<', index);
            if (textLast == std::string::npos)
            {
                _status = TagNotMatchedError;
                _errorIndex = index;
                return "";
            }
            // DATA OR CDATA
            auto firstCharIndex = contents.find_first_not_of(Blank, index);
            index = textLast;
            if (firstCharIndex != std::string::npos && firstCharIndex < textLast)
            {
                return std::string(contents.substr(firstIndex, textLast - firstIndex));
            }
            else
            {
                return "";
            }
        }

        void _ReadAttribute(std::string_view contents, size_t &index, XMLNode &newNode)
        {
            while (_IsBlankChar(contents[index]))
            {
                // read space between tag and attribute name
                // this blank is necessary
                // while is judge have blank
                // so _ReadBlank can't set index = std::string::pos
                _ReadBlank(contents, index);
                if (contents[index] != '>' && contents[index] != '/')
                {
                    // Attribute Name
                    // name(space)*=(space)*\"content\"
                    // while (contents[index] != '=' || contents[index] != ' ')
                    auto attributeName = _ReadName(contents, index);
                    // TODO:error process
                    // read (space)*
                    _ReadBlank(contents, index);
                    if (contents[index] != '=')
                    {
                        _status = AttributeSyntaxError;
                        _errorIndex = index;
                        return;
                    }
                    ++index;

                    _ReadBlank(contents, index);
                    // Attribute Value
                    // can't use '&'
                    auto attributeValue = _ReadAttributeValue(contents, index);
                    if (_status != NoError)
                    {
                        return;
                    }

                    //repeat attribute
                    auto attr = newNode.GetAttribute();
                    if (attr.find(attributeName) != attr.end())
                    {
                        _status = AttributeRepeatError;
                        _errorIndex = index;
                        return;
                    }
                    newNode.AddAttribute(attributeName, attributeValue);
                }
            }
        }

        void _ReadStartTag(std::string_view contents, size_t &index, XMLNode& current, std::stack<std::string>& tagStack)
        {
            // new tag start
            // read start tag name
            auto tag = _ReadName(contents, index);
            if (index >= contents.size())
            {
                _status = TagBadCloseError;
                _errorIndex = index;
                return;
            }
            if (!(_IsBlankChar(contents[index]) || contents[index] == '/' || contents[index] == '>'))
            {
                _status = TagSyntaxError;
                _errorIndex = index;
                return;
            }

            tagStack.push(tag);
            auto newNode = XMLNode(tag);

            // will read all space
            _ReadAttribute(contents, index, newNode);
            if(_status != NoError)
            {
                return;
            }

            // <tag/>
            if (contents[index] == '/')
            {
                // tag end
                ++index;
                if (contents[index] != '>')
                {
                    _status = TagBadCloseError;
                    _errorIndex = index;
                    return;
                }
                current.AddChild(newNode);
                tagStack.pop();
                ++index;
                return;
            }

                // tag end by >
            else if (contents[index] == '>')
            {
                current.AddChild(newNode);
                current = newNode;
                ++index;
                return;
            }

            else
            {
                // > can't match
                _status = TagBadCloseError;
                _errorIndex = index;
                return;
            }
        }

        void _ReadEndTag(std::string_view contents, size_t &i, XMLNode& current, std::stack<std::string>& tagStack)
        {
            ++i;
            // < (space)* /
            _ReadBlank(contents, i);
            // TODO:error process
            auto tag = _ReadName(contents, i);
            // </tag (space)* >
            _ReadBlank(contents, i);
            if (contents[i] != '>')
            {
                _status = TagBadCloseError;
                return;
            }
            auto stackTop = tagStack.top();
            if (tag != stackTop)
            {
                _status = TagNotMatchedError;
                _errorIndex = i;
                return;
            }
            tagStack.pop();
            ++i;
            if (current.GetTag().empty())
            {
                _status = NullTagError;
                _errorIndex = i;
                return;
            }
            current = current.GetParent();
        }

        // TODO: complete
        std::string _ReadCData(std::string_view contents, size_t &index)
        {
            // can't nested
            // starts with <![CDATA[
            // ends with ]]>
            auto first = index;
            index = contents.find("]]>", index);
            if(index != std::string::npos)
            {
                return std::string(contents.substr(first, index - first));
            }
            else
            {
                _status = CDATASyntaxError;
            }
            return "";
        }

        XMLNode _Parse(std::string_view XMLString)
        {
            auto contents = XMLString;
            auto i = contents.find_first_not_of(Blank);

            auto root = XMLNode(XMLNode::NodeType::NodeDocument);
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

            _ParseDeclaration(XMLString, i);

            std::stack<std::string> tagStack;
            auto current = root;
            while (i < contents.size())
            {
                // tag start
                if (contents[i] == '<')
                {
                    ++i;
                    switch (contents[i])
                    {
                        case '?': // declaration must be first line which not null
                            _status = DeclarationPositionError;
                            break;
                        case '/': // end tag </tag>
                            _ReadEndTag(contents, i, current, tagStack);
                            break;
                            // TODO: compare with char compare contents[i] == '!'
                        case '!': // comment <!--   -->
                            if(contents[i+1] == '-')
                            {
                                _ReadComment(contents, i);
                            }
                            else
                            {
                                // TODO: place by if
                                if(contents.substr(i, 8) == CDATA)
                                {
                                    i += 8;
                                    current.SetContext(_ReadCData(contents, i));
                                    // TODO:NodeType
                                }
                            }
                            break;
                        default: // <tag>
                            _ReadStartTag(contents, i, current, tagStack);
                    }
                }
                // content start
                else
                {
                    current.SetContext(_ReadElementContent(contents, i));
                }
                if(_status != NoError)
                {
                    return root;
                }
            }

            if (current._node != root._node)
            {
                _status = TagNotMatchedError;
                _errorIndex = i;
                return root;
            }

            root.SetContext("");
            assert(root.GetParent()._node == nullptr);
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
    class XMLDocument : public XMLNode
    {
    public:
        XMLParserResult LoadFile(const std::string &fileName)
        {
            XMLParser parser;
            auto root = parser.ParseFile(fileName);
            // except children, other member don't changed
            _node->_children = root.Children();
            return XMLParserResult(parser.Status(), parser.ErrorIndex());
        }

        XMLParserResult LoadString(const std::string &str)
        {
            XMLParser parser;
            auto root = parser.ParseString(str);
            _node->_children = root.Children();
            return XMLParserResult(parser.Status(), parser.ErrorIndex());
        }

        void Write(XMLNode root);
    };
}
#endif //CRAFT_XML_HPP
