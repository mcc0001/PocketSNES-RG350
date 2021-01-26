/*
 * https://raw.github.com/dmitrysmagin/snes9x4d-rzx50/master/dingux-sdl/scaler.cpp
 */

#include "scaler.h"

#define AVERAGE(z, x) ((((z) & 0xF7DEF7DE) >> 1) + (((x) & 0xF7DEF7DE) >> 1))
#define AVERAGEHI(AB) ((((AB) & 0xF7DE0000) >> 1) + (((AB) & 0xF7DE) << 15))
#define AVERAGELO(CD) ((((CD) & 0xF7DE) >> 1) + (((CD) & 0xF7DE0000) >> 17))

void (*upscale_p)(uint32_t *dst, uint32_t *src, int width) = upscale_256x224_to_320x240;

/*
 * Approximately bilinear scaler, 256x224 to 320x240
 *
 * Copyright (C) 2014 hi-ban, Nebuleon <nebuleon.fumika@gmail.com>
 *
 * This function and all auxiliary functions are free software; you can
 * redistribute them and/or modify them under the terms of the GNU Lesser
 * General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * These functions are distributed in the hope that they will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Support math
#define Half(A) (((A) >> 1) & 0x7BEF)
#define Quarter(A) (((A) >> 2) & 0x39E7)
// Error correction expressions to piece back the lower bits together
#define RestHalf(A) ((A) & 0x0821)
#define RestQuarter(A) ((A) & 0x1863)

// Error correction expressions for quarters of pixels
#define Corr1_3(A, B)     Quarter(RestQuarter(A) + (RestHalf(B) << 1) + RestQuarter(B))
#define Corr3_1(A, B)     Quarter((RestHalf(A) << 1) + RestQuarter(A) + RestQuarter(B))

// Error correction expressions for halves
#define Corr1_1(A, B)     ((A) & (B) & 0x0821)

// Quarters
#define Weight1_3(A, B)   (Quarter(A) + Half(B) + Quarter(B) + Corr1_3(A, B))
#define Weight3_1(A, B)   (Half(A) + Quarter(A) + Quarter(B) + Corr3_1(A, B))

// Halves
#define Weight1_1(A, B)   (Half(A) + Half(B) + Corr1_1(A, B))

#define cR(A) (((A) & 0xf800) >> 8)
#define cG(A) (((A) & 0x7e0) >> 3)
#define cB(A) (((A) & 0x1f) << 3)

#define Weight2_1(A, B)  ((((cR(A) + cR(A) + cR(B)) / 3) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(B)) / 3) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(B)) / 3) & 0xf8) >> 3)


uint16_t hexcolor_to_rgb565(const uint32_t color)
{
    uint8_t colorr = ((color >> 16) & 0xFF);
    uint8_t colorg = ((color >> 8) & 0xFF);
    uint8_t colorb = ((color) & 0xFF);

    uint16_t r = ((colorr >> 3) & 0x1f) << 11;
    uint16_t g = ((colorg >> 2) & 0x3f) << 5;
    uint16_t b = (colorb >> 3) & 0x1f;

    return (uint16_t) (r | g | b);
}

/* Upscales a 256x224 image to 320x240 using an approximate bilinear
 * resampling algorithm that only uses integer math.
 *
 * Input:
 *   src: A packed 256x224 pixel image. The pixel format of this image is
 *     RGB 565.
 *   width: The width of the source image. Should always be 256.
 * Output:
 *   dst: A packed 320x240 pixel image. The pixel format of this image is
 *     RGB 565.
 */
