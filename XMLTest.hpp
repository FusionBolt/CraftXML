//
// Created by fusionbolt on 2020/7/7.
//
#include <iostream>

#include "XML.hpp"

using namespace Craft;

#define LOAD_STRING(str) XMLDocument document; \
                         auto result = document.LoadString(str); \
                         if(result._status != XMLParser::NoError){ \
                             std::cout << result.ErrorInfo() << std::endl; return false;}

#define LOAD_ERROR_STRING(str, parseStatus) XMLDocument document;\
                                            auto result = document.LoadString(str);\
                                            if(result._status != parseStatus)\
                                            {std::cout << result.ErrorInfo() << std::endl; return false;}\
                                            else{return true;}
bool OneTagTest1()
{
    LOAD_STRING("<hr />");
    return !document.FindChildByTagName("hr").empty();
}
bool OneTagTest2()
{
    LOAD_STRING("<tag attr=\"test\" />");
    auto attr = document.FirstChild().GetAttribute();
    if (!attr.empty() && attr["attr"] == "test")
    {
        return true;
    }
    return false;
}

bool OnePairTagTest()
{
    LOAD_STRING("<tag></tag>")
    if(document.FindChildByTagName("tag").empty())
    {
        return false;
    }
    return true;
}

bool ContentTest()
{
    LOAD_STRING("<tag>content</tag>")
    auto elements = document.FindChildByTagName("tag");
    if(elements.empty())
    {
        return false;
    }
    if(elements[0].StringValue() != "content")
    {
        return false;
    }
    return true;
}

bool CommentTest()
{
    LOAD_STRING("<!--tag-->")
    if(document.FindFirstChildByTagName("").GetContent() == "tag")
    {
        return true;
    }
    return false;
}

bool OneAttributeTest()
{
    LOAD_STRING("<tag attr =   \"first\"></tag>")
    auto attr = document.FirstChild().GetAttribute();
    if(attr.size() == 1 && attr["attr"] == "first")
    {
        return true;
    }
    return false;
}

bool AttributeTest()
{
    LOAD_STRING("<tag attr =   \'first\'></tag>")
    auto attr = document.FirstChild().GetAttribute();
    if(attr.size() == 1 && attr["attr"] == "first")
    {
        return true;
    }
    return false;
}

bool MultiAttributeTest()
{
    LOAD_STRING(R"(<tag attr = "first"   att ="second" at= "third"></tag>)")
    auto attr = document.FirstChild().GetAttribute();
    if(attr.size() == 3)
    {
        if(attr["attr"] != "first")
        {
            return false;
        }
        if(attr["att"] != "second")
        {
            return false;
        }
        if(attr["at"] != "third")
        {
            return false;
        }
        return true;
    }
    return false;
}



bool FileOpenFailedTest()
{
    XMLDocument document;
    return document.LoadFile("")._status == XMLParser::FileOpenFailed;
}

bool StartTagSpaceTest()
{
    LOAD_ERROR_STRING("<tag></  tag>", XMLParser::NoError);
}

bool EndTagSpaceTest1()
{
    LOAD_ERROR_STRING("<tag></tag  >", XMLParser::NoError);
}

bool EndTagSpaceTest2()
{
    LOAD_ERROR_STRING("<tag  ></tag>", XMLParser::NoError);
}


bool TagSyntaxErrorTest1()
{
    LOAD_ERROR_STRING("  <<", XMLParser::TagSyntaxError)
}

bool TagSyntaxErrorTest3()
{
    LOAD_ERROR_STRING("<w%></w%>", XMLParser::TagSyntaxError);
}

bool TagBadCloseError()
{
    LOAD_ERROR_STRING("<tag", XMLParser::TagBadCloseError)
}

bool TagNotMatchedErrorTest1()
{
    LOAD_ERROR_STRING("<address>This is wrong syntax</Address>", XMLParser::TagNotMatchedError);
}

bool TagNotMatchedErrorTest2()
{
    LOAD_ERROR_STRING("<tag><newTag></newTa></tag>", XMLParser::TagNotMatchedError);
}

bool TagNotMatchedErrorTest3()
{
    LOAD_ERROR_STRING("<tag>content", XMLParser::TagNotMatchedError);
}

bool AttributeSyntaxErrorNoValueTest()
{
    LOAD_ERROR_STRING("<tag a></tag>", XMLParser::AttributeSyntaxError)
}

bool AttributeSyntaxErrorTest1()
{
    LOAD_ERROR_STRING("<tag attr=att></tag>", XMLParser::AttributeSyntaxError);
}

bool AttributeSyntaxErrorTest2()
{
    LOAD_ERROR_STRING("<tag att%r=\"att\"></tag>", XMLParser::AttributeSyntaxError);
}

bool AttributeRepeatError()
{
    LOAD_ERROR_STRING(R"(<tag attr="first" attr="second"></tag>)", XMLParser::AttributeRepeatError);
}

inline std::map<std::string, std::function<bool(void)>> testFunction;

void TestBind()
{
    testFunction["OneTagTest1"] = OneTagTest1;
    testFunction["OneTagTest2"] = OneTagTest2;
    testFunction["OnePairTagTest"] = OnePairTagTest;
    testFunction["ContentTest"] = ContentTest;
    testFunction["CommentTest"] = CommentTest;
    testFunction["OneAttributeTest"] = OneAttributeTest;
    testFunction["AttributeTest"] = AttributeTest;
    testFunction["MultiAttributeTest"] = MultiAttributeTest;

    testFunction["FileOpenFailedTest"] = FileOpenFailedTest;
    testFunction["StartTagSpaceTest"] = StartTagSpaceTest;
    testFunction["EndTagSpaceTest1"] = EndTagSpaceTest1;
    testFunction["EndTagSpaceTest2"] = EndTagSpaceTest2;

    testFunction["TagSyntaxErrorTest1"] = TagSyntaxErrorTest1;
    testFunction["TagSyntaxErrorTest3"] = TagSyntaxErrorTest3;
    testFunction["TagBadCloseError"] = TagBadCloseError;
    testFunction["TagNotMatchedErrorTest1"] = TagNotMatchedErrorTest1;
    testFunction["TagNotMatchedErrorTest2"] = TagNotMatchedErrorTest2;
    testFunction["TagNotMatchedErrorTest3"] = TagNotMatchedErrorTest3;
    testFunction["AttributeSyntaxErrorNoValueTest"] = AttributeSyntaxErrorNoValueTest;
    testFunction["AttributeSyntaxErrorTest1"] = AttributeSyntaxErrorTest1;
    testFunction["AttributeSyntaxErrorTest2"] = AttributeSyntaxErrorTest2;
    testFunction["AttributeRepeatError"] = AttributeRepeatError;
}

void Test()
{
    // TODO:结果彩色显示
    TestBind();
    auto isOK = true;
    std::vector<std::string> passTest, notPassTest;
    for(const auto& v : testFunction)
    {
        if(v.second())
        {
            passTest.push_back(v.first);
        }
        else
        {
            notPassTest.push_back(v.first);
            isOK = false;
        }
    }

    for(const auto& v : passTest)
    {
        std::cout << "Pass Test: " << v << std::endl;
    }
    for(const auto& v : notPassTest)
    {
        std::cout << "Failed Test:" << v << std::endl;
    }

    std::cout << "Pass Test:" << passTest.size() << std::endl;
    std::cout << "Not Pass Test:" << notPassTest.size() << std::endl;
    if(isOK)
    {
        std::cout << "All Test Pass" << std::endl;
    }
}