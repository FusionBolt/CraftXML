//
// Created by fusionbolt on 2020/7/7.
//
#include <iostream>

#include "XML.hpp"

using namespace Craft;

#define DEBUG_INFO std::cout << "ErrorLine:" << __LINE__ << \
                         " ErrorFunction:" << __FUNCTION__ << std::endl;

#define ERROR_STR_OUTPUT(InputStr) std::cout << "Error Str:" << InputStr << "\nParseErrorType:" <<\
                         result.ErrorInfo() << std::endl;DEBUG_INFO;

#define ASSERT_EQ(Val1, Val2) if(Val1 != Val2) {DEBUG_INFO;return false;}
#define ASSERT_NOT_EQ(Val1, Val2) if(Val1 == Val2) {DEBUG_INFO;return false;}
#define ASSERT_TRUE(Val1) ASSERT_EQ(Val1, true)
#define ASSERT_FALSE(Val1) ASSERT_NOT_EQ(Val1, true)

#define LOAD_PARSE_STRING(str, parseStatus) XMLDocument document;\
                                            auto result = document.LoadString(str);\
                                            if(result._status != parseStatus)\
                                            {ERROR_STR_OUTPUT(str); return false;}

#define ASSERT_PARSE_STRING(str, parseStatus) {LOAD_PARSE_STRING(str, parseStatus);}

#define ASSERT_NO_ERROR_PARSE_STRING(str) LOAD_PARSE_STRING(str, XMLParser::NoError)

bool OneTagTest1()
{
    ASSERT_NO_ERROR_PARSE_STRING("<hr />");
    ASSERT_FALSE(document.FindChildrenByTagName("hr").empty())
    return true;
}
bool OneTagTest2()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag attr=\"test\" />");
    auto attr = document.FirstChild().GetAttributes();
    ASSERT_TRUE(!attr.empty())
    ASSERT_EQ(attr["attr"], "test")
    return true;
}

bool OnePairTagTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag></tag>")
    ASSERT_FALSE(document.FindChildrenByTagName("tag").empty())
    return true;
}

bool ContentTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag>content</tag>")
    auto elements = document.FindChildrenByTagName("tag");
    ASSERT_FALSE(elements.empty());
    ASSERT_EQ(elements[0].StringValue(), "content")
    return true;
}

bool CommentTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<!--tag-->")
    ASSERT_EQ(document.FindFirstChildByTagName("").GetContent(), "tag")
    return true;
}

bool CommentSyntaxErrorTest()
{
     ASSERT_PARSE_STRING("<!-- B+, B, or B--->", Craft::XMLParser::CommentSyntaxError)
     return true;
}

bool DeclarationTest()
{
     ASSERT_NO_ERROR_PARSE_STRING("<?xml version=\"1.0\" standalone='yes'?>")
     auto attributes = document.FirstChild().GetAttributes();
     ASSERT_EQ(attributes["version"], "1.0")
     ASSERT_EQ(attributes["standalone"], "yes")
     return true;
}

bool DeclarationErrorTest()
{
    ASSERT_PARSE_STRING(R"(<?xml version="1.0" standalone="t")",
                        Craft::XMLParser::DeclarationSyntaxError);
    ASSERT_PARSE_STRING(R"(<?xml standalone="t")",
                        Craft::XMLParser::DeclarationSyntaxError);
    return true;
}

bool PrologTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<?welcome to pg=10 of tutorials point?>")
    ASSERT_EQ(document.FindFirstChildByTagName("welcome").GetContent(),
            "to pg=10 of tutorials point")
    auto attrs = document.FindFirstChildByTagName("").GetAttributes();
    ASSERT_EQ(attrs["version"], "1.0")
    ASSERT_EQ(attrs["encoding"], "UTF-8")
    return true;
}

bool PrologErrorTest()
{
    ASSERT_PARSE_STRING("<?welcome to pg=10 of tutorials point?>\n"
                      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
                        Craft::XMLParser::DeclarationPositionError)
    return true;
}

bool OneAttributeTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag attr =   \"first\"></tag>")
    auto attr = document.FirstChild().GetAttributes();
    ASSERT_EQ(attr.size(), 1)
    ASSERT_EQ(attr["attr"], "first")
    return true;
}

bool AttributeQuoteTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag attr =   \'first\'></tag>")
    auto attr = document.FirstChild().GetAttributes();
    ASSERT_EQ(attr.size(), 1)
    ASSERT_EQ(attr["attr"], "first")
    return true;
}