void upscale_256x224_to_320x240_bilinearish(uint32_t* dst, uint32_t* src, int width)
{

    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 14 of 16 vertically.
    // Each block of 4x16 becomes 5x17.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 14; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 256 * 16;
        BlockDst = Dst16 + BlockY * 320 * 17;
        for (BlockX = 0; BlockX < 64; BlockX++)
        {
            /* Horizontally:
             * Before(4):
             * (a)(b)(c)(d)
             * After(5):
             * (a)(abbb)(bc)(cccd)(d)
             *
             * Vertically:
             * Before(16): After(17):
             * (a)       (a)
             * (b)       (b)
             * (c)       (c)
             * (d)       (cddd)
             * (e)       (deee)
             * (f)       (efff)
             * (g)       (fggg)
             * (h)       (ghhh)
             * (i)       (hi)
             * (j)       (iiij)
             * (k)       (jjjk)
             * (l)       (kkkl)
             * (m)       (lllm)
             * (n)       (mmmn)
             * (o)       (n)
             * (p)       (o)
             *           (p)
             */

            // -- Row 1 --
            uint16_t  _1 = *(BlockSrc               );
            *(BlockDst               ) = _1;
            uint16_t  _2 = *(BlockSrc            + 1);
            *(BlockDst            + 1) = Weight1_3( _1,  _2);
            uint16_t  _3 = *(BlockSrc            + 2);
            *(BlockDst            + 2) = Weight1_1( _2,  _3);
            uint16_t  _4 = *(BlockSrc            + 3);
            *(BlockDst            + 3) = Weight3_1( _3,  _4);
            *(BlockDst            + 4) = _4;

            // -- Row 2 --
            uint16_t  _5 = *(BlockSrc + 256 *  1    );
            *(BlockDst + 320 *  1    ) = _5;
            uint16_t  _6 = *(BlockSrc + 256 *  1 + 1);
            *(BlockDst + 320 *  1 + 1) = Weight1_3( _5,  _6);
            uint16_t  _7 = *(BlockSrc + 256 *  1 + 2);
            *(BlockDst + 320 *  1 + 2) = Weight1_1( _6,  _7);
            uint16_t  _8 = *(BlockSrc + 256 *  1 + 3);
            *(BlockDst + 320 *  1 + 3) = Weight3_1( _7,  _8);
            *(BlockDst + 320 *  1 + 4) = _8;

            // -- Row 3 --
            uint16_t  _9 = *(BlockSrc + 256 *  2    );
            *(BlockDst + 320 *  2    ) = _9;
            uint16_t  _10 = *(BlockSrc + 256 *  2 + 1);
            *(BlockDst + 320 *  2 + 1) = Weight1_3( _9, _10);
            uint16_t  _11 = *(BlockSrc + 256 *  2 + 2);
            *(BlockDst + 320 *  2 + 2) = Weight1_1(_10, _11);
            uint16_t  _12 = *(BlockSrc + 256 *  2 + 3);
            *(BlockDst + 320 *  2 + 3) = Weight3_1(_11, _12);
            *(BlockDst + 320 *  2 + 4) = _12;

            // -- Row 4 --
            uint16_t _13 = *(BlockSrc + 256 *  3    );
            *(BlockDst + 320 *  3    ) = Weight1_3( _9, _13);
            uint16_t _14 = *(BlockSrc + 256 *  3 + 1);
            *(BlockDst + 320 *  3 + 1) = Weight1_3(Weight1_3( _9, _10), Weight1_3(_13, _14));
            uint16_t _15 = *(BlockSrc + 256 *  3 + 2);
            *(BlockDst + 320 *  3 + 2) = Weight1_3(Weight1_1(_10, _11), Weight1_1(_14, _15));
            uint16_t _16 = *(BlockSrc + 256 *  3 + 3);
            *(BlockDst + 320 *  3 + 3) = Weight1_3(Weight3_1(_11, _12), Weight3_1(_15, _16));
            *(BlockDst + 320 *  3 + 4) = Weight1_3(_12, _16);

            // -- Row 5 --
            uint16_t _17 = *(BlockSrc + 256 *  4    );
            *(BlockDst + 320 *  4    ) = Weight1_3(_13, _17);
            uint16_t _18 = *(BlockSrc + 256 *  4 + 1);
            *(BlockDst + 320 *  4 + 1) = Weight1_3(Weight1_3(_13, _14), Weight1_3(_17, _18));
            uint16_t _19 = *(BlockSrc + 256 *  4 + 2);
            *(BlockDst + 320 *  4 + 2) = Weight1_3(Weight1_1(_14, _15), Weight1_1(_18, _19));
            uint16_t _20 = *(BlockSrc + 256 *  4 + 3);
            *(BlockDst + 320 *  4 + 3) = Weight1_3(Weight3_1(_15, _16), Weight3_1(_19, _20));
            *(BlockDst + 320 *  4 + 4) = Weight1_3(_16, _20);

            // -- Row 6 --
            uint16_t _21 = *(BlockSrc + 256 *  5    );
            *(BlockDst + 320 *  5    ) = Weight1_3(_17, _21);
            uint16_t _22 = *(BlockSrc + 256 *  5 + 1);
            *(BlockDst + 320 *  5 + 1) = Weight1_3(Weight1_3(_17, _18), Weight1_3(_21, _22));
            uint16_t _23 = *(BlockSrc + 256 *  5 + 2);
            *(BlockDst + 320 *  5 + 2) = Weight1_3(Weight1_1(_18, _19), Weight1_1(_22, _23));
            uint16_t _24 = *(BlockSrc + 256 *  5 + 3);
            *(BlockDst + 320 *  5 + 3) = Weight1_3(Weight3_1(_19, _20), Weight3_1(_23, _24));
            *(BlockDst + 320 *  5 + 4) = Weight1_3(_20, _24);

            // -- Row 7 --
            uint16_t _25 = *(BlockSrc + 256 *  6    );
            *(BlockDst + 320 *  6    ) = Weight1_3(_21, _25);
            uint16_t _26 = *(BlockSrc + 256 *  6 + 1);
            *(BlockDst + 320 *  6 + 1) = Weight1_3(Weight1_3(_21, _22), Weight1_3(_25, _26));
            uint16_t _27 = *(BlockSrc + 256 *  6 + 2);
            *(BlockDst + 320 *  6 + 2) = Weight1_3(Weight1_1(_22, _23), Weight1_1(_26, _27));
            uint16_t _28 = *(BlockSrc + 256 *  6 + 3);
            *(BlockDst + 320 *  6 + 3) = Weight1_3(Weight3_1(_23, _24), Weight3_1(_27, _28));
            *(BlockDst + 320 *  6 + 4) = Weight1_3(_24, _28);

            // -- Row 8 --
            uint16_t _29 = *(BlockSrc + 256 *  7    );
            *(BlockDst + 320 *  7    ) = Weight1_3(_25, _29);
            uint16_t _30 = *(BlockSrc + 256 *  7 + 1);
            *(BlockDst + 320 *  7 + 1) = Weight1_3(Weight1_3(_25, _26), Weight1_3(_29, _30));
            uint16_t _31 = *(BlockSrc + 256 *  7 + 2);
            *(BlockDst + 320 *  7 + 2) = Weight1_3(Weight1_1(_26, _27), Weight1_1(_30, _31));
            uint16_t _32 = *(BlockSrc + 256 *  7 + 3);
            *(BlockDst + 320 *  7 + 3) = Weight1_3(Weight3_1(_27, _28), Weight3_1(_31, _32));
            *(BlockDst + 320 *  7 + 4) = Weight1_3(_28, _32);

            // -- Row 9 --
            uint16_t _33 = *(BlockSrc + 256 *  8    );
            *(BlockDst + 320 *  8    ) = Weight1_1(_29, _33);
            uint16_t _34 = *(BlockSrc + 256 *  8 + 1);
            *(BlockDst + 320 *  8 + 1) = Weight1_1(Weight1_3(_29, _30), Weight1_3(_33, _34));
            uint16_t _35 = *(BlockSrc + 256 *  8 + 2);
            *(BlockDst + 320 *  8 + 2) = Weight1_1(Weight1_1(_30, _31), Weight1_1(_34, _35));
            uint16_t _36 = *(BlockSrc + 256 *  8 + 3);
            *(BlockDst + 320 *  8 + 3) = Weight1_1(Weight3_1(_31, _32), Weight3_1(_35, _36));
            *(BlockDst + 320 *  8 + 4) = Weight1_1(_32, _36);

            // -- Row 10 --
            uint16_t _37 = *(BlockSrc + 256 *  9    );
            *(BlockDst + 320 *  9    ) = Weight3_1(_33, _37);
            uint16_t _38 = *(BlockSrc + 256 *  9 + 1);
            *(BlockDst + 320 *  9 + 1) = Weight3_1(Weight1_3(_33, _34), Weight1_3(_37, _38));
            uint16_t _39 = *(BlockSrc + 256 *  9 + 2);
            *(BlockDst + 320 *  9 + 2) = Weight3_1(Weight1_1(_34, _35), Weight1_1(_38, _39));
            uint16_t _40 = *(BlockSrc + 256 *  9 + 3);
            *(BlockDst + 320 *  9 + 3) = Weight3_1(Weight3_1(_35, _36), Weight3_1(_39, _40));
            *(BlockDst + 320 *  9 + 4) = Weight3_1(_36, _40);

            // -- Row 11 --
            uint16_t _41 = *(BlockSrc + 256 * 10    );
            *(BlockDst + 320 * 10    ) = Weight3_1(_37, _41);
            uint16_t _42 = *(BlockSrc + 256 * 10 + 1);
            *(BlockDst + 320 * 10 + 1) = Weight3_1(Weight1_3(_37, _38), Weight1_3(_41, _42));
            uint16_t _43 = *(BlockSrc + 256 * 10 + 2);
            *(BlockDst + 320 * 10 + 2) = Weight3_1(Weight1_1(_38, _39), Weight1_1(_42, _43));
            uint16_t _44 = *(BlockSrc + 256 * 10 + 3);
            *(BlockDst + 320 * 10 + 3) = Weight3_1(Weight3_1(_39, _40), Weight3_1(_43, _44));
            *(BlockDst + 320 * 10 + 4) = Weight3_1(_40, _44);

            // -- Row 12 --
            uint16_t _45 = *(BlockSrc + 256 * 11    );
            *(BlockDst + 320 * 11    ) = Weight3_1(_41, _45);
            uint16_t _46 = *(BlockSrc + 256 * 11 + 1);
            *(BlockDst + 320 * 11 + 1) = Weight3_1(Weight1_3(_41, _42), Weight1_3(_45, _46));
            uint16_t _47 = *(BlockSrc + 256 * 11 + 2);
            *(BlockDst + 320 * 11 + 2) = Weight3_1(Weight1_1(_42, _43), Weight1_1(_46, _47));
            uint16_t _48 = *(BlockSrc + 256 * 11 + 3);
            *(BlockDst + 320 * 11 + 3) = Weight3_1(Weight3_1(_43, _44), Weight3_1(_47, _48));
            *(BlockDst + 320 * 11 + 4) = Weight3_1(_44, _48);

            // -- Row 13 --
            uint16_t _49 = *(BlockSrc + 256 * 12    );
            *(BlockDst + 320 * 12    ) = Weight3_1(_45, _49);
            uint16_t _50 = *(BlockSrc + 256 * 12 + 1);
            *(BlockDst + 320 * 12 + 1) = Weight3_1(Weight1_3(_45, _46), Weight1_3(_49, _50));
            uint16_t _51 = *(BlockSrc + 256 * 12 + 2);
            *(BlockDst + 320 * 12 + 2) = Weight3_1(Weight1_1(_46, _47), Weight1_1(_50, _51));
            uint16_t _52 = *(BlockSrc + 256 * 12 + 3);
            *(BlockDst + 320 * 12 + 3) = Weight3_1(Weight3_1(_47, _48), Weight3_1(_51, _52));
            *(BlockDst + 320 * 12 + 4) = Weight3_1(_48, _52);

            // -- Row 14 --
            uint16_t _53 = *(BlockSrc + 256 * 13    );
            *(BlockDst + 320 * 13    ) = Weight3_1(_49, _53);
            uint16_t _54 = *(BlockSrc + 256 * 13 + 1);
            *(BlockDst + 320 * 13 + 1) = Weight3_1(Weight1_3(_49, _50), Weight1_3(_53, _54));
            uint16_t _55 = *(BlockSrc + 256 * 13 + 2);
            *(BlockDst + 320 * 13 + 2) = Weight3_1(Weight1_1(_50, _51), Weight1_1(_54, _55));
            uint16_t _56 = *(BlockSrc + 256 * 13 + 3);
            *(BlockDst + 320 * 13 + 3) = Weight3_1(Weight3_1(_51, _52), Weight3_1(_55, _56));
            *(BlockDst + 320 * 13 + 4) = Weight3_1(_52, _56);

            // -- Row 15 --
            *(BlockDst + 320 * 14    ) = _53;
            *(BlockDst + 320 * 14 + 1) = Weight1_3(_53, _54);
            *(BlockDst + 320 * 14 + 2) = Weight1_1(_54, _55);
            *(BlockDst + 320 * 14 + 3) = Weight3_1(_55, _56);
            *(BlockDst + 320 * 14 + 4) = _56;

            // -- Row 16 --
            uint16_t _57 = *(BlockSrc + 256 * 14    );
            *(BlockDst + 320 * 15    ) = _57;
            uint16_t _58 = *(BlockSrc + 256 * 14 + 1);
            *(BlockDst + 320 * 15 + 1) = Weight1_3(_57, _58);
            uint16_t _59 = *(BlockSrc + 256 * 14 + 2);
            *(BlockDst + 320 * 15 + 2) = Weight1_1(_58, _59);
            uint16_t _60 = *(BlockSrc + 256 * 14 + 3);
            *(BlockDst + 320 * 15 + 3) = Weight3_1(_59, _60);
            *(BlockDst + 320 * 15 + 4) = _60;

            // -- Row 17 --
            uint16_t _61 = *(BlockSrc + 256 * 15    );
            *(BlockDst + 320 * 16    ) = _61;
            uint16_t _62 = *(BlockSrc + 256 * 15 + 1);
            *(BlockDst + 320 * 16 + 1) = Weight1_3(_61, _62);
            uint16_t _63 = *(BlockSrc + 256 * 15 + 2);
            *(BlockDst + 320 * 16 + 2) = Weight1_1(_62, _63);
            uint16_t _64 = *(BlockSrc + 256 * 15 + 3);
            *(BlockDst + 320 * 16 + 3) = Weight3_1(_63, _64);
            *(BlockDst + 320 * 16 + 4) = _64;

            BlockSrc += 4;
            BlockDst += 5;
        }
    }
}

void upscale_256x240_to_320x240_bilinearish(uint32_t* dst, uint32_t* src, int width)
{
	uint16_t* Src16 = (uint16_t*) src;
	uint16_t* Dst16 = (uint16_t*) dst;
	// There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
	// Each block of 4x1 becomes 5x1.
	uint32_t BlockX, BlockY;
	uint16_t* BlockSrc;
	uint16_t* BlockDst;
	for (BlockY = 0; BlockY < 239; BlockY++)
	{
		BlockSrc = Src16 + BlockY * 256 * 1;
		BlockDst = Dst16 + BlockY * 320 * 1;
		for (BlockX = 0; BlockX < 64; BlockX++)
		{
			/* Horizontally:
			 * Before(4):
			 * (a)(b)(c)(d)
			 * After(5):
			 * (a)(abbb)(bc)(cccd)(d)
			 */

			// -- Row 1 --
			uint16_t  _1 = *(BlockSrc               );
			*(BlockDst               ) = _1;
			uint16_t  _2 = *(BlockSrc            + 1);
			*(BlockDst            + 1) = Weight1_3( _1,  _2);
			uint16_t  _3 = *(BlockSrc            + 2);
			*(BlockDst            + 2) = Weight1_1( _2,  _3);
			uint16_t  _4 = *(BlockSrc            + 3);
			*(BlockDst            + 3) = Weight3_1( _3,  _4);
			*(BlockDst            + 4) = _4;

			BlockSrc += 4;
			BlockDst += 5;
		}
	}
}


