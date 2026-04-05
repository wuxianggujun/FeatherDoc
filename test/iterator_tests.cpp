#include <sstream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <featherdoc.hpp>
#include <featherdoc_iterator.hpp>

TEST_CASE("checks contents of my_test.docx with iterator")
{
    featherdoc::Document doc("my_test.docx");
    CHECK_FALSE(doc.open());

    std::ostringstream ss;

    for (featherdoc::Paragraph p : doc.paragraphs())
    {
        for (featherdoc::Run r : p.runs())
        {
            ss << r.get_text() << std::endl;
            //std::puts(r.get_text().c_str());
        }
    }
    std::puts(ss.str().c_str());
    CHECK_EQ("This is a test\nokay?\n", ss.str());
}

namespace featherdoc {
struct MyTestObject final {
    int current = 42;
    int parent = 1;
    int j = 86;
    MyTestObject(int parent, int current) : parent(parent), current(current){}
    MyTestObject() = default;
    MyTestObject& next() { ++current; return *this;}
    bool has_next() const { return current!=j;}
    bool operator== (MyTestObject const& other) const {return other.current==current && other.j==j;}
};
// Entry point
auto begin(MyTestObject const& obj) -> Iterator<MyTestObject, int> {
  return Iterator<MyTestObject, int, int>(obj.parent, obj.current);
}

auto end(MyTestObject const& obj) -> Iterator<MyTestObject, int> {
  return Iterator<MyTestObject, int, int>(obj.parent, static_cast<decltype(obj.current)>(0));
}
}  // namespace featherdoc

TEST_CASE("Check equality in")
{
    auto const testObject = featherdoc::MyTestObject{};
    auto p1 = begin(testObject);
    auto p2 = begin(testObject);
    CHECK_EQ(p1, p2);
    featherdoc::Document doc("my_test.docx");
    CHECK_FALSE(doc.open());

    CHECK_EQ(begin(doc.paragraphs()), begin(doc.paragraphs()));
    
}
