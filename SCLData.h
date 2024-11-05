#ifndef SCLDATA_INCLUDED
#define SCLDATA_INCLUDED

#include <map>
#include <vector>
#include <string>
#include <fstream>
#include "stdio.h"
#include "string.h"

typedef unsigned char command;
typedef signed int address;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef char* string;

enum SCL_INSTRUCTION : u8 {


	/***** [ 定数 ] *****/

	// ＳＣＬ専用命令 //
 SCR_NOP			=0x30,	// 停止命令[SCL/ECL/TCL/ExAnm]   Cmd(1) + t(2)
 SCR_SET			=0x31,	// セット命令 Cmd(1) + (x(2), y(2), Addr(4))



// テクスチャ定義専用命令 //

// テクスチャのロード     : RegID(1) + Cmd(1) + Num(1) + string + \0  //
// この命令は続く ANIME 命令の対象となるテクスチャを指定する //
 SCR_LOAD		=0x40,

// 矩形型の代入命令(絶対) : Cmd(1) + Index(1) + rect(2Byte * 4) //
 SCR_RECT		=0x41,

// アニメーションの定義   : Cmd(1) + Index(1) + (n(1), ID[n]) //
// 上に示したように、LOAD 命令にてテクスチャは示される        //
 SCR_ANIME		=0x42,


// 条件付きテクスチャロード : Cmd(1) + string + \0 //
 SCR_LOADEX		=0x44,

 SCR_STOP		=0x45,	// アニメーション停止モード Cmd(1)



// ＥＣＬ専用命令 //
 SCR_CALL		= 0x50,	// プロシージャを呼び出す   Cmd(1) + Addr(4)
 SCR_ATK		= 0x51,	// 攻撃スレッドを発生させる Cmd(1) + dx(2) + dy(2) + Addr(4)
 SCR_ESET		= 0x52,	// 敵セット                 Cmd(1) + Addr(4)
 SCR_RET		= 0x53,	// プロシージャからの復帰   Cmd(1) + pop(1)
 SCR_ANM		= 0x54,	// アニメーション設定  Cmd(1) + Ptn(1) + Spd(1)
 SCR_FATK		= 0x55,	// 死亡時に発動する攻撃コマンド Cmd(1) + Addr(4)
 SCR_ATKNP		= 0x56,	// 親無しオブジェクトセット Cmd(1) + Addr(4)

 SCR_MOV		= 0x57,	// 直線移動     Cmd(1) + v0(2) + t(2)
 SCR_ACC		= 0x58,	// 加速付き移動 Cmd(1) + v0(2) + a(2)  + t(2)
 SCR_ROL		= 0x59,	// 回転付き移動 Cmd(1) + v0(2) + vd(1) + t(2)
 SCR_WAITATOBJ	= 0x5a,	// 攻撃オブジェクトの終了を待つ Cmd(1)

 SCR_PSE		= 0x5b,	// [ECL/TCL] 効果音を再生する Cmd(1) + ID(1)
 SCR_KILL		= 0x5c,	// 敵を殺して爆発させる(fatk 有効) Cmd(1)

 SCR_MDMG		= 0x5d,	// マッドネスゲージに対するダメージ Cmd(1) + Damage(2)

 SCR_CHILD		= 0x5e,	// 子を発生させる   Cmd(1) + ID(1) + Addr(4)
 SCR_CHGTASK	= 0x5f,	// 子のタスクを変更 Cmd(1) + ID(1) + Addr(4)
 SCR_PARENT		= 0x60,	// 親のデータ参照   Cmd(1) + RegsID(1)

 SCR_PMOV		= 0x61,	// 親と同期・直線移動   SCR_MOV に同じ / break gr6, gr7
 SCR_PACC		= 0x62,	// 親と同期・加速移動   SCR_ACC に同じ / break gr6, gr7
 SCR_PROL		= 0x63,	// 親と同期・回転移動   SCR_ROL に同じ / break gr6, gr7
 SCR_PNOP		= 0x64,	// 親と同期・一時停止   SCR_NOP に同じ / braek gr6, gr7

