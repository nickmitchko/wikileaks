#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>
#include <dirent.h>
#include "sha256.h"
#include "ripe160.h"

typedef uint64_t vli_t;
typedef struct blk_t blk_t;
typedef struct tx_t tx_t;
typedef struct txin_t txin_t;
typedef struct txout_t txout_t;

struct blk_t
{
	uint32_t blk_magic;
	uint32_t blk_header_len;
	uint32_t blk_ver;
	uint8_t blk_prev_sha[32];
	uint8_t blk_merkle_sha[32];
	uint32_t blk_ts;
	uint32_t blk_bits;
	uint32_t blk_nonce;
	uint8_t blk_hash[32];
	vli_t blk_tx_cnt;
	tx_t** blk_tx;
};

struct tx_t
{
	uint32_t tx_ver;
	vli_t tx_input_cnt;
	txin_t** tx_input;
	vli_t tx_output_cnt;
	txout_t** tx_output;
	uint32_t tx_locktime;
	uint8_t tx_hash[32];
};

struct txin_t
{
	uint8_t txi_prev_out[32];
	uint32_t txi_idx;
	vli_t txi_script_len;
	uint8_t* txi_script_data;
	uint32_t txi_seq;
};

struct txout_t
{
	uint64_t txo_val;
	vli_t txo_script_len;
	uint8_t* txo_script_data;
};

uint8_t read_u8(uint8_t** datap);
uint16_t read_u16(uint8_t** datap);
uint32_t read_u32(uint8_t** datap);
uint64_t read_u64(uint8_t** datap);
uint64_t read_vli(uint8_t** datap);
void read_sha(uint8_t** datap,uint8_t sha[32]);
void read_bin(uint8_t** datap,uint8_t* buf,uint64_t sz);
blk_t* alloc_blk();
tx_t** alloc_tx(uint64_t cnt);
txin_t** alloc_txin(uint64_t cnt);
txout_t** alloc_txout(uint64_t cnt);
void free_txout(txout_t* txout);
void free_txin(txin_t* txin);
void free_tx(tx_t* tx);
void free_blk(blk_t* blk);
void print_sha(const char* s,uint8_t sha[32]);
void print_bin(const char* s,uint8_t* dat,uint64_t sz);
void read_blk(uint8_t** datap,blk_t* blk);
void read_tx(uint8_t** datap,tx_t* tx);
void read_txin(uint8_t** datap,txin_t* txin);
void read_txout(uint8_t** datap,txout_t* txout);

static sqlite3 *db;

typedef struct op_t op_t;
struct op_t
{
	uint8_t op_code;
	uint8_t op_en;
	char* op_desc;
};

op_t ops[] = {
	{ 0x0,  1, "OP_0" },
	{ 0x4c, 1, "OP PUSHDATA1" },
	{ 0x4d, 1, "OP PUSHDATA2" },
	{ 0x4e, 1, "OP PUSHDATA4" },
	{ 0x4f, 1, "OP_1NEGATE" },
	{ 0x51, 1, "OP_1" },
	{ 0x52, 1, "OP_2" },
	{ 0x53, 1, "OP_3" },
	{ 0x54, 1, "OP_4" },
	{ 0x55, 1, "OP_5" },
	{ 0x56, 1, "OP_6" },
	{ 0x57, 1, "OP_7" },
	{ 0x58, 1, "OP_8" },
	{ 0x59, 1, "OP_9" },
	{ 0x5a, 1, "OP_10" },
	{ 0x5b, 1, "OP_11" },
	{ 0x5c, 1, "OP_12" },
	{ 0x5d, 1, "OP_13" },
	{ 0x5e, 1, "OP_14" },
	{ 0x5f, 1, "OP_15" },
	{ 0x60, 1, "OP_16" },
	{ 0x61, 1, "OP_NOP" },
	{ 0x63, 1, "OP_IF" },
	{ 0x64, 1, "OP_NOTIF" },
	{ 0x67, 1, "OP_ELSE" },
	{ 0x68, 1, "OP_ENDIF" },
	{ 0x69, 1, "OP_VERIFY" },
	{ 0x6a, 1, "OP_RETURN" },
	{ 0x6b, 1, "OP_TOALTSTACK" },
	{ 0x6c, 1, "OP_FROMALTSTACK" },
	{ 0x73, 1, "OP_IFDUP" },
	{ 0x74, 1, "OP_DEPTH" },
	{ 0x75, 1, "OP_DROP" },
	{ 0x76, 1, "OP_DUP" },
	{ 0x77, 1, "OP_NIP" },
	{ 0x78, 1, "OP_OVER" },
	{ 0x79, 1, "OP_PICK" },
	{ 0x7a, 1, "OP_ROLL" },
	{ 0x7b, 1, "OP_ROT" },
	{ 0x7c, 1, "OP_SWAP" },
	{ 0x7d, 1, "OP_TUCK" },
	{ 0x6d, 1, "OP_2DROP" },
	{ 0x6e, 1, "OP_2DUP" },
	{ 0x6f, 1, "OP_3DUP" },
	{ 0x70, 1, "OP_2OVER" },
	{ 0x71, 1, "OP_2ROT" },
	{ 0x72, 1, "OP_2SWAP" },
	{ 0x7e, 0, "OP_CAT" },
	{ 0x7f, 0, "OP_SUBSTR" },
	{ 0x80, 0, "OP_LEFT" },
	{ 0x81, 0, "OP_RIGHT" },
	{ 0x82, 1, "OP_SIZE" },
	{ 0x83, 0, "OP_INVERT" },
	{ 0x84, 0, "OP_AND" },
	{ 0x85, 0, "OP_OR" },
	{ 0x86, 0, "OP_XOR" },
	{ 0x87, 1, "OP_EQUAL" },
	{ 0x88, 1, "OP_EQUALVERIFY" },
	{ 0x8b, 1, "OP_1ADD" },
	{ 0x8c, 1, "OP_1SUB" },
	{ 0x8d, 0, "OP_2MUL" },
	{ 0x8e, 0, "OP_2DIV" },
	{ 0x8f, 1, "OP_NEGATE" },
	{ 0x90, 1, "OP_ABS" },
	{ 0x91, 1, "OP_NOT" },
	{ 0x92, 1, "OP_0NOTEQUAL" },
	{ 0x93, 1, "OP_ADD" },
	{ 0x94, 1, "OP_SUB" },
	{ 0x95, 0, "OP_MUL" },
	{ 0x96, 0, "OP_DIV" },
	{ 0x97, 0, "OP_MOD" },
	{ 0x98, 0, "OP_LSHIFT" },
	{ 0x99, 0, "OP_RSHIFT" },
	{ 0x9a, 1, "OP_BOOLAND" },
	{ 0x9b, 1, "OP_BOOLOR" },
	{ 0x9c, 1, "OP_NUMEQUAL" },
	{ 0x9d, 1, "OP_NUMEQUALVERIFY" },
	{ 0x9e, 1, "OP_NUMNOTEQUAL" },
	{ 0x9d, 1, "OP_NUMEQUALVERIFY" },
	{ 0x9e, 1, "OP_NUMNOTEQUAL" },
	{ 0x9f, 1, "OP_LESSTHAN" },
	{ 0xa0, 1, "OP_GREATERTHAN" },
	{ 0xa1, 1, "OP_LESSTHANOREQUAL" },
	{ 0xa2, 1, "OP_GREATERTHANOREQUAL" },
	{ 0xa3, 1, "OP_MIN" },
	{ 0xa4, 1, "OP_MAX" },
	{ 0xa5, 1, "OP_WITHIN" },
	{ 0xa6, 1, "OP_RIPEMD160" },
	{ 0xa7, 1, "OP_SHA1" },
	{ 0xa8, 1, "OP_SHA256" },
	{ 0xa9, 1, "OP_HASH160" },
	{ 0xaa, 1, "OP_HASH256" },
	{ 0xab, 1, "OP_CODESEPARATOR" },
	{ 0xac, 1, "OP_CHECKSIG" },
	{ 0xad, 1, "OP_CHECKSIGVERIFY" },
	{ 0xae, 1, "OP_CHECKMULTISIG" },
	{ 0xaf, 1, "OP_CHECKMULTISIGVERIFY" },
	{ 0xb1, 1, "OP_CHECKLOCKTIMEVERIFY" },
	{ 0xb2, 1, "OP_CHECKSEQUENCEVERIFY" },
	{ 0xff, 0, "NOTANOP" }
};

