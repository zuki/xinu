/**
 * @file drawShapes.c
 *
 * Allows shapes to be rendered onscreen.
 */
/* Embedded Xinu, Copyright (C) 2013.  All rights reserved. */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <framebuffer.h>
#if defined(_XINU_PLATFORM_ARM_RPI_3_)
#include <bcm2837.h>
#elif defined (_XINU_PLATFORM_ARM_RPI_)
#include <bcm2835.h>
#endif

extern void _inval_area(uint32_t);

/**
 * @ingroup framebuffer
 *
 * 指定された座標 (x, y) にカラーピクセルを描画する.
 * @param x    ピクセルのx座標
 * @param y    ピクセルのY座標
 * @param color    ピクセルのカラー
 */
void drawPixel(int x, int y, ulong color)
{

    //check if somebody tried to draw something out of range.
    if ( (y < DEFAULT_HEIGHT) && (x < DEFAULT_WIDTH) && (y >= 0) && (x >= 0) ) {
        pre_peripheral_write_mb();

        // compute address of pixel to write.
        // framebuffer address + (y*pitch)+(x*(depth/8))
        volatile ulong *address = (volatile ulong *)(framebufferAddress +
                    (y * pitch) + (x * (BIT_DEPTH/8)));
        dmb();
        *address = color;
        dmb();
        _inval_area((uint32_t)address);
        dmb();
    }
}

/**
 * @ingroup framebuffer
 *
 * Bresenhamアルゴリズムに基づいてラインを描画する. 割り算や小数をいじる
 * 必要がない.
 * @param x1    ラインの開始地点のx座標
 * @param y1    ラインの開始地点のy座標
 * @param x2    ラインの終了地点のx座標
 * @param y2    ラインの終了地点のy座標
 * @param color    ラインのカラー
 */
void drawLine(int x1, int y1, int x2, int y2, ulong color)
{
    int deltax, stepx, deltay, stepy, error;
    if (x2 > x1) {
        deltax = x2 - x1;
        stepx = 1;
    } else {
        deltax = x1 - x2;
        stepx = -1;
    }
    if (y2 > y1) {
        deltay = y2 - y1;
        stepy = 1;
    } else {
        deltay = y1 - y2;
        stepy = -1;
    }
    error = deltax - deltay;
    while ( (x1 != x2 + stepx) && (y1 != y2 + stepy) ) {
        drawPixel(x1, y1, color);
        if (error * 2 >= -1 * deltay) {
            x1 = x1 + stepx;
            error = error - deltay;
        }
        if (error * 2 <= deltax) {
            y1 = y1 + stepy;
            error = error + deltax;
        }
    }
}

/* Modified drawLine specifically for turtle graphics */
/**
 * @ingroup framebuffer
 *
 * タートルグラフィックス専用に変更したdrawLine.
 * @param x1    開始地点のx座標
 * @param y1    開始地点のy座標
 * @param x2    終了地点のx座標
 * @param y2    終了地点のy座標
 * @param color    ラインのカラー
 */
void drawSegment(int x1, int y1, int x2, int y2, ulong color)
{
    int deltax, stepx, deltay, stepy, error, i;
    int maxy = DEFAULT_HEIGHT - (CHAR_HEIGHT * MINISHELLMINROW) - 1;
    if (x2 > x1) {
        deltax = x2 - x1;
        stepx = 1;
    } else {
        deltax = x1 - x2;
        stepx = -1;
    }
    if (y2 > y1) {
        deltay = y2 - y1;
        stepy = 1;
    } else {
        deltay = y1 - y2;
        stepy = -1;
    }
    error = deltax - deltay;
    while ( (x1 != x2 + stepx) && (y1 != y2 + stepy) ) {
        //error checking
        if ( (y1 < maxy) && (y1 >= 0) && (x1 < DEFAULT_WIDTH) && (x1 >= 0) ) {
            drawPixel(x1, y1, color);
            i = y1 * DEFAULT_WIDTH + x1;
            linemap[i] = color; //add it to the linemap.
        }
        if (error * 2 >= -1 * deltay) {
            x1 = x1 + stepx;
            error = error - deltay;
        }
        if (error * 2 <= deltax) {
            y1 = y1 + stepy;
            error = error + deltax;
        }
    }
}

/**
 * @ingroup framebuffer
 *
 * 矩形の輪郭を描画する.
 * @param x1    左上角のx座標
 * @param y1    左上角のy座標
 * @param x2    右下角のx座標
 * @param y2    右下角のy座標
 * @param color    矩形のカラー
 */
void drawRect(int x1, int y1, int x2, int y2, ulong color) {
    drawLine(x1, y1, x2, y1, color);
    drawLine(x1, y2, x2, y2, color);
    drawLine(x2, y1, x2, y2, color);
    drawLine(x1, y1, x1, y2, color);
}

/**
 * @ingroup framebuffer
 *
 * 指定された座標と色付けオプションで塗りつぶされた矩形を描画する.
 * @param x1    左上角のx座標
 * @param y1    左上角のy座標
 * @param x2    右下角のx座標
 * @param y2    右下角のy座標
 * @param color    矩形のカラー
 * @param gradient  グラデーションを書ける場合はtrue; そうでなければfalse
 */
