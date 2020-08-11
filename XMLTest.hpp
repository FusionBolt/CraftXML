//
// Created by fusionbolt on 2020/7/7.
//
#include <iostream>

#include "CraftXML.hpp"

using namespace Craft;

#ifndef _WIN32
    #define RED "\033[31m"
    #define RESET "\033[0m"
#else
    #define RED ""
    #define RESET ""
#endif

#define DEBUG_INFO                                                             \
    std::cout << "ErrorLine:" << __LINE__ << " ErrorFunction:" << __FUNCTION__ \
              << std::endl;

#define OUTPUT_DIFF_VAL(Val1, Val2)                                            \
    std::cout << "Expect Val:" << Val2 << "\nActual Val:" << Val1 << std::endl;

#define ERROR_INFO(Val1, Val2)                                                 \
    std::cout << RED << "-----------------------------------" << std::endl;    \
    OUTPUT_DIFF_VAL(Val1, Val2)                                                \
    DEBUG_INFO;                                                                \
    std::cout << "-----------------------------------" << RESET << std::endl;

#define ERROR_STR_OUTPUT(InputStr)                                             \
    std::cout << "Error Str:" << InputStr                                      \
              << "\nParseErrorType:" << result.ErrorInfo() << std::endl;       \
    DEBUG_INFO;

#define ASSERT_EQ(Val1, Val2)                                                  \
    if (Val1 != Val2)                                                          \
    {                                                                          \
        ERROR_INFO(Val1, Val2);                                                \
        return false;                                                          \
    }
#define ASSERT_NOT_EQ(Val1, Val2)                                              \
    if (Val1 == Val2)                                                          \
    {                                                                          \
        ERROR_INFO(Val1, Val2);                                                \
        return false;                                                          \
    }
#define ASSERT_TRUE(Val1) ASSERT_EQ(Val1, true)
#define ASSERT_FALSE(Val1) ASSERT_NOT_EQ(Val1, true)

#define LOAD_PARSE_STRING(str, parseStatus)                                    \
    XMLDocument document;                                                      \
    auto result = document.LoadString(str);                                    \
    if (result._status != parseStatus)                                         \
    {                                                                          \
        ERROR_STR_OUTPUT(str);                                                 \
        return false;                                                          \
    }

#define ASSERT_PARSE_STRING(str, parseStatus)                                  \
    {                                                                          \
        LOAD_PARSE_STRING(str, parseStatus);                                   \
    }

#define ASSERT_NO_ERROR_PARSE_STRING(str)                                      \
    LOAD_PARSE_STRING(str, XMLParser::NoError)

bool NodeStructTest()
{
    XMLNode root;
    XMLNode n1("tag1");
    XMLNode n2("tag2");
    XMLNode n3("tag3");
    XMLNode n4("tag4");
    XMLNode n5("tag5");
    XMLNode n6("tag6");
    n1.AddChild(n2);
    n1.AddChild(n3);
    root.AddChild(n1);
    root.AddChild(n4);
    n2.AddChild(n5);
    n2.AddChild(n6);
    auto newN1 = root.FirstChild();
    ASSERT_EQ(newN1.GetNodeTag(), "tag1")
    auto newN2 = newN1.FirstChild();
    ASSERT_EQ(newN2.GetNodeTag(), "tag2")
    ASSERT_EQ(newN2.NextSibling().GetNodeTag(), "tag3")
    ASSERT_EQ(newN1.NextSibling().GetNodeTag(), "tag4")
    auto newN5 = newN2.FirstChild();
    ASSERT_EQ(newN5.GetNodeTag(), "tag5")
    ASSERT_EQ(newN5.NextSibling().GetNodeTag(), "tag6")
    return true;
}

bool OneTagTest1()
{
    ASSERT_NO_ERROR_PARSE_STRING("<hr />")
    ASSERT_FALSE(document.FindChildrenByTagName("hr").empty())
    return true;
}

bool OneTagTest2()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag attr=\"test\" />");
    auto attr = document.FirstChild().GetNodeAttributes();
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

bool ContentCharRefTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag>content&lt;contents</tag>")
    auto elements = document.FindChildrenByTagName("tag");
    ASSERT_FALSE(elements.empty());
    ASSERT_EQ(elements[0].StringValue(), "content<contents")
    return true;
}

bool ContentReferenceTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag>content&ref;contents</tag>")
    auto element = document.FirstChild();
    ASSERT_EQ(element.GetNodeTag(), "tag")
    ASSERT_EQ(element.GetNodeContent(), "content&ref;contents")
    return true;
}

bool ContentCDATATest()
{
    ASSERT_NO_ERROR_PARSE_STRING(
        "<script>text1\n"
        "<![CDATA["
        "<message> Welcome to TutorialsPoint </message>"
        "]]>text2"
        "</script>")
    auto scriptNode = document.FindFirstChildByTagName("script");
    ASSERT_EQ(scriptNode.GetNodeContent(), "text1\n")
    ASSERT_EQ(
        scriptNode.FindFirstChildByType(XMLNode::NodeCData).GetNodeContent(),
        "<message> Welcome to TutorialsPoint </message>");
    auto dataNode = scriptNode.FindChildrenByType(XMLNode::NodeData);
    ASSERT_EQ(dataNode[0].GetNodeContent(), "text1\n")
    ASSERT_EQ(dataNode[1].GetNodeContent(), "text2")
    return true;
}