char op_buf[32];
const char* op_print(op_t* op)
{
	memset(op_buf,0,sizeof(op_buf));
	snprintf(op_buf,sizeof(op_buf),"%s",op->op_desc);
	if(!op->op_en)
		snprintf(op_buf,sizeof(op_buf)," DISABLED");
	return op_buf;
}

op_t* op_get(uint8_t o)
{
	int32_t idx = 0;
	while(1)
	{
		op_t* op = &ops[idx++];
		if(op->op_code == 0xff || op->op_code == o)
			break;
	}
	return &ops[idx];
}

void script_read(uint8_t* data,uint64_t sz)
{
	uint8_t* p = data;
	while(p - data < sz)
	{
		uint8_t opcode = *p++;
		if(opcode >= 0x01 && opcode <= 0x4b)
		{
			uint8_t len = opcode;
			//printf("------PUSH DATA (%u):\n",(uint32_t)len & 0xff);
			uint32_t i,j=0;
			for(i=0;i<opcode;i++)
			{
				//printf("%02x ",*p++);
				j++;
				if(j == 16)
				{
					j = 0;
					//printf("\n");
				}
			}
			//printf("\n");
		}
		else if(opcode == 0x4c)
		{
			uint8_t len = *p++;
			//printf("------PUSH DATA1 (%u):\n",(uint32_t)len & 0xff);
			uint32_t i,j=0;
			for(i=0;i<opcode;i++)
			{
				//printf("%02x ",*p++);
				j++;
				if(j == 16)
				{
					j = 0;
					//printf("\n");
				}
			}
			//printf("\n");
		}
		else if(opcode == 0x4d)
		{
			uint16_t len = 0;
			len += *p++;
			len += *p++ << 8;
			//printf("------PUSH DATA2 (%u):\n",len);
			uint32_t i,j=0;
			for(i=0;i<opcode;i++)
			{
				//printf("%02x ",*p++);
				j++;
				if(j == 16)
				{
					j = 0;
					//printf("\n");
				}
			}
			//printf("\n");
		}
		else if(opcode == 0x4e)
		{
			uint32_t len = 0;
			len += *p++;
			len += *p++ << 8;
			len += *p++ << 16;
			len += *p++ << 24;
			//printf("------PUSH DATA4 (%u):\n",len);
			uint32_t i,j=0;
			for(i=0;i<opcode;i++)
			{
				//printf("%02x ",*p++);
				j++;
				if(j == 16)
				{
					j = 0;
					//printf("\n");
				}
			}
			//printf("\n");
		}
		else
		{
			op_t* op = op_get(opcode);
			//printf("%s\n",op_print(op));
		}
	}
}

uint8_t read_u8(uint8_t** datap)
{
	uint8_t r = *(*datap)++;
	return r;
}

uint16_t read_u16(uint8_t** datap)
{
	uint16_t r = 0;
	r += *(*datap)++;
	r += *(*datap)++ << 8;
	return r;
}

