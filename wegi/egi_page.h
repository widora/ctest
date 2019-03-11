#ifndef __EGI_PAGE_H__
#define __EGI_PAGE_H__

#include <stdio.h>
#include "egi.h"

EGI_PAGE * egi_page_new(char *tag);
int egi_page_free(EGI_PAGE *page);
int egi_page_addlist(EGI_PAGE *page, EGI_EBOX *ebox);
int egi_page_travlist(EGI_PAGE *page);
int egi_page_activate(EGI_PAGE *page);
int egi_page_refresh(EGI_PAGE *page);
int egi_page_routine(EGI_PAGE *page);
int egi_page_needrefresh(EGI_PAGE *page);
EGI_EBOX *egi_page_pickebox(EGI_PAGE *page, enum egi_ebox_type type, unsigned int id);

#endif
