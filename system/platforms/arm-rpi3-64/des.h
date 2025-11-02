/**
 * @file des.h
 *
 * 以下によるDESの実装:
 *     - David Wong, moi@davidwong.fr
 *     - Jacques Monin, jacques.monin@u-bordeaux.fr
 *     - Hugo Bonnin, hugo.bonnin@u-bordeaux.fr
 */

#ifndef DES_H
#define DES_H

/** 8バイトの先頭ビット=1 */
#define FIRSTBIT 0x8000000000000000     // 1000000000...

/* 関数プロトタイプ */

/**
 * Addbitヘルパー関数.
 * "from"からビット番号"position_from"を取り出し、
 * "block"の"position_to"の位置に追加する。
 *
 * @param block ビットを追加するブロックへのポインタ
 * @param from ビットを取り出す値
 * @param position_from ビットを取り出す位置
 * @param position_to ビットを追加する位置
 */
void addbit(uint64_t *block, uint64_t from,
            int position_from, int position_to);

/**
 * 並べ替えの最初と最後.
 *
 * @param data 並べ替えるデータ
 * @param initial 最初か?
 */
void Permutation(uint64_t* data, bool initial);

/**
 * パリティビットが正しいか検証する.
 *
 * @param key パリティを検証する値
 * @return 正しいか否か
 */
bool key_parity_verify(uint64_t key);

/**
 * 鍵スケジュール.
 *   ( http://en.wikipedia.org/wiki/File:DES-key-schedule.png )
 * 入力:
 *  - encrypt : 暗号解読の場合はfalse
 *  - next_key : uint64_t next_key = 0
 *  - round : [[0, 15]]
 * 変化 :
 *  - [key] はラウンドのXORで使用される
 *  - [next_key] は次のラウンドのkey_scheduleで使用される
 *    leftkey+rightkeyの組み合わせである
 *
 * @param key 鍵
 * @param next_key 次のラウンドの鍵
 * @param round ラウンド
 */
void key_schedule(uint64_t* key, uint64_t* next_key, int round);

void rounds(uint64_t *data, uint64_t key);

void genkey(uint64_t *key);
void des_encrypt(char *in, int isize, char *out, uint64_t key);
void des_decrypt(char *in, int isize, char *out, uint64_t key);


#endif