/*
    Upscale 256x224 -> 320x240

    Horizontal upscale:
        320/256=1.25  --  do some horizontal interpolation
        8p -> 10p
        4dw -> 5dw

        coarse interpolation:
        [ab][cd][ef][gh] -> [ab][(bc)c][de][f(fg)][gh]

        fine interpolation
        [ab][cd][ef][gh] -> [a(0.25a+0.75b)][(0.5b+0.5c)(0.75c+0.25d)][de][(0.25e+0.75f)(0.5f+0.5g)][(0.75g+0.25h)h]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 320x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
*/

void upscale_256x224_to_320x240(uint32_t *dst, uint32_t *src, int width)
{
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

    for (y = 0; y < 240; y++)
    {
        source = dh * width / 2;

        for (x = 0; x < 320/10; x++)
        {
            register uint32_t ab, cd, ef, gh;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;
            ef = src[source + 2] & 0xF7DEF7DE;
            gh = src[source + 3] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + width/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + width/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
                ef = AVERAGE(ef, src[source + width/2 + 2]) & 0xF7DEF7DE; // to prevent overflow
                gh = AVERAGE(gh, src[source + width/2 + 3]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = ab;
            *dst++  = ((ab >> 17) + ((cd & 0xFFFF) >> 1)) + (cd << 16);
            *dst++  = (cd >> 16) + (ef << 16);
            *dst++  = (ef >> 16) + (((ef & 0xFFFF0000) >> 1) + ((gh & 0xFFFF) << 15));
            *dst++  = gh;

            source += 4;

        }
        Eh += 224; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}

void upscale_256x240_to_320x240(uint32_t *dst, uint32_t *src, int width)
{
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

    for (y = 0; y < 239; y++)
    {
        source = dh * width / 2;

        for (x = 0; x < 320/10; x++)
        {
            register uint32_t ab, cd, ef, gh;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;
            ef = src[source + 2] & 0xF7DEF7DE;
            gh = src[source + 3] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + width/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + width/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
                ef = AVERAGE(ef, src[source + width/2 + 2]) & 0xF7DEF7DE; // to prevent overflow
                gh = AVERAGE(gh, src[source + width/2 + 3]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = ab;
            *dst++  = ((ab >> 17) + ((cd & 0xFFFF) >> 1)) + (cd << 16);
            *dst++  = (cd >> 16) + (ef << 16);
            *dst++  = (ef >> 16) + (((ef & 0xFFFF0000) >> 1) + ((gh & 0xFFFF) << 15));
            *dst++  = gh;

            source += 4;

        }
        Eh += 239; if(Eh >= 239) { Eh -= 239; dh++; }
    }
}

/*
    Upscale 256x224 -> 384x240 (for 400x240)

    Horizontal interpolation
        384/256=1.5
        4p -> 6p
        2dw -> 3dw

        for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
        [ab][cd] => [a(ab)][bc][(cd)d]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 400x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
        pitch correction is made
*/

void upscale_256x224_to_384x240_for_400x240(uint32_t *dst, uint32_t *src, int width)
{
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

    dst += (400 - 384) / 4;

    for (y = 0; y < 240; y++)
    {
        source = dh * width / 2;

        for (x = 0; x < 384/6; x++)
        {
            register uint32_t ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + width/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + width/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
            *dst++ = (ab >> 16) + ((cd & 0xFFFF) << 16);
            *dst++ = (cd & 0xFFFF0000) + AVERAGELO(cd);

            source += 2;

        }
        dst += (400 - 384) / 2; 
        Eh += 224; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}

/*
    Upscale 256x224 -> 384x272 (for 480x240)

    Horizontal interpolation
        384/256=1.5
        4p -> 6p
        2dw -> 3dw

        for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
        [ab][cd] => [a(ab)][bc][(cd)d]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 480x272x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
        pitch correction is made
*/

void upscale_256x224_to_384x272_for_480x272(uint32_t *dst, uint32_t *src, int width)
{
    int midh = 272 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

    dst += (480 - 384) / 4;

    for (y = 0; y < 272; y++)
    {
        source = dh * width / 2;

        for (x = 0; x < 384/6; x++)
        {
            register uint32_t ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + width/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + width/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
            *dst++ = (ab >> 16) + ((cd & 0xFFFF) << 16);
            *dst++ = (cd & 0xFFFF0000) + AVERAGELO(cd);

            source += 2;

        }
        dst += (480 - 384) / 2; 
        Eh += 224; if(Eh >= 272) { Eh -= 272; dh++; }
    }
}


void upscale_256x224_to_640x480_bilinearish(uint32_t *dst, uint32_t *src, int width)
{

    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 14 of 16 vertically.
    // Each block of 4x16 becomes 5x17.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 14; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 256 * 16;
        BlockDst = Dst16 + BlockY * 640 * 34;
        for (BlockX = 0; BlockX < 64; BlockX++)
        {
            /* Horizontally:
             * Before(4):
             * (a)(b)(c)(d)
             * After(10):
             * (a)(abbb)(bc)(cccd)(d)
             * (a)(a)(abbb)(abbb)(bc)(bc)(cccd)(cccd)(d)(d)
             *
             * Vertically:
             * Before(16): After(34):
             * (a)       (a)
             *           (a)
             * (b)       (b)
             *           (b)
             * (c)       (c)
             *           (c)
             * (d)       (cddd)
             *           (cddd)
             * (e)       (deee)
             *           (deee)
             * (f)       (efff)
             *           (efff)
             * (g)       (fggg)
             *           (fggg)
             * (h)       (ghhh)
             *           (ghhh)
             * (i)       (hi)
             *           (hi)
             * (j)       (iiij)
             *           (iiij)
             * (k)       (jjjk)
             *           (jjjk)
             * (l)       (kkkl)
             *           (kkkl)
             * (m)       (lllm)
             *           (lllm)
             * (n)       (mmmn)
             *           (mmmn)
             * (o)       (n)
             *           (n)
             * (p)       (o)
             *           (o)
             *           (p)
             *           (p)
             *
             */
            uint16_t  _1 = *(BlockSrc               );
            uint16_t  _2 = *(BlockSrc            + 1);
            uint16_t  _3 = *(BlockSrc            + 2);
            uint16_t  _4 = *(BlockSrc            + 3);

            uint16_t  _5 = *(BlockSrc + 256 *  1    );
            uint16_t  _6 = *(BlockSrc + 256 *  1 + 1);
            uint16_t  _7 = *(BlockSrc + 256 *  1 + 2);
            uint16_t  _8 = *(BlockSrc + 256 *  1 + 3);

            uint16_t  _9 = *(BlockSrc + 256 *   2    );
            uint16_t  _10 = *(BlockSrc + 256 *  2 + 1);
            uint16_t  _11 = *(BlockSrc + 256 *  2 + 2);
            uint16_t  _12 = *(BlockSrc + 256 *  2 + 3);

            uint16_t _13 = *(BlockSrc + 256 *  3    );
            uint16_t _14 = *(BlockSrc + 256 *  3 + 1);
            uint16_t _15 = *(BlockSrc + 256 *  3 + 2);
            uint16_t _16 = *(BlockSrc + 256 *  3 + 3);

            uint16_t _17 = *(BlockSrc + 256 *  4    );
            uint16_t _18 = *(BlockSrc + 256 *  4 + 1);
            uint16_t _19 = *(BlockSrc + 256 *  4 + 2);
            uint16_t _20 = *(BlockSrc + 256 *  4 + 3);

            uint16_t _21 = *(BlockSrc + 256 *  5    );
            uint16_t _22 = *(BlockSrc + 256 *  5 + 1);
            uint16_t _23 = *(BlockSrc + 256 *  5 + 2);
            uint16_t _24 = *(BlockSrc + 256 *  5 + 3);

            uint16_t _25 = *(BlockSrc + 256 *  6    );
            uint16_t _26 = *(BlockSrc + 256 *  6 + 1);
            uint16_t _27 = *(BlockSrc + 256 *  6 + 2);
            uint16_t _28 = *(BlockSrc + 256 *  6 + 3);

            uint16_t _29 = *(BlockSrc + 256 *  7    );
            uint16_t _30 = *(BlockSrc + 256 *  7 + 1);
            uint16_t _31 = *(BlockSrc + 256 *  7 + 2);
            uint16_t _32 = *(BlockSrc + 256 *  7 + 3);

            uint16_t _33 = *(BlockSrc + 256 *  8    );
            uint16_t _34 = *(BlockSrc + 256 *  8 + 1);
            uint16_t _35 = *(BlockSrc + 256 *  8 + 2);
            uint16_t _36 = *(BlockSrc + 256 *  8 + 3);

            uint16_t _37 = *(BlockSrc + 256 *  9    );
            uint16_t _38 = *(BlockSrc + 256 *  9 + 1);
            uint16_t _39 = *(BlockSrc + 256 *  9 + 2);
            uint16_t _40 = *(BlockSrc + 256 *  9 + 3);

            uint16_t _41 = *(BlockSrc + 256 * 10    );
            uint16_t _42 = *(BlockSrc + 256 * 10 + 1);
            uint16_t _43 = *(BlockSrc + 256 * 10 + 2);
            uint16_t _44 = *(BlockSrc + 256 * 10 + 3);

            uint16_t _45 = *(BlockSrc + 256 * 11    );
            uint16_t _46 = *(BlockSrc + 256 * 11 + 1);
            uint16_t _47 = *(BlockSrc + 256 * 11 + 2);
            uint16_t _48 = *(BlockSrc + 256 * 11 + 3);

            uint16_t _49 = *(BlockSrc + 256 * 12    );
            uint16_t _50 = *(BlockSrc + 256 * 12 + 1);
            uint16_t _51 = *(BlockSrc + 256 * 12 + 2);
            uint16_t _52 = *(BlockSrc + 256 * 12 + 3);

            uint16_t _53 = *(BlockSrc + 256 * 13    );
            uint16_t _54 = *(BlockSrc + 256 * 13 + 1);
            uint16_t _55 = *(BlockSrc + 256 * 13 + 2);
            uint16_t _56 = *(BlockSrc + 256 * 13 + 3);

            uint16_t _57 = *(BlockSrc + 256 * 14    );
            uint16_t _58 = *(BlockSrc + 256 * 14 + 1);
            uint16_t _59 = *(BlockSrc + 256 * 14 + 2);
            uint16_t _60 = *(BlockSrc + 256 * 14 + 3);

            uint16_t _61 = *(BlockSrc + 256 * 15    );
            uint16_t _62 = *(BlockSrc + 256 * 15 + 1);
            uint16_t _63 = *(BlockSrc + 256 * 15 + 2);
            uint16_t _64 = *(BlockSrc + 256 * 15 + 3);

            int rendrow = 1;
            int offset = 2;

            // -- Row 1 --
/*			*(BlockDst               ) = _1;
			*(BlockDst            + 1) = Weight1_3( _1,  _2);
			*(BlockDst            + 2) = Weight1_1( _2,  _3);
			*(BlockDst            + 3) = Weight3_1( _3,  _4);
			*(BlockDst            + 4) = _4;*/

            *(BlockDst            + 0  ) = _1;
            *(BlockDst            + 1  ) = _1;
            *(BlockDst            + 2  ) = Weight1_3( _1,  _2);
            *(BlockDst            + 3  ) = Weight1_3( _1,  _2);
            *(BlockDst            + 4  ) = Weight1_1( _2,  _3);
            *(BlockDst            + 5  ) = Weight1_1( _2,  _3);
            *(BlockDst            + 6) = Weight3_1( _3,  _4);
            *(BlockDst            + 7) = Weight3_1( _3,  _4);
            *(BlockDst            + 8) = _4;
            *(BlockDst            + 9) = _4;

            // -- Row 1_2
            *(BlockDst     + 640 *  1        + 0  ) = _1;
            *(BlockDst     + 640 *  1        + 1  ) = _1;
            *(BlockDst     + 640 *  1        + 2  ) = Weight1_3( _1,  _2);
            *(BlockDst     + 640 *  1        + 3  ) = Weight1_3( _1,  _2);
            *(BlockDst     + 640 *  1        + 4  ) = Weight1_1( _2,  _3);
            *(BlockDst     + 640 *  1        + 5  ) = Weight1_1( _2,  _3);
            *(BlockDst     + 640 *  1        + 6) = Weight3_1( _3,  _4);
            *(BlockDst     + 640 *  1        + 7) = Weight3_1( _3,  _4);
            *(BlockDst     + 640 *  1        + 8) = _4;
            *(BlockDst     + 640 *  1        + 9) = _4;



            // -- Row 2 --
/*            *(BlockDst + 640 *  1    ) = _5;
            *(BlockDst + 640 *  1 + 1) = Weight1_3( _5,  _6);
            *(BlockDst + 640 *  1 + 2) = Weight1_1( _6,  _7);
            *(BlockDst + 640 *  1 + 3) = Weight3_1( _7,  _8);
            *(BlockDst + 640 *  1 + 4) = _8;*/
            rendrow = 2;
            *(BlockDst    + 640 *  (rendrow * 2 - 2)        + 0  ) = _5;
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 1  ) = _5;
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 2  ) = Weight1_3( _5,  _6);
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 3  ) = Weight1_3( _5,  _6);
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 4  ) = Weight1_1( _6,  _7);
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 5  ) = Weight1_1( _6,  _7);
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 6) = Weight3_1( _7,  _8);
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 7) = Weight3_1( _7,  _8);
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 8) = _8;
            *(BlockDst    + 640 *  (rendrow * 2 - 2)         + 9) = _8;

            // -- Row 2_2
            *(BlockDst    + 640 *  (rendrow * 2 - 1)         + 0  ) = _5;
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 1  ) = _5;
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 2  ) = Weight1_3( _5,  _6);
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 3  ) = Weight1_3( _5,  _6);
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 4  ) = Weight1_1( _6,  _7);
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 5  ) = Weight1_1( _6,  _7);
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 6) = Weight3_1( _7,  _8);
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 7) = Weight3_1( _7,  _8);
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 8) = _8;
            *(BlockDst    + 640 *  (rendrow * 2 - 1)        + 9) = _8;

            // -- Row 3 --
