﻿//lhttp.c
#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include<stdarg.h>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <time.h>

// enums
//---------------------------------------------------------------------
// token枚举
enum hs_token {
    HS_TOK_NONE, HS_TOK_METHOD, HS_TOK_TARGET, HS_TOK_VERSION,
    HS_TOK_HEADER_KEY, HS_TOK_HEADER_VAL, HS_TOK_CHUNK_BODY, HS_TOK_BODY,
    HS_TOK_BODY_STREAM, HS_TOK_REQ_END, HS_TOK_EOF, HS_TOK_ERROR
};

// 词法分析中的状态
enum hs_state {
    ST, MT, MS, TR, TS, VN, RR, RN, HK, HS, HV, HR, HE,
    ER, HN, BD, CS, CB, CE, CR, CN, CD, C1, C2, BR, HS_STATE_LEN
};

// 字符类型
enum hs_char_type {
    /*空格    LF     CR     冒号    TAB      分号     */
    HS_SPC, HS_NL, HS_CR, HS_COLN, HS_TAB, HS_SCOLN,
    /*0-9      A-Fa-f  g-zG-Z     符号  []{}()/\<=>?@" 其他 */
    HS_DIGIT, HS_HEX, HS_ALPHA, HS_TCHAR, HS_VCHAR, HS_ETC, HS_CHAR_TYPE_LEN
};

// 元数据状态
enum hs_meta_state {
    M_WFK, M_ANY, M_MTE, M_MCL, M_CLV, M_MCK, M_SML, M_CHK, M_BIG, M_ZER, M_CSZ,
    M_CBD, M_LST, M_STR, M_SEN, M_BDY, M_END, M_ERR
};

// 元数据类型
enum hs_meta_type {
    HS_META_NOT_CONTENT_LEN, HS_META_NOT_TRANSFER_ENC, HS_META_END_KEY,
    HS_META_END_VALUE, HS_META_END_HEADERS, HS_META_LARGE_BODY,
    HS_META_TYPE_LEN
};

// constants
//-----------------------------------------------------------------
#define HTTP_CLOSE      0
#define HTTP_KEEP_ALIVE 1

// http session states
#define HTTP_REQUEST_PARSE      0
#define HTTP_SESSION_FINISH     1
#define HTTP_REQUEST_ERROR      2

// http session flags
#define HTTP_END_SESSION        0x2
#define HTTP_AUTOMATIC          0x8
#define HTTP_CHUNKED_RESPONSE   0x20

// Application configurable
#define HTTP_REQUEST_BUF_SIZE           1024
#define HTTP_RESPONSE_BUF_SIZE          1024
#define HTTP_REQUEST_TIMEOUT            20
#define HTTP_KEEP_ALIVE_TIMEOUT         120
#define HTTP_MAX_HEADER_COUNT           127
#define HTTP_MAX_TOKEN_LENGTH           8192        // 8kb
#define HTTP_MAX_REQUEST_BUF_SIZE       8388608     // 8mb
#define HTTP_MAX_TOTAL_EST_MEM_USAGE    4294967296  // 4gb

#define HS_META_NEXT            0
#define HS_META_NON_ZERO        0
#define HS_META_NOT_CHUNKED     0
#define HS_META_END_CHK_SIZE    1
#define HS_META_END_CHUNK       2

// stream flags
#define HS_SF_CONSUMED          0x1

// parser flags
#define HS_PF_IN_CONTENT_LEN    0x1
#define HS_PF_IN_TRANSFER_ENC   0x2
#define HS_PF_CHUNKED           0x4
#define HS_PF_CKEND             0x8
#define HS_PF_REQ_END           0x10

// 当请求正文被分块或正文太大而无法放入内存时，将设置此标志。这意味着必须使用http_request_read_chunk函数逐段读取正文。
#define HTTP_FLG_CHUNK          0x1

#define HTTP_FLAG_SET(var, flag)    var |= flag
#define HTTP_FLAG_CLEAR(var, flag)  var &= ~flag
#define HTTP_FLAG_CHECK(var, flag)  (var & flag)