uint32_t read_u32(uint8_t** datap)
{
	uint32_t r = 0;
	r += *(*datap)++;
	r += *(*datap)++ << 8;
	r += *(*datap)++ << 16;
	r += *(*datap)++ << 24;
	return r;
}

uint64_t read_u64(uint8_t** datap)
{
	uint64_t r = 0;
	r += *(*datap)++;
	r += *(*datap)++ << 8;
	r += *(*datap)++ << 16;
	r += *(*datap)++ << 24;
	r += (uint64_t)*(*datap)++ << 32;
	r += (uint64_t)*(*datap)++ << 40;
	r += (uint64_t)*(*datap)++ << 48;
	r += (uint64_t)*(*datap)++ << 56;
	return r;
}

uint64_t read_vli(uint8_t** datap)
{
	uint64_t r = read_u8(datap);
	if(r == 0xFD)
		return read_u16(datap);
	else if(r == 0xFE)
		return read_u32(datap);
	else if(r == 0xFF)
		return read_u64(datap);
	return r;
}

void read_sha(uint8_t** datap,uint8_t sha[32])
{
	int32_t i;
	for(i=0;i<32;i++)
		sha[i] = read_u8(datap);
}

void read_bin(uint8_t** datap,uint8_t* buf,uint64_t sz)
{
	uint64_t i;
	for(i=0;i<sz;i++)
		buf[i] = read_u8(datap);
}

blk_t* alloc_blk()
{
        blk_t* blk = calloc(1,sizeof(blk_t));
        return blk;
}

tx_t** alloc_tx(uint64_t cnt)
{
        tx_t** arr = calloc(cnt,sizeof(tx_t*));
        uint64_t i;
        for(i=0;i<cnt;i++)
                arr[i] = calloc(1,sizeof(tx_t));
        return arr;
}

txin_t** alloc_txin(uint64_t cnt)
{
        txin_t** arr = calloc(cnt,sizeof(txin_t*));
        uint64_t i;
        for(i=0;i<cnt;i++)
                arr[i] = calloc(1,sizeof(txin_t));
        return arr;
}

txout_t** alloc_txout(uint64_t cnt)
{
        txout_t** arr = calloc(cnt,sizeof(txout_t*));
        uint64_t i;
        for(i=0;i<cnt;i++)
                arr[i] = calloc(1,sizeof(txout_t));
        return arr;
}

void free_txout(txout_t* txout)
{
        free(txout->txo_script_data);
        free(txout);
}

void free_txin(txin_t* txin)
{
        free(txin->txi_script_data);
        free(txin);
}

void free_tx(tx_t* tx)
{
        uint64_t i;
        for(i=0;i<tx->tx_input_cnt;i++)
                free_txin(tx->tx_input[i]);
        for(i=0;i<tx->tx_output_cnt;i++)
                free_txout(tx->tx_output[i]);
	free(tx->tx_input);
	free(tx->tx_output);
        free(tx);
}

void free_blk(blk_t* blk)
{
        uint64_t i;
        for(i=0;i<blk->blk_tx_cnt;i++)
                free_tx(blk->blk_tx[i]);
	free(blk->blk_tx);
        free(blk);
}

void print_sha(const char* s,uint8_t sha[32])
{
	printf("%s",s);
	uint32_t i;
	for(i=0;i<32;i++)
		printf("%02x",(uint32_t)sha[31-i] & 0xff);
	printf("\n");
}

void get_sha(char buf[65],uint8_t sha[32])
{
	memset(buf,0,65);
	uint32_t i;
	for(i=0;i<32;i++)
	{
		char tmp[3];
		snprintf(tmp,sizeof(tmp),"%02x",(uint32_t)sha[31-i] & 0xff);
		strlcat(buf,tmp,65);
	}
}

void print_bin(const char* s,uint8_t* dat,uint64_t sz)
{
	printf("%s",s);
	uint64_t i;
	for(i=0;i<sz;i++)
		printf("%02x ",(uint32_t)dat[i] & 0xff);
	printf("\n");
}

void read_blk(uint8_t** datap,blk_t* blk)
{
	//printf("\n--------------------\nBLOCK:\n");

	SHA256_CTX ctx;

        blk->blk_magic = read_u32(datap);
        blk->blk_header_len = read_u32(datap);
	
	uint8_t sha_hash1[32];

	uint8_t* block_hash_data = *datap;

        blk->blk_ver = read_u32(datap);
        read_sha(datap,blk->blk_prev_sha);
	//print_sha("prev: ",blk->blk_prev_sha);
        read_sha(datap,blk->blk_merkle_sha);
	//print_sha("merkle: ",blk->blk_merkle_sha);
        blk->blk_ts = read_u32(datap);
        blk->blk_bits = read_u32(datap);
        blk->blk_nonce = read_u32(datap);

	uint8_t* block_hash_data_end = *datap;
	memset(blk->blk_hash,0,sizeof(blk->blk_hash));

	//Block hash calculation
	sha256_init(&ctx);
	sha256_update(&ctx, block_hash_data, block_hash_data_end - block_hash_data);
	sha256_final(&ctx, sha_hash1);
	sha256_init(&ctx);
	sha256_update(&ctx, sha_hash1, sizeof(sha_hash1));
	sha256_final(&ctx, blk->blk_hash);

	//print_sha("block hash: ",blk->blk_hash);

        blk->blk_tx_cnt = read_vli(datap);
	//printf("TS: %u TXs: %llu\n",blk->blk_ts,blk->blk_tx_cnt);

	blk->blk_tx = alloc_tx(blk->blk_tx_cnt);
	uint64_t i;
	for(i=0;i<blk->blk_tx_cnt;i++)
		read_tx(datap,blk->blk_tx[i]);
}