 SCR_ATK2		= 0x65,	// 攻撃オブジェクトセット(Param)
								// Cmd(1) + dx(2) + dy(2) + Param(4) + Addr(4)

 SCR_EFC		= 0x66,	// エフェクト発生
								// Cmd(1) + dx(2) + dy(2) + type(1)


// ＴＣＬ専用命令 //
 SCR_TAMA		= 0x70,	// 弾発射モードで動作                  Cmd(1)
 SCR_LASER		= 0x71,	// レーザー発射モードで動作            Cmd(1)
 SCR_DEGE		= 0x72,	// 角度を敵の進行方向にセットする      Cmd(1)
 SCR_DEGS		= 0x73,	// 角度を自機方向にセットする[TCL/ECL] Cmd(1)
 SCR_LLCHARGE	= 0x74,	// レーザーを溜め状態にする    Cmd(1)
 SCR_LLOPEN		= 0x75,	// レーザーをオープンする      Cmd(1) + w(4)
 SCR_LLCLOSE	= 0x76,	// レーザーを閉じる            Cmd(1)
 SCR_HLASER		= 0x77,	// ホーミングレーザーを発射 Cmd(1)
 SCR_LSPHERE	= 0x78,	// ライトニング・すふぃぁ   Cmd(1)
 SCR_RLASER		= 0x79,	// ラウンドレーザーを発射   Cmd(1)
 SCR_CROSS		= 0x7a,	// 十字架を発動させる       Cmd(1)
 SCR_FLOWER		= 0x7b,	// 花を発生させる           Cmd(1)
 SCR_GFIRE		= 0x7c,	// Ｇ．ＦＩＲＥ             Cmd(1)
 SCR_IONRING	= 0x7d,	// イオンリング             Cmd(1)


// ＴＡＬＫ専用命令 //
 SCR_TALKMSG	= 0x90,	// [TALK] メッセージ挿入  Cmd(1) + msg(文字列長+1)
 SCR_TALKKEY	= 0x91,	// [TALK] キー入力待ち @  Cmd(1)
 SCR_TALKNEWL	= 0x92,	// [TALK] 改行要請     ;  Cmd(1)
 SCR_TALKWAIT	= 0x93,	// [TALK] 待ち命令     $  Cmd(1) + t(2) Skip可能



// ＥｘＡｎｍ専用命令 //
 SCR_TASK		= 0xa0,	// [ExAnm] タスクを発生させる Cmd(1) + Addr(4)
								// 注意：スクリプト上の表記は Set となる

 SCR_TEXMODE	= 0xa1,	// [ExAnm] モード変更を行う Cmd(1) + Mode(1)
								// 通常・加算半透明 / Single・Double テクスチャモード



// 標準命令(必ずサポートしている必要がある) //
 SCR_PUSHR		= 0xc0,	// [後] スタックにレジスタの内容を積む
 SCR_POPR		= 0xc1,	// [後] レジスタにスタックの内容をコピー
 SCR_MOVC		= 0xc2,	// [中] レジスタに定数の代入を行う(MOVC REG CONST)

 SCR_PUSHC		= 0xc4,	// [前] 定数をスタックにＰＵＳＨする
 SCR_TJMP		= 0xc5,	// [前] POP して真ならばジャンプする
 SCR_FJMP		= 0xc6,	// [前] POP して偽ならばジャンプする
 SCR_JMP		= 0xc7,	// [前] 無条件ジャンプ
 SCR_OJMP		= 0xca,	// [前] POP して真ならPUSH(TRUE),  JMP
 SCR_AJMP		= 0xcb,	// [前] POP して偽ならPUSH(FALSE), JMP
 SCR_EXIT		= 0xcc,	// [前] 終了する
 SCR_LPOP		= 0xcd,	// [前] 演算スタックトップ->JMP スタックトップ
 SCR_LJMP		= 0xce,	// [前] ０ならジャンプ、真ならデクリメント

 SCR_ADD			= 0xd0,	// [前] Push(Pop(1) + Pop(0))
 SCR_SUB			= 0xd1,	// [前] Push(Pop(1) - Pop(0))
 SCR_MUL			= 0xd2,	// [前] Push(Pop(1) * Pop(0))
 SCR_DIV			= 0xd3,	// [前] Push(Pop(1) / Pop(0))
 SCR_MOD			= 0xd4,	// [前] Push(Pop(1) % Pop(0))
 SCR_NEG			= 0xd5,	// [前] Push(-Pop(0))

