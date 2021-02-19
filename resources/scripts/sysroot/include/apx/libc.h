/*
 * APX Autopilot project <http://docs.uavos.com>
 *
 * Copyright (c) 2003-2020, Aliaksei Stratsilatau <sa@uavos.com>
 * All rights reserved
 *
 * This file is part of APX Shared Libraries.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "visibility.h"
#include "types.h"

__BEGIN_DECLS

int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
void *memchr(const void *s, int c, size_t n);

size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strstr(const char *s, const char *find);
char *strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
size_t strlen(const char *s);
int strncmp(const char *str1, const char *str2, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
int strncasecmp(const char *s1, const char *s2, size_t n);

int isupper(int c);
int isalpha(int c);
int isspace(int c);
int isgraph(int c);
int isprint(int c);
int isdigit(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);
int isalnum(int c);

int atoi(const char *s);
long strtol(const char *nptr, char **endptr, register int base);
unsigned long strtoul(const char *nptr, char **endptr, register int base);

int snprintf_f(char *s, size_t n, const char *fmt, float v);
int snprintf_i(char *s, size_t n, const char *fmt, int32_t v);

__END_DECLS

#ifdef __cplusplus
int snprintf(char *s, size_t n, const char *fmt, float v)
{
    return snprintf_f(s, n, fmt, v);
}
int snprintf(char *s, size_t n, const char *fmt, int32_t v)
{
    return snprintf_i(s, n, fmt, v);
}
#endif
