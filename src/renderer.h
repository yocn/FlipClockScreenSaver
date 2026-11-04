#pragma once
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <memory>
#include "core.h"

namespace flip {
struct Options { bool seconds=true, twelve=false, date=false, drift=true; };

inline void rounded(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float r) {
    float d=r*2;
    path.AddArc(x,y,d,d,180,90); path.AddArc(x+w-d,y,d,d,270,90);
    path.AddArc(x+w-d,y+h-d,d,d,0,90); path.AddArc(x,y+h-d,d,d,90,90);
    path.CloseFigure();
}

class Renderer {
    struct Card {
        std::wstring oldValue, newValue;
        std::unique_ptr<Gdiplus::Bitmap> oldFace, newFace;
    };
    std::array<Card,3> cards;
    int faceWidth=0, faceHeight=0;
    std::unique_ptr<Gdiplus::Bitmap> buffer;

    std::unique_ptr<Gdiplus::Bitmap> face(const std::wstring& value) {
        using namespace Gdiplus;
        auto b=std::make_unique<Bitmap>(faceWidth,faceHeight,PixelFormat32bppPARGB);
        Graphics g(b.get());
        g.Clear(Color(0,0,0,0));
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        float w=static_cast<float>(faceWidth),h=static_cast<float>(faceHeight);
        GraphicsPath shape; rounded(shape,0,0,w-1,h-1,w*0.045f);
        LinearGradientBrush body(PointF(0,0),PointF(0,h),Color(255,34,34,36),Color(255,23,23,25));
        g.FillPath(&body,&shape);
        auto state=g.Save(); g.SetClip(&shape);
        SolidBrush bottom(Color(255,28,28,30)); g.FillRectangle(&bottom,0.0f,h/2,w,h/2);
        Font font(L"Arial",w*0.69f,FontStyleBold,UnitPixel);
        StringFormat format(StringFormat::GenericTypographic());
        format.SetAlignment(StringAlignmentCenter); format.SetLineAlignment(StringAlignmentCenter);
        SolidBrush white(Color(255,237,236,232));
        g.DrawString(value.c_str(),-1,&font,RectF(0,-h*0.014f,w,h),&format,&white);
        SolidBrush shade(Color(18,0,0,0)); g.FillRectangle(&shade,0.0f,0.0f,w,h/2);
        g.Restore(state);
        Pen edge(Color(255,46,46,48),1); g.DrawPath(&edge,&shape);
        return b;
    }

public:
    void paint(HDC dc, int width, int height, const SYSTEMTIME& time, const Options& options,
               const std::array<std::wstring,3>& previous,
               const std::array<std::wstring,3>& current, float progress, bool preview) {
        using namespace Gdiplus;
        if(width<=0 || height<=0) return;
        if(!buffer || buffer->GetWidth()!=static_cast<UINT>(width) || buffer->GetHeight()!=static_cast<UINT>(height))
            buffer=std::make_unique<Bitmap>(width,height,PixelFormat32bppPARGB);
        Graphics g(buffer.get()); g.Clear(Color(255,0,0,0));
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.SetPixelOffsetMode(PixelOffsetModeHalf);
        auto l=layout(width,height,options.seconds);
        if(options.drift && !preview) {
            double minute=time.wHour*60+time.wMinute;
            float amplitude=std::min(width,height)*0.012f;
            l.x+=static_cast<float>(std::sin(minute*0.17))*amplitude;
            l.y+=static_cast<float>(std::cos(minute*0.13))*amplitude;
        }
        int fw=std::max(1,static_cast<int>(std::ceil(l.width))),fh=std::max(2,static_cast<int>(std::ceil(l.height)));
        if(fw!=faceWidth || fh!=faceHeight) { cards={}; faceWidth=fw; faceHeight=fh; }
        for(int i=0;i<l.count;++i) {
            auto& c=cards[i];
            if(!c.newFace || c.newValue!=current[i]) {
                if(c.newFace && c.newValue==previous[i]) { c.oldFace=std::move(c.newFace); c.oldValue=c.newValue; }
                c.newValue=current[i]; c.newFace=face(current[i]);
            }
            float x=l.x+i*(l.width+l.gap),y=l.y,h=l.height,w=l.width,half=h/2;
            auto drawHalf=[&](Bitmap* source,bool top,float destY,float destH) {
                if(destH<0.1f) return;
                g.DrawImage(source,RectF(x,destY,w,destH),0.0f,top?0.0f:faceHeight/2.0f,
                    static_cast<float>(faceWidth),faceHeight/2.0f,UnitPixel);
            };
            bool flipping=previous[i]!=current[i] && progress<1;
            if(flipping) {
                if(!c.oldFace || c.oldValue!=previous[i]) { c.oldValue=previous[i]; c.oldFace=face(previous[i]); }
                drawHalf(c.newFace.get(),true,y,half);
                drawHalf(c.oldFace.get(),false,y+half,half);
                auto p=phase(progress); float moving=half*p.scale;
                float movingY=p.oldTop?y+half-moving:y+half;
                drawHalf(p.oldTop?c.oldFace.get():c.newFace.get(),p.oldTop,movingY,moving);
                SolidBrush shadow(Color(static_cast<BYTE>(65*(1-p.scale)),0,0,0));
                g.FillRectangle(&shadow,x,movingY,w,moving);
            } else {
                g.DrawImage(c.newFace.get(),RectF(x,y,w,h));
            }
            // The fold and small side hinges remain stationary while the leaf turns.
            Pen fold(Color(255,4,4,5),std::max(1.0f,w*0.007f));
            g.DrawLine(&fold,x+1,y+half,x+w-1,y+half);
            Pen lip(Color(255,46,46,48),std::max(0.6f,w*0.002f));
            g.DrawLine(&lip,x+2,y+half+std::max(1.0f,w*0.005f),x+w-2,y+half+std::max(1.0f,w*0.005f));
            SolidBrush hinge(Color(255,9,9,10));
            for(float hx : {x+w*0.018f,x+w*0.962f})
                g.FillRectangle(&hinge,hx,y+half-h*0.036f,w*0.02f,h*0.072f);
        }
        if(options.twelve || options.date) {
            std::wstring label;
            if(options.twelve) label=time.wHour<12?L"AM":L"PM";
            if(options.date) {
                wchar_t date[100]{};
                GetDateFormatEx(LOCALE_NAME_USER_DEFAULT,DATE_SHORTDATE,&time,nullptr,date,100,nullptr);
                if(!label.empty()) label+=L"   ";
                label+=date;
            }
            Font font(L"Segoe UI",std::max(8.0f,l.width*0.055f),FontStyleRegular,UnitPixel);
            SolidBrush muted(Color(255,125,125,128));
            StringFormat format; format.SetAlignment(StringAlignmentCenter);
            g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
            g.DrawString(label.c_str(),-1,&font,RectF(0,l.y+l.height+l.height*0.12f,static_cast<float>(width),l.height*0.18f),&format,&muted);
        }
        Graphics screen(dc);
        screen.DrawImage(buffer.get(),Rect(0,0,width,height),0,0,width,height,UnitPixel);
    }
};
}
