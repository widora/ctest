/*----------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
-----------------------------------------------------------------*/
#ifndef __EGI_CSTRING_H__
#define __EGI_CSTRING_H__


#define EGI_CONFIG_PATH "/home/egi.conf"

char * cstr_dup_repextname(char *fpath, char *new_extname);
char * cstr_split_nstr(char *str, char *split, unsigned n);
char * cstr_trim_space(char *buf);
inline int cstr_charlen_uft8(const unsigned char *cp);
int cstr_strcount_uft8(const unsigned char *pstr);
inline int char_uft8_to_unicode(const unsigned char *src, wchar_t *dest);
int egi_get_config_value(char *sect, char *key, char* value);


#endif