 SCR_SINL		= 0xd6,	// [前] Push(sinl(Pop(1), Pop(0))
 SCR_COSL		= 0xd7,	// [前] Push(cosl(Pop(1), Pop(0))
 SCR_RND		= 0xd8,	// [前] Push(rnd() % Pop(0))
 SCR_ATAN		= 0xd9,	// [前] Push(atan(Pop(1), Pop(0))

 SCR_EQUAL		= 0xda,	// [前] Push(Pop(1) == Pop(0))
 SCR_NOTEQ		= 0xdb,	// [前] Push(Pop(1) != Pop(0))
 SCR_ABOVE		= 0xdc,	// [前] Push(Pop(1) >  Pop(0))
 SCR_LESS		= 0xdd,	// [前] Push(Pop(1) <  Pop(0))
 SCR_ABOVEEQ	= 0xde,	// [前] Push(Pop(1) >= Pop(0))
 SCR_LESSEQ		= 0xdf,	// [前] Push(Pop(1) <= Pop(0))

 SCR_MAX		= 0xe0,	// [前] Push( max(Pop(0), Pop(1)) )
 SCR_MIN		= 0xe1,	// [前] Push( min(Pop(0), Pop(1)) )


};

enum SCL_DATATYPE {
	NONE = -1,
	COMMAND,
	ADDRESS,
	U8,
	U16,
	U32,
	I8,
	I16,
	I32,
	STRING
};


/***** [ 定数 ] *****/
#define SCL_NUMREGS			0		// ＳＣＬ命令のレジスタ本数
#define NUM_CHARACTERS		10		// キャラクタの最大数
#define SCLBUFFER_SIZE		50		// ＳＣＬ配置用バッファのサイズ
#define LOADTEXTURE_MAX		12		// 同時ロードできるテクスチャの最大枚数

/***** [クラス定義] *****/

// テクスチャロード命令のエントリポイント集 //
typedef struct tagLoadTextureEntry {
	int		EntryPoint[LOADTEXTURE_MAX];	// エントリポイント
	int		NumTextures;					// テクスチャ枚数
} LoadTextureEntry;


// ＳＣＬヘッダ //
typedef struct tagSCLHeader {
	// 編隊の定義数 //
	int NumLv1SCL;		// Level １ ＳＣＬ定義数
	int NumLv2SCL;		// Level ２ ＳＣＬ定義数
	int NumLv3SCL;		// Level ３ ＳＣＬ定義数
	int NumLv4SCL;		// Level ４ ＳＣＬ定義数

	// テクスチャ初期化ブロックの開始アドレス //
	int TexInitializer;

	// プレイヤー初期化用テーブル //
	int Lv1Attack[NUM_CHARACTERS];	// Ｌｅｖｅｌ１アタック
	int Lv2Attack[NUM_CHARACTERS];	// Ｌｅｖｅｌ２アタック
	int BossAddr[NUM_CHARACTERS];	// ボス定義の開始アドレス
	int ComboAddr[NUM_CHARACTERS];	// コンボアタックの開始アドレス

	// アニメーション定義テーブル //
	int ExAnmLv1[NUM_CHARACTERS];	// Ｌｅｖｅｌ１アタック用アニメ
	int ExAnmLv2[NUM_CHARACTERS];	// Ｌｅｖｅｌ２アタック用アニメ
	int ExAnmBoss[NUM_CHARACTERS];	// ボスアタック用アニメ
	int ExAnmWin[NUM_CHARACTERS];	// 勝った時のアニメ

	// 編隊定義用テーブル //
	int SCL_Lv1[SCLBUFFER_SIZE];	// Ｌｅｖｅｌ１ 編隊定義
	int SCL_Lv2[SCLBUFFER_SIZE];	// Ｌｅｖｅｌ２ 編隊定義
	int SCL_Lv3[SCLBUFFER_SIZE];	// Ｌｅｖｅｌ３ 編隊定義
	int SCL_Lv4[SCLBUFFER_SIZE];	// Ｌｅｖｅｌ４ 編隊定義

	// テクスチャロードのエントリポイント //
	LoadTextureEntry	LTEntry[NUM_CHARACTERS];
} SCLHeader;