void read_tx(uint8_t** datap,tx_t* tx)
{
	uint64_t i;
	SHA256_CTX ctx;
	uint8_t sha_hash1[32];

	uint8_t* tx_hash_data = *datap;

        tx->tx_ver = read_u32(datap);
        tx->tx_input_cnt = read_vli(datap);
	tx->tx_input = alloc_txin(tx->tx_input_cnt);
	for(i=0;i<tx->tx_input_cnt;i++)
		read_txin(datap,tx->tx_input[i]);
        tx->tx_output_cnt = read_vli(datap);
	tx->tx_output = alloc_txout(tx->tx_output_cnt);
	for(i=0;i<tx->tx_output_cnt;i++)
		read_txout(datap,tx->tx_output[i]);
        tx->tx_locktime = read_u32(datap);

	uint8_t* tx_hash_data_end = *datap;
	memset(tx->tx_hash,0,sizeof(tx->tx_hash));

	sha256_init(&ctx);
        sha256_update(&ctx, tx_hash_data, tx_hash_data_end - tx_hash_data);
        sha256_final(&ctx, sha_hash1);
        sha256_init(&ctx);
        sha256_update(&ctx, sha_hash1, sizeof(sha_hash1));
        sha256_final(&ctx, tx->tx_hash);
        //print_sha("tx_hash: ",tx->tx_hash);
}

void read_txin(uint8_t** datap,txin_t* txin)
{
        read_sha(datap,txin->txi_prev_out);
        txin->txi_idx = read_u32(datap);
        txin->txi_script_len = read_vli(datap);
        txin->txi_script_data = malloc(txin->txi_script_len);
        read_bin(datap,txin->txi_script_data,txin->txi_script_len);
	//script_read(txin->txi_script_data,txin->txi_script_len);
        txin->txi_seq = read_u32(datap);
}

void read_txout(uint8_t** datap,txout_t* txout)
{
        txout->txo_val = read_u64(datap);
        txout->txo_script_len = read_vli(datap);
	txout->txo_script_data = malloc(txout->txo_script_len);
        read_bin(datap,txout->txo_script_data,txout->txo_script_len);
	//script_read(txout->txo_script_data,txout->txo_script_len);
}

int sqlcb(void *p, int argc, char** args, char** col)
{
	int i;
	for(i=0;i<argc;i++)
		printf("%s = %s\n", col[i], args[i] ? args[i] : "(null)");
	printf("\n");
	return 0;
}

static char* query_buf;
int32_t sql_query(const char* fmt, ...)
{
	if(!query_buf)
	{
		query_buf = malloc(1024 * 1024 * 2); //Way more than enough...
		if(!query_buf)
			return -1;
	}
	memset(query_buf,0,1024*1024*2);

        char *err = 0;

        va_list args;
        va_start (args, fmt);
	int32_t sz = vsnprintf(query_buf,1024*1024*2,fmt,args);
        va_end (args);

        int32_t r = sqlite3_exec(db, query_buf, sqlcb, 0, &err);
        if(r != SQLITE_OK)
        {
                fprintf(stderr, "SQL error: %s\n", err);
                sqlite3_free(err);
		//free(buf);
                return -1;
        }
        return 0;
}

sqlite3_stmt* blk_insert_stmt;
sqlite3_stmt* file_insert_stmt;
sqlite3_stmt* tx_insert_stmt;
sqlite3_stmt* rel_blk_insert_stmt;
sqlite3_stmt* tx_input_insert_stmt;
sqlite3_stmt* rel_tx_input_insert_stmt;
sqlite3_stmt* tx_output_insert_stmt;
sqlite3_stmt* rel_tx_output_insert_stmt;

int32_t sql_prepare()
{
	if(!query_buf)
        {
                query_buf = malloc(1024 * 1024 * 2); //Way more than enough...
                if(!query_buf)
                        return -1;
        }
        memset(query_buf,0,1024*1024*2);
	const char* tail;

	snprintf(query_buf,1024*1024*2, "INSERT INTO BLOCK (ID,PREV,MERKLE,HASH,BITS,NONCE,TIME) VALUES (?,?,?,?,?,?,?)");
	if(sqlite3_prepare_v2(db, query_buf, -1, &blk_insert_stmt, 0) != SQLITE_OK)
		return -1;
	memset(query_buf,0,1024*1024*2);

	snprintf(query_buf,1024*1024*2, "INSERT INTO FILE (ID,NAME) VALUES (?,?)");
        if(sqlite3_prepare_v2(db, query_buf, -1, &file_insert_stmt, 0) != SQLITE_OK)
                return -1;
        memset(query_buf,0,1024*1024*2);

	snprintf(query_buf,1024*1024*2, "INSERT INTO TX (ID,HASH,FILE,OFFSET) VALUES (?,?,?,?)");
        if(sqlite3_prepare_v2(db, query_buf, -1, &tx_insert_stmt, 0) != SQLITE_OK)
		return -1;
        memset(query_buf,0,1024*1024*2);

	snprintf(query_buf,1024*1024*2, "INSERT INTO REL_BLOCK_TX (ID,BLOCK,TX) VALUES (?,?,?)");
        if(sqlite3_prepare_v2(db, query_buf, -1, &rel_blk_insert_stmt, 0) != SQLITE_OK)
		return -1;
        memset(query_buf,0,1024*1024*2);

	snprintf(query_buf,1024*1024*2, "INSERT INTO TX_INPUT (ID,TX,PREV,IDX,SEQ,DATA) VALUES (?,?,?,?,?,?)");
        if(sqlite3_prepare_v2(db, query_buf, -1, &tx_input_insert_stmt, 0) != SQLITE_OK)
		return -1;
        memset(query_buf,0,1024*1024*2);

	snprintf(query_buf,1024*1024*2, "INSERT INTO REL_TX_INPUT (ID,TX,INPUT) VALUES (?,?,?)");
        if(sqlite3_prepare_v2(db, query_buf, -1, &rel_tx_input_insert_stmt, 0) != SQLITE_OK)
		return -1;
        memset(query_buf,0,1024*1024*2);

	snprintf(query_buf,1024*1024*2, "INSERT INTO TX_OUTPUT (ID,TX,VAL,DATA,ADDR) VALUES (?,?,?,?,?)");
        if(sqlite3_prepare_v2(db, query_buf, -1, &tx_output_insert_stmt, 0) != SQLITE_OK)
		return -1;
        memset(query_buf,0,1024*1024*2);

	snprintf(query_buf,1024*1024*2, "INSERT INTO REL_TX_OUTPUT (ID,TX,OUTPUT) VALUES (?,?,?)");
        if(sqlite3_prepare_v2(db, query_buf, -1, &rel_tx_output_insert_stmt, 0) != SQLITE_OK)
		return -1;
        memset(query_buf,0,1024*1024*2);

	return 0;
}