void fillRect(int x1, int y1, int x2, int y2, ulong color, bool gradient) {
    int iterations;
    if ( (x2 - x1) > (y2 - y1) )
        iterations = (x2 - x1) / 2;
    else iterations = (y2 - y1) / 2;

    while (iterations != 0) {
        drawRect(x1 + iterations, y1 + iterations, x2 - iterations, y2 - iterations, color);
        iterations--;
        if (gradient) color += 2;
    }
    drawRect(x1, y1, x2, y2, color);
}

/* Midpoint Circle Algorithm calculation of a circle.
 * Based on "x^2 + y^2 = r^2" */
/**
 * @ingroup framebuffer
 *
 * 中点円アルゴリズムによる円の計算.
 * "x^2 + y^2 = r^2" に基づく.
 * @param x0        円の中心のx座標
 * @param y0        円の中心のy座標
 * @param radius    円の半径
 * @param color     円のカラー
 */
void drawCircle (int x0, int y0, int radius, ulong color)
{
    int x = radius, y = 0;
    int radiusError = 1 - x;

    while(x >= y)
    {
        drawPixel(x + x0, y + y0, color);
        drawPixel(y + x0, x + y0, color);
        drawPixel(-x + x0, y + y0, color);
        drawPixel(-y + x0, x + y0, color);
        drawPixel(-x + x0, -y + y0, color);
        drawPixel(-y + x0, -x + y0, color);
        drawPixel(x + x0, -y + y0, color);
        drawPixel(y + x0, -x + y0, color);
        y++;
        if (radiusError < 0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    }
}

/**
 * @ingroup framebuffer
 *
 * 塗りつぶされた円を描画する.
 * @param x0        円の中心のx座標
 * @param y0        円の中心のy座標
 * @param radius    円の半径
 * @param color     円のカラー
 * @param gradient  グラデーションを書ける場合はtrue; そうでなければfalse
 */
void fillCircle (int x0, int y0, int radius, ulong color, bool gradient) {
    int rad = radius;
    while (rad != 0) {
        drawCircle(x0, y0, rad, color);
        rad--;
        if (gradient) color++;
    }
}

/**
 * @ingroup framebuffer
 *
 * タートルグラフィックス専用の円の描画関数.
 * @param x0        円の中心のx座標
 * @param y0        円の中心のy座標
 * @param radius    円の半径
 * @param color     円のカラー
 */
void drawBody (int x0, int y0, int radius, ulong color)
{
    int x = radius, y = 0;
    int radiusError = 1 - x;

    //the maximum y-value we can have so that we don't draw any part of turtle in the minishell area
    int maxy = DEFAULT_HEIGHT - (MINISHELLMINROW * CHAR_HEIGHT) - BODY_RADIUS - (HEAD_RADIUS * 2);
    //don't even try to draw turtle if y is out of bounds
    if ((y0 > maxy) || (y0 < 0) || (x0 > DEFAULT_WIDTH) || (x0 < 0)) {
        return;
    }

    while(x >= y)
    {
        if (color == background) {
            //if we are drawing in background color, we're erasing
            //an existing circle. in that case, replace with
            //saved linemap data.
            drawPixel(x + x0, y + y0, linemap[(y+y0) * DEFAULT_WIDTH + (x+x0)] );
            drawPixel(y + x0, x + y0, linemap[(x+y0) * DEFAULT_WIDTH + (y+x0)] );
            drawPixel(-x + x0, y + y0, linemap[(y+y0) * DEFAULT_WIDTH + (-x+x0)] );
            drawPixel(-y + x0, x + y0, linemap[(x+y0) * DEFAULT_WIDTH + (-y+x0)] );
            drawPixel(-x + x0, -y + y0, linemap[(-y+y0) * DEFAULT_WIDTH + (-x+x0)] );
            drawPixel(-y + x0, -x + y0, linemap[(-x+y0) * DEFAULT_WIDTH + (-y+x0)] );
            drawPixel(x + x0, -y + y0, linemap[(-y+y0) * DEFAULT_WIDTH + (x+x0)] );
            drawPixel(y + x0, -x + y0, linemap[(-x+y0) * DEFAULT_WIDTH + (y+x0)] );
        } else { //draw the circle normally.
            drawPixel(x + x0, y + y0, color);
            drawPixel(y + x0, x + y0, color);
            drawPixel(-x + x0, y + y0, color);
            drawPixel(-y + x0, x + y0, color);
            drawPixel(-x + x0, -y + y0, color);
            drawPixel(-y + x0, -x + y0, color);
            drawPixel(x + x0, -y + y0, color);
            drawPixel(y + x0, -x + y0, color);
        }
        y++;
        if(radiusError < 0)
            radiusError += 2 * y + 1;
        else
        {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    }
}

/**
 * @ingroup framebuffer
 *
 * タートルグラフィックス専用の塗りつぶされた円を描画関数.
 * @param x0        円の中心のx座標
 * @param y0        円の中心のy座標
 * @param radius    円の半径
 * @param color     円のカラー
 * @param gradient  グラデーションを書ける場合はtrue; そうでなければfalse
 */
void fillBody (int x0, int y0, int radius, ulong color, bool gradient) {
    int rad = radius;
    while (rad != 0) {
        drawBody(x0, y0, rad, color);
        rad--;
        if (gradient) color++;
    }
}