/*            *(BlockDst + 640 *  2    ) = _9;
            *(BlockDst + 640 *  2 + 1) = Weight1_3( _9, _10);
            *(BlockDst + 640 *  2 + 2) = Weight1_1(_10, _11);
            *(BlockDst + 640 *  2 + 3) = Weight3_1(_11, _12);
            *(BlockDst + 640 *  2 + 4) = _12;*/

            rendrow = 3;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = _9;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) = _9;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3( _9, _10);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3( _9, _10);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_1(_10, _11);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_1(_10, _11);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(_11, _12);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(_11, _12);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = _12;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = _12;
            }

            // -- Row 4 --
/*            *(BlockDst + 640 *  3    ) = Weight1_3( _9, _13);
            *(BlockDst + 640 *  3 + 1) = Weight1_3(Weight1_3( _9, _10), Weight1_3(_13, _14));
            *(BlockDst + 640 *  3 + 2) = Weight1_3(Weight1_1(_10, _11), Weight1_1(_14, _15));
            *(BlockDst + 640 *  3 + 3) = Weight1_3(Weight3_1(_11, _12), Weight3_1(_15, _16));
            *(BlockDst + 640 *  3 + 4) = Weight1_3(_12, _16);*/

            rendrow = 4;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3( _9, _13);;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) = Weight1_3( _9, _13);;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(Weight1_3( _9, _10), Weight1_3(_13, _14));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(Weight1_3( _9, _10), Weight1_3(_13, _14));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(Weight1_1(_10, _11), Weight1_1(_14, _15));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(Weight1_1(_10, _11), Weight1_1(_14, _15));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight1_3(Weight3_1(_11, _12), Weight3_1(_15, _16));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight1_3(Weight3_1(_11, _12), Weight3_1(_15, _16));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight1_3(_12, _16);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight1_3(_12, _16);
            }


            // -- Row 5 --
/*            *(BlockDst + 640 *  4    ) = Weight1_3(_13, _17);
            *(BlockDst + 640 *  4 + 1) = Weight1_3(Weight1_3(_13, _14), Weight1_3(_17, _18));
            *(BlockDst + 640 *  4 + 2) = Weight1_3(Weight1_1(_14, _15), Weight1_1(_18, _19));
            *(BlockDst + 640 *  4 + 3) = Weight1_3(Weight3_1(_15, _16), Weight3_1(_19, _20));
            *(BlockDst + 640 *  4 + 4) = Weight1_3(_16, _20);*/

            rendrow = 5;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(_13, _17);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) = Weight1_3(_13, _17);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) =  Weight1_3(Weight1_3(_13, _14), Weight1_3(_17, _18));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(Weight1_3(_13, _14), Weight1_3(_17, _18));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(Weight1_1(_14, _15), Weight1_1(_18, _19));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(Weight1_1(_14, _15), Weight1_1(_18, _19));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight1_3(Weight3_1(_15, _16), Weight3_1(_19, _20));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight1_3(Weight3_1(_15, _16), Weight3_1(_19, _20));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight1_3(_16, _20);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight1_3(_16, _20);
            }

            // -- Row 6 --
/*            *(BlockDst + 640 *  5    ) = Weight1_3(_17, _21);
            *(BlockDst + 640 *  5 + 1) = Weight1_3(Weight1_3(_17, _18), Weight1_3(_21, _22));
            *(BlockDst + 640 *  5 + 2) = Weight1_3(Weight1_1(_18, _19), Weight1_1(_22, _23));
            *(BlockDst + 640 *  5 + 3) = Weight1_3(Weight3_1(_19, _20), Weight3_1(_23, _24));
            *(BlockDst + 640 *  5 + 4) = Weight1_3(_20, _24);*/

            rendrow = 6;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(_17, _21);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) =  Weight1_3(_17, _21);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(Weight1_3(_17, _18), Weight1_3(_21, _22));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(Weight1_3(_17, _18), Weight1_3(_21, _22));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(Weight1_1(_18, _19), Weight1_1(_22, _23));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(Weight1_1(_18, _19), Weight1_1(_22, _23));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight1_3(Weight3_1(_19, _20), Weight3_1(_23, _24));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight1_3(Weight3_1(_19, _20), Weight3_1(_23, _24));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight1_3(_20, _24);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight1_3(_20, _24);
            }

            // -- Row 7 --