int32_t sql_insert_block(uint32_t id,char prev[65],char merkle[65],char hash[65],uint32_t bits,uint32_t nonce,uint32_t time)
{
	if(sqlite3_bind_int(blk_insert_stmt, 1, id) < 0) return -1;
	if(sqlite3_bind_text(blk_insert_stmt, 2, prev, -1, SQLITE_TRANSIENT) < 0) return -1;
	if(sqlite3_bind_text(blk_insert_stmt, 3, merkle, -1, SQLITE_TRANSIENT) < 0) return -1;
	if(sqlite3_bind_text(blk_insert_stmt, 4, hash, -1, SQLITE_TRANSIENT) < 0) return -1;
	if(sqlite3_bind_int(blk_insert_stmt, 5, bits) < 0) return -1;
	if(sqlite3_bind_int(blk_insert_stmt, 6, nonce) < 0) return -1;
	if(sqlite3_bind_int(blk_insert_stmt, 7, time) < 0) return -1;
	if(sqlite3_step(blk_insert_stmt) != SQLITE_DONE) return -1;
	sqlite3_reset(blk_insert_stmt);
	return 0;
}

int32_t sql_insert_file(uint32_t id,const char* file)
{
        if(sqlite3_bind_int(file_insert_stmt, 1, id) < 0) return -1;
        if(sqlite3_bind_text(file_insert_stmt, 2, file, -1, SQLITE_TRANSIENT) < 0) return -1;
        if(sqlite3_step(file_insert_stmt) != SQLITE_DONE) return -1;
        sqlite3_reset(file_insert_stmt);
        return 0;
}

int32_t sql_insert_tx(uint32_t id,char hash[65],uint32_t file,uint32_t offset)
{
        if(sqlite3_bind_int(tx_insert_stmt, 1, id) < 0) return -1;
        if(sqlite3_bind_text(tx_insert_stmt, 2, hash, -1, SQLITE_TRANSIENT) < 0) return -1;
	if(sqlite3_bind_int(tx_insert_stmt, 3, file) < 0) return -1;
	if(sqlite3_bind_int(tx_insert_stmt, 4, offset) < 0) return -1;
	if(sqlite3_step(tx_insert_stmt)  != SQLITE_DONE) return -1;
	sqlite3_reset(tx_insert_stmt);
        return 0;
}

int32_t sql_insert_rel_blk_tx(uint32_t id,uint32_t blk,uint32_t tx)
{
        if(sqlite3_bind_int(rel_blk_insert_stmt, 1, id) < 0) return -1;
        if(sqlite3_bind_int(rel_blk_insert_stmt, 2, blk) < 0) return -1;
	if(sqlite3_bind_int(rel_blk_insert_stmt, 3, tx) < 0) return -1;
	if(sqlite3_step(rel_blk_insert_stmt) != SQLITE_DONE) return -1;
	sqlite3_reset(rel_blk_insert_stmt);
        return 0;
}

int32_t sql_insert_tx_input(uint32_t id,uint32_t tx,char prev[65],uint32_t idx,uint32_t seq,uint8_t* data,uint64_t datalen)
{
        if(sqlite3_bind_int(tx_input_insert_stmt, 1, id) < 0) return -1;
	if(sqlite3_bind_int(tx_input_insert_stmt, 2, tx) < 0) return -1;
        if(sqlite3_bind_text(tx_input_insert_stmt, 3, prev, -1, SQLITE_TRANSIENT) < 0) return -1;
	if(sqlite3_bind_int(tx_input_insert_stmt, 4, idx) < 0) return -1;
        if(sqlite3_bind_int(tx_input_insert_stmt, 5, seq) < 0) return -1;
	if(sqlite3_bind_blob(tx_input_insert_stmt, 6, data, datalen, SQLITE_TRANSIENT) < 0) return -1;
	if(sqlite3_step(tx_input_insert_stmt) != SQLITE_DONE) return -1;
	sqlite3_reset(tx_input_insert_stmt);
        return 0;
}

int32_t sql_insert_rel_tx_input(uint32_t id,uint32_t tx,uint32_t input)
{
        if(sqlite3_bind_int(rel_tx_input_insert_stmt, 1, id) < 0) return -1;
        if(sqlite3_bind_int(rel_tx_input_insert_stmt, 2, tx) < 0) return -1;
        if(sqlite3_bind_int(rel_tx_input_insert_stmt, 3, input) < 0) return -1;
	if(sqlite3_step(rel_tx_input_insert_stmt) != SQLITE_DONE) return -1;
	sqlite3_reset(rel_tx_input_insert_stmt);
        return 0;
}

