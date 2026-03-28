#ifndef LIB_CTYPE_H
#define LIB_CTYPE_H

static inline int isdigit(int c) { return (c >= '0' && c <= '9'); }
static inline int isspace(int c) { return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'); }
static inline int isalpha(int c) { return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); }

#endif
