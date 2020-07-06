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

namespace Craft
{
    class XMLElement
    {
    public:
        using XMLElementIterator = std::vector<XMLElement*>::iterator;
        XMLElement(std::string tag, std::string context = "", XMLElement *parent = nullptr) :
                _tag(std::move(tag)), _context(std::move(context)), _parent(parent)
        {
            if (parent != nullptr)
            {
                parent->AddChild(this);
            }
        }

        XMLElementIterator begin()
        {
            return _childs.begin();
        }

        XMLElementIterator end()
        {
            return _childs.end();
        }

        void AddChild(XMLElement *child)
        {
            _childs.push_back(child);
            child->_parent = this;
        }

        bool HasChild()
        {
            return !_childs.empty();
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
        std::vector<XMLElement *> _childs;
        XMLElement *_parent;
    };

    inline void Boom(const std::string &info, int i = 0)
    {
        std::cout << "Error:" << info << "index:" << i << std::endl;
        exit(-1);
    }

    class XMLParser
    {
    public:
        XMLParser()
        {
        }

        XMLElement *LoadFile(const std::string &fileName)
        {

            std::ifstream file(fileName, std::ios::in);
            if (!file.is_open())
            {
                std::cout << "Error:can't open file" << std::endl;
                return nullptr;
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string contents(buffer.str());
            return _Parser(contents);
        }

        XMLElement *LoadString(const std::string &XMLString)
        {
            return _Parser(XMLString);
        }

    private:
        XMLElement *_Parser(const std::string &XMLString)
        {
            auto contents = XMLString;
            auto i = contents.find_first_not_of(' ');

            if (contents[i] != '<')
            {
                Boom("syntax error", i);
            }

            std::stack<std::string> tagStack;

            auto root = new XMLElement("");
            auto current = root;
            while (i < contents.size())
            {
                std::string tag;
                std::string text;
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
                            Boom("</tag> error", i);
                        }
                        tagStack.pop();
                        ++i;

                        if (current->HasChild())
                        {
                            // if not leaf node, then clear context
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
                            Boom("comment syntax error", i);
                        }
                        i = contents.find("-->", comment) + 3;
                        // if nops
                        ++i;
                        if (i >= contents.size())
                        {
                            break;
                        }
                    }

                    // start tag
                    while (contents[i] != '>' && contents[i] != ' ')
                    {
                        if (contents[i] == '<')
                        {
                            Boom("syntax error, double >", i);
                        }
                        tag.push_back(contents[i]);
                        ++i;
                    }

                    tagStack.push(tag);
                    std::cout << "tag:" << tag << std::endl;
                    auto newNode = new XMLElement(tag);

                    // repeat
                    // Attribute
                    while(contents[i] == ' ')
                    {
                        std::string attributeName;
                        std::string attributeValue;
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
                                Boom("attribute syntax error");
                            }
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
                        Boom("> can't match", i);
                    }

                }
                else
                {
                    while (contents[i] != '<')
                    {
                        text.push_back(contents[i]);
                        ++i;
                    }
                    std::cout << "text:" << text << std::endl;
                    current->SetContext(text);
                }
            }
            if (current != root)
            {
                Boom("tag level mismatch");
            }
            return current;
        }

    };
}
#endif //CRAFT_XML_HPP
