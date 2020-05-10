
#ifndef CASE 
typedef unsigned char uchar;
typedef unsigned long long ullong;

#define MAX_LEN 9
constexpr ullong raise_128_to(const uchar power)
{
    return 0x1ULL << 7 * power;
}

constexpr bool str_is_correct(const char*const str)
{
    return (static_cast<signed char>(*str) > 0) ? str_is_correct(str + 1) : (*str ? false : true);
}

constexpr uchar str_len(const char*const str)
{
    return *str ? (1 + str_len(str + 1)) : 0;
}


constexpr ullong str_hash(const char*const str, const uchar current_len)
{
    return *str ? (raise_128_to(current_len - 1) * static_cast<uchar>(*str)
            + str_hash(str + 1, current_len - 1)) : 0;
}

#define CASE(str) static_assert(str_is_correct(str) && (str_len(str) <= MAX_LEN), \
        "CASE string contains wrong characters, or its length is greater than 9");\
        case str_hash(str, str_len(str))


#define SWITCH(str) switch(str_hash_for_switch(str))
const ullong N_HASH = static_cast<ullong>(-1);

inline ullong str_hash_for_switch(const char*const str)
{
    return (str_is_correct(str) && (str_len(str) <= MAX_LEN)) ?
        str_hash(str, str_len(str)) : N_HASH;
}

#endif // CASE