int32_t sql_insert_tx_output(uint32_t id,uint32_t tx,uint64_t val,uint8_t* data,uint64_t datalen,char* addr)
{
        if(sqlite3_bind_int(tx_output_insert_stmt, 1, id) < 0) return -1;
	if(sqlite3_bind_int(tx_output_insert_stmt, 2, tx) < 0) return -1;
        if(sqlite3_bind_int64(tx_output_insert_stmt, 3, val) < 0) return -1;
	if(sqlite3_bind_blob(tx_output_insert_stmt, 4, data, datalen, SQLITE_TRANSIENT) < 0) return -1;
	if(sqlite3_bind_text(tx_output_insert_stmt, 5, addr, -1, SQLITE_TRANSIENT) < 0) return -1;
	if(sqlite3_step(tx_output_insert_stmt) != SQLITE_DONE) return -1;
	sqlite3_reset(tx_output_insert_stmt);
        return 0;
}

int32_t sql_insert_rel_tx_output(uint32_t id,uint32_t tx,uint32_t output)
{
        if(sqlite3_bind_int(rel_tx_output_insert_stmt, 1, id) < 0) return -1;
        if(sqlite3_bind_int(rel_tx_output_insert_stmt, 2, tx) < 0) return -1;
        if(sqlite3_bind_int(rel_tx_output_insert_stmt, 3, output) < 0) return -1;
	if(sqlite3_step(rel_tx_output_insert_stmt) != SQLITE_DONE) return -1;
	sqlite3_reset(rel_tx_output_insert_stmt);
        return 0;
}

char* bin2hex(uint8_t* data,uint64_t len)
{
	char* ret = calloc(1,(len * 2) + 1);
	if(!ret)
		return NULL;
	uint64_t i;
	for(i=0;i<len;i++)
	{
		char tmp[3];
		snprintf(tmp,3,"%02x",data[i]);
		strlcat(ret,tmp,(len * 2) + 1);
	}
	return ret;
}

static char hextbl[16] = "0123456789abcdef";

uint8_t* hex2bin(char* str,uint64_t* len)
{
	(*len) = 0;
	uint64_t i;
	for(i=0;i<strlen(str);i++)
		(*len)++;
	uint8_t* ret = calloc(1,*len);
		return NULL;
	char* p = str;
	for(i=0;i<*len;i++)
	{
		uint8_t byte;
		int j;
		for(j=0;j<16;j++)
		{
			if(*p++ == hextbl[j])
				byte = j << 8;
		}
		for(j=0;j<16;j++)
		{
			if(*p++ == hextbl[j])
				byte += j;
		}
		ret[i] = byte;
	}
	return ret;
}

int32_t read_block_file(const char* filename);

uint32_t blk_cnt;
uint32_t file_cnt;
uint32_t tx_cnt;
uint32_t rel_blk_tx_cnt;
uint32_t txin_cnt;
uint32_t rel_tx_input_cnt;
uint32_t txout_cnt;
uint32_t rel_tx_output_cnt;

int main(int argc,char** argv)
{
	if(argc != 3)
	{
		fprintf(stderr,"Usage: %s <block directory> <output db>\n",argv[0]);
		return -1;
	}

	int r;
	char *create1;
	char *create2;
	char *create3;
	char *create4;
	char *create5;
	char *create6;
	char *create7;
	char *create8;
	char *create9;
	char *create10;

	r = sqlite3_open(argv[2], &db);
	if(r)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return 0;
	}

	char* wee;
	sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, &wee);
	sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &wee);

	create1 = "CREATE TABLE BLOCK("  \
		"ID INT PRIMARY KEY		NOT NULL," \
		"PREV           CHAR(64)	NOT NULL," \
		"MERKLE         CHAR(64)	NOT NULL," \
		"HASH		CHAR(64)	NOT NULL," \
		"NUM		INT		," \
		"BITS		INT		NOT NULL," \
		"NONCE		INT		NOT NULL," \
		"TIME		INT		NOT NULL);";

	create2 = "CREATE TABLE TX(" \
		"ID INT PRIMARY KEY		NOT NULL," \
		"HASH		CHAR(64)	NOT NULL," \
		"FILE		TEXT		NOT NULL," \
		"OFFSET		INT		NOT NULL);";

	create3 = "CREATE TABLE TX_INPUT(" \
		"ID INT PRIMARY KEY		NOT NULL," \
		"TX		INT		NOT NULL," \
		"PREV		CHAR(64)	NOT NULL," \
		"DATA		BLOB		NOT NULL," \
		"IDX		INT		," \
		"SEQ		INT		);";

	create4 = "CREATE TABLE TX_OUTPUT(" \
		"ID INT PRIMARY KEY		NOT NULL," \
		"TX		INT		NOT NULL," \
		"DATA		BLOB		NOT NULL," \
		"ADDR		TEXT		," \
		"VAL		BIGINT		);";

	create5 = "CREATE TABLE FILE(" \
                "ID INT PRIMARY KEY             NOT NULL," \
                "NAME           FILE            NOT NULL);";

	create6 = "CREATE TABLE REL_BLOCK_TX(" \
		"ID INT PRIMARY KEY		NOT NULL," \
		"BLOCK		INT		NOT NULL," \
		"TX		INT		NOT NULL);";

	create7 = "CREATE TABLE REL_TX_INPUT(" \
		"ID INT PRIMARY KEY		NOT NULL," \
		"TX		INT		NOT NULL," \
		"INPUT		INT		NOT NULL);";

	create8 = "CREATE TABLE REL_TX_OUTPUT(" \
		"ID INT PRIMARY KEY		NOT NULL," \
		"TX		INT		NOT NULL," \
		"OUTPUT		INT		NOT NULL);";

	create9 = "CREATE TABLE REL_BLOCK_NEXT(" \
		"ID INT PRIMARY KEY		NOT NULL," \
		"BLOCK		INT		NOT NULL," \
		"NEXT		INT		NOT NULL);";

	create10 = "CREATE TABLE REL_BLOCK_PREV(" \
		"ID INT PRIMARY KEY		NOT NULL," \
		"BLOCK		INT		NOT NULL," \
		"PREV		INT		NOT NULL);";

	printf("Creating database...\n");
	if(sql_query(create1) < 0) return -1;
	if(sql_query(create2) < 0) return -1;
	if(sql_query(create3) < 0) return -1;
	if(sql_query(create4) < 0) return -1;
	if(sql_query(create5) < 0) return -1;
	if(sql_query(create6) < 0) return -1;
	if(sql_query(create7) < 0) return -1;
	if(sql_query(create8) < 0) return -1;
	if(sql_query(create9) < 0) return -1;
	if(sql_query(create10) < 0) return -1;

	if(sql_prepare() < 0)
	{
		printf("Prepare failed.\n");
                return -1;
	}
	blk_cnt = 0;
	file_cnt = 0;
        tx_cnt = 0;
        rel_blk_tx_cnt = 0;
        txin_cnt = 0;
        rel_tx_input_cnt = 0;
        txout_cnt = 0;
        rel_tx_output_cnt = 0;

	DIR *dir;
	struct dirent *ent;
	if((dir = opendir(argv[1])) != NULL) 
	{
		while((ent = readdir(dir)) != NULL)
		{
			if(strstr(ent->d_name,".dat") && strstr(ent->d_name,"blk"))
			{
				char filebuf[1024];
				memset(filebuf,0,1024);
				snprintf(filebuf,1024,"%s/%s",argv[1],ent->d_name);
				if(read_block_file(filebuf) < 0)
					return -1;
			}
		}
		closedir(dir);
	}
	else
	{
		printf("Error opening blockchain data directory.\n");
		return -1;
	}

	sqlite3_close(db);
	return 0;
}

