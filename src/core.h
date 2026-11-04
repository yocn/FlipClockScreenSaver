#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <limits>
namespace flip {
enum class Mode { Configure, Saver, Preview, Invalid };
struct Launch { Mode mode = Mode::Invalid; std::uintptr_t parent = 0; };
inline Launch parse(const std::wstring& raw) {
    auto start=raw.find_first_not_of(L" \t\r\n");
    if(start==std::wstring::npos) return {Mode::Configure,0};
    auto s=raw.substr(start,raw.find_last_not_of(L" \t\r\n")-start+1);
    if(s.size()<2 || (s[0]!=L'/' && s[0]!=L'-')) return {};
    auto flag=std::towlower(s[1]);
    Mode mode=flag==L's'?Mode::Saver:flag==L'c'?Mode::Configure:flag==L'p'?Mode::Preview:Mode::Invalid;
    if(mode==Mode::Invalid) return {};
    if(s.size()==2) return mode==Mode::Preview?Launch{}:Launch{mode,0};
    if(mode==Mode::Saver || (s[2]!=L':' && !std::iswspace(s[2]))) return {};
    auto pos=s.find_first_not_of(L" \t",s[2]==L':'?3:2);
    if(pos==std::wstring::npos) return {};
    std::uintptr_t handle=0;
    for(;pos<s.size();++pos) {
        if(s[pos]<L'0' || s[pos]>L'9') return {};
        auto digit=static_cast<unsigned>(s[pos]-L'0');
        if(handle>(std::numeric_limits<std::uintptr_t>::max()-digit)/10) return {};
        handle=handle*10+digit;
    }
    if(mode==Mode::Preview && handle==0) return {};
    return {mode,handle};
}
inline std::array<std::wstring, 3> digits(int hour, int minute, int second, bool twelve) {
    if(twelve) hour=hour%12==0?12:hour%12;
    auto pad=[](int n) { return (n<10?L"0":L"")+std::to_wstring(n); };
    return {pad(hour),pad(minute),pad(second)};
}
struct Layout { float x=0, y=0, width=0, height=0, gap=0; int count=0; };
inline Layout layout(int width, int height, bool seconds) {
    if(width<=0 || height<=0) return {};
    int count=seconds?3:2;
    float card=std::min(width*0.84f/(count+(count-1)*0.12f),height*0.58f/1.08f);
    float gap=card*0.12f, h=card*1.08f;
    return {(width-count*card-(count-1)*gap)/2,(height-h)/2,card,h,gap,count};
}
struct Phase { bool oldTop=true; float scale=1; };
inline Phase phase(float progress) {
    progress=std::clamp(progress,0.0f,1.0f);
    return {progress<0.5f,std::abs(std::cos(progress*3.14159265358979323846f))};
}
}
