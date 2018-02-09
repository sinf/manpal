#ifndef VEC3
#define VEC3

template<typename T> struct vec3 {
    T s[3];

    vec3() {}
    vec3(T x,T y, T z) { s[0]=x; s[1]=y; s[2]=z; }
    vec3(T x) { s[0]=x; s[1]=x; s[2]=x; }

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

    int rgb() {
        int b = 8, m = 255;
        return ( s[0] & m ) | ( s[1] & m ) << b | ( s[2] & m ) << 2*b;
    }
};

typedef vec3<int> ivec3;

#endif // VEC3

