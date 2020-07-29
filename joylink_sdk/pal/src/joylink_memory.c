#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// joylink platform layer header files
#include "joylink_memory.h"


/** @defgroup group_platform_memory_manage memory
 *  @{
 */


 /**
 * @brief Allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 *
 * @param [in] size @n specify block size in bytes.
 * @return A pointer to the beginning of the block.
 * @see None.
 * @note Block value is indeterminate.
 */
void *jl_platform_malloc(uint32_t size)
{
#ifdef __LINUX_PAL__
    return malloc(size);
#else
#ifdef __ESP_PAL__
    return malloc(size);
#endif
#endif
}

/**
 * @brief Changes the size of the memory block pointed to by ptr to size bytes.
 *
 * @param [in] ptr  @n pointer to be realloc
 * @param [in] size @n specify block size in bytes for newly allocated memory
 * @return A pointer to the beginning of newly allocated memory.
 * @see None.
 * @note Block value is indeterminate.
 */
void *jl_platform_realloc(void *ptr, uint32_t size)
{
    return NULL;
}

/**
 * @brief Allocates memory for an array of nmemb elements of size bytes each and returns a pointer to the allocated memory.
 *
 * @param [in] nmemb  @n array elements item counts
 * @param [in] size @n specify block size in bytes for every array elements
 * @return A pointer to the beginning of allocated memory.
 * @see None.
 * @note Block value is indeterminate.
 */
void *jl_platform_calloc(uint32_t nmemb, uint32_t size)
{
    return NULL;
}

/**
 * @brief Deallocate memory block
 *
 * @param[in] ptr @n Pointer to a memory block previously allocated with platform_malloc.
 * @return None.
 * @see None.
 * @note None.
 */
void  jl_platform_free(void *ptr)
{
#ifdef __LINUX_PAL__
    free(ptr);
#else
#ifdef __ESP_PAL__
    free(ptr);
#endif
#endif
}

/**
 * @brief 复制字符 value（一个无符号字符）到参数 prt 所指向的字符串的前 num 个字符
 *
 * @param[in] ptr @n 指向要填充的内存块.
 * @param[in] value @n 要被设置的值。该值以 int 形式传递，但是函数在填充内存块时是使用该值的无符号字符形式.
 * @param[in] num @n 要被设置为该值的字节数.
 * @return 
 	该值返回一个指向存储区 str 的指针
 * @see None.
 * @note None.
 */
void *jl_platform_memset(void* ptr, int32_t value, uint32_t num)
{
#ifdef __LINUX_PAL__
    return memset(ptr, value, num);
#else
#ifdef __ESP_PAL__
    return memset(ptr, value, num);
#endif
#endif
}

/**
 * @brief 从存储区 src 复制 num 个字符到存储区 dst
 *
 * @param[in] dst @n 指向用于存储复制内容的目标数组，类型强制转换为 void* 指针.
 * @param[in] src @n 指向要复制的数据源，类型强制转换为 void* 指针.
 * @param[in] num @n 要被复制的字节数.
 * @return 
 	该函数返回一个指向目标存储区 str1 的指针
 * @see None.
 * @note None.
 */
void *jl_platform_memcpy(void* dst, const void* src, uint32_t num)
{
#ifdef __LINUX_PAL__
    return memcpy(dst, src, num);
#else
#ifdef __ESP_PAL__
    return memcpy(dst, src, num);
#endif
#endif
}

/**
 * @brief 把存储区 ptr1 和存储区 ptr2 的前 num 个字节进行比较
 *
 * @param[in] ptr1 @n 指向内存块的指针
 * @param[in] ptr2 @n 指向内存块的指针
 * @param[in] num  @n 要被比较的字节数
 * @return 
 	如果返回值 < 0，则表示 ptr1 小于 ptr2
	如果返回值 > 0，则表示 ptr2 小于 ptr1
	如果返回值 = 0，则表示 ptr1 等于 ptr2
 * @see None.
 * @note None.
 */
int32_t   jl_platform_memcmp(const void* ptr1, const void* ptr2, uint32_t num)
{
#ifdef __LINUX_PAL__
    return memcmp(ptr1, ptr2, num);
#else
#ifdef __ESP_PAL__
    return memcmp(ptr1, ptr2, num);
#endif
#endif
}


/** @} */ /* end of platform_memory_manage */