// 分析过程中状态对应字符转换状态
static int const http_transitions[] = {
    //                                            A-Z G-Z
    //                spc \n  \r  :   \t  ;   0-9 a-f g-z tch vch etc
    /* ST start */    BR, BR, BR, BR, BR, BR, BR, MT, MT, MT, BR, BR,
    /* MT method */   MS, BR, BR, BR, BR, BR, MT, MT, MT, MT, BR, BR,
    /* MS methodsp */ BR, BR, BR, BR, BR, BR, TR, TR, TR, TR, TR, BR,
    /* TR target */   TS, BR, BR, TR, BR, TR, TR, TR, TR, TR, TR, BR,
    /* TS targetsp */ BR, BR, BR, BR, BR, BR, VN, VN, VN, VN, VN, BR,
    /* VN version */  BR, BR, RR, BR, BR, BR, VN, VN, VN, VN, VN, BR,
    /* RR rl \r */    BR, RN, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR,
    /* RN rl \n */    BR, BR, BR, BR, BR, BR, HK, HK, HK, HK, BR, BR,
    /* HK headkey */  BR, BR, BR, HS, BR, BR, HK, HK, HK, HK, BR, BR,
    /* HS headspc */  HS, HS, HS, HV, HS, HV, HV, HV, HV, HV, HV, BR,
    /* HV headval */  HV, BR, HR, HV, HV, HV, HV, HV, HV, HV, HV, BR,
    /* HR head\r */   BR, HE, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR,
    /* HE head\n */   BR, BR, ER, BR, BR, BR, HK, HK, HK, HK, BR, BR,
    /* ER hend\r */   BR, HN, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR,
    /* HN hend\n */   BD, BD, BD, BD, BD, BD, BD, BD, BD, BD, BD, BD,
    /* BD body */     BD, BD, BD, BD, BD, BD, BD, BD, BD, BD, BD, BD,
    /* CS chksz */    BR, BR, CR, BR, BR, CE, CS, CS, BR, BR, BR, BR,
    /* CB chkbd */    CB, CB, CB, CB, CB, CB, CB, CB, CB, CB, CB, CB,
    /* CE chkext */   BR, BR, CR, CE, CE, CE, CE, CE, CE, CE, CE, BR,
    /* CR chksz\r */  BR, CN, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR,
    /* CN chksz\n */  CB, CB, CB, CB, CB, CB, CB, CB, CB, CB, CB, CB,
    /* CD chkend */   BR, BR, C1, BR, BR, BR, BR, BR, BR, BR, BR, BR,
    /* C1 chkend\r */ BR, C2, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR,
    /* C2 chkend\n */ BR, BR, BR, BR, BR, BR, CS, CS, BR, BR, BR, BR
};

static int const http_meta_transitions[] = {
    //                 no chk
    //                 not cl not te endkey endval end h  toobig
    /* WFK wait */     M_WFK, M_WFK, M_WFK, M_ANY, M_END, M_ERR,
    /* ANY matchkey */ M_MTE, M_MCL, M_WFK, M_ERR, M_END, M_ERR,
    /* MTE matchte */  M_MTE, M_WFK, M_MCK, M_ERR, M_ERR, M_ERR,
    /* MCL matchcl */  M_WFK, M_MCL, M_CLV, M_ERR, M_ERR, M_ERR,
    /* CLV clvalue */  M_ERR, M_ERR, M_ERR, M_SML, M_ERR, M_ERR,
    /* MCK matchchk */ M_WFK, M_ERR, M_ERR, M_CHK, M_ERR, M_ERR,
    /* SML smallbdy */ M_SML, M_SML, M_SML, M_SML, M_BDY, M_BIG,
    /* CHK chunkbdy */ M_CHK, M_CHK, M_CHK, M_CHK, M_ZER, M_ERR,
    /* BIG bigbody */  M_BIG, M_BIG, M_BIG, M_BIG, M_STR, M_ERR,

    //                         *** chunked body ***

    //                 nonzer endsz  endchk
    /* ZER zerochk */  M_CSZ, M_LST, M_ERR, M_ERR, M_ERR, M_ERR,
    /* CSZ chksize */  M_CSZ, M_CBD, M_ERR, M_ERR, M_ERR, M_ERR,
    /* CBD readchk */  M_CBD, M_CBD, M_ZER, M_ERR, M_ERR, M_ERR,
    /* LST lastchk */  M_LST, M_END, M_END, M_ERR, M_ERR, M_ERR,

    //                         *** streamed body ***

    //                 next
    /* STR readstr */  M_SEN, M_ERR, M_ERR, M_ERR, M_ERR, M_ERR,
    /* SEN strend */   M_END, M_ERR, M_ERR, M_ERR, M_ERR, M_ERR,

    //                         *** small body ***

    //                 next
    /* BDY readbody */ M_END, M_ERR, M_ERR, M_ERR, M_ERR, M_ERR,
    /* END reqend */   M_WFK, M_ERR, M_ERR, M_ERR, M_ERR, M_ERR
};

