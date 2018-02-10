#ifndef VEC3
#define VEC3

#define qcolor_to_ivec3(c) ivec3(c.redF()*0x7fff, c.greenF()*0x7fff, c.blueF()*0x7fff)
#define qcolor_to_ivec3_s(c) ivec3(sRGBtoLf(c.redF())*0x7fff, sRGBtoLf(c.greenF())*0x7fff, sRGBtoLf(c.blueF())*0x7fff)

template<typename T> struct vec3 {
    T s[3];

    vec3() {}
    vec3(T x,T y, T z) { s[0]=x; s[1]=y; s[2]=z; }
    vec3(T x) { s[0]=x; s[1]=x; s[2]=x; }

    template<typename C>
    vec3(C c) { s[0]=c.red(); s[1]=c.green(); s[2]=c.blue(); }

auto operator+(vec3<T> a) { return vec3<T>(a.s[0]+s[0], a.s[1]+s[1], a.s[2]+s[2]); }
auto operator-(vec3<T> a) { return vec3<T>(s[0]-a.s[0], s[1]-a.s[1], s[2]-a.s[2]); }
auto operator*(vec3<T> a) { return vec3<T>(a.s[0]*s[0], a.s[1]*s[1], a.s[2]*s[2]); }
auto operator/(vec3<T> a) { return vec3<T>(a.s[0]/s[0], a.s[1]/s[1], a.s[2]/s[2]); }

    void operator+=(vec3<T>a) { *this = *this + a; }
    void operator-=(vec3<T>a) { *this = *this - a; }
    void operator*=(vec3<T>a) { *this = *this * a; }
    void operator/=(vec3<T>a) { *this = *this / a; }

    auto operator+(T a) { return vec3<T>(s[0]+a, s[1]+a, s[2]+a); }
    auto operator-(T a) { return vec3<T>(s[0]-a, s[1]-a, s[2]-a); }
    auto operator*(T a) { return vec3<T>(s[0]*a, s[1]*a, s[2]*a); }
    auto operator/(T a) { return vec3<T>(s[0]/a, s[1]/a, s[2]/a); }

    auto operator<<(T i) { return vec3<T>(s[0]<<i, s[1]<<i, s[2]<<i); }
    auto operator>>(T i) { return vec3<T>(s[0]>>i, s[1]>>i, s[2]>>i); }
    auto operator&(T i) { return vec3<T>(s[0]&i, s[1]&i, s[2]&i); }
    auto operator^(T i) { return vec3<T>(s[0]^i, s[1]^i, s[2]^i); }

    auto step(T edge,T lo,T hi) {
        return vec3<T>(
        s[0] < edge ? lo : hi,
        s[1] < edge ? lo : hi,
        s[2] < edge ? lo : hi);
    }

    template<typename H>
    auto lookup(H table[]) { return vec3<T>(table[s[0]], table[s[1]], table[s[2]]); }

    template<typename A>
    A lensq() { return (A)s[0]*s[0] + (A)s[1]*s[1] + (A)s[2]*s[2]; }

    template<typename F>
    auto f(F f) {
        return vec3<T>(f(s[0]), f(s[1]), f(s[2]));
    }
};

typedef vec3<int> ivec3;

#endif // VEC3

