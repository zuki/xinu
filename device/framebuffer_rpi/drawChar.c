/**
 * @file drawChars.c
 *
 * Allows characters to be rendered onscreen.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <framebuffer.h>

/**
 * @ingroup framebuffer
 *
 * 指定の位置、色でしての文字を描画する. font.cにはASCII文字の最初の128
 * 文字の12ビイト表現が含まれている。各バイトは0と1から成る一行である。
 * ここで1はその位置にピクセルを描画することを表す。
 *
 * @param c 文字コード
 * @param x 描画する位置のx座標
 * @param y 描画する位置のy座標
 * @param color 描画する文字の色
 */
void drawChar(char c, int x, int y, ulong color)
{
    int i, j;
    uchar line;

    /* stupid proof error checking: make sure we have a representation for this
     * char.  */
    if (c < 0 || c > 127)
    {
        return;
    }

    for (i = 0; i < CHAR_HEIGHT; i++)
    {
        line = FONT[c * CHAR_HEIGHT + i];
        for (j = 0; j < CHAR_WIDTH; j++)
        {
            if (line & 0x1)
            {
                drawPixel(x + j, y, color);
            }
            line >>= 1;
        }
        y++;
    }
}
