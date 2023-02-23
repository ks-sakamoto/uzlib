/*
 * tgzip  -  gzip compressor example
 *
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 * Copyright (c) 2014-2018 by Paul Sokolovsky
 *
 * http://www.ibsensoftware.com/
 *
 * This software is provided 'as-is', without any express
 * or implied warranty.  In no event will the authors be
 * held liable for any damages arising from the use of
 * this software.
 *
 * Permission is granted to anyone to use this software
 * for any purpose, including commercial applications,
 * and to alter it and redistribute it freely, subject to
 * the following restrictions:
 *
 * 1. The origin of this software must not be
 *    misrepresented; you must not claim that you
 *    wrote the original software. If you use this
 *    software in a product, an acknowledgment in
 *    the product documentation would be appreciated
 *    but is not required.
 *
 * 2. Altered source versions must be plainly marked
 *    as such, and must not be misrepresented as
 *    being the original software.
 *
 * 3. This notice may not be removed or altered from
 *    any source distribution.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "uzlib.h"

void exit_error(const char *what)
{
   printf("ERROR: %s\n", what);
   exit(1);
}

int main(int argc, char *argv[])
{
    FILE *fin, *fout;
    unsigned int len;
    unsigned char *source;

    printf("tgzip - example from the uzlib library\n\n");

    if (argc < 3)
    {
       printf(
          "Syntax: tgunzip <source> <destination>\n\n"
          "Both input and output are kept in memory, so do not use this on huge files.\n");

       return 1;
    }

    /* -- open files -- */

    if ((fin = fopen(argv[1], "rb")) == NULL) exit_error("source file");

    if ((fout = fopen(argv[2], "wb")) == NULL) exit_error("destination file");

    /* -- read source -- */
    // ファイルの読み書き位置を移動する, https://www.k-cube.co.jp/wakaba/server/func/fseek.html
    // int fseek(FILE *fp, long offset, int origin)(FILEポインタ, 移動バイト数, SEEK_END（ファイルの終端）)
    // ファイルポインタを末尾まで移動
    fseek(fin, 0, SEEK_END);
    // ファイルの読み書き位置を取得する, https://www.k-cube.co.jp/wakaba/server/func/ftell.html
    // ftell(FILE *stream)
    len = ftell(fin);
    // SEEK_SET（ファイルの先頭から）
    // ファイルポインタを先頭まで移動
    fseek(fin, 0, SEEK_SET);
    // malloc(), 動的メモリを確保する関数, https://www.sejuku.net/blog/25002
    // 引数lenのバイト数分のメモリを確保, ファイル内の文字数分メモリ確保
    source = (unsigned char *)malloc(len);

    if (source == NULL) exit_error("memory");
    // ファイルの読み込み, http://9cguide.appspot.com/17-02.html
    // fread(読み込む変数のポインタ, 1項目のサイズ, 項目数, ファイルポインタ)
    if (fread(source, 1, len, fin) != len) exit_error("read");

    fclose(fin);

    /* -- compress data -- */

    struct uzlib_comp comp = {0};
    comp.dict_size = 32768;  // 構造体compのdict_sizeの初期化
    comp.hash_bits = 12;     // 構造体compのhash_bitsの初期化
    // size_tはオブジェクトのバイト数を表現できる程度に十分に大きい符号なし整数型
    // sizeofは変数や型のメモリサイズを調べるための演算子, メモリサイズをバイト単位で返す
    size_t hash_size = sizeof(uzlib_hash_entry_t) * (1 << comp.hash_bits);
    comp.hash_table = malloc(hash_size);
    // メモリを初期化する, https://www.sejuku.net/blog/25002
    // memset(メモリを設定するオブジェクトのアドレス, メモリにセットする値, メモリにセットする値の文字数)
    memset(comp.hash_table, 0, hash_size);

    zlib_start_block(&comp.out);
    uzlib_compress(&comp, source, len);
    zlib_finish_block(&comp.out);

    printf("compressed to %u raw bytes\n", comp.out.outlen);

    /* -- write output -- */
    // putc(int c, FILE *stream)（出力文字, ファイルポインタ）
    // 指定したストリームに文字を出力
    // ここがヘッダ
    // https://www.rainorshine.asia/2016/06/05/post2696.html#toc2, https://qiita.com/mpyw/items/eb6ef5e444c2250361b5
   //  putc(0x1f, fout); // 1バイト目31, 00011111, 
   //  putc(0x8b, fout); 
   //  putc(0x08, fout);
   //  putc(0x00, fout); // FLG:ビット 0 ～ 4 FCHECK （CMF と FLG のチェックビット）ビット 5 FDICT （プリセット辞書）ビット 6 ～ 7 FLEVEL （圧縮レベル
   //  int mtime = 0;
   //  fwrite(&mtime, sizeof(mtime), 1, fout);
   //  putc(0x04, fout); // XFL:圧縮レベル
   //  putc(0x03, fout); // OS
    // fwrite(書き込む変数アドレス,1項目のサイズ,項目数,ファイルポインタ)
    // ファイルの書き込み, http://9cguide.appspot.com/17-02.html
    // ここが本体
    fwrite(comp.out.outbuf, 1, comp.out.outlen, fout);
    printf("%s\n", comp.out.outbuf);
    
    // フッタ
   //  unsigned crc = ~uzlib_crc32(source, len, ~0);
   //  fwrite(&crc, sizeof(crc), 1, fout);
   //  fwrite(&len, sizeof(len), 1, fout);

    fclose(fout);

    return 0;
}
