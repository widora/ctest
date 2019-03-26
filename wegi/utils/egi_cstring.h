#ifndef __EGI_CSTRING_H__
#define __EGI_CSTRING_H__


#define EGI_CONFIG_PATH "/home/egi.conf"

char * cstr_trim_space(char *buf);
int egi_get_config_value(char *sect, char *key, char* value);


#endif