/*            *(BlockDst + 640 *  6    ) = Weight1_3(_21, _25);
            *(BlockDst + 640 *  6 + 1) = Weight1_3(Weight1_3(_21, _22), Weight1_3(_25, _26));
            *(BlockDst + 640 *  6 + 2) = Weight1_3(Weight1_1(_22, _23), Weight1_1(_26, _27));
            *(BlockDst + 640 *  6 + 3) = Weight1_3(Weight3_1(_23, _24), Weight3_1(_27, _28));
            *(BlockDst + 640 *  6 + 4) = Weight1_3(_24, _28);*/

            rendrow = 7;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(_21, _25);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) =   Weight1_3(_21, _25);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(Weight1_3(_21, _22), Weight1_3(_25, _26));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(Weight1_3(_21, _22), Weight1_3(_25, _26));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(Weight1_1(_22, _23), Weight1_1(_26, _27));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(Weight1_1(_22, _23), Weight1_1(_26, _27));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight1_3(Weight3_1(_23, _24), Weight3_1(_27, _28));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight1_3(Weight3_1(_23, _24), Weight3_1(_27, _28));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight1_3(_24, _28);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight1_3(_24, _28);
            }
            // -- Row 8 --
/*            *(BlockDst + 640 *  7    ) = Weight1_3(_25, _29);
            *(BlockDst + 640 *  7 + 1) = Weight1_3(Weight1_3(_25, _26), Weight1_3(_29, _30));
            *(BlockDst + 640 *  7 + 2) = Weight1_3(Weight1_1(_26, _27), Weight1_1(_30, _31));
            *(BlockDst + 640 *  7 + 3) = Weight1_3(Weight3_1(_27, _28), Weight3_1(_31, _32));
            *(BlockDst + 640 *  7 + 4) = Weight1_3(_28, _32);*/

            rendrow = 8;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(_25, _29);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) =  Weight1_3(_25, _29);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(Weight1_3(_25, _26), Weight1_3(_29, _30));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(Weight1_3(_25, _26), Weight1_3(_29, _30));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(Weight1_1(_26, _27), Weight1_1(_30, _31));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(Weight1_1(_26, _27), Weight1_1(_30, _31));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight1_3(Weight3_1(_27, _28), Weight3_1(_31, _32));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight1_3(Weight3_1(_27, _28), Weight3_1(_31, _32));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight1_3(_28, _32);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight1_3(_28, _32);
            }

            // -- Row 9 --
/*            *(BlockDst + 640 *  8    ) = Weight1_1(_29, _33);
            *(BlockDst + 640 *  8 + 1) = Weight1_1(Weight1_3(_29, _30), Weight1_3(_33, _34));
            *(BlockDst + 640 *  8 + 2) = Weight1_1(Weight1_1(_30, _31), Weight1_1(_34, _35));
            *(BlockDst + 640 *  8 + 3) = Weight1_1(Weight3_1(_31, _32), Weight3_1(_35, _36));
            *(BlockDst + 640 *  8 + 4) = Weight1_1(_32, _36);*/

            rendrow = 9;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight1_1(_29, _33);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) =  Weight1_1(_29, _33);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_1(Weight1_3(_29, _30), Weight1_3(_33, _34));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) =  Weight1_1(Weight1_3(_29, _30), Weight1_3(_33, _34));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_1(Weight1_1(_30, _31), Weight1_1(_34, _35));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_1(Weight1_1(_30, _31), Weight1_1(_34, _35));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight1_1(Weight3_1(_31, _32), Weight3_1(_35, _36));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight1_1(Weight3_1(_31, _32), Weight3_1(_35, _36));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight1_1(_32, _36);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight1_1(_32, _36);
            }
            // -- Row 10 --
/*            *(BlockDst + 640 *  9    ) = Weight3_1(_33, _37);
            *(BlockDst + 640 *  9 + 1) = Weight3_1(Weight1_3(_33, _34), Weight1_3(_37, _38));
            *(BlockDst + 640 *  9 + 2) = Weight3_1(Weight1_1(_34, _35), Weight1_1(_38, _39));
            *(BlockDst + 640 *  9 + 3) = Weight3_1(Weight3_1(_35, _36), Weight3_1(_39, _40));
            *(BlockDst + 640 *  9 + 4) = Weight3_1(_36, _40);*/

            rendrow = 10;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight3_1(_33, _37);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) =  Weight3_1(_33, _37);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(Weight1_3(_33, _34), Weight1_3(_37, _38));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(Weight1_3(_33, _34), Weight1_3(_37, _38));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(Weight1_1(_34, _35), Weight1_1(_38, _39));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(Weight1_1(_34, _35), Weight1_1(_38, _39));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(Weight3_1(_35, _36), Weight3_1(_39, _40));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(Weight3_1(_35, _36), Weight3_1(_39, _40));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight3_1(_36, _40);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight3_1(_36, _40);
            }

            // -- Row 11 --
/*            *(BlockDst + 640 * 10    ) = Weight3_1(_37, _41);
            *(BlockDst + 640 * 10 + 1) = Weight3_1(Weight1_3(_37, _38), Weight1_3(_41, _42));
            *(BlockDst + 640 * 10 + 2) = Weight3_1(Weight1_1(_38, _39), Weight1_1(_42, _43));
            *(BlockDst + 640 * 10 + 3) = Weight3_1(Weight3_1(_39, _40), Weight3_1(_43, _44));
            *(BlockDst + 640 * 10 + 4) = Weight3_1(_40, _44);*/

            rendrow = 11;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight3_1(_37, _41);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) =  Weight3_1(_37, _41);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(Weight1_3(_37, _38), Weight1_3(_41, _42));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(Weight1_3(_37, _38), Weight1_3(_41, _42));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(Weight1_1(_38, _39), Weight1_1(_42, _43));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(Weight1_1(_38, _39), Weight1_1(_42, _43));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(Weight3_1(_39, _40), Weight3_1(_43, _44));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(Weight3_1(_39, _40), Weight3_1(_43, _44));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight3_1(_40, _44);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight3_1(_40, _44);
            }

            // -- Row 12 --
/*            *(BlockDst + 640 * 11    ) = Weight3_1(_41, _45);
            *(BlockDst + 640 * 11 + 1) = Weight3_1(Weight1_3(_41, _42), Weight1_3(_45, _46));
            *(BlockDst + 640 * 11 + 2) = Weight3_1(Weight1_1(_42, _43), Weight1_1(_46, _47));
            *(BlockDst + 640 * 11 + 3) = Weight3_1(Weight3_1(_43, _44), Weight3_1(_47, _48));
            *(BlockDst + 640 * 11 + 4) = Weight3_1(_44, _48);*/

            rendrow = 12;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight3_1(_41, _45);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) = Weight3_1(_41, _45);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(Weight1_3(_41, _42), Weight1_3(_45, _46));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(Weight1_3(_41, _42), Weight1_3(_45, _46));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(Weight1_1(_42, _43), Weight1_1(_46, _47));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(Weight1_1(_42, _43), Weight1_1(_46, _47));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(Weight3_1(_43, _44), Weight3_1(_47, _48));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(Weight3_1(_43, _44), Weight3_1(_47, _48));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight3_1(_44, _48);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight3_1(_44, _48);
            }
            // -- Row 13 --
/*            *(BlockDst + 640 * 12    ) = Weight3_1(_45, _49);
            *(BlockDst + 640 * 12 + 1) = Weight3_1(_45, _49);
            *(BlockDst + 640 * 12 + 2) = Weight3_1(Weight1_1(_46, _47), Weight1_1(_50, _51));
            *(BlockDst + 640 * 12 + 3) = Weight3_1(Weight3_1(_47, _48), Weight3_1(_51, _52));
            *(BlockDst + 640 * 12 + 4) = Weight3_1(_48, _52);*/

            rendrow = 13;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = Weight3_1(_45, _49);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) = Weight3_1(_45, _49);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(_45, _49);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(_45, _49);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(Weight1_1(_46, _47), Weight1_1(_50, _51));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(Weight1_1(_46, _47), Weight1_1(_50, _51));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(Weight3_1(_47, _48), Weight3_1(_51, _52));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(Weight3_1(_47, _48), Weight3_1(_51, _52));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight3_1(_48, _52);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight3_1(_48, _52);
            }
            // -- Row 14 --
/*            *(BlockDst + 640 * 13    ) = Weight3_1(_49, _53);
            *(BlockDst + 640 * 13 + 1) = Weight3_1(Weight1_3(_49, _50), Weight1_3(_53, _54));
            *(BlockDst + 640 * 13 + 2) = Weight3_1(Weight1_1(_50, _51), Weight1_1(_54, _55));
            *(BlockDst + 640 * 13 + 3) = Weight3_1(Weight3_1(_51, _52), Weight3_1(_55, _56));
            *(BlockDst + 640 * 13 + 4) = Weight3_1(_52, _56);*/

            rendrow = 14;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 0  ) = Weight3_1(_49, _53);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) = Weight3_1(_49, _53);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(Weight1_3(_49, _50), Weight1_3(_53, _54));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(Weight1_3(_49, _50), Weight1_3(_53, _54));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(Weight1_1(_50, _51), Weight1_1(_54, _55));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(Weight1_1(_50, _51), Weight1_1(_54, _55));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(Weight3_1(_51, _52), Weight3_1(_55, _56));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(Weight3_1(_51, _52), Weight3_1(_55, _56));
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = Weight3_1(_52, _56);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = Weight3_1(_52, _56);
            }
            // -- Row 15 --
