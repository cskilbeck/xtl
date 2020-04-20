#pragma once

//////////////////////////////////////////////////////////////////////

// pi = 1024, amplitude = +255 .. -255
int sin(int x);

// 16 bit random number
int rand();
byte rand_byte();

// set the seed
void srand(int);

int strcmp(const char *p1, const char *p2);

int tolower(int c);

int atoi(char const *str);
uint32 atohex(char const *str);
int strchr(char const *p, int ch);                      // index of found char or strlen(p) so if(p[result] == 0) { not found }
int strncmp(char const *p1, char const *p2, int len);
int strlen(char const *p);
    
int ease(int x);

//////////////////////////////////////////////////////////////////////

inline void memcpy(byte *d, byte *s, int len)
{
    while(len-- != 0) {
        *d++ = *s++;
    }
}

//////////////////////////////////////////////////////////////////////

template<typename D, typename S> void copy(D *d, S *s)
{
    byte *ps = (byte *)s;
    byte *pd = (byte *)d;
    byte *pe = pd + sizeof(D);
    do {
        *pd++ = *ps++;
    } while(pd < pe);
}

//////////////////////////////////////////////////////////////////////
// set some memory to a byte value

inline void set_mem(uint8 *s, uint8 *e, uint8 value)
{
    do {
        *s++ = value;
    } while(s < e);
}

inline void memset(byte *d, byte v, int l)
{
    set_mem(d, d + l, v);
}

//////////////////////////////////////////////////////////////////////
// set some memory to zero

inline void clear_mem(uint8 *s, uint8 *e)
{
    do {
        *s++ = 0;
    } while(s < e);
}

inline void memclr(byte *d, int l)
{
    clear_mem(d, d + l);
}

//////////////////////////////////////////////////////////////////////
// get uppercase of a char

inline int toupper(int a)
{
    if(a >= 'a' && a <= 'z') {
        return a - ('a' - 'A');
    }
    return a;
}

//////////////////////////////////////////////////////////////////////

struct Random
{
    uint32 seed;

    Random(int start_seed);
    int next();
    byte next_byte();
    bool boolean();
};

//////////////////////////////////////////////////////////////////////

// # of elements in an array

template <typename T, uint32 N> static inline uint32 countof(T const (&)[N])
{
    return N;
}

// clear a var to zero

template <typename T> static inline void clear(T &i)
{
    uint8 *p = (uint8 *)&i;
    clear_mem(p, p + sizeof(i));
}

// set a var to a byte value

template <typename T> inline void set(T &i, uint8 b)
{
    uint8 *p = (uint8 *)&i;
    set_mem(p, p + sizeof(i), b);
}

//////////////////////////////////////////////////////////////////////

inline byte rand_channel()
{
	return (rand() & 0x78) + 128;
}
	
//////////////////////////////////////////////////////////////////////

struct color;

struct color32
{
    uint32 rgb;
    
    color32(uint32 c) : rgb(c)
    {
    }
    
    color32(int red, int green, int blue)
    {
        rgb = ((red & 0xff) << 16) | ((green & 0xff) << 8) | (blue & 0xff);
    }
    
    color32 &operator=(uint32 c)
    {
        rgb = c;
        return *this;
    }
    
    operator uint32() const
    {
        return rgb;
    }
    
    uint8 red() const
    {
        return rgb >> 16;
    }

    uint8 green() const
    {
        return rgb >> 8;
    }

    uint8 blue() const
    {
        return rgb;
    }
    
    operator color() const;

};

//////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)

struct color
{
    byte r, g, b;

    color()
    {
    }
    
    color(int red, int green, int blue) : r(red), g(green), b(blue)
    {
    }
    
    color(uint32 c) : r(c >> 16), g(c >> 8), b(c)
    {
    }

    color &operator=(uint32 const c)
    {
        r = c >> 16;
        g = c >> 8;
        b = c;
        return *this;
    }

    uint8 red() const
    {
        return r;
    }

    uint8 green() const
    {
        return g;
    }

    uint8 blue() const
    {
        return b;
    }
    
    operator uint32() const
    {
        return ((int)r << 16) | ((int)g << 8) | b;
    }
    
    operator color32() const
    {
        return color32(r, g, b);
    }
    
    void randomize()
    {
        int x = (rand() % 7) + 1;
        r = (x & 1) ? rand_channel() : 0;
        g = (x & 2) ? rand_channel() : 0;
        b = (x & 4) ? rand_channel() : 0;
    }

    static color random()
    {
        color c;
        c.randomize();
        return c;
    }

    enum
    {
        Red = 0xff0000,
        Green = 0x00ff00,
        Blue = 0x0000ff,
        Cyan = 0x00ffff,
        Magenta = 0xff00ff,
        Yellow = 0xffff00,
        White = 0xffffff
    };

};

#pragma pack(pop)

inline color32::operator color() const
{
    return color(rgb);
}

//////////////////////////////////////////////////////////////////////

template <typename T> static T lerp(T const &s, T const &d, int l)
{
    int r = (((d.red() - s.red()) * l) >> 8) + s.red();
    int g = (((d.green() - s.green()) * l) >> 8) + s.green();
    int b = (((d.blue() - s.blue()) * l) >> 8) + s.blue();
    return T(r, g, b);
}
