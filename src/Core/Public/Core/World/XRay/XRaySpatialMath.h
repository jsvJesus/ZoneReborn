#pragma once

#include "Core/Math/Vector3.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace core::world::xray::spatial
{
    using math::Vector3;
    inline Vector3 Sub(Vector3 a, Vector3 b) noexcept { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
    inline Vector3 Add(Vector3 a, Vector3 b) noexcept { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
    inline Vector3 Mul(Vector3 a, float s) noexcept { return {a.x*s,a.y*s,a.z*s}; }
    inline float Dot(Vector3 a, Vector3 b) noexcept { return a.x*b.x+a.y*b.y+a.z*b.z; }
    inline Vector3 Cross(Vector3 a, Vector3 b) noexcept
    { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
    inline Vector3 Normalize(Vector3 v) noexcept
    { const float n=std::sqrt(Dot(v,v)); return n>1e-8f ? Mul(v,1.0f/n) : Vector3{}; }
    inline bool Finite(Vector3 v) noexcept
    { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }

    struct Bounds final
    {
        Vector3 minimum{std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max()};
        Vector3 maximum{-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};
        void Include(Vector3 p) noexcept
        {
            minimum={std::min(minimum.x,p.x),std::min(minimum.y,p.y),std::min(minimum.z,p.z)};
            maximum={std::max(maximum.x,p.x),std::max(maximum.y,p.y),std::max(maximum.z,p.z)};
        }
        void Include(const Bounds& b) noexcept { Include(b.minimum); Include(b.maximum); }
        Vector3 Center() const noexcept { return Mul(Add(minimum,maximum),0.5f); }
        float Radius() const noexcept { const auto d=Sub(maximum,minimum); return std::sqrt(Dot(d,d))*0.5f; }
        bool Valid() const noexcept
        { return Finite(minimum)&&Finite(maximum)&&minimum.x<=maximum.x&&minimum.y<=maximum.y&&minimum.z<=maximum.z; }
    };

    struct Plane final
    {
        Vector3 normal;
        float d=0.0f;
        float Distance(Vector3 p) const noexcept { return Dot(normal,p)+d; }
        void Normalize() noexcept
        { const float n=std::sqrt(Dot(normal,normal)); if(n>1e-8f){normal=Mul(normal,1.0f/n);d/=n;} }
    };

    struct Frustum final
    {
        std::array<Plane, 16> planes{};
        std::uint32_t count=0;
        bool Intersects(const Bounds& b) const noexcept
        {
            for(std::uint32_t i=0;i<count;++i)
            {
                const auto& p=planes[i];
                const Vector3 support{p.normal.x>=0?b.maximum.x:b.minimum.x,
                    p.normal.y>=0?b.maximum.y:b.minimum.y,p.normal.z>=0?b.maximum.z:b.minimum.z};
                if(p.Distance(support)<-0.002f) return false;
            }
            return true;
        }
        static Frustum FromMatrix(const std::array<float,16>& m) noexcept
        {
            // Row-vector DX11 convention: columns form clip half spaces, z in [0,w].
            Frustum f; f.count=6;
            f.planes[0]={{m[3]+m[0],m[7]+m[4],m[11]+m[8]},m[15]+m[12]};
            f.planes[1]={{m[3]-m[0],m[7]-m[4],m[11]-m[8]},m[15]-m[12]};
            f.planes[2]={{m[3]+m[1],m[7]+m[5],m[11]+m[9]},m[15]+m[13]};
            f.planes[3]={{m[3]-m[1],m[7]-m[5],m[11]-m[9]},m[15]-m[13]};
            f.planes[4]={{m[2],m[6],m[10]},m[14]};
            f.planes[5]={{m[3]-m[2],m[7]-m[6],m[11]-m[10]},m[15]-m[14]};
            for(auto& p:f.planes) p.Normalize();
            return f;
        }
    };

    inline bool RayBox(Vector3 start, Vector3 delta, const Bounds& b, float maximum) noexcept
    {
        float lo=0.0f,hi=maximum;
        const float s[3]{start.x,start.y,start.z},d[3]{delta.x,delta.y,delta.z};
        const float mn[3]{b.minimum.x,b.minimum.y,b.minimum.z},mx[3]{b.maximum.x,b.maximum.y,b.maximum.z};
        for(int a=0;a<3;++a)
        {
            if(std::abs(d[a])<1e-12f){if(s[a]<mn[a]||s[a]>mx[a])return false;continue;}
            float t0=(mn[a]-s[a])/d[a],t1=(mx[a]-s[a])/d[a];
            if(t0>t1)std::swap(t0,t1);
            lo=std::max(lo,t0);hi=std::min(hi,t1);if(lo>hi)return false;
        }
        return true;
    }

    inline bool RayTriangle(Vector3 start,Vector3 delta,Vector3 a,Vector3 b,Vector3 c,float& nearest,Vector3& normal) noexcept
    {
        const auto e1=Sub(b,a),e2=Sub(c,a),p=Cross(delta,e2);
        const float det=Dot(e1,p);if(std::abs(det)<1e-10f)return false;
        const float inv=1.0f/det;const auto t=Sub(start,a);const float u=Dot(t,p)*inv;
        if(u<0||u>1)return false;
        const auto q=Cross(t,e1);const float v=Dot(delta,q)*inv;
        if(v<0||u+v>1)return false;
        const float fraction=Dot(e2,q)*inv;
        if(fraction<0||fraction>nearest)return false;
        nearest=fraction;normal=Normalize(Cross(e1,e2));return true;
    }
}
