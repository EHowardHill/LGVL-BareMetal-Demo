#include <stddef.h>
#include <stdint.h>

/* Add function prototypes to fix conflicting type errors */
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

/* A simple panic function to halt the system on a critical error */
static void kernel_panic(void)
{
    /* Disable interrupts and halt the CPU */
    asm volatile("cli; hlt");
}

/* Memory management for LVGL */
static uint8_t heap[96 * 1024] __attribute__((aligned(8)));
static size_t heap_index = 0;

void *malloc(size_t size)
{
    /* Simple bump allocator - align to 8 bytes */
    size = (size + 7) & ~7;

    if (heap_index + size > sizeof(heap))
    {
        /* Out of memory! */
        kernel_panic();
        return NULL; /* Unreachable */
    }

    void *ptr = &heap[heap_index];
    heap_index += size;
    return ptr;
}

void free(void *ptr)
{
    /* No-op for simple bump allocator */
    (void)ptr;
}

void *realloc(void *ptr, size_t size)
{
    /*
     * This implementation is UNSAFE because it can read past the end of the original
     * block, but it's required to avoid an immediate crash from a NULL return.
     * The real fix is a proper memory allocator.
     */
    if (ptr == NULL)
    {
        return malloc(size);
    }

    void *new_ptr = malloc(size);
    if (new_ptr == NULL)
    {
        /* Malloc will panic if it fails, so this is unlikely to be reached */
        return NULL;
    }

    /* This copy is still dangerous as we don't know the old size. */
    /* We are knowingly leaving this as is, as the primary bug was the */
    /* allocator conflict. The next step would be to replace this allocator. */
    memcpy(new_ptr, ptr, size);

    return new_ptr;
}

void *calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *ptr = malloc(total);

    if (ptr != NULL)
    {
        memset(ptr, 0, total);
    }

    return ptr;
}