//In is 64 byte pub key, out is 20 byte digest
void ecdsa2ripe(uint8_t* in,uint8_t* out)
{
        uint8_t inkey[65];
        inkey[0] = 0x04;
        memcpy(&inkey[1],in,64);

        SHA256_CTX ctx;
        uint8_t sha_hash[32];
        memset(sha_hash,0,32);

        sha256_init(&ctx);
        sha256_update(&ctx, inkey, 65);
        sha256_final(&ctx, sha_hash);

        ripemd160(sha_hash,32,out);
}

//Take 20 byte digest, outputs 25 bytes address
void ripe2addr(uint8_t* in,char* out)
{
        uint8_t inkey[21];
        inkey[0] = 0x00;
        memcpy(&inkey[1],in,20);

        SHA256_CTX ctx;
        uint8_t sha_hash[32];
        uint8_t sha_final[32];
        memset(sha_hash,0,32);
        sha256_init(&ctx);
        sha256_update(&ctx, inkey, sizeof(inkey));
        sha256_final(&ctx, sha_hash);
        sha256_init(&ctx);
        sha256_update(&ctx, sha_hash, sizeof(sha_hash));
        sha256_final(&ctx,sha_final);
        memcpy(out,inkey,21);
        memcpy(out+21,sha_final,4);
}

static const char* b58sym = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

uint8_t b58enc(char *b58, size_t *b58sz, const void *data, size_t binsz)
{
        const uint8_t *bin = data;
        int carry;
        ssize_t i, j, high, zcount = 0;
        size_t size;

        while (zcount < binsz && !bin[zcount])
                ++zcount;

        size = (binsz - zcount) * 138 / 100 + 1;
        uint8_t buf[size];
        memset(buf, 0, size);

        for (i = zcount, high = size - 1; i < binsz; ++i, high = j)
        {
                for (carry = bin[i], j = size - 1; (j > high) || carry; --j)
                {
                        carry += 256 * buf[j];
                        buf[j] = carry % 58;
                        carry /= 58;
                }
        }

        for (j = 0; j < size && !buf[j]; ++j);

        if (*b58sz <= zcount + size - j)
        {
                *b58sz = zcount + size - j + 1;
                return 0;
        }

        if (zcount)
                memset(b58, '1', zcount);
        for (i = zcount; j < size; ++i, ++j)
                b58[i] = b58sym[buf[j]];
        b58[i] = '\0';
        *b58sz = i + 1;

        return 1;
}

char* addr_from_script(uint8_t* data,uint64_t len)
{
	uint8_t addrhash[25];
	size_t addrlen;
	char* ret = calloc(1,128); //overkill but whatever.
	if(!ret)
		return NULL;
	if(len == 67) //format 1
	{
		uint8_t* hash = &data[1];
		uint8_t ripe[20];
		ecdsa2ripe(hash,ripe);
		ripe2addr(ripe,addrhash);
                b58enc(ret, &addrlen, addrhash, 25);
		return ret;
	}
	else if(len == 66) //format 2
	{
		uint8_t* hash = &data[0];
		uint8_t ripe[20];
		ecdsa2ripe(hash,ripe);
		ripe2addr(ripe,addrhash);
                b58enc(ret, &addrlen, addrhash, 25);
		return ret;
	}
	else if(len >= 25) //format 3,4,5
	{
		uint64_t i;
		for(i=0;i<len;i++)
		{
			if(data[i] == 0xA9) //OP_HASH160
			{
				if(len-i-1 >= 21) //hash + len
				{
					uint8_t* hash = &data[i+2];
					ripe2addr(hash,addrhash);
					b58enc(ret, &addrlen, addrhash, 25);
					return ret;
				}
				else
				{
					strcpy(ret,"STRANGE");
					return ret;
				}
			}
		}
	}
	strcpy(ret,"STRANGE");
	return ret;
}

