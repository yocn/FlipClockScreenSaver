#include "../src/core.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

int checks=0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) throw std::runtime_error(label);
}
int main() {
    using namespace flip;
    try {
        check(parse(L"").mode == Mode::Configure, "no args opens settings");
        check(parse(L" /S ").mode == Mode::Saver, "uppercase fullscreen");
        check(parse(L"-s").mode == Mode::Saver, "hyphen fullscreen");
        check(parse(L"/c:123").parent == 123, "config owner");
        check(parse(L"/p 4294967296").parent == 4294967296ULL, "64 bit preview handle");
        check(parse(L"/P:42").mode == Mode::Preview, "colon preview");
        for (auto s : {L"/p",L"/p 0",L"/p -1",L"/p 12abc",L"/p 18446744073709551616",L"/s junk",L"/unknown",L"/preview",L"/c:"})
            check(parse(s).mode == Mode::Invalid, "reject malformed argument");
        check(digits(0,0,0,false)[0] == L"00", "24h midnight");
        check(digits(0,9,5,true) == std::array<std::wstring,3>{L"12",L"09",L"05"}, "12h midnight and padding");
        check(digits(12,0,0,true)[0] == L"12", "noon");
        check(digits(23,59,59,true)[0] == L"11", "12h evening");
        for (auto size : {std::array<int,2>{1920,1080},{3840,2160},{1080,1920},{160,90},{1,1}}) {
            for (bool seconds : {false,true}) {
                auto l=layout(size[0],size[1],seconds);
                check(l.count==(seconds?3:2), "card count");
                check(l.width>0 && l.height>0, "nonzero card size");
                check(l.x>=0 && l.y>=0 && l.x+l.count*l.width+(l.count-1)*l.gap<=size[0]+0.01f && l.y+l.height<=size[1]+0.01f, "cards fit viewport");
            }
        }
        check(layout(0,0,true).count==0, "zero sized window");
        check(phase(0).oldTop && std::abs(phase(0).scale-1)<0.001f, "flip starts at old top");
        check(std::abs(phase(0.5f).scale)<0.001f, "flip edge on at midpoint");
        check(!phase(1).oldTop && std::abs(phase(1).scale-1)<0.001f, "flip ends at new bottom");
        std::cout << checks << " checks passed\n";
    } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