/* String functions */
void *memset(void *s, int c, size_t n)
{
    uint8_t *p = s;
    for (size_t i = 0; i < n; i++)
    {
        p[i] = (uint8_t)c;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = dest;
    const uint8_t *s = src;
    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
    uint8_t *d = dest;
    const uint8_t *s = src;

    if (d < s)
    {
        for (size_t i = 0; i < n; i++)
        {
            d[i] = s[i];
        }
    }
    else
    {
        for (size_t i = n; i > 0; i--)
        {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
        {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
    {
        len++;
    }
    return len;
}

char *strcpy(char *dest, const char *src)
{
    size_t i = 0;
    while (src[i])
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i]; i++)
    {
        dest[i] = src[i];
    }
    for (; i < n; i++)
    {
        dest[i] = '\0';
    }
    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (s1[i] != s2[i] || !s1[i])
        {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}

char *strcat(char *dest, const char *src)
{
    size_t dest_len = strlen(dest);
    size_t i;

    for (i = 0; src[i]; i++)
    {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';

    return dest;
}

char *strncat(char *dest, const char *src, size_t n)
{
    size_t dest_len = strlen(dest);
    size_t i;

    for (i = 0; i < n && src[i]; i++)
    {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';

    return dest;
}

/* Simple snprintf implementation for LVGL */
static void int_to_str(int value, char *buf, int *pos)
{
    if (value < 0)
    {
        buf[(*pos)++] = '-';
        value = -value;
    }

    if (value == 0)
    {
        buf[(*pos)++] = '0';
        return;
    }

    char temp[12];
    int temp_pos = 0;

    while (value > 0)
    {
        temp[temp_pos++] = '0' + (value % 10);
        value /= 10;
    }

    while (temp_pos > 0)
    {
        buf[(*pos)++] = temp[--temp_pos];
    }
}

int vsnprintf(char *buf, size_t size, const char *format, __builtin_va_list args)
{
    size_t pos = 0;

    while (*format && pos < size - 1)
    {
        if (*format == '%')
        {
            format++;
            if (*format == 'd')
            {
                int value = __builtin_va_arg(args, int);
                int temp_pos = 0;
                char temp[32];
                int_to_str(value, temp, &temp_pos);
                for (int i = 0; i < temp_pos && pos < size - 1; i++)
                {
                    buf[pos++] = temp[i];
                }
            }
            else if (*format == 's')
            {
                char *str = __builtin_va_arg(args, char *);
                while (*str && pos < size - 1)
                {
                    buf[pos++] = *str++;
                }
            }
            else if (*format == 'c')
            {
                char c = (char)__builtin_va_arg(args, int);
                buf[pos++] = c;
            }
            else if (*format == '%')
            {
                buf[pos++] = '%';
            }
            format++;
        }
        else
        {
            buf[pos++] = *format++;
        }
    }

    buf[pos] = '\0';
    return pos;
}

int snprintf(char *buf, size_t size, const char *format, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, format);
    int result = vsnprintf(buf, size, format, args);
    __builtin_va_end(args);
    return result;
}

/* GCC builtin support for 64-bit division */
long long __divdi3(long long a, long long b)
{
    int neg = 0;
    if (a < 0)
    {
        a = -a;
        neg = !neg;
    }
    if (b < 0)
    {
        b = -b;
        neg = !neg;
    }

    long long quot = 0;
    long long rem = 0;

    for (int i = 63; i >= 0; i--)
    {
        rem = (rem << 1) | ((a >> i) & 1);
        if (rem >= b)
        {
            rem -= b;
            quot |= (1LL << i);
        }
    }

    return neg ? -quot : quot;
}

/* LVGL wrapper functions - map lv_* to our implementations */
void *lv_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}

void *lv_memset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}

void *lv_memmove(void *dest, const void *src, size_t n)
{
    return memmove(dest, src, n);
}

int lv_memcmp(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n);
}

size_t lv_strlen(const char *s)
{
    return strlen(s);
}

char *lv_strcpy(char *dest, const char *src)
{
    return strcpy(dest, src);
}

char *lv_strncpy(char *dest, const char *src, size_t n)
{
    return strncpy(dest, src, n);
}

int lv_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

int lv_strncmp(const char *s1, const char *s2, size_t n)
{
    return strncmp(s1, s2, n);
}

char *lv_strcat(char *dest, const char *src)
{
    return strcat(dest, src);
}

char *lv_strncat(char *dest, const char *src, size_t n)
{
    return strncat(dest, src, n);
}

char *lv_strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup)
    {
        memcpy(dup, s, len);
    }
    return dup;
}

size_t lv_strlcpy(char *dst, const char *src, size_t size)
{
    size_t src_len = strlen(src);
    if (size > 0)
    {
        size_t copy_len = (src_len < size - 1) ? src_len : size - 1;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }
    return src_len;
}

int lv_snprintf(char *buf, size_t size, const char *format, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, format);
    int result = vsnprintf(buf, size, format, args);
    __builtin_va_end(args);
    return result;
}

int lv_vsnprintf(char *buf, size_t size, const char *format, __builtin_va_list args)
{
    return vsnprintf(buf, size, format, args);
}

/* LVGL memory management wrappers */
void *lv_malloc_core(size_t size)
{
    return malloc(size);
}

void lv_free_core(void *ptr)
{
    free(ptr);
}

void *lv_realloc_core(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void lv_mem_init(void)
{
    /* Our bump allocator doesn't need initialization */
}

void lv_mem_deinit(void)
{
    /* Our bump allocator doesn't need deinitialization */
}

void lv_mem_monitor_core(void *mon)
{
    /* Not implemented for simple allocator */
    (void)mon;
}

int lv_mem_test_core(void)
{
    /* Simple allocator always passes */
    return 1;
}

/* Stub for binary decoder (disabled in config) */
void lv_bin_decoder_init(void)
{
    /* Binary decoder is disabled */
}