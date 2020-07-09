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
    class XMLElement
    {
    public:
        using XMLElementIterator = std::vector<XMLElement*>::iterator;
        using XMLElements = std::vector<XMLElement*>;

        XMLElement(std::string tag = "", std::string context = "", XMLElement *parent = nullptr) :
                _tag(std::move(tag)), _context(std::move(context)), _parent(parent)
        {
            if (parent != nullptr)
            {
                parent->AddChild(this);
            }
        }

        virtual ~XMLElement() = default;

        XMLElementIterator begin()
        {
            return _childs.begin();
        }

        XMLElementIterator end()
        {
            return _childs.end();
        }

        std::string GetTag()
        {
            return _tag;
        }

        bool HasChild()
        {
            return !_childs.empty();
        }

        void AddChild(XMLElement *child)
        {
            _childs.push_back(child);
            child->_parent = this;
        }

        void AddAttribute(const std::string &name, const std::string &value)
        {
            _attributes[name] = value;
        }

        std::vector<XMLElement *> Childs()
        {
            return _childs;
        }

        void SetParent(XMLElement *parent)
        {
            _parent = parent;
            parent->_childs.push_back(this);
        }

        XMLElement *GetParent()
        {
            return _parent;
        }

        void SetContext(std::string context)
        {
            _context = std::move(context);
        }

        XMLElements FindChildByTagName(const std::string& tagName)
        {
            XMLElements childs;
            for(auto& child : _childs)
            {
                if(child->GetTag() == "tagName")
                {
                    childs.push_back(child);
                }
            }
            return childs;
        }

        std::string StringValue()
        {
            return _context;
        }

        void output()
        {
            for (auto &c : _childs)
            {
                std::cout << "<" << c->_tag << ">" << std::endl;
                std::cout << c->_context << std::endl;
                c->output();
                std::cout << "<\\" << c->_tag << ">" << std::endl;
            }
        }

        std::map<std::string, std::string> _attributes;
        std::string _tag, _context;
        XMLElements _childs;
        XMLElement *_parent;
    };

    inline void Boom(const std::string &info, int i = 0)
    {
        std::cout << "Error:" << info << "index:" << i << std::endl;
        //exit(-1);
    }


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
            AttributeError,

        };

        XMLParser() = default;

        XMLElement *ParseFile(const std::string &fileName)
        {
            std::ifstream file(fileName, std::ios::in);
            if (!file.is_open())
            {
                std::cout << "Error:can't open file" << std::endl;
                _status = FileOpenFailed;
                return nullptr;
            }
            std::string contents((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
            return _Parse(contents);
        }

        XMLElement *ParseString(const std::string &XMLString)
        {
            return _Parse(XMLString);
        }

        ParseStatus Status()
        {
            return _status;
        }

    private:
        ParseStatus _status = NoError;

        XMLElement *_Parse(const std::string &XMLString)
        {
            auto contents = XMLString;
            auto i = contents.find_first_not_of(' ');

            //TODO: XML declaration Parse
            auto root = new XMLElement();
            if (contents[i] != '<')
            {
                _status = TagSyntaxError;
                Boom("syntax error", i);
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
                        std::cout << "last:" << tag << " stackTop:" << stackTop << std::endl;
                        if (tag != stackTop)
                        {
                            _status = TagNestedError;
                            // Boom("</tag> error", i);
                            return root;
                        }
                        tagStack.pop();
                        ++i;

                        if (current->HasChild())
                        {
                            // if not leaf node, then clear context
                            std::cout << "current text:" << current->_context << ":end" << std::endl;
                            current->SetContext("");
                        }

                        current = current->GetParent();
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
                    std::cout << "tag:" << tag << std::endl;
                    auto newNode = new XMLElement(tag);

                    // TODO: Attribute is unique
                    // Repeat read Attribute
                    while(contents[i] == ' ')
                    {
                        std::string attributeName;
                        std::string attributeValue;
                        // read space
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
                                _status = AttributeError;
                                // Boom("attribute syntax error");
                                return root;
                            }
                            // TODO:repeat attribute is error
                            newNode->AddAttribute(attributeName, attributeValue);
                        }
                    }

                    // tag end by >
                    if(contents[i] == '>')
                    {
                        current->AddChild(newNode);
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
                    if(text.empty())
                    {
                        _status = NullTagError;
                        //Boom("null tag");
                        return root;
                    }
                    std::cout << "text:" << text << std::endl;
                    current->SetContext(text);
                }
            }

            if (current != root)
            {
                _status = TagNestedError;
                // Boom("tag level mismatch");
                return root;
            }

            root->_context.clear();
            assert(root->_parent == nullptr);
            assert(root->_tag.empty());
            assert(root->_context.empty());
            assert(root->_attributes.empty());
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
            _childs = root->Childs();
            return parser.Status();
        }

        XMLParser::ParseStatus LoadString(const std::string& str)
        {
            XMLParser parser;
            auto root = parser.ParseString(str);
            _childs = root->Childs();
            return parser.Status();
        }

        void Write(XMLElement* root);
    };
}
#endif //CRAFT_XML_HPP
