/*----------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Midas Zhou
-----------------------------------------------------------------*/
#ifndef __EGI_CSTRING_H__
#define __EGI_CSTRING_H__


#define EGI_CONFIG_PATH "/home/egi.conf"

char * cstr_split_nstr(char *str, char *split, unsigned n);
char * cstr_trim_space(char *buf);
int egi_get_config_value(char *sect, char *key, char* value);


#endif
