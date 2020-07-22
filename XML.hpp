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
    class XMLNodeIterator;

    class XMLNode
    {
    protected:
        class XMLNodeStruct;

        friend class XMLDocument;
        friend class XMLParser;
        friend class XMLNodeIterator;

        XMLNode(std::shared_ptr<XMLNodeStruct> node) : _node(std::move(node))
        {}

        XMLNode(std::weak_ptr<XMLNodeStruct> node) : _node(node.lock())
        {}

    public:
        enum NodeType {
            NodeDocument,
            NodeElement,
            NodeData,
            NodeCData,
            NodeComment,
            NodeDoctype,
            NodeDeclaration,
            NodePI,
            NullNode // Is Empty
        };
        using Iterator = XMLNodeIterator;
        using XMLNodes = std::vector<XMLNode>;

        XMLNode(std::string tag = "", std::string content = "", NodeType type = NodeElement) :
                _node(new XMLNodeStruct(std::move(tag), std::move(content), type))
        {}

        XMLNode(NodeType type) : XMLNode("", "", type)
        {

        }

        virtual ~XMLNode() = default;

        Iterator begin() noexcept;

        Iterator end() noexcept;

        // empty node, when can't find child then will return empty node
        [[nodiscard]] bool IsEmpty() const noexcept
        {
            return _node->_type == NullNode;
        }

        // node modified
        void AddChild(XMLNode &child)
        {
            if(_node->_firstChild == _node->_lastChild)
            {
                // firstChild->next default nullptr
                // firstChild->prev default reset()
                // lastChild default point to firstChild(a empty node)
                // so lastChild->next default nullptr
                _node->_firstChild = child._node;
                _node->_firstChild->_next = _node->_lastChild;
                _node->_lastChild->_prev = _node->_firstChild;
                // do in constructor
                // _node->_firstChild->_prev.reset();
                // _node->_lastChild->_next = nullptr;
            }
            else
            {
                child._node->_prev = _node->_lastChild->_prev;
                child._node->_next = _node->_lastChild;
                _node->_lastChild->_prev.lock()->_next = child._node;
                _node->_lastChild->_prev = child._node;
            }
            child._node->_parent = _node;
        }

        // Set
        void SetParent(XMLNode &parent)
        {
            _node->_parent = parent._node;
            parent.AddChild(*this);
        }

        void SetNodeTag(std::string tag)
        {
            _node->_tag = std::move(tag);
        }

        void SetNodeType(NodeType type) noexcept
        {
            _node->_type = type;
        }

        void SetNodeContent(std::string context)
        {
            _node->_content = std::move(context);
        }

        void AddNodeAttribute(const std::string &name, const std::string &value)
        {
            _node->_attributes[name] = value;
        }

        // Get
        [[nodiscard]] std::string GetNodeTag() const
        {
            return _node->_tag;
        }

        [[nodiscard]] NodeType GetNodeType() const
        {
            return _node->_type;
        }

        [[nodiscard]] std::string GetNodeContent() const
        {
            return _node->_content;
        }

        [[nodiscard]] std::string GetNodeAttribute(const std::string& attributeName) const
        {
            return _node->_attributes[attributeName];
        }

        [[nodiscard]] std::map<std::string, std::string> GetNodeAttributes() const
        {
            return _node->_attributes;
        }

        [[nodiscard]] bool HasChild() const noexcept
        {
            return _node->_firstChild != _node->_lastChild;
        }

        // Node Accessor
        [[nodiscard]] XMLNode GetParent() const noexcept
        {
            return XMLNode(_node->_parent.lock());
        }

        [[nodiscard]] XMLNode NextSibling() const
        {
            return _node->_next != _node->_lastChild ? _node->_next : XMLNode(NullNode);
        }

        [[nodiscard]] XMLNode PrevSibling() const
        {
            return _node->_prev.lock() != _node->_lastChild ? _node->_next : XMLNode(NullNode);
        }

        [[nodiscard]] XMLNodes operator[](const std::string& TagName) const
        {
            return FindChildrenByTagName(TagName);
        }

        [[nodiscard]] XMLNode FirstChild() const
        {
            return _node->_firstChild != _node->_lastChild ? _node->_firstChild : XMLNode(NullNode);
        }

        [[nodiscard]] XMLNode LastChild() const
        {
            return _node->_firstChild != _node->_lastChild ? _node->_lastChild->_prev : XMLNode(NullNode);
        }

        [[nodiscard]] XMLNode FindFirstChildByTagName(const std::string &tagName) const
        {
            auto first = _node->_firstChild;
            while(first != _node->_lastChild)
            {
                if(first->_tag == tagName)
                {
                    return first;
                }
                first = first->_next;
            }
            return XMLNode(NodeType::NullNode);
        }

        [[nodiscard]] XMLNodes FindChildrenByTagName(const std::string &tagName) const
        {
            XMLNodes children;
            auto first = _node->_firstChild;
            while(first != _node->_lastChild)
            {
                if(first->_tag == tagName)
                {
                    children.push_back(first);
                }
                first = first->_next;
            }
            return children;
        }

        [[nodiscard]] XMLNode FindFirstChildByType(NodeType type) const
        {
            auto first = _node->_firstChild;
            while(first != _node->_lastChild)
            {
                if(first->_type == type)
                {
                    return first;
                }
                first = first->_next;
            }
            return XMLNode(NodeType::NullNode);
        }

        [[nodiscard]] XMLNodes FindChildrenByType(NodeType type) const
        {
            XMLNodes children;
            auto first = _node->_firstChild;
            while(first != _node->_lastChild)
            {
                if(first->_type == type)
                {
                    children.push_back(first);
                }
                first = first->_next;
            }
            return children;
        }

        [[nodiscard]] std::string StringValue() const
        {
            return _node->_content;
        }
    protected:
        struct XMLNodeStruct
        {
            XMLNodeStruct() = default;

            XMLNodeStruct(std::string tag, std::string content, NodeType type, const std::shared_ptr<XMLNodeStruct>& parent = nullptr) :
                    _tag(std::move(tag)), _content(std::move(content)), _type(type), _parent(parent), _prev(), _next(nullptr),
                    _firstChild(std::make_shared<XMLNodeStruct>())
            {
                _lastChild = _firstChild;
                _lastChild->_type = NullNode;
            }

            // smart pointer rule to prevent circular reference
            // parent to child : shared_ptr
            // child to parent : weak_ptr
            // next : shared_ptr
            // prev : weak_ptr

            // not circular list
            std::map<std::string, std::string> _attributes;
            std::string _tag, _content;
            NodeType _type;
            std::weak_ptr<XMLNodeStruct> _parent;
            std::shared_ptr<XMLNodeStruct> _firstChild;
            std::shared_ptr<XMLNodeStruct> _lastChild;
            std::weak_ptr<XMLNodeStruct> _prev;
            std::shared_ptr<XMLNodeStruct> _next;
        };

        std::shared_ptr<XMLNodeStruct> _node;
    };

    class XMLNodeIterator
    {
    public:
        XMLNodeIterator() = default;

        XMLNodeIterator(const XMLNode &node)
        {
            _root = node;
        }

        bool operator==(const XMLNodeIterator &other) const
        {
            return _root._node == other._root._node;
        }

        bool operator!=(const XMLNodeIterator &other) const
        {
            return _root._node != other._root._node;
        }

        XMLNodeIterator operator++()
        {
            _root._node = _root._node->_next;
            return *this;
        }

        XMLNodeIterator operator--()
        {
            _root._node = _root._node->_prev.lock();
            return *this;
        }

        XMLNode& operator*()
        {
            return _root;
        }

        XMLNode* operator->()
        {
            return &_root;
        }

    private:
        XMLNode _root;
    };

    XMLNode::Iterator XMLNode::begin() noexcept
    {
        return Craft::XMLNode::Iterator(_node->_firstChild);
    }

    XMLNode::Iterator XMLNode::end() noexcept
    {
        return Craft::XMLNode::Iterator(_node->_lastChild);
    }

    class XMLParser
    {
    public:
        enum ParseStatus
        {
            NoError = 0,
            FileOpenFailed,
            TagSyntaxError,
            TagBadCloseError,
            CommentSyntaxError,
            TagNotMatchedError,
            AttributeSyntaxError,
            AttributeRepeatError,
            DeclarationSyntaxError,
            DeclarationPositionError,
            CDATASyntaxError,
            PISyntaxError,
            PrologSyntaxError,
            ElementSyntaxError,
            MiscSyntaxError,
            EntityReferenceError
        };
        XMLParser() = default;

        XMLNode ParseFile(const std::string &fileName)
        {
            std::ifstream file(fileName, std::ios::in);
            if (!file.is_open())
            {
                _status = FileOpenFailed;
                return XMLNode(XMLNode::NodeType::NullNode);
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

        // [66]CharRef ::= '&#' [0-9]+ ';'
        //              | '&#x' [0-9a-fA-F]+ ';'
        // [67]Reference ::= EntityRef | CharRef
        // [68]EntityRef ::= '&' Name ';'
        // [69]PEReference ::= '%' Name ';'
        char ParseCharReference(std::string_view contents, size_t &i)
        {
            char c = '\0';
            if(contents[i] == 'l' && contents[i + 1] == 't')
            {
                c = '<';
                i += 2;
            }
            else if(contents[i] == 'g' && contents[i + 1] == 't')
            {
                c = '>';
                i += 2;
            }
            else if(contents[i] == 'a' && contents[i + 1] == 'm' && contents[i + 2] == 'p')
            {
                c = '&';
                i += 3;
            }
            else if(contents[i] == 'a' && contents[i + 1] == 'p' && contents[i + 2] == 'o' && contents[i + 3] == 's')
            {
                c = '\'';
                i += 4;
            }
            else if(contents[i] == 'q' && contents[i + 1] == 'u' && contents[i + 2] == 'o' && contents[i + 3] == 't')
            {
                c = '"';
                i += 4;
            }
            else if(contents[i] == '#')
            {
                ++i;
                std::string num;
                if(contents[i] == 'x')
                {
                    ++i;
                    num = "0x";
                    while(_IsNumber(contents[i]) ||
                          (contents[i] >= 'A' && contents[i] <= 'F'))
                    {
                        ++i;
                        num.push_back(contents[i]);
                    }
                    c = static_cast<char>(strtol(num.c_str(), nullptr, 16));
                }
                else
                {
                    while(_IsNumber(contents[i]))
                    {
                        num.push_back(contents[i]);
                        ++i;
                    }
                    c = static_cast<char>(std::stoi(num));
                }
            }
            else
            {
                return '\0';
            }
            if(contents[i] != ';')
            {
                return '\0';
            }
            ++i;
            return c;
        }

    private:
        constexpr static std::string_view SymbolNotUsedInName = R"(!"#$%&'()*+,/;<=>?@[\]^`{|}~ )";
        // constexpr static std::string_view Blank = "\t\r\n ";
        constexpr static std::string_view Blank = "\x20\x09\x0d\x0A";

        ParseStatus _status = NoError;

        int _errorIndex = -1;

        [[nodiscard]] bool _IsNameChar(char c) const noexcept
        {
            return SymbolNotUsedInName.find(c) == std::string::npos;
        }

        [[nodiscard]] bool _IsXML(std::string_view contents, size_t i) const
        {
            return ((contents[i + 0] == 'x' || contents[i + 0] == 'X') &&
            (contents[i + 1] == 'm' || contents[i + 1] == 'M') &&
            (contents[i + 2] == 'l' || contents[i + 2] == 'L'));
        }

        [[nodiscard]] bool _IsXMLDeclarationStart(std::string_view contents, size_t i) const
        {
            return (contents[i + 0] == '<' && contents[i + 1] == '?' &&
                _IsXML(contents, i + 2));
        }

        // in XML, \t \r \n space are blank character
        bool _IsBlankChar(char c) const noexcept
        {
            return Blank.find(c) != std::string::npos;
        }

        // TODO:finish
        // [2]Char	::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
        /* 除了代用块（surrogate block），FFFE 和 FFFF 以外的任意 Unicode 字符。*/
        bool _IsChar(char c)
        {
            if(c == 0x09 || c == 0x0A || c == 0x0D || (c >= 0x20 && c <= 0xD7FF))
            {
                return true;
            }
            return false;
        }

        //[4]NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' | CombiningChar | Extender
        //[5]Name ::= (Letter | '_' | ':') (NameChar)*/
        std::string _ParseName(std::string_view contents, size_t &index)
        {
//            if(!(contents[index] == '_' || contents[index] == ':' || _IsLetter(contents[index])))
//            {
//                _status = TagSyntaxError;
//                _errorIndex = index;
//                return "";
//            }
            // if find
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


        // must have version
        // order must be version encoding standalone
        // use " or '
        // [23]XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'
        // [24]VersionInfo ::= S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"')/* */
        // [25]Eq ::= S? '=' S?
        // [26]VersionNum ::= ([a-zA-Z0-9_.:] | '-')+
        void _ParseDeclaration(std::string_view contents, size_t &i, XMLNode& current)
        {
            auto newChild = XMLNode(XMLNode::NodeType::NodeDeclaration);
            _ParseAttribute(contents, i, newChild);
            if(!(contents[i] == '?' && contents[i + 1] == '>'))
            {
                _status = DeclarationSyntaxError;
                _errorIndex = i;
                return;
            }
            i += 2;
            if(newChild.GetNodeAttributes()["standalone"].empty())
            {
                newChild.AddNodeAttribute("standalone", "no");
            }
            auto standalone = newChild.GetNodeAttributes()["standalone"];
            // standard 2.9
            if(_status != NoError || newChild.GetNodeAttributes()["version"].empty() || !(standalone == "yes" || standalone == "no"))
            {
                _status = DeclarationSyntaxError;
                return;
            }
            // about encoding https://web.archive.org/web/20091015072716/http://lightning.prohosting.com/~qqiu/REC-xml-20001006-cn.html#NT-EncodingDecl
            current.AddChild(newChild);
        }

        void _ParseBlank(std::string_view contents, size_t &index)
        {
            // not 0 is find, may be in
            index = contents.find_first_not_of(Blank, index);
            index = index == std::string::npos ? contents.size() : index;
        }

//        [10]AttValue ::= '"' ([^<&"] | Reference)* '"'
//                    |  "'" ([^<&'] | Reference)* "'"
        std::string _ParseAttributeValue(std::string_view contents, size_t &index)
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
            std::string attributeValue;
            while(index < contents.size() && contents[index] != firstQuotation)
            {
                if(contents[index] == '&')
                {
                    attributeValue += std::string(contents.substr(valueFirstIndex, index - valueFirstIndex));
                    ++index;
                    auto refChar = ParseCharReference(contents, index);
                    if(refChar == '\0')
                    {
                        _status = EntityReferenceError;
                        _errorIndex = index;
                        return "";
                    }
                    attributeValue.push_back(refChar);
                    valueFirstIndex = index;
                    if(_status != NoError)
                    {
                        return "";
                    }
                }
                else
                {
                    ++index;
                }
            }
            attributeValue += std::string(contents.substr(valueFirstIndex, index - valueFirstIndex));
            ++index;
            return attributeValue;
        }

        // [15]Comment::='<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
        // <!  incoming index is point to '!'
        void _ParseComment(std::string_view contents, size_t &i, XMLNode& current)
        {
            auto commentFirst = i;
            while(i < contents.size())
            {
                if(contents[i] == '-' && contents[i + 1] == '-' )
                {
                    if(contents[i + 2] == '>')
                    {
                        break;
                    }
                    else
                    {
                        _status = CommentSyntaxError;
                        _errorIndex = i;
                        return;
                    }
                }
                ++i; // is char
            }
            auto newNode = XMLNode("", std::string(contents.substr(commentFirst, i - commentFirst)),
                                   XMLNode::NodeType::NodeComment);
            current.AddChild(newNode);
            i += 3;
        }

        [[nodiscard]] constexpr bool _IsNumber(char c) const
        {
            return c >= '0' && c <= '9';
        }

        // 	[14]CharData	   ::=   	[^<&]* - ([^<&]* ']]>' [^<&]*)
        //	[67]Reference	   ::=   	EntityRef | CharRef
        void _ParseElementCharData(std::string_view contents, size_t &i, XMLNode& current)
        {
            auto firstIndex = i;
            std::string charData;
            while(i < contents.size() && contents[i] != '<')
            {
                if(contents[i] == '&')
                {
                    charData += std::string(contents.substr(firstIndex, i - firstIndex));
                    ++i;
                    auto refChar = ParseCharReference(contents, i);
                    if( refChar == '\0')
                    {
                        _status = EntityReferenceError;
                        _errorIndex = i;
                        return;
                    }
                    charData.push_back(refChar);
                    firstIndex = i;
                }
                else
                {
                    ++i;
                }
            }

            charData += std::string(contents.substr(firstIndex, i - firstIndex));
            auto newChild = XMLNode("", charData,
                    XMLNode::NodeType::NodeData);
            current.AddChild(newChild);
            if(current.GetNodeContent().empty())
            {
                current.SetNodeContent(charData);
            }
        }

        // [43]content ::= CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*	/* */
        //  CDSect : CDATA[21]
        void _ParseElementContent(std::string_view contents, size_t &i, XMLNode& current)
        {
            auto firstIndex = i;
            std::string content;
            while(i < contents.size())
            {
                _ParseBlank(contents, i);
                if(contents[i] == '<')
                {
                    if(contents[i + 1] == '!')
                    {
                        ++i;
                        if(contents[i + 1] == '-' && contents[i + 2] == '-')
                        {
                            i += 3;
                            _ParseComment(contents, i, current);
                        }
                        else if(contents[i + 1] == '[')
                        {
                            if(contents.substr(i + 1, 7) == "[CDATA[")
                            {
                                i += 8;
                                _ParseCDATA(contents, i, current);
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    else if(contents[i + 1] == '?')
                    {
                        i += 2;
                        _ParsePI(contents, i, current);
                    }
                    else
                    {
                        // parse to child element start <
                        break;
                    }
                }
                else
                {
                    _ParseElementCharData(contents, i, current);
                }
            }
        }

        // [41]Attribute ::= Name Eq AttValue
        void _ParseAttribute(std::string_view contents, size_t &index, XMLNode &newNode)
        {
            while (_IsBlankChar(contents[index]))
            {
                // read space between tag and attribute name
                // this blank is necessary
                // while is judge have blank
                // so _ParseBlank can't set index = std::string::pos
                _ParseBlank(contents, index);
                // tag end
                if (contents[index] == '>' || contents[index] == '/' || contents[index] == '?')
                {
                    return;
                }
                // Attribute Name
                // name(space)*=(space)*\"content\"
                // while (contents[index] != '=' || contents[index] != ' ')
                auto attributeName = _ParseName(contents, index);
                if (_status != NoError)
                {
                    return;
                }

                // read (space)*
                _ParseBlank(contents, index);
                if (contents[index] != '=')
                {
                    _status = AttributeSyntaxError;
                    _errorIndex = index;
                    return;
                }
                ++index;
                _ParseBlank(contents, index);
                // Attribute Value
                // can't use '&'
                auto attributeValue = _ParseAttributeValue(contents, index);
                if (_status != NoError)
                {
                    return;
                }

                // repeat attribute check
                auto attr = newNode.GetNodeAttributes();
                if (attr.find(attributeName) != attr.end())
                {
                    _status = AttributeRepeatError;
                    _errorIndex = index;
                    return;
                }
                newNode.AddNodeAttribute(attributeName, attributeValue);
            }
        }


        // [40] STag ::= '<' Name (S Attribute)* S? '>'
        void _ParseStartTag(std::string_view contents, size_t &index, XMLNode& current, std::stack<std::string>& tagStack, bool& isEmptyTag)
        {
            // read start tag name
            auto tag = _ParseName(contents, index);
            if(_status != NoError)
            {
                return;
            }
            if (index >= contents.size())
            {
                _status = TagBadCloseError;
                _errorIndex = index;
                return;
            }
            if(tag.empty())
            {
                _status = TagSyntaxError;
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
            _ParseAttribute(contents, index, newNode);
            if(_status != NoError)
            {
                return;
            }

            // EmptyElemTag <tag/>
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
                isEmptyTag = true;
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

        // [42]ETag	::= '</' Name S? '>'
        void _ParseEndTag(std::string_view contents, size_t &i, XMLNode& current, std::stack<std::string>& tagStack)
        {
            // < (space)* /
            _ParseBlank(contents, i);
            auto tag = _ParseName(contents, i);
            if(_status != NoError)
            {
                return;
            }
            // </tag (space)* >
            _ParseBlank(contents, i);
            if (contents[i] != '>')
            {
                _status = TagBadCloseError;
                _errorIndex = i;
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
            current = current.GetParent();
        }

        // TODO: test
//        [18]   	CDSect	   ::=   	CDStart CData CDEnd
//        [19]   	CDStart	   ::=   	'<![CDATA['
//        [20]   	CData	   ::=   	(Char* - (Char* ']]>' Char*))
//        [21]   	CDEnd	   ::=   	']]>'
        void _ParseCDATA(std::string_view contents, size_t &index, XMLNode& current)
        {
            // can't nested
            // starts with <![CDATA[
            // ends with ]]>
            auto first = index;
            index = contents.find("]]>", index);
            if(index != std::string::npos)
            {
                auto newNode = XMLNode("", std::string(contents.substr(first, index - first)),
                        XMLNode::NodeType::NodeCData);
                current.AddChild(newNode);
                index += 3;
            }
            else
            {
                _status = CDATASyntaxError;
                _errorIndex = index;
            }
        }

        [[nodiscard]] bool _IsDOCTYPE(std::string_view contents, size_t i) const
        {
            return contents.substr(0, 8) == "DOCTYPE ";
        }

        [[nodiscard]] bool _IsEngLetter(char c) const
        {
            return (c > 'A' && c < 'Z') || (c > 'a' && c < 'z');
        }

        void _ParseDoctypeDecl(std::string_view contents, size_t &index, XMLNode& current)
        {
            auto first = index;
            while(index < contents.size() && contents[index] != '>')
            {
                if(contents[index] == '[')
                {
                    int depth = 1;
                    index++;
                    while(depth > 0)
                    {
                        if (contents[index] == '[')
                        {
                            ++depth;
                        }
                        else if(contents[index] == ']')
                        {
                            --depth;
                        }
                        ++index;
                    }
                }
                else
                {
                    ++index;
                }
            }
            ++index;
            auto content = std::string(contents.substr(first, index - first - 2));
            auto newNode = XMLNode("", content, XMLNode::NodeType::NodeDoctype);
            current.AddChild(newNode);
        }

        // [22]prolog ::= XMLDecl? Misc* (doctypedecl Misc*)?
        // [27]Misc ::= Comment | PI | S
        void _ParseProlog(std::string_view contents, size_t &index, XMLNode& current)
        {
            _ParseBlank(contents, index);
            if(_IsXMLDeclarationStart(contents, index))
            {
                index += 5;
                _ParseDeclaration(contents, index, current);
            }
            while(index < contents.size())
            {
                //comment pi space
                _ParseBlank(contents, index);
                if (contents[index] == '<')
                {
                    if (contents[index + 1] == '!')
                    {
                        index += 2;
                        if (contents[index] == '-' && contents[index + 1] == '-')
                        {
                            index += 2;
                            _ParseComment(contents, index, current);
                        }
                        else if (contents[index] == 'D')
                        {
                            index += 2;
                            _ParseDoctypeDecl(contents, index, current);
                        }
                        else
                        {
                            _status = PrologSyntaxError;
                            _errorIndex = index;
                            return;
                        }
                    }
                    else if (contents[index + 1] == '?')
                    {
                        index += 2;
                        _ParsePI(contents, index, current);
                    }
                    else
                    {
                        // element
                        return;
                    }
                }
                else
                {
                    // error
                    _status = PrologSyntaxError;
                    _errorIndex = index;
                    return;
                }
                if(_status != NoError)
                {
                    return;
                }
            }
        }

        //[16]PI ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
        //[17]PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))
        void _ParsePI(std::string_view contents, size_t &index, XMLNode& current)
        {
            // match ? (0|1)
            if(_IsXML(contents, index))
            {
                _status = DeclarationPositionError;
                _errorIndex = index;
                return;
            }
            auto name = _ParseName(contents, index); // PITarget
            _ParseBlank(contents, index);
            // read value
            auto last = contents.find("?>", index);
            // save all text and space
            if(last == std::string::npos)
            {
                _status = PISyntaxError;
                _errorIndex = index;
                return;
            }
            else
            {
                auto piContent = std::string(contents.substr(index, last - index));
                auto newChild = XMLNode(name, piContent, XMLNode::NodeType::NodePI);
                current.AddChild(newChild);
                index = last + 2;
            }
        }

        // Misc	::= Comment | PI | S
        void _ParseMisc(std::string_view contents, size_t &i, XMLNode& current)
        {
            while(i < contents.size() && _status == NoError)
            {
                _ParseBlank(contents, i);
                if (contents[i] == '<')
                {
                    ++i;
                    if (contents[i + 1] == '!' && contents[i + 2] == '-' && contents[i + 3] == '-')
                    {
                        _ParseComment(contents, i, current);
                    }
                    else if (contents[i + 1] == '?')
                    {
                        i += 4;
                        _ParsePI(contents, i, current);
                    }
                    else
                    {
                        _status = MiscSyntaxError;
                    }
                }
            }
        }

        XMLNode _Parse(std::string_view contents)
        {
            auto root = XMLNode(XMLNode::NodeType::NodeDocument);
            size_t i = 0;
            // parse prolog and read to first <
            _ParseProlog(contents, i, root);

            std::stack<std::string> tagStack;
            auto current = root;
            while (i < contents.size())
            {
                if (contents[i] == '<')
                {
                    ++i;
                    switch (contents[i])
                    {
                        case '?':
                            // declaration must be first line which not null
                            if(_IsXMLDeclarationStart(contents, i - 1))
                            {
                                _status = DeclarationPositionError;
                                _errorIndex = i;
                            }
                            else
                            {
                                ++i;
                                _ParsePI(contents, i, current);
                            }
                            break;
                        case '/': // end tag </tag>
                            i += 1;
                            _ParseEndTag(contents, i, current, tagStack);
                            break;
                        case '!':
                            if(contents[i + 1] == '-' && contents[i + 2] == '-')
                            {
                                i += 3;
                                _ParseComment(contents, i, current);
                            }
                            break;
                        default: // <tag>
                            auto isEmptyTag = false;
                            _ParseStartTag(contents, i, current, tagStack, isEmptyTag);
                            if(_status != NoError)
                            {
                                return root;
                            }
                            if(isEmptyTag)
                            {
                                return root;
                            }
                            _ParseElementContent(contents, i, current);
                            if(_status != NoError)
                            {
                                return root;
                            }
                    }
                }
                else
                {
                    _ParseElementContent(contents, i, current);
                }
            }

            if (current._node != root._node)
            {
                _status = TagNotMatchedError;
                _errorIndex = i;
                return root;
            }

            root.SetNodeContent("");
            assert(root.GetParent()._node == nullptr);
            assert(root.GetNodeTag().empty());
            assert(root.GetNodeContent().empty());
            assert(root.GetNodeAttributes().empty());
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

    class XMLDocument : public XMLNode
    {
    public:
        XMLDocument()
        {
            SetNodeType(XMLNode::NodeType::NodeDocument);
        }

        XMLParserResult LoadFile(const std::string &fileName)
        {
            XMLParser parser;
            _node = parser.ParseFile(fileName)._node;
            return XMLParserResult(parser.Status(), parser.ErrorIndex());
        }

        XMLParserResult LoadString(const std::string &str)
        {
            XMLParser parser;
            _node = parser.ParseString(str)._node;
            return XMLParserResult(parser.Status(), parser.ErrorIndex());
        }
    };

    class XMLWriter
    {
        void Writer(XMLDocument& node)
        {
            if(node.GetNodeType() != XMLNode::NodeDocument)
            {
                return;
            }
            auto child = node.FindFirstChildByType(XMLNode::NodeDeclaration);
            if(child.GetNodeType() != XMLNode::NullNode)
            {

            }
        }
    };
}
#endif //CRAFT_XML_HPP