int32_t read_block_file(const char* filename)
{
	int32_t fd = open(filename,O_RDONLY);
        if(fd < 0)
        {
                fprintf(stderr,"open: %s\n",strerror(errno));
                return -1;
        }
        struct stat st;
        fstat(fd,&st);
        int32_t file_size = st.st_size;
        uint8_t* file_data = malloc(file_size);
        if(!file_data)
        {
                fprintf(stderr,"Memory allocation failed.\n");
                return -1;
        }
        int32_t rbytes = 0;
        while(rbytes < file_size)
        {
		printf("blk %u tx %u (%u/%u) %s --READING--\n            ", blk_cnt, tx_cnt, txin_cnt, txout_cnt, filename);
                int32_t n = read(fd,file_data+rbytes,file_size-rbytes);
                if(n < 0)
                {
                        fprintf(stderr,"read: %s\n",strerror(errno));
                        return -1;
                }
                rbytes += n;
        }
	char* errmsg;

	if(sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &errmsg) < 0)
                {
                        printf("SQL Error: %s\n",errmsg);
                        return -1;
                }

	uint32_t file_id = file_cnt++;

	if(sql_insert_file(file_id,filename) < 0)
	{
		printf("ERROR INSERTING FILE %u\n",file_id);
		return -1;
	}

        uint8_t* filep = file_data;
        while(filep != (file_data+file_size))
        {
                blk_t* blk = alloc_blk();
                read_blk(&filep,blk);

                char prev[65];
                char merkle[65];
                char hash[65];
                get_sha(prev,blk->blk_prev_sha);
                get_sha(merkle,blk->blk_merkle_sha);
                get_sha(hash,blk->blk_hash);

                uint32_t blk_id = blk_cnt++;

		if(sql_insert_block(blk_id,prev,merkle,hash,blk->blk_bits,blk->blk_nonce,blk->blk_ts) < 0)
		{
			printf("ERROR INSERTING BLOCK %u\n",blk_id);
			return -1;
		}

                uint64_t i;
                for(i=0;i<blk->blk_tx_cnt;i++)
                {
                        tx_t* tx = blk->blk_tx[i];
                        char tx_hash[65];
                        get_sha(tx_hash,tx->tx_hash);
                        uint32_t tx_id = tx_cnt++;

			if(sql_insert_tx(tx_id, tx_hash, file_id, filep - file_data) < 0)
			{
				printf("ERROR INSERTING TX %u\n",tx_id);
				return -1;
			}

                        uint32_t rel_blk_tx_id = rel_blk_tx_cnt++;
			if(sql_insert_rel_blk_tx(rel_blk_tx_id,blk_id,tx_id) < 0)
			{
				printf("ERROR INSERTING REL_BLOCK_TX %u\n",rel_blk_tx_id);
				return -1;
			}

                        uint64_t j;
                        for(j=0;j<tx->tx_input_cnt;j++)
                        {
                                txin_t* txin = tx->tx_input[j];
                                char txin_prev[65];
                                get_sha(txin_prev,txin->txi_prev_out);
                                uint32_t txin_id = txin_cnt++;

				if(sql_insert_tx_input(txin_id,tx_id,txin_prev,txin->txi_idx,txin->txi_seq,txin->txi_script_data,txin->txi_script_len) < 0)
				{
					printf("ERROR INSERTING TX_INPUT %u\n",txin_id);
					printf("tx: %u idx: %u seq: %u prev: '%s'\n",txin_id,txin->txi_idx,txin->txi_seq,txin_prev);
					return -1;
				}

                                uint32_t rel_tx_input_id = rel_tx_input_cnt++;
				if(sql_insert_rel_tx_input(rel_tx_input_id,tx_id,txin_id) < 0)
				{
					printf("ERROR INSERTING REL_TX_INPUT %u\n",rel_tx_input_id);
					return -1;
				}
                        }
                        for(j=0;j<tx->tx_output_cnt;j++)
                        {
                                txout_t* txout = tx->tx_output[j];
                                uint32_t txout_id = txout_cnt++;

				char* addr = addr_from_script(txout->txo_script_data,txout->txo_script_len);

				if(sql_insert_tx_output(txout_id,tx_id,txout->txo_val,txout->txo_script_data,txout->txo_script_len,addr) < 0)
				{
					printf("ERROR INSERTING TX_OUTPUT %u\n",txout_id);
					return -1;
				}
				free(addr);

                                uint32_t rel_tx_output_id = rel_tx_output_cnt++;
				if(sql_insert_rel_tx_output(rel_tx_output_id,tx_id,txout_id) < 0)
				{
					printf("ERROR INSERTING REL_TX_OUTPUT %u\n",rel_tx_output_id);
					return -1;
				}
                        }
                }

		printf("blk %u tx %u (%u/%u) %s (%.2f%%)     \r",blk_cnt,tx_cnt,txin_cnt,txout_cnt,filename,(float)((float)(filep - file_data) / (float)file_size) * 100.0f);
		fflush(stdout);
                free_blk(blk);
        }
	if(sqlite3_exec(db, "COMMIT TRANSACTION", NULL, NULL, &errmsg) < 0)
	{
		printf("SQL Error: %s\n",errmsg);
		return -1;
	}
	free(file_data);
        return 0;
}