// 将ASCII码隐射到ctype
static int const hs_ctype[] = {
  HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,
  HS_ETC,   HS_ETC,   HS_TAB,   HS_NL,    HS_ETC,   HS_ETC,   HS_CR,
  HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,
  HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,
  HS_ETC,   HS_ETC,   HS_ETC,   HS_ETC,   HS_SPC,   HS_TCHAR, HS_VCHAR,
  HS_TCHAR, HS_TCHAR, HS_TCHAR, HS_TCHAR, HS_TCHAR, HS_VCHAR, HS_VCHAR,
  HS_TCHAR, HS_TCHAR, HS_TCHAR, HS_TCHAR, HS_TCHAR, HS_VCHAR, HS_DIGIT,
  HS_DIGIT, HS_DIGIT, HS_DIGIT, HS_DIGIT, HS_DIGIT, HS_DIGIT, HS_DIGIT,
  HS_DIGIT, HS_DIGIT, HS_COLN,  HS_SCOLN, HS_VCHAR, HS_VCHAR, HS_VCHAR,
  HS_VCHAR, HS_VCHAR, HS_HEX,   HS_HEX,   HS_HEX,   HS_HEX,   HS_HEX,
  HS_HEX,   HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA,
  HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA,
  HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA,
  HS_VCHAR, HS_VCHAR, HS_VCHAR, HS_TCHAR, HS_TCHAR, HS_TCHAR, HS_HEX,
  HS_HEX,   HS_HEX,   HS_HEX,   HS_HEX,   HS_HEX,   HS_ALPHA, HS_ALPHA,
  HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA,
  HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA,
  HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_ALPHA, HS_VCHAR, HS_TCHAR, HS_VCHAR,
  HS_TCHAR, HS_ETC
};

static int const http_token_start_states[] = {
    //ST MT           MS TR             TS VN              RR RN HK
    0, HS_TOK_METHOD, 0, HS_TOK_TARGET, 0, HS_TOK_VERSION, 0, 0, HS_TOK_HEADER_KEY,
    //HS HV               HR HE ER HN  BD           CS CB                 CE CR CN
    0, HS_TOK_HEADER_VAL, 0, 0, 0, 0,  HS_TOK_BODY, 0, HS_TOK_CHUNK_BODY, 0, 0, 0,
    //CD C1 C2
    0, 0, 0,
};

char const* http_status_text[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //100s
  "Continue", "Switching Protocols", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //200s
  "OK", "Created", "Accepted", "Non-Authoritative Information", "No Content",
  "Reset Content", "Partial Content", "", "", "",

  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //300s
  "Multiple Choices", "Moved Permanently", "Found", "See Other", "Not Modified",
  "Use Proxy", "", "Temporary Redirect", "", "",

  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //400s
  "Bad Request", "Unauthorized", "Payment Required", "Forbidden", "Not Found",
  "Method Not Allowed", "Not Acceptable", "Proxy Authentication Required",
  "Request Timeout", "Conflict",

  "Gone", "Length Required", "", "Payload Too Large", "", "", "", "", "", "",

  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //500s
  "Internal Server Error", "Not Implemented", "Bad Gateway", "Service Unavailable",
  "Gateway Timeout", "", "", "", "", ""

  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", ""
};

// structs
//-----------------------------------------------------------------
// 用于读取请求详细信息的字符串类型
struct http_string_s {
    char const* buf;
    int len;
};
typedef struct http_string_s http_string_t;

typedef struct {
    int index;
    int len;
    int type;
} http_token_t;