/*            *(BlockDst + 640 * 14    ) = _53;
            *(BlockDst + 640 * 14 + 1) = Weight1_3(_53, _54);
            *(BlockDst + 640 * 14 + 2) = Weight1_1(_54, _55);
            *(BlockDst + 640 * 14 + 3) = Weight3_1(_55, _56);
            *(BlockDst + 640 * 14 + 4) = _56;*/

            rendrow = 15;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 0) = _53;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1) = _53;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(_53, _54);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(_53, _54);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_1(_54, _55);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_1(_54, _55);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(_55, _56);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(_55, _56);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = _56;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = _56;
            }

            // -- Row 16 --
/*            *(BlockDst + 640 * 15    ) = _57;
            *(BlockDst + 640 * 15 + 1) = Weight1_3(_57, _58);
            *(BlockDst + 640 * 15 + 2) = Weight1_1(_58, _59);
            *(BlockDst + 640 * 15 + 3) = Weight3_1(_59, _60);
            *(BlockDst + 640 * 15 + 4) = _60;*/

            rendrow = 16;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = _57;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) = _57;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(_57, _58);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(_57, _58);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_1(_58, _59);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_1(_58, _59);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(_59, _60);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(_59, _60);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = _60;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = _60;
            }

            // -- Row 17 --
/*            *(BlockDst + 640 * 16    ) = _61;
            *(BlockDst + 640 * 16 + 1) = Weight1_3(_61, _62);
            *(BlockDst + 640 * 16 + 2) = Weight1_1(_62, _63);
            *(BlockDst + 640 * 16 + 3) = Weight3_1(_63, _64);
            *(BlockDst + 640 * 16 + 4) = _64;*/

            rendrow = 17;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 640 *  (rendrow * 2 - offset)        + 0  ) = _61;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 1  ) = _61;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(_61, _62);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(_61, _62);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 4  ) = Weight1_1(_62, _63);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 5  ) = Weight1_1(_62, _63);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 6) = Weight3_1(_63, _64);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 7) = Weight3_1(_63, _64);
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 8) = _64;
                *(BlockDst    + 640 *  (rendrow * 2 - offset)         + 9) = _64;
            }

            BlockSrc += 4;
            BlockDst += 10;
        }
    }

}

void upscale_256x240_to_640x480_bilinearish(uint32_t *dst, uint32_t *src, int width)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 320 * 1 * 2;
        for (BlockX = 0; BlockX < 64; BlockX++)
        {
            /* Horizontally:
             * Before(4):
             * (a)(b)(c)(d)
             * After(10):
             * (a)(abbb)(bc)(cccd)(d)
             * (a)(a)(abbb)(abbb)(bc)(bc)(cccd)(cccd)(d)(d)
             */
            //one
            uint16_t  _1 = *(BlockSrc               );
            //two
            uint16_t  _2 = *(BlockSrc            + 1);

            uint16_t  _3 = *(BlockSrc            + 2);
            uint16_t  _4 = *(BlockSrc            + 3);

            //three
            uint16_t _12 = Weight1_3( _1,  _2);
            //four
            uint16_t _23 = Weight1_1( _2,  _3);
            //five
            uint16_t _34 = Weight3_1( _3,  _4);

            uint16_t enhence_1 = _1;
            uint16_t enhence_2 = _2;
            uint16_t enhence_3 = _12;
            uint16_t enhence_4 = _23;
            uint16_t enhence_5 = _34;

            // -- Row 1 --
            *(BlockDst               ) = _1;
            *(BlockDst            + 1) = enhence_1;
            *(BlockDst            + 2) = _12;
            *(BlockDst            + 3) = enhence_2;
            *(BlockDst            + 4) = _23;
            *(BlockDst            + 5) = enhence_3;
            *(BlockDst            + 6) = _34;
            *(BlockDst            + 7) = enhence_4;
            *(BlockDst            + 8) = _4;
            *(BlockDst            + 9) = enhence_5;


            // -- Row 2 --
            *(BlockDst      +   640         ) = _1;
            *(BlockDst      +   640      + 1) = enhence_1;
            *(BlockDst      +   640      + 2) = _12;
            *(BlockDst      +   640      + 3) = enhence_2;
            *(BlockDst      +   640      + 4) = _23;
            *(BlockDst      +   640      + 5) = enhence_3;
            *(BlockDst      +   640      + 6) = _34;
            *(BlockDst      +   640      + 7) = enhence_4;
            *(BlockDst      +   640      + 8) = _4;
            *(BlockDst      +   640      + 9) = enhence_5;

            BlockSrc += 4;
            BlockDst += 5 * 2;
        }
    }
}


void upscale_256x224_to_512x480(uint32_t *dst, uint32_t *src, int width) {


    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 14 of 16 vertically.
    // Each block of 4x16 becomes 5x17.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 14; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 256 * 16;
        BlockDst = Dst16 + BlockY * 512 * 17 * 2;
        for (BlockX = 0; BlockX < 64; BlockX++)
        {
            /* Horizontally:
             * Before(4):
             * (1)(2)(3)(4)
             * After(8):
             * (1)(1)(2)(2)(3)(3)(4)(4)
             *
             * Vertically:
             * Before(16): After(34):
             * (a)       (a)
             *           (a)
             * (b)       (b)
             *           (b)
             * (c)       (c)
             *           (c)
             * (d)       (cddd)
             *           (cddd)
             * (e)       (deee)
             *           (deee)
             * (f)       (efff)
             *           (efff)
             * (g)       (fggg)
             *           (fggg)
             * (h)       (ghhh)
             *           (ghhh)
             * (i)       (hi)
             *           (hi)
             * (j)       (iiij)
             *           (iiij)
             * (k)       (jjjk)
             *           (jjjk)
             * (l)       (kkkl)
             *           (kkkl)
             * (m)       (lllm)
             *           (lllm)
             * (n)       (mmmn)
             *           (mmmn)
             * (o)       (n)
             *           (n)
             * (p)       (o)
             *           (o)
             *           (p)
             *           (p)
             *
             */
            uint16_t  a_1 = *(BlockSrc               );
            uint16_t  a_2 = *(BlockSrc            + 1);
            uint16_t  a_3 = *(BlockSrc            + 2);
            uint16_t  a_4 = *(BlockSrc            + 3);

            uint16_t  b_1 = *(BlockSrc + 256 *  1    );
            uint16_t  b_2 = *(BlockSrc + 256 *  1 + 1);
            uint16_t  b_3 = *(BlockSrc + 256 *  1 + 2);
            uint16_t  b_4 = *(BlockSrc + 256 *  1 + 3);

            uint16_t  c_1 = *(BlockSrc + 256 *   2    );
            uint16_t  c_2 = *(BlockSrc + 256 *  2 + 1);
            uint16_t  c_3 = *(BlockSrc + 256 *  2 + 2);
            uint16_t  c_4 = *(BlockSrc + 256 *  2 + 3);

            uint16_t d_1 = *(BlockSrc + 256 *  3    );
            uint16_t d_2 = *(BlockSrc + 256 *  3 + 1);
            uint16_t d_3 = *(BlockSrc + 256 *  3 + 2);
            uint16_t d_4 = *(BlockSrc + 256 *  3 + 3);

            uint16_t e_1 = *(BlockSrc + 256 *  4    );
            uint16_t e_2 = *(BlockSrc + 256 *  4 + 1);
            uint16_t e_3 = *(BlockSrc + 256 *  4 + 2);
            uint16_t e_4 = *(BlockSrc + 256 *  4 + 3);

            uint16_t f_1 = *(BlockSrc + 256 *  5    );
            uint16_t f_2 = *(BlockSrc + 256 *  5 + 1);
            uint16_t f_3 = *(BlockSrc + 256 *  5 + 2);
            uint16_t f_4 = *(BlockSrc + 256 *  5 + 3);

            uint16_t g_1 = *(BlockSrc + 256 *  6    );
            uint16_t g_2 = *(BlockSrc + 256 *  6 + 1);
            uint16_t g_3 = *(BlockSrc + 256 *  6 + 2);
            uint16_t g_4 = *(BlockSrc + 256 *  6 + 3);

            uint16_t h_1 = *(BlockSrc + 256 *  7    );
            uint16_t h_2 = *(BlockSrc + 256 *  7 + 1);
            uint16_t h_3 = *(BlockSrc + 256 *  7 + 2);
            uint16_t h_4 = *(BlockSrc + 256 *  7 + 3);

            uint16_t i_1 = *(BlockSrc + 256 *  8    );
            uint16_t i_2 = *(BlockSrc + 256 *  8 + 1);
            uint16_t i_3 = *(BlockSrc + 256 *  8 + 2);
            uint16_t i_4 = *(BlockSrc + 256 *  8 + 3);

            uint16_t j_1 = *(BlockSrc + 256 *  9    );
            uint16_t j_2 = *(BlockSrc + 256 *  9 + 1);
            uint16_t j_3 = *(BlockSrc + 256 *  9 + 2);
            uint16_t j_4 = *(BlockSrc + 256 *  9 + 3);

            uint16_t k_1 = *(BlockSrc + 256 * 10    );
            uint16_t k_2 = *(BlockSrc + 256 * 10 + 1);
            uint16_t k_3 = *(BlockSrc + 256 * 10 + 2);
            uint16_t k_4 = *(BlockSrc + 256 * 10 + 3);

            uint16_t l_1 = *(BlockSrc + 256 * 11    );
            uint16_t l_2 = *(BlockSrc + 256 * 11 + 1);
            uint16_t l_3 = *(BlockSrc + 256 * 11 + 2);
            uint16_t l_4 = *(BlockSrc + 256 * 11 + 3);

            uint16_t m_1 = *(BlockSrc + 256 * 12    );
            uint16_t m_2 = *(BlockSrc + 256 * 12 + 1);
            uint16_t m_3 = *(BlockSrc + 256 * 12 + 2);
            uint16_t m_4 = *(BlockSrc + 256 * 12 + 3);

            uint16_t n_1 = *(BlockSrc + 256 * 13    );
            uint16_t n_2 = *(BlockSrc + 256 * 13 + 1);
            uint16_t n_3 = *(BlockSrc + 256 * 13 + 2);
            uint16_t n_4 = *(BlockSrc + 256 * 13 + 3);

            uint16_t o_1 = *(BlockSrc + 256 * 14    );
            uint16_t o_2 = *(BlockSrc + 256 * 14 + 1);
            uint16_t o_3 = *(BlockSrc + 256 * 14 + 2);
            uint16_t o_4 = *(BlockSrc + 256 * 14 + 3);

            uint16_t p_1 = *(BlockSrc + 256 * 15    );
            uint16_t p_2 = *(BlockSrc + 256 * 15 + 1);
            uint16_t p_3 = *(BlockSrc + 256 * 15 + 2);
            uint16_t p_4 = *(BlockSrc + 256 * 15 + 3);

            int rendrow = 1;
            int offset = 2;


            // -- Row 1 --
/*			*(BlockDst               ) = _1;
			*(BlockDst            + 1) = Weight1_3( _1,  _2);
			*(BlockDst            + 2) = Weight1_1( _2,  _3);
			*(BlockDst            + 3) = Weight3_1( _3,  _4);
			*(BlockDst            + 4) = _4;*/
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 0  ) = a_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = a_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = a_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = a_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = a_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = a_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6  ) = a_4;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7  ) = a_4;
            }

            // -- Row 2 --