bool MultiAttributeTest()
{
    ASSERT_NO_ERROR_PARSE_STRING(R"(<tag attr = "first"   att ="second" at= "third"></tag>)")
    auto attr = document.FirstChild().GetAttributes();
    ASSERT_EQ(attr.size(), 3)
    ASSERT_EQ(attr["attr"], "first")
    ASSERT_EQ(attr["att"], "second")
    ASSERT_EQ(attr["at"], "third")
    return true;
}

bool AttributeErrorTest()
{
#define ATTRIBUTE_ERROR_TEST(str) ASSERT_PARSE_STRING(str, XMLParser::AttributeSyntaxError)
    ATTRIBUTE_ERROR_TEST("<tag a></tag>)")
    ATTRIBUTE_ERROR_TEST("<tag attr=att></tag>")
    ATTRIBUTE_ERROR_TEST("<tag att%r=\"att\"></tag>")
    ASSERT_PARSE_STRING(R"(<tag attr="first" attr="second"></tag>)", XMLParser::AttributeRepeatError)
    return true;
}

bool FileOpenFailedTest()
{
    XMLDocument document;
    ASSERT_EQ(document.LoadFile("")._status, XMLParser::FileOpenFailed);
    return true;
}

bool TagSpaceTest()
{
    ASSERT_PARSE_STRING("<tag></  tag>", XMLParser::NoError);
    ASSERT_PARSE_STRING("<tag></tag  >", XMLParser::NoError);
    ASSERT_PARSE_STRING("<tag  ></tag>", XMLParser::NoError);
    return true;
}

bool TagSyntaxErrorTest()
{
    ASSERT_PARSE_STRING("  <<", XMLParser::TagSyntaxError)
    ASSERT_PARSE_STRING("<w%></w%>", XMLParser::TagSyntaxError);
    return true;
}

bool TagNotMatchedErrorTest()
{
   ASSERT_PARSE_STRING("<address>This is wrong syntax</Address>", XMLParser::TagNotMatchedError);
   ASSERT_PARSE_STRING("<tag><newTag></newTa></tag>", XMLParser::TagNotMatchedError);
   ASSERT_PARSE_STRING("<tag>content", XMLParser::TagNotMatchedError);
   return true;
}

bool TagBadCloseError()
{
    ASSERT_PARSE_STRING("<tag", XMLParser::TagBadCloseError)
    return true;
}

bool OperatorOverLoadTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag1></tag1><tag1></tag1><tag2></tag2>")
    ASSERT_EQ(document["tag1"].size(), 2)
    ASSERT_EQ(document["tag2"].size(), 1)
    ASSERT_EQ(document["tag3"].size(), 0)
    return true;
}

inline std::map<std::string, std::function<bool(void)>> testFunction;

void TestBind()
{
    testFunction["PrologTest"] = PrologTest;
    testFunction["PrologErrorTest"] = PrologErrorTest;

    testFunction["DeclarationTest"] = DeclarationTest;
    testFunction["DeclarationErrorTest"] = DeclarationErrorTest;

    testFunction["OneTagTest1"] = OneTagTest1;
    testFunction["OneTagTest2"] = OneTagTest2;
    testFunction["OnePairTagTest"] = OnePairTagTest;
    testFunction["ContentTest"] = ContentTest;
    testFunction["TagSyntaxErrorTest"] = TagSyntaxErrorTest;
    testFunction["TagSpaceTest"] = TagSpaceTest;
    testFunction["TagBadCloseError"] = TagBadCloseError;
    testFunction["TagNotMatchedErrorTest"] = TagNotMatchedErrorTest;

    testFunction["CommentTest"] = CommentTest;
    testFunction["CommentSyntaxErrorTest"] = CommentSyntaxErrorTest;

    testFunction["OneAttributeTest"] = OneAttributeTest;
    testFunction["AttributeQuoteTest"] = AttributeQuoteTest;
    testFunction["MultiAttributeTest"] = MultiAttributeTest;
    testFunction["AttributeErrorTest"] = AttributeErrorTest;

    testFunction["FileOpenFailedTest"] = FileOpenFailedTest;
    testFunction["OperatorOverLoadTest"] = OperatorOverLoadTest;
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
            std::cout << "Pass Test:" << v.first << std::endl;
            passTest.push_back(v.first);
        }
        else
        {
            notPassTest.push_back(v.first);
            std::cout << "Failed Test:" << v.first << std::endl;
            isOK = false;
        }
    }

    std::cout << "Pass Test:" << passTest.size() << std::endl;
    std::cout << "Not Pass Test:" << notPassTest.size() << std::endl;
    if(isOK)
    {
        std::cout << "All Test Pass" << std::endl;
    }
}