typedef struct {
    http_token_t* buf;
    int capacity;
    int size;
} http_token_dyn_t;

typedef struct {
    char* buf;
    int64_t total_bytes;
    int32_t capacity;
    int32_t length;
    int32_t index;
    int32_t anchor;
    http_token_t token;
    uint8_t flags;
} hs_stream_t;

//http_parser_t
typedef struct {
    int64_t content_length;
    int64_t body_consumed;
    int16_t match_index;
    int16_t header_count;
    int8_t state;
    int8_t meta;
} http_parser_t;

typedef struct http_header_s {
    char const* key;
    char const* value;
    struct http_header_s* next;
} http_header_t;

typedef struct http_response_s {
    int status;
    int content_length;
    char const* body;
    http_header_t* headers;
} http_response_t;

typedef struct http_request_s {
    char flags;
    int state;
    int timeout;
    hs_stream_t stream;
    http_parser_t parser;
    http_token_dyn_t tokens;
} http_request_t;

//http utils
//----------------------------------------------------------------
int http_char_case_cmp(char const* a, char const* b, int len) {
    for (int i = 0; i < len; i++) {
        char c1 = a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i];
        char c2 = b[i] >= 'A' && b[i] <= 'Z' ? b[i] + 32 : b[i];
        if (c1 != c2) return 0;
    }
    return 1;
}

void http_generate_datetime(char* datetime) {
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(datetime, 32, "%a, %d %b %Y %T GMT", timeinfo);
}

void http_token_dyn_push(http_token_dyn_t* dyn, http_token_t a) {
    if (dyn->size == dyn->capacity) {
        dyn->capacity *= 2;
        dyn->buf = (http_token_t*)realloc(dyn->buf, dyn->capacity * sizeof(http_token_t));
        assert(dyn->buf != NULL);
    }
    dyn->buf[dyn->size] = a;
    dyn->size++;
}

void http_token_dyn_init(http_token_dyn_t* dyn, int capacity) {
    dyn->buf = (http_token_t*)malloc(sizeof(http_token_t) * capacity);
    assert(dyn->buf != NULL);
    dyn->size = 0;
    dyn->capacity = capacity;
}

//http stream
//----------------------------------------------------------------
int http_stream_append(hs_stream_t* stream, const char* buf, int len) {
    if (!stream->buf) {
        stream->buf = (char*)calloc(1, HTTP_REQUEST_BUF_SIZE);
        assert(stream->buf != NULL);
        stream->capacity = HTTP_REQUEST_BUF_SIZE;
    }
    if (stream->length + len >= stream->capacity && stream->capacity < HTTP_MAX_REQUEST_BUF_SIZE) {
        stream->capacity *= 2;
        if (stream->capacity > HTTP_MAX_REQUEST_BUF_SIZE) {
            stream->capacity = HTTP_MAX_REQUEST_BUF_SIZE;
        }
        stream->buf = (char*)realloc(stream->buf, stream->capacity);
        assert(stream->buf != NULL);
    }
    memcpy(stream->buf + stream->length, buf, len);
    stream->length += len;
    return len;
}

void http_stream_begin_token(hs_stream_t* stream, int token_type) {
    stream->token.index = stream->index;
    stream->token.type = token_type;
}

int http_stream_can_contain(hs_stream_t* stream, int64_t size) {
    return HTTP_MAX_REQUEST_BUF_SIZE - stream->index + 1 >= size;
}

int http_stream_next(hs_stream_t* stream, char* c) {
    HTTP_FLAG_CLEAR(stream->flags, HS_SF_CONSUMED);
    if (stream->index >= stream->length) return 0;
    *c = stream->buf[stream->index];
    return 1;
}

void http_stream_consume(hs_stream_t* stream) {
    if (HTTP_FLAG_CHECK(stream->flags, HS_SF_CONSUMED)) return;
    HTTP_FLAG_SET(stream->flags, HS_SF_CONSUMED);
    stream->index++;
    int new_len = stream->token.len + 1;
    stream->token.len = stream->token.type == 0 ? 0 : new_len;
}

