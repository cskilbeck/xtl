(function() {

    const color_table = {
        "alice blue": 0xf0f8ff,
        "antique white": 0xfaebd7,
        "blue": 0x0000ff,
        "blue violet": 0x8a2be2,
        "cadet blue": 0x5f9ea0,
        "chartreuse": 0x7fff00,
        "chocolate": 0xd2691e,
        "coral": 0xff7f50,
        "cornflower": 0x6495ed,
        "cornsilk": 0xfff8dc,
        "crimson": 0xdc143c,
        "cyan": 0x00ffff,
        "dark blue": 0x00008b,
        "dark cyan": 0x008b8b,
        "dark goldenrod": 0xb8860b,
        "dark green": 0x006400,
        "dark khaki": 0xbdb76b,
        "dark magenta": 0x8b008b,
        "dark olive green": 0x556b2f,
        "dark orange": 0xff8c00,
        "dark orchid": 0x9932cc,
        "dark red": 0x8b0000,
        "dark salmon": 0xe9967a,
        "dark sea green": 0x8fbc8f,
        "dark slate blue": 0x483d8b,
        "dark turquoise": 0x00ced1,
        "dark violet": 0x9400d3,
        "deep pink": 0xff1493,
        "deep sky blue": 0x00bfff,
        "dodger blue": 0x1e90ff,
        "firebrick": 0xb22222,
        "floral white": 0xfffaf0,
        "forest green": 0x228b22,
        "fuchsia": 0xff00ff,
        "gainsboro": 0xdcdcdc,
        "ghost white": 0xf8f8ff,
        "gold": 0xffd700,
        "goldenrod": 0xdaa520,
        "green": 0x00ff00,
        "web green": 0x008000,
        "green yellow": 0xadff2f,
        "honeydew": 0xf0fff0,
        "hot pink": 0xff69b4,
        "indigo": 0x4b0082,
        "ivory": 0xfffff0,
        "khaki": 0xf0e68c,
        "lavender": 0xe6e6fa,
        "lavender blush": 0xfff0f5,
        "lawn green": 0x7cfc00,
        "lemon chiffon": 0xfffacd,
        "light blue": 0xadd8e6,
        "light coral": 0xf08080,
        "light cyan": 0xe0ffff,
        "light goldenrod": 0xfafad2,
        "light green": 0x90ee90,
        "light pink": 0xffb6c1,
        "light salmon": 0xffa07a,
        "light sea green": 0x20b2aa,
        "light sky blue": 0x87cefa,
        "light steel blue": 0xb0c4de,
        "light yellow": 0xffffe0,
        "lime": 0x00ff00,
        "lime green": 0x32cd32,
        "linen": 0xfaf0e6,
        "magenta": 0xff00ff,
        "maroon": 0xb03060,
        "web maroon": 0x7f0000,
        "medium blue": 0x0000cd,
        "medium orchid": 0xba55d3,
        "medium purple": 0x9370db,
        "medium sea green": 0x3cb371,
        "medium spring green": 0x00fa9a,
        "medium turquoise": 0x48d1cc,
        "medium violet red": 0xc71585,
        "midnight blue": 0x191970,
        "mint cream": 0xf5fffa,
        "misty rose": 0xffe4e1,
        "moccasin": 0xffe4b5,
        "navy blue": 0x000080,
        "old lace": 0xfdf5e6,
        "olive": 0x808000,
        "olive drab": 0x6b8e23,
        "orange": 0xffa500,
        "orange red": 0xff4500,
        "orchid": 0xda70d6,
        "pale goldenrod": 0xeee8aa,
        "pale green": 0x98fb98,
        "pale turquoise": 0xafeeee,
        "pale violet red": 0xdb7093,
        "papaya whip": 0xffefd5,
        "peach puff": 0xffdab9,
        "peru": 0xcd853f,
        "pink": 0xffc0cb,
        "plum": 0xdda0dd,
        "powder blue": 0xb0e0e6,
        "purple": 0xa020f0,
        "web purple": 0x7f007f,
        "rebecca purple": 0x663399,
        "red": 0xff0000,
        "rosy brown": 0xbc8f8f,
        "royal blue": 0x4169e1,
        "salmon": 0xfa8072,
        "sea green": 0x2e8b57,
        "seashell": 0xfff5ee,
        "sienna": 0xa0522d,
        "silver": 0xc0c0c0,
        "sky blue": 0x87ceeb,
        "slate blue": 0x6a5acd,
        "snow": 0xfffafa,
        "spring green": 0x00ff7f,
        "steel blue": 0x4682b4,
        "tan": 0xd2b48c,
        "teal": 0x008080,
        "thistle": 0xd8bfd8,
        "tomato": 0xff6347,
        "turquoise": 0x40e0d0,
        "violet": 0xee82ee,
        "wheat": 0xf5deb3,
        "white": 0xffffff,
        "white smoke": 0xf5f5f5,
        "yellow": 0xffff00,
        "yellow green": 0x9acd32,
        "off": 0x000000,
        "random": 0x80000000
    };

    exports.rgb_from_color_name = function(color_name) {
        return color_table[color_name] || 0;
    };

    exports.rgb_from_hsv = function(hsb) {
        let h = hsb.hue / 60;
        let s = hsb.saturation;
        let v = hsb.brightness;
        let i = Math.floor(h);
        let f = h - i;
        let p = v * (1 - s);
        let q = v * (1 - f * s);
        let t = v * (1 - (1 - f) * s);
        let r, g, b;
        switch (i % 6) {
            case 0:
                r = v, g = t, b = p;
                break;
            case 1:
                r = q, g = v, b = p;
                break;
            case 2:
                r = p, g = v, b = t;
                break;
            case 3:
                r = p, g = q, b = v;
                break;
            case 4:
                r = t, g = p, b = v;
                break;
            case 5:
                r = v, g = p, b = q;
                break;
        }
        return {
            r: Math.round(r * 255),
            g: Math.round(g * 255),
            b: Math.round(b * 255)
        };
    }

})();