/*            *(BlockDst + 512 *  1    ) = _5;
            *(BlockDst + 512 *  1 + 1) = Weight1_3( _5,  _6);
            *(BlockDst + 512 *  1 + 2) = Weight1_1( _6,  _7);
            *(BlockDst + 512 *  1 + 3) = Weight3_1( _7,  _8);
            *(BlockDst + 512 *  1 + 4) = _8;*/

            rendrow = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 0  ) = b_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = b_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = b_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = b_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = b_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = b_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = b_4;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = b_4;
            }

            // -- Row 3 --
/*            *(BlockDst + 512 *  2    ) = _9;
            *(BlockDst + 512 *  2 + 1) = Weight1_3( _9, _10);
            *(BlockDst + 512 *  2 + 2) = Weight1_1(_10, _11);
            *(BlockDst + 512 *  2 + 3) = Weight3_1(_11, _12);
            *(BlockDst + 512 *  2 + 4) = _12;*/

            rendrow = 3;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = c_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = c_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = c_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = c_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = c_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = c_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = c_4;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = c_4;
            }

            // -- Row 4 --
/*            *(BlockDst + 512 *  3    ) = Weight1_3( _9, _13);
            *(BlockDst + 512 *  3 + 1) = Weight1_3(Weight1_3( _9, _10), Weight1_3(_13, _14));
            *(BlockDst + 512 *  3 + 2) = Weight1_3(Weight1_1(_10, _11), Weight1_1(_14, _15));
            *(BlockDst + 512 *  3 + 3) = Weight1_3(Weight3_1(_11, _12), Weight3_1(_15, _16));
            *(BlockDst + 512 *  3 + 4) = Weight1_3(_12, _16);*/

            rendrow = 4;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(c_1, d_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight1_3(c_1, d_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(c_2, d_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(c_2, d_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(c_3, d_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(c_3, d_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight1_3(c_4, d_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight1_3(c_4, d_4);
            }


            // -- Row 5 --
/*            *(BlockDst + 512 *  4    ) = Weight1_3(_13, _17);
            *(BlockDst + 512 *  4 + 1) = Weight1_3(Weight1_3(_13, _14), Weight1_3(_17, _18));
            *(BlockDst + 512 *  4 + 2) = Weight1_3(Weight1_1(_14, _15), Weight1_1(_18, _19));
            *(BlockDst + 512 *  4 + 3) = Weight1_3(Weight3_1(_15, _16), Weight3_1(_19, _20));
            *(BlockDst + 512 *  4 + 4) = Weight1_3(_16, _20);*/

            rendrow = 5;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(d_1, e_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight1_3(d_1, e_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(d_2, e_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(d_2, e_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(d_3, e_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(d_3, e_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight1_3(d_4, e_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight1_3(d_4, e_4);
            }

            // -- Row 6 --
/*            *(BlockDst + 512 *  5    ) = Weight1_3(_17, _21);
            *(BlockDst + 512 *  5 + 1) = Weight1_3(Weight1_3(_17, _18), Weight1_3(_21, _22));
            *(BlockDst + 512 *  5 + 2) = Weight1_3(Weight1_1(_18, _19), Weight1_1(_22, _23));
            *(BlockDst + 512 *  5 + 3) = Weight1_3(Weight3_1(_19, _20), Weight3_1(_23, _24));
            *(BlockDst + 512 *  5 + 4) = Weight1_3(_20, _24);*/

            rendrow = 6;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(e_1, f_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight1_3(e_1, f_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(e_2, f_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(e_2, f_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(e_3, f_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(e_3, f_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight1_3(e_4, f_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight1_3(e_4, f_4);
            }

            // -- Row 7 --
/*            *(BlockDst + 512 *  6    ) = Weight1_3(_21, _25);
            *(BlockDst + 512 *  6 + 1) = Weight1_3(Weight1_3(_21, _22), Weight1_3(_25, _26));
            *(BlockDst + 512 *  6 + 2) = Weight1_3(Weight1_1(_22, _23), Weight1_1(_26, _27));
            *(BlockDst + 512 *  6 + 3) = Weight1_3(Weight3_1(_23, _24), Weight3_1(_27, _28));
            *(BlockDst + 512 *  6 + 4) = Weight1_3(_24, _28);*/

            rendrow = 7;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(f_1, g_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) =  Weight1_3(f_1, g_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(f_2, g_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(f_2, g_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(f_3, g_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(f_3, g_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight1_3(f_4, g_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight1_3(f_4, g_4);
            }
            // -- Row 8 --
/*            *(BlockDst + 512 *  7    ) = Weight1_3(_25, _29);
            *(BlockDst + 512 *  7 + 1) = Weight1_3(Weight1_3(_25, _26), Weight1_3(_29, _30));
            *(BlockDst + 512 *  7 + 2) = Weight1_3(Weight1_1(_26, _27), Weight1_1(_30, _31));
            *(BlockDst + 512 *  7 + 3) = Weight1_3(Weight3_1(_27, _28), Weight3_1(_31, _32));
            *(BlockDst + 512 *  7 + 4) = Weight1_3(_28, _32);*/

            rendrow = 8;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight1_3(g_1, h_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight1_3(g_1, h_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight1_3(g_2, h_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight1_3(g_2, h_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight1_3(g_3, h_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight1_3(g_3, h_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight1_3(g_4, h_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight1_3(g_4, h_4);
            }

            // -- Row 9 --
/*            *(BlockDst + 512 *  8    ) = Weight1_1(_29, _33);
            *(BlockDst + 512 *  8 + 1) = Weight1_1(Weight1_3(_29, _30), Weight1_3(_33, _34));
            *(BlockDst + 512 *  8 + 2) = Weight1_1(Weight1_1(_30, _31), Weight1_1(_34, _35));
            *(BlockDst + 512 *  8 + 3) = Weight1_1(Weight3_1(_31, _32), Weight3_1(_35, _36));
            *(BlockDst + 512 *  8 + 4) = Weight1_1(_32, _36);*/

            rendrow = 9;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight1_1(h_1, i_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight1_1(h_1, i_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight1_1(h_2, i_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight1_1(h_2, i_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight1_1(h_3, i_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight1_1(h_3, i_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight1_1(h_4, i_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight1_1(h_4, i_4);
            }
            // -- Row 10 --
/*            *(BlockDst + 512 *  9    ) = Weight3_1(_33, _37);
            *(BlockDst + 512 *  9 + 1) = Weight3_1(Weight1_3(_33, _34), Weight1_3(_37, _38));
            *(BlockDst + 512 *  9 + 2) = Weight3_1(Weight1_1(_34, _35), Weight1_1(_38, _39));
            *(BlockDst + 512 *  9 + 3) = Weight3_1(Weight3_1(_35, _36), Weight3_1(_39, _40));
            *(BlockDst + 512 *  9 + 4) = Weight3_1(_36, _40);*/

            rendrow = 10;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight3_1(i_1, j_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) =  Weight3_1(i_1, j_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(i_2, j_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(i_2, j_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(i_3, j_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(i_3, j_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight3_1(i_4, j_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight3_1(i_4, j_4);
            }

            // -- Row 11 --
/*            *(BlockDst + 512 * 10    ) = Weight3_1(_37, _41);
            *(BlockDst + 512 * 10 + 1) = Weight3_1(Weight1_3(_37, _38), Weight1_3(_41, _42));
            *(BlockDst + 512 * 10 + 2) = Weight3_1(Weight1_1(_38, _39), Weight1_1(_42, _43));
            *(BlockDst + 512 * 10 + 3) = Weight3_1(Weight3_1(_39, _40), Weight3_1(_43, _44));
            *(BlockDst + 512 * 10 + 4) = Weight3_1(_40, _44);*/

            rendrow = 11;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight3_1(j_1, k_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight3_1(j_1, k_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(j_2, k_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(j_2, k_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(j_3, k_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(j_3, k_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight3_1(j_4, k_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight3_1(j_4, k_4);
            }

            // -- Row 12 --
/*            *(BlockDst + 512 * 11    ) = Weight3_1(_41, _45);
            *(BlockDst + 512 * 11 + 1) = Weight3_1(Weight1_3(_41, _42), Weight1_3(_45, _46));
            *(BlockDst + 512 * 11 + 2) = Weight3_1(Weight1_1(_42, _43), Weight1_1(_46, _47));
            *(BlockDst + 512 * 11 + 3) = Weight3_1(Weight3_1(_43, _44), Weight3_1(_47, _48));
            *(BlockDst + 512 * 11 + 4) = Weight3_1(_44, _48);*/

            rendrow = 12;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight3_1(k_1, l_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight3_1(k_1, l_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(k_2, l_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(k_2, l_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(k_3, l_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(k_3, l_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight3_1(k_4, l_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight3_1(k_4, l_4);
            }
            // -- Row 13 --
/*            *(BlockDst + 512 * 12    ) = Weight3_1(_45, _49);
            *(BlockDst + 512 * 12 + 1) = Weight3_1(_45, _49);
            *(BlockDst + 512 * 12 + 2) = Weight3_1(Weight1_1(_46, _47), Weight1_1(_50, _51));
            *(BlockDst + 512 * 12 + 3) = Weight3_1(Weight3_1(_47, _48), Weight3_1(_51, _52));
            *(BlockDst + 512 * 12 + 4) = Weight3_1(_48, _52);*/

            rendrow = 13;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = Weight3_1(l_1, m_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight3_1(l_1, m_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(l_2, m_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(l_2, m_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(l_3, m_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(l_3, m_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight3_1(l_4, m_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight3_1(l_4, m_4);
            }
            // -- Row 14 --
/*            *(BlockDst + 512 * 13    ) = Weight3_1(_49, _53);
            *(BlockDst + 512 * 13 + 1) = Weight3_1(Weight1_3(_49, _50), Weight1_3(_53, _54));
            *(BlockDst + 512 * 13 + 2) = Weight3_1(Weight1_1(_50, _51), Weight1_1(_54, _55));
            *(BlockDst + 512 * 13 + 3) = Weight3_1(Weight3_1(_51, _52), Weight3_1(_55, _56));
            *(BlockDst + 512 * 13 + 4) = Weight3_1(_52, _56);*/

            rendrow = 14;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 0  ) = Weight3_1(m_1, n_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = Weight3_1(m_1, n_1);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = Weight3_1(m_2, n_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = Weight3_1(m_2, n_2);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = Weight3_1(m_3, n_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = Weight3_1(m_3, n_3);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = Weight3_1(m_4, n_4);
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = Weight3_1(m_4, n_4);
            }
            // -- Row 15 --
/*            *(BlockDst + 512 * 14    ) = _53;
            *(BlockDst + 512 * 14 + 1) = Weight1_3(_53, _54);
            *(BlockDst + 512 * 14 + 2) = Weight1_1(_54, _55);
            *(BlockDst + 512 * 14 + 3) = Weight3_1(_55, _56);
            *(BlockDst + 512 * 14 + 4) = _56;*/

            rendrow = 15;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 0) = n_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1) = n_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = n_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = n_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = n_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) =n_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = n_4;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = n_4;
            }

            // -- Row 16 --
/*            *(BlockDst + 512 * 15    ) = _57;
            *(BlockDst + 512 * 15 + 1) = Weight1_3(_57, _58);
            *(BlockDst + 512 * 15 + 2) = Weight1_1(_58, _59);
            *(BlockDst + 512 * 15 + 3) = Weight3_1(_59, _60);
            *(BlockDst + 512 * 15 + 4) = _60;*/

            rendrow = 16;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = o_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = o_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = o_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = o_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = o_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = o_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = o_4;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = o_4;
            }

            // -- Row 17 --
/*            *(BlockDst + 512 * 16    ) = _61;
            *(BlockDst + 512 * 16 + 1) = Weight1_3(_61, _62);
            *(BlockDst + 512 * 16 + 2) = Weight1_1(_62, _63);
            *(BlockDst + 512 * 16 + 3) = Weight3_1(_63, _64);
            *(BlockDst + 512 * 16 + 4) = _64;*/

            rendrow = 17;
            offset = 2;
            for (offset = 2; offset > 0; offset--) {
                *(BlockDst    + 512 *  (rendrow * 2 - offset)        + 0  ) = p_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 1  ) = p_1;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 2  ) = p_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 3  ) = p_2;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 4  ) = p_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 5  ) = p_3;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 6) = p_4;
                *(BlockDst    + 512 *  (rendrow * 2 - offset)         + 7) = p_4;
            }
            BlockSrc += 4;
            BlockDst += 8;
        }
    }
}

void upscale_256x240_to_512x480(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = a;
            *(BlockDst +  512 + 1)  = a;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_512x480_scanline(uint32_t *dst, uint32_t *src, int width) {

}
void upscale_256x240_to_512x480_scanline(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_512x480_grid(uint32_t *dst, uint32_t *src, int width) {

}

void upscale_256x240_to_512x480_grid(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  color = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( color, gcolor);

            //uint16_t scanline_color = (color + (color & 0x7474)) >> 1;
            //scanline_color = (color + scanline_color + ((color ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (BlockX + 1) >= 256 ? 0 : (BlockX + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));

            // -- Row 1 --
            *(BlockDst) = color;
            *(BlockDst + 1) = scanline_color;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}


void upscale_256x224_to_512x448(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = a;
            *(BlockDst +  512 + 1)  = a;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_512x448_scanline(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}


void upscale_256x224_to_512x448_scanline_v(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight3_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            *(BlockDst + 1) = scanline_color;
            // -- next row 2 --
            *(BlockDst +  512 )  = a;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_512x448_grid(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  color = *(BlockSrc);
            uint16_t  scanline_color = Weight3_1(color, gcolor);

            //uint16_t scanline_color = (color + (color & 0x7474)) >> 1;
            //scanline_color = (color + scanline_color + ((color ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (x + 1) >= GBC_SCREEN_WIDTH ? 0 : (x + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));
            // -- Row 1 --
            *(BlockDst) = color;
            *(BlockDst + 1) = scanline_color;
            // -- next row 2 --
            *(BlockDst +  512 )  = scanline_color;
            *(BlockDst +  512 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_512x448_grid1(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 512 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  color0 = *(BlockSrc);
            uint16_t  scanline_color = Weight3_1(color0, gcolor);
            //scanline_color = Weight2_1(color0, scanline_color);

            //uint16_t  scanline_color = gcolor;
            //uint16_t scanline_color = (color0 + (color0 & 0x7474)) >> 1;
            //scanline_color = (color0 + scanline_color + ((color0 ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (x + 1) >= GBC_SCREEN_WIDTH ? 0 : (x + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));

            // -- Row 1 --
            *(BlockDst) = color0;

            // -- next row 2 --
            //*(BlockDst +  512 )  = color0;

            if (BlockX%2 == 0) {
                *(BlockDst + 1) = color0;
                *(BlockDst +  512 + 1)  = color0;
            } else {
                *(BlockDst + 1) = scanline_color;
                *(BlockDst +  512 + 1)  = scanline_color;
            }
            if (BlockY%2==0) {
                *(BlockDst +  512 )  = color0;
                *(BlockDst +  512 + 1)  = color0;
            } else {
                *(BlockDst +  512 )  = scanline_color;
                *(BlockDst +  512 + 1)  = scanline_color;
            }

            // -- next row 3 --

            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

void upscale_256x224_to_256x448_scanline(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    // There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
    // Each block of 4x1 becomes 5x1.
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 223; BlockY++)
    {
        //littlehui * 2
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 256 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  a = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( a, gcolor);
            // -- Row 1 --
            *(BlockDst) = a;
            //*(BlockDst + 1) = a;
            // -- next row 2 --
            *(BlockDst +  256 )  = scanline_color;
            //*(BlockDst +  256 + 1)  = scanline_color;
            BlockSrc += 1;
            BlockDst += 1;
        }
    }
}

void upscale_256x240_to_256x480_scanline(uint32_t *dst, uint32_t *src, int width) {
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);
    for (BlockY = 0; BlockY < 239; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 256 * 1;
        BlockDst = Dst16 + BlockY * 256 * 1 * 2;
        for (BlockX = 0; BlockX < 256; BlockX++)
        {
            /* Horizontally:
             * Before(1):
             * (a)
             * After(4):
             * (a)(a)
             * (a)(a)
             */
            //one
            uint16_t  color = *(BlockSrc);
            uint16_t  scanline_color = Weight2_1( color, gcolor);

            //uint16_t scanline_color = (color + (color & 0x7474)) >> 1;
            //scanline_color = (color + scanline_color + ((color ^ scanline_color) & 0x0421)) >> 1;

            //uint32_t next_offset = (BlockX + 1) >= 256 ? 0 : (BlockX + 1);
            //uint32_t scanline_color = (uint32_t)bgr555_to_native_16(*(src + next_offset));

            // -- Row 1 --
            *(BlockDst) = color;
            // -- next row 2 --
            *(BlockDst +  256 )  = scanline_color;
            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}