void http_stream_shift(hs_stream_t* stream) {
    if (stream->token.index == stream->anchor) return;
    if (stream->token.len > 0) {
        char* dst = stream->buf + stream->anchor;
        char const* src = stream->buf + stream->token.index;
        int bytes = stream->length - stream->token.index;
        memcpy(dst, src, bytes);
    }
    stream->token.index = stream->anchor;
    stream->index = stream->token.len + stream->anchor;
    stream->length = stream->anchor + stream->token.len;
}

int http_stream_jump(hs_stream_t* stream, int offset) {
    HTTP_FLAG_SET(stream->flags, HS_SF_CONSUMED);
    if (stream->index + offset > stream->length) return 0;
    stream->index += offset;
    int new_len = stream->token.len + offset;
    stream->token.len = stream->token.type == 0 ? 0 : new_len;
    return 1;
}

int http_stream_jumpall(hs_stream_t* stream) {
    int offset = stream->length - stream->index;
    stream->index += offset;
    int new_len = stream->token.len + offset;
    HTTP_FLAG_SET(stream->flags, HS_SF_CONSUMED);
    stream->token.len = stream->token.type == 0 ? 0 : new_len;
    return offset;
}

http_token_t http_stream_emit(hs_stream_t* stream) {
    http_token_t token = stream->token;
    http_token_t none = { 0, 0, 0 };
    stream->token = none;
    return token;
}

//http parser
//----------------------------------------------------------------
void http_trigger_meta(http_parser_t* parser, int event) {
    int to = http_meta_transitions[parser->meta * HS_META_TYPE_LEN + event];
    parser->meta = to;
}

#define HS_MATCH(str, meta) \
    in_bounds = parser->match_index < (int)sizeof(str) - 1; \
    m = in_bounds ? str[parser->match_index] : m; \
    low = c >= 'A' && c <= 'Z' ? c + 32 : c; \
    if (low != m) http_trigger_meta(parser, meta);

http_token_t http_transition_action(http_parser_t* parser, hs_stream_t* stream, char c, int8_t from, int8_t to) {
    http_token_t emitted = { 0, 0, 0 };
    if (from == HN) {
        stream->anchor = stream->index;
    }
    if (from != to) {
        int type = http_token_start_states[to];
        if (type != HS_TOK_NONE) http_stream_begin_token(stream, type);
        if (from == CS) http_trigger_meta(parser, HS_META_END_CHK_SIZE);
        if (to == HK) {
            parser->header_count++;
            if (parser->header_count > HTTP_MAX_HEADER_COUNT) {
                emitted.type = HS_TOK_ERROR;
            }
        }
        else if (to == HS) {
            http_trigger_meta(parser, HS_META_END_KEY);
            emitted = http_stream_emit(stream);
        }
        parser->match_index = 0;
    }
    char low, m = '\0';
    int in_bounds = 0;
    int body_left = 0;
    switch (to) {
    case MS:
    case TS:
        emitted = http_stream_emit(stream);
        break;
    case RR:
    case HR:
        http_trigger_meta(parser, HS_META_END_VALUE);
        emitted = http_stream_emit(stream);
        break;
    case HK:
        HS_MATCH("transfer-encoding", HS_META_NOT_TRANSFER_ENC)
            HS_MATCH("content-length", HS_META_NOT_CONTENT_LEN)
            parser->match_index++;
        break;
    case HV:
        if (parser->meta == M_MCK) {
            HS_MATCH("chunked", HS_META_NOT_CHUNKED)
                parser->match_index++;
        }
        else if (parser->meta == M_CLV) {
            parser->content_length *= 10;
            parser->content_length += c - '0';
        }
        break;
    case HN:
        if (parser->meta == M_SML && !http_stream_can_contain(stream, parser->content_length)) {
            http_trigger_meta(parser, HS_META_LARGE_BODY);
        }
        if (parser->meta == M_BIG || parser->meta == M_CHK) {
            emitted.type = HS_TOK_BODY_STREAM;
        }
        //if (parser->meta == M_CHK) parser->state = CS;
        http_trigger_meta(parser, HS_META_END_HEADERS);
        if (parser->content_length == 0 && parser->meta == M_BDY) {
            parser->meta = M_END;
        }
        if (parser->meta == M_END) {
            emitted.type = HS_TOK_BODY;
        }
        break;
    case CS:
        if (c != '0') http_trigger_meta(parser, HS_META_NON_ZERO);
        if (c >= 'A' && c <= 'F') {
            parser->content_length *= 0x10;
            parser->content_length += c - 55;
        }
        else if (c >= 'a' && c <= 'f') {
            parser->content_length *= 0x10;
            parser->content_length += c - 87;
        }
        else if (c >= '0' && c <= '9') {
            parser->content_length *= 0x10;
            parser->content_length += c - '0';
        }
        break;
    case CB:
    case BD:
        if (parser->meta == M_STR) http_stream_begin_token(stream, HS_TOK_CHUNK_BODY);
        body_left = parser->content_length - parser->body_consumed;
        if (http_stream_jump(stream, body_left)) {
            emitted = http_stream_emit(stream);
            http_trigger_meta(parser, HS_META_NEXT);
            if (to == CB) parser->state = CD;
            parser->content_length = 0;
            parser->body_consumed = 0;
        }
        else {
            parser->body_consumed += http_stream_jumpall(stream);
            if (parser->meta == M_STR) {
                emitted = http_stream_emit(stream);
                http_stream_shift(stream);
            }
        }
        break;
    case C2:
        http_trigger_meta(parser, HS_META_END_CHUNK);
        break;
    case BR:
        emitted.type = HS_TOK_ERROR;
        break;
    }
    return emitted;
}

