#ifndef DITHERED_H
#define DITHERED_H

int ed_err_fract = 1024; // 10 bits of fraction. used to limit error diffusion to reduce color bleeding
int ed_pingpong_enable = 0; // alternative left-right and right-left iteration

/*
 * Error diffusion dithering class
 *
COLOR is a thing that can be calculated like an integer
R0, R1, R2 are rows of the weight table
shr specifies how many fractional bits the weight values have
*/
template<
    int const R0[], int const R1[], int const R2[],
    int cols, int off_x, int shr, typename COLOR>
struct DitherED {

    COLOR *buf[4];
    int img_w;
    int cur_x_=0;
    int pingpong=0;
    int pingpong_counter=0;
    int cur_x_inc=1;

    void forward()
    {
        cur_x_inc = 1;
        pingpong = 0;
        cur_x_ = 0;
    }

    void reverse()
    {
        cur_x_inc = -1;
        pingpong = -1;
        cur_x_ = img_w - 1;
    }

    DitherED(int w)
    {
        img_w = w;
        int bufsize = w * sizeof(COLOR);
        for( int i=0; i<4; ++i ) {
            buf[i] = new COLOR[w + 32] + 16;
            memset(buf[i], 0, bufsize);
        }
    }

    ~DitherED()
    {
        for( int i=0; i<4; ++i )
            delete ( buf[i] - 16 );
    }

    void endln() {
        auto b0 = buf[0];
        buf[0] = buf[1];
        buf[1] = buf[2];
        buf[2] = buf[3];
        buf[3] = b0;
    }

    template<typename Q>
    COLOR pixel1(COLOR c0, int cur_x, int neg, Q quantized)
    {
        int cur_x1 = cur_x + ( neg & 1 );
        COLOR cur_e = buf[0][cur_x] >> shr;
        COLOR c1 = quantized(c0 - cur_e);
        COLOR e = c1 - c0;

        e = e * ed_err_fract >> 10; // reduce distributed error by some fraction
        buf[3][cur_x] = 0; // wipe the next bottom line
        for( int dx=1; dx<cols-off_x; dx++)
            buf[0][cur_x1 + (dx^neg)] += e * R0[dx-1];
        for( int dx=0; dx<cols; dx++) {
            int xx = cur_x1 + ((dx - off_x) ^ neg );
            if (R1 != nullptr) buf[1][xx] += e * R1[dx];
            if (R2 != nullptr) buf[2][xx] += e * R2[dx];
        }
        return c1;
    }

    template<typename Q>
    COLOR pixel(COLOR c0, Q qqqq)
    {
        COLOR c = pixel1(c0, cur_x_, pingpong, qqqq);
        cur_x_ += cur_x_inc;
        if ( (unsigned) cur_x_ >= (unsigned) img_w ) {
            cur_x_ = 0;
            endln();
            if (ed_pingpong_enable && ++pingpong_counter == 15) {
                pingpong_counter = 0;
                if (pingpong) forward(); else reverse();
            }
        }
        return c;
    }
};

// jarvis judis ninke
#define K(x) (8192/48*x)
static constexpr int JJN0[] =                {K(7),K(5)};
static constexpr int JJN1[] = {K(3),K(5),K(7),K(5),K(3)};
static constexpr int JJN2[] = {K(1),K(3),K(5),K(3),K(1)};
typedef DitherED<JJN0,JJN1,JJN2, 5, 3, 13, ivec3> DitherJJN;

// sierra 2 row
static constexpr int S2R0[] =       {4,3};
static constexpr int S2R1[] = {1,2,3,2,1};
typedef DitherED<S2R0,S2R1,nullptr, 5, 3, 4, ivec3> DitherS2;

// sierra 3 row
static constexpr int S3R0[] =       {5,3};
static constexpr int S3R1[] = {2,4,5,4,2};
static constexpr int S3R2[] = {0,2,3,2,0};
typedef DitherED<S3R0,S3R1,nullptr, 5, 3, 5, ivec3> DitherS3;

// sierra lite
static constexpr int SL0[] =    {2};
static constexpr int SL1[] = {1, 1};
typedef DitherED<SL0,SL1,nullptr, 2, 1, 2, ivec3> DitherSL;

// floyd steinberg
static constexpr int FS0[] =       {7};
static constexpr int FS1[] = {3, 5, 1};
typedef DitherED<FS0,FS1,nullptr, 3, 1, 4, ivec3> DitherFS;

#endif // DITHERED_H
