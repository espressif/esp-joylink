#ifndef __JOYLINK_UTILS_H__
#define __JOYLINK_UTILS_H__
#include "joylink_log.h"

enum {
    RET_ERROR = -1,
    RET_OK    = 0
};

/**
 * brief:
 *
 * @Param: pBytes
 * @Param: srcLen
 * @Param: pDstStr
 * @Param: dstLen
 *
 * @Returns:
 */
int jl_util_reverse_buf(const uint8_t *src, uint8_t *dst, int32_t len);

int jl_util_byte2hexstr(const uint8_t *pBytes, int32_t srcLen, uint8_t *pDstStr, int32_t dstLen);


int jl_util_byte2hexcapstr(const uint8_t *pBytes, int32_t srcLen, uint8_t *pDstStr, int32_t dstLen);

/**
 * brief:
 *
 * @Param: buf
 * @Param: buf_len
 * @Param: value
 * @Param: value_len
 *
 * @Returns:
 */
int jl_util_str2byte(const uint8_t *buf, int32_t buf_len, void *value, int32_t value_len);


/**
 * brief:
 *
 * @Param: hexStr
 * @Param: buf
 * @Param: bufLen
 *
 * @Returns:
 */
int jl_util_hexStr2bytes(const char *hexStr, uint8_t *buf, int32_t bufLen);


/**
 * @name:jl_util_random_buf
 *
 * @param: dest
 * @param: size
 *
 * @returns:
 */
int32_t jl_util_random_buf(uint8_t *dest, unsigned size);

/**
 * @name:jl_util_print_buffer
 *
 * @param: msg
 * @param: is_fmt
 * @param: num_line
 * @param: buff
 * @param: len
 */
void jl_util_print_buffer(const char *msg, int32_t is_fmt, int32_t num_line, const char *buff, int32_t len);


#define JL_UTILS_P_FMT        (1)
#define JL_UTILS_P_NO_FMT     (0)

#define joylink_util_fmt_p(msg, buff, len) jl_util_print_buffer(msg, JL_UTILS_P_FMT, 20, (char *)buff, len)

/**
 * @name:joylink_get_ltv_offset
 *
 * @param: src
 * @param: length
 * @param: destag
 *
 * @returns:   -1,not find the tag;>=0,the offset of the destag block
 */
int32_t joylink_get_ltv_offset(uint8_t *src,uint8_t length,uint8_t destag);

/**
 * @name:joylink_string_upsidedown
 *
 * @param: src
 * @param: dest
 * @param: len_src
 *
 * @returns:
 */
int32_t joylink_string_upsidedown(uint8_t *src,uint8_t *dest,int32_t len_src);
/**
 * @name:joylink_buf_mix
 *
 * @param: buf1_16
 * @param: buf2_6
 * @param: len_buf1_16
 * @param: len_buf2_6
 * @param: output
 * @param: outputlen
 *
 * @returns:
 */
int32_t joylink_buf_mix(const uint8_t *buf1_16,const uint8_t *buf2_6,uint8_t len_buf1_16,uint8_t len_buf2_6,uint8_t *output,uint8_t outputlen);



uint16_t joylink_tlvintToStr_Insert(uint8_t *dstbuf,int offset,int tagid,int value);

uint16_t joylink_tlvshort_Insert(uint8_t *dstbuf,int offset,int tagid,short value);

#endif /* __JOYLINK_UTILS_H__ */