http_token_t http_meta_emit(http_parser_t* parser) {
    http_token_t token = { 0, 0, 0 };
    switch (parser->meta) {
    case M_SEN:
        token.type = HS_TOK_CHUNK_BODY;
        http_trigger_meta(parser, HS_META_NEXT);
        break;
    case M_END:
        token.type = HS_TOK_REQ_END;
        memset(parser, 0, sizeof(http_parser_t));
        break;
    }
    return token;
}

http_token_t http_parse(http_parser_t* parser, hs_stream_t* stream) {
    char c = 0;
    http_token_t token = http_meta_emit(parser);
    if (token.type != HS_TOK_NONE) return token;
    while (http_stream_next(stream, &c)) {
        int type = c < 0 ? HS_ETC : hs_ctype[(int)c];
        int to = http_transitions[parser->state * HS_CHAR_TYPE_LEN + type];
        if (parser->meta == M_ZER && parser->state == HN && to == BD) {
            to = CS;
        }
        int from = parser->state;
        parser->state = to;
        http_token_t emitted = http_transition_action(parser, stream, c, from, to);
        http_stream_consume(stream);
        if (emitted.type != HS_TOK_NONE) return emitted;
    }
    if (parser->state == CB) http_stream_shift(stream);
    token = http_meta_emit(parser);
    http_token_t current = stream->token;
    if (current.type != HS_TOK_CHUNK_BODY && current.type != HS_TOK_BODY && current.len > HTTP_MAX_TOKEN_LENGTH) {
        token.type = HS_TOK_ERROR;
    }
    return token;
}

//http request 接口
//--------------------------------------------------------------------------
http_string_t http_get_token_string(http_request_t* request, int token_type) {
    http_string_t str = { 0, 0 };
    if (request->tokens.buf == NULL) return str;
    for (int i = 0; i < request->tokens.size; i++) {
        http_token_t token = request->tokens.buf[i];
        if (token.type == token_type) {
            str.buf = &request->stream.buf[token.index];
            str.len = token.len;
            return str;
        }
    }
    return str;
}

int http_request_has_flag(http_request_t* request, int flag) {
    return HTTP_FLAG_CHECK(request->flags, flag);
}

http_string_t http_request_method(http_request_t* request) {
    return http_get_token_string(request, HS_TOK_METHOD);
}

http_string_t http_request_target(http_request_t* request) {
    return http_get_token_string(request, HS_TOK_TARGET);
}

http_string_t http_request_body(http_request_t* request) {
    return http_get_token_string(request, HS_TOK_BODY);
}

int http_headers_iterator(http_request_t* request, http_string_t* key, http_string_t* val, int* iter) {
    http_token_t token = request->tokens.buf[*iter];
    if (token.type == HS_TOK_BODY) return 0;
    *key = (http_string_t){
        .buf = &request->stream.buf[token.index],
        .len = token.len
    };
    (*iter)++;
    token = request->tokens.buf[*iter];
    *val = (http_string_t){
        .buf = &request->stream.buf[token.index],
        .len = token.len
    };
    return 1;
}