bool EntityReferenceTest()
{
#define ENTITY_OK(Str, Char) i = 1;

    // ASSERT_EQ(p.ParseCharReference(Str, i), Char)
    XMLParser p;
    size_t i = 0;
    ENTITY_OK("&lt;", '<')
    ENTITY_OK("&gt;", '>')
    ENTITY_OK("&amp;", '&')
    ENTITY_OK("&apos;", '\'')
    ENTITY_OK("&quot;", '"')
    return true;
}

bool DoctypeTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<?xml version=\"1.0\"?>\n"
                                 "<!DOCTYPE student [\n"
                                 "\t<!ELEMENT student (#PCDATA)> \n"
                                 " \t<!ENTITY FullName \"\">\n"
                                 "]>\n"
                                 "\n"
                                 "<student>My Name</student>");
    auto s = document.FindFirstChildByTagName("student");
    ASSERT_EQ(s.GetNodeContent(), "My Name");
    return true;
}

bool CommentTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<!--tag-->")
    ASSERT_EQ(document.FindFirstChildByTagName("").GetNodeContent(), "tag")
    return true;
}

bool CommentSyntaxErrorTest()
{
    ASSERT_PARSE_STRING("<!-- B+, B, or B--->",
                        Craft::XMLParser::CommentSyntaxError)
    return true;
}

bool DeclarationTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<?xml version=\"1.0\" standalone='yes'?>")
    auto attributes = document.FirstChild().GetNodeAttributes();
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
    ASSERT_EQ(document.FindFirstChildByTagName("welcome").GetNodeContent(),
              "to pg=10 of tutorials point")
    auto attrs = document.FindFirstChildByTagName("").GetNodeAttributes();
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
    auto attr = document.FirstChild().GetNodeAttributes();
    ASSERT_EQ(attr.size(), 1)
    ASSERT_EQ(attr["attr"], "first")
    return true;
}

bool AttributeQuoteTest()
{
    ASSERT_NO_ERROR_PARSE_STRING("<tag attr =   \'first\'></tag>")
    auto attr = document.FirstChild().GetNodeAttributes();
    ASSERT_EQ(attr.size(), 1)
    ASSERT_EQ(attr["attr"], "first")
    return true;
}

bool MultiAttributeTest()
{
    ASSERT_NO_ERROR_PARSE_STRING(
        R"(<tag attr = "first"   att ="second" at= "third"></tag>)")
    auto attr = document.FirstChild().GetNodeAttributes();
    ASSERT_EQ(attr.size(), 3)
    ASSERT_EQ(attr["attr"], "first")
    ASSERT_EQ(attr["att"], "second")
    ASSERT_EQ(attr["at"], "third")
    return true;
}

bool AttributeErrorTest()
{
#define ATTRIBUTE_ERROR_TEST(str)                                              \
    ASSERT_PARSE_STRING(str, XMLParser::AttributeSyntaxError)
    ATTRIBUTE_ERROR_TEST("<tag a></tag>)")
    ATTRIBUTE_ERROR_TEST("<tag attr=att></tag>")
    ATTRIBUTE_ERROR_TEST("<tag att%r=\"att\"></tag>")
    ASSERT_PARSE_STRING(R"(<tag attr="first" attr="second"></tag>)",
                        XMLParser::AttributeRepeatError)
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
    ASSERT_PARSE_STRING("<address>This is wrong syntax</Address>",
                        XMLParser::TagNotMatchedError);
    ASSERT_PARSE_STRING("<tag><newTag></newTa></tag>",
                        XMLParser::TagNotMatchedError);
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
    testFunction["NodeStructTest"] = NodeStructTest;

    testFunction["PrologTest"] = PrologTest;
    testFunction["PrologErrorTest"] = PrologErrorTest;

    testFunction["DeclarationTest"] = DeclarationTest;
    testFunction["DeclarationErrorTest"] = DeclarationErrorTest;

    testFunction["OneTagTest1"] = OneTagTest1;
    testFunction["OneTagTest2"] = OneTagTest2;
    testFunction["OnePairTagTest"] = OnePairTagTest;
    testFunction["ContentTest"] = ContentTest;
    testFunction["ContentCharRefTest"] = ContentCharRefTest;
    testFunction["ContentReferenceTest"] = ContentReferenceTest;
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

    testFunction["DOCTYPETest"] = DoctypeTest;

    testFunction["ContentCDATATest"] = ContentCDATATest;

    testFunction["FileOpenFailedTest"] = FileOpenFailedTest;
    testFunction["OperatorOverLoadTest"] = OperatorOverLoadTest;

    testFunction["EntityReferenceTest"] = EntityReferenceTest;
}
#define RED "\033[31m" /* Red */
void Test()
{
    TestBind();
    auto isOK = true;
    std::vector<std::string> passTest, notPassTest;
    for (const auto &v : testFunction)
    {
        if (v.second())
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
    if (isOK)
    {
        std::cout << "All Test Pass" << std::endl;
    }
}