struct SCLParamData {
	SCL_DATATYPE datatype;
	//Diferent datatypes
	union {
		command cmd;
		address ads;
		u8 byte;
		u16 word;
		u32 dword;
		u8 sbyte;
		u16 sword;
		u32 sdword;
		string stringdata;
	};
};

struct SCLInstructionDefine {
	const char* name;
	int cnt;
	SCL_DATATYPE paramdatatype[15];
};

struct SCLInstructionData {
	SCL_INSTRUCTION cmd;
	address add;
	int cnt;
	std::vector<SCLParamData> param;
};

enum PROC_TYPE {
	PROC_LABEL,
	PROC_TEXINIT,
	PROC_ENEMY,
	PROC_ATK,
	PROC_EXANM,
	PROC_SET,
	PROC_LOADTEX,
};

struct ProcData {
	PROC_TYPE type;
	std::string name;
};

static std::map<SCL_INSTRUCTION, SCLInstructionDefine> g_InstructionSize = {
	{SCR_NOP,		{"NOP", 1 + 1, {COMMAND, U16}}},
	{SCR_SET,		{"SET", 1 + 3, {COMMAND, I16, I16, ADDRESS}}},

	{SCR_LOAD,		{"LOAD", 1 + 3, {U8, COMMAND, U8, STRING}} },
	{SCR_RECT,		{"RECT", 1 + 5, {COMMAND, U8, I16, I16, I16, I16}} },
	{SCR_ANIME,		{"ANIME", 1 + 2, {COMMAND, U8, U8}} },
	{SCR_LOADEX,	{"LOADEX", 1 + 1, {COMMAND, STRING}} },
	{SCR_STOP,		{"STOP", 1, {COMMAND}} },

	{SCR_CALL,		{"CALL", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_ATK,		{"ATK", 1 + 3, {COMMAND, I16, I16, ADDRESS}}},
	{SCR_ESET,		{"ESET", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_RET,		{"RET", 1 + 1, {COMMAND, U8}}},
	{SCR_ANM,		{"ANM", 1 + 2, {COMMAND, U8, U8}}},
	{SCR_FATK,		{"FATK", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_ATKNP,		{"ATKNP", 1 + 1, {COMMAND, ADDRESS}}},

	{SCR_MOV,		{"MOV", 1 + 2, {COMMAND, I16, U16}}},
	{SCR_ACC,		{"ACC", 1 + 3, {COMMAND, I16, I16, U16}}},
	{SCR_ROL,		{"ROL", 1 + 3, {COMMAND, I16, I8, U16}}},
	{SCR_WAITATOBJ, {"WAITATOBJ", 1, {COMMAND}}},

	{SCR_PSE,		{"PSE", 1 + 1, {COMMAND, U8}}},
	{SCR_KILL,		{"KILL", 1, {COMMAND}}},

	{SCR_MDMG,		{"MDMG", 1 + 1, {COMMAND, I16}}},
	{SCR_CHILD,		{"CHILD", 1 + 2, {COMMAND, U8, ADDRESS}}},
	{SCR_CHGTASK,	{"CHGTASK", 1 + 2, {COMMAND, U8, ADDRESS}}},
	{SCR_PARENT,	{"PARENT", 1 + 1, {U8, COMMAND}}},

	{SCR_PMOV,		{"PMOV", 1 + 2, {COMMAND, I16, U16}}},
	{SCR_PACC,		{"PACC", 1 + 3, {COMMAND, I16, I16, U16}}},
	{SCR_PROL,		{"PROL", 1 + 3, {COMMAND, I16, I8, U16}}},
	{SCR_PNOP,		{"PNOP", 1 + 1, {COMMAND, U16}}},

	{SCR_ATK2,		{"ATK2", 1 + 4, {COMMAND, I16, I16, I32, ADDRESS}}},

	{SCR_EFC,		{"EFC", 1 + 3, {COMMAND, I16, I16, U8}}},

	{SCR_TAMA,		{"TAMA", 1, {COMMAND}}},
	{SCR_LASER,		{"LASER", 1, {COMMAND}}},
	{SCR_DEGE,		{"DEGE", 1, {COMMAND}}},
	{SCR_DEGS,		{"DEGS", 1, {COMMAND}}},
	{SCR_LLCHARGE,	{"LLCHARGE", 1, {COMMAND}}},
	{SCR_LLOPEN,	{"LLOPEN", 1 + 1, {COMMAND, U32}}},
	{SCR_LLCLOSE,	{"LLCLOSE", 1, {COMMAND}}},
	{SCR_HLASER,	{"HLASER", 1, {COMMAND}}},
	{SCR_LSPHERE,	{"LSPHERE", 1, {COMMAND}}},
	{SCR_RLASER,	{"RLASER", 1, {COMMAND}}},
	{SCR_CROSS,		{"CROSS", 1, {COMMAND}}},
	{SCR_FLOWER,	{"FLOWER", 1, {COMMAND}}},
	{SCR_GFIRE,		{"GFIRE", 1, {COMMAND}}},
	{SCR_IONRING,	{"IONRING", 1, {COMMAND}}},

	{SCR_TASK,		{"TASK", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_TEXMODE,	{"TEXMODE", 1 + 1, {COMMAND, U8}}},

	{SCR_PUSHR,		{"PUSHR", 1 + 1, {U8, COMMAND}}},
	{SCR_POPR,		{"POPR", 1 + 1, {U8, COMMAND}}},
	{SCR_MOVC,		{"MOVC", 1 + 2, {COMMAND, U8, I32}}},

	{SCR_PUSHC,		{"PUSHC", 1 + 1, {COMMAND, I32}}},
	{SCR_TJMP,		{"TJMP", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_FJMP,		{"FJMP", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_JMP,		{"JMP", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_OJMP,		{"OJMP", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_AJMP,		{"AJMP", 1 + 1, {COMMAND, ADDRESS}}},
	{SCR_EXIT,		{"EXIT", 1, {COMMAND}}},
	{SCR_LPOP,		{"LPOP", 1, {COMMAND}}},
	{SCR_LJMP,		{"LJMP", 1 + 1, {COMMAND, ADDRESS}}},

	{SCR_ADD,		{"ADD", 1, {COMMAND}}},
	{SCR_SUB,		{"SUB", 1, {COMMAND}}},
	{SCR_MUL,		{"MUL", 1, {COMMAND}}},
	{SCR_DIV,		{"DIV", 1, {COMMAND}}},
	{SCR_MOD,		{"MOD", 1, {COMMAND}}},
	{SCR_NEG,		{"NEG", 1, {COMMAND}}},

	{SCR_SINL,		{"SINL", 1, {COMMAND}}},
	{SCR_COSL,		{"COSL", 1, {COMMAND}}},
	{SCR_RND,		{"RND", 1, {COMMAND}}},
	{SCR_ATAN,		{"ATAN", 1, {COMMAND}}},

	{SCR_EQUAL,		{"EQUAL", 1, {COMMAND}}},
	{SCR_NOTEQ,		{"NOTEQ", 1, {COMMAND}}},
	{SCR_ABOVE,		{"ABOVE", 1, {COMMAND}}},
	{SCR_LESS,		{"LESS", 1, {COMMAND}}},
	{SCR_ABOVEEQ,	{"ABOVEEQ", 1, {COMMAND}}},
	{SCR_LESSEQ,	{"LESSEQ", 1, {COMMAND}}},

	{SCR_MAX,		{"MAX", 1, {COMMAND}}},
	{SCR_MIN,		{"MIN", 1, {COMMAND}}},
};

inline const char* ID2String(int id) {
	switch (id) {
	case 0:		return "VIVIT";
	case 1:		return "STG1";
	case 2:		return "STG2";
	case 3:		return "STG3";
	case 4:		return "STG4";
	case 5:		return "STG5";
	case 6:		return "MORGAN";
	case 7:		return "MUSE";
	case 8:		return "YUKA";
	default:	return "";
	}
}

#endif