int http_request_headers_iterator(http_request_t* request, http_string_t* key, http_string_t* val, int* iter) {
    if (*iter == 0) {
        for (; *iter < request->tokens.size; (*iter)++) {
            http_token_t token = request->tokens.buf[*iter];
            if (token.type == HS_TOK_HEADER_KEY) {
                return http_headers_iterator(request, key, val, iter);
            }
        }
        return 0;
    }
    else {
        (*iter)++;
        return http_headers_iterator(request, key, val, iter);
    }
}

http_string_t http_request_header(http_request_t* request, char const* key) {
    int len = strlen(key);
    for (int i = 0; i < request->tokens.size; i++) {
        http_token_t token = request->tokens.buf[i];
        if (token.type == HS_TOK_HEADER_KEY && token.len == len) {
            if (http_char_case_cmp(&request->stream.buf[token.index], key, len)) {
                token = request->tokens.buf[i + 1];
                return (http_string_t) {
                    .buf = &request->stream.buf[token.index],
                        .len = token.len
                };
            }
        }
    }
    return (http_string_t) { 0 };
}

void http_request_free_buffer(http_request_t* session) {
    if (session->stream.buf) {
        free(session->stream.buf);
        session->stream.buf = NULL;
    }
}

http_string_t http_request_chunk(struct http_request_s* request) {
    http_token_t token = request->tokens.buf[request->tokens.size - 1];
    return (http_string_t) {
        .buf = &request->stream.buf[token.index],
            .len = token.len
    };
}

//http response 接口
//--------------------------------------------------------------------------
http_response_t* http_response_init() {
    http_response_t* response = (http_response_t*)calloc(1, sizeof(http_response_t));
    assert(response != NULL);
    response->status = 200;
    return response;
}

void http_response_header(http_response_t* response, char const* key, char const* value) {
    http_header_t* header = (http_header_t*)malloc(sizeof(http_header_t));
    assert(header != NULL);
    header->key = key;
    header->value = value;
    http_header_t* prev = response->headers;
    header->next = prev;
    response->headers = header;
}

void http_response_status(http_response_t* response, int status) {
    response->status = status > 599 || status < 100 ? 500 : status;
}

void http_response_body(http_response_t* response, char const* body, int length) {
    response->body = body;
    response->content_length = length;
}

typedef struct {
    char* buf;
    int capacity;
    int size;
} grwprintf_t;

void grwprintf_init(grwprintf_t* ctx, int capacity) {
    ctx->size = 0;
    ctx->buf = (char*)malloc(capacity);
    assert(ctx->buf != NULL);
    ctx->capacity = capacity;
}

void grwmemcpy(grwprintf_t* ctx, char const* src, int size) {
    if (ctx->size + size > ctx->capacity) {
        ctx->capacity = ctx->size + size;
        ctx->buf = (char*)realloc(ctx->buf, ctx->capacity);
        assert(ctx->buf != NULL);
    }
    memcpy(ctx->buf + ctx->size, src, size);
    ctx->size += size;
}

void grwprintf(grwprintf_t* ctx, char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int bytes = vsnprintf(ctx->buf + ctx->size, ctx->capacity - ctx->size, fmt, args);
    if (bytes + ctx->size > ctx->capacity) {
        while (bytes + ctx->size > ctx->capacity) ctx->capacity *= 2;
        ctx->buf = (char*)realloc(ctx->buf, ctx->capacity);
        assert(ctx->buf != NULL);
        bytes += vsnprintf(ctx->buf + ctx->size, ctx->capacity - ctx->size, fmt, args);
    }
    ctx->size += bytes;
    va_end(args);
}

void http_buffer_headers(http_request_t* request, http_response_t* response, grwprintf_t* printctx) {
    http_header_t* header = response->headers;
    while (header) {
        grwprintf(printctx, "%s: %s\r\n", header->key, header->value);
        header = header->next;
    }
    if (!HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
        grwprintf(printctx, "Content-Length: %d\r\n", response->content_length);
    }
    grwprintf(printctx, "\r\n");
}

void http_respond_headers(http_request_t* request, http_response_t* response, grwprintf_t* printctx) {
    if (HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE)) {
        http_response_header(response, "Connection", "keep-alive");
    }
    else {
        http_response_header(response, "Connection", "close");
    }
    char date[32];
    http_generate_datetime(date);
    grwprintf(printctx, "HTTP/1.1 %d %s\r\nDate: %s\r\n", response->status, http_status_text[response->status], date);
    http_buffer_headers(request, response, printctx);
}

void http_close_response(http_response_t* response) {
    http_header_t* header = response->headers;
    while (header) {
        http_header_t* tmp = header;
        header = tmp->next;
        free(tmp);
    }
    free(response);
}

void http_close_request(http_request_t* request) {
    http_request_free_buffer(request);
    free(request->tokens.buf);
    request->tokens.buf = NULL;
    free(request);
}

http_string_t http_respond_chunk(http_request_t* request, http_response_t* response) {
    grwprintf_t printctx;
    grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE);
    if (!HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
        HTTP_FLAG_SET(request->flags, HTTP_CHUNKED_RESPONSE);
        http_response_header(response, "Transfer-Encoding", "chunked");
        http_respond_headers(request, response, &printctx);
    }
    grwprintf(&printctx, "%X\r\n", response->content_length);
    grwmemcpy(&printctx, response->body, response->content_length);
    grwprintf(&printctx, "\r\n");
    http_close_request(request);
    http_close_response(response);
    return (http_string_t) {
        .buf = printctx.buf,
            .len = printctx.size
    };
}

http_string_t http_respond_chunk_end(http_request_t* request, http_response_t* response) {
    grwprintf_t printctx;
    grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE);
    grwprintf(&printctx, "0\r\n");
    http_buffer_headers(request, response, &printctx);
    grwprintf(&printctx, "\r\n");
    HTTP_FLAG_CLEAR(request->flags, HTTP_CHUNKED_RESPONSE);
    http_close_request(request);
    http_close_response(response);
    return (http_string_t) {
        .buf = printctx.buf,
            .len = printctx.size
    };
}

http_string_t http_respond(http_request_t* request, http_response_t* response) {
    grwprintf_t printctx;
    grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE);
    http_respond_headers(request, response, &printctx);
    if (response->body) {
        grwmemcpy(&printctx, response->body, response->content_length);
    }
    http_close_request(request);
    http_close_response(response);
    return (http_string_t) {
        .buf = printctx.buf,
            .len = printctx.size
    };
}

http_string_t http_request_response(http_request_t* request, int code, char const* type, char const* message) {
    struct http_response_s* response = http_response_init();
    http_response_status(response, code);
    http_response_header(response, "Content-Type", type);
    http_response_body(response, message, strlen(message));
    return http_respond(request, response);
}

http_request_t* http_request_init() {
    http_request_t* request = (http_request_t*)calloc(1, sizeof(http_request_t));
    request->flags = HTTP_AUTOMATIC;
    request->state = HTTP_REQUEST_PARSE;
    request->parser = (http_parser_t){ 0 };
    request->stream = (hs_stream_t){ 0 };
    if (request->tokens.buf) {
        free(request->tokens.buf);
        request->tokens.buf = NULL;
    }
    http_token_dyn_init(&request->tokens, 32);
    return request;
}

void http_process_request(http_request_t* request) {
    http_token_t token = { 0, 0, 0 };
    do {
        token = http_parse(&request->parser, &request->stream);
        if (token.type != HS_TOK_NONE) http_token_dyn_push(&request->tokens, token);
        switch (token.type) {
        case HS_TOK_ERROR:
            request->state = HTTP_REQUEST_ERROR;
            break;
        case HS_TOK_BODY:
            request->state = HTTP_SESSION_FINISH;
            break;
        case HS_TOK_BODY_STREAM:
        case HS_TOK_CHUNK_BODY:
            request->state = HTTP_SESSION_FINISH;
            HTTP_FLAG_SET(request->flags, HTTP_FLG_CHUNK);
            break;
        }
    } while (token.type != HS_TOK_NONE && request->state == HTTP_REQUEST_PARSE);
}
