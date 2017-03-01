    /*
 * Linux kernel module fucker
 *
 * by wzt       <wzt.wzt@gmail.com>
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MODULE_NAME_LEN         (64 - sizeof(unsigned long))

struct modversion_info
{
        unsigned long crc;
        char name[MODULE_NAME_LEN];
};

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
Elf32_Shdr *shdr = NULL;
Elf32_Shdr *shstrtab = NULL;
Elf32_Sym *dynsym_ptr = NULL;
Elf32_Sym *symtab_ptr = NULL;
Elf32_Sym *dynstr_ptr = NULL;

char *Real_strtab = NULL;
char *dynstr = NULL;
char *strtab_ptr = NULL;
char dynstr_buffer[2048];
char strtab_buffer[4096];
char *real_strtab = NULL;

unsigned int shstrtab_off, shstrtab_len, shstrtab_num;
unsigned int strtab_off, strtab_size;

int elf_fd;
struct stat f_stat;

void usage(char *pro)
{
        fprintf(stderr, "usage: %s <options> <module>\n\n", pro);
        fprintf(stderr, "-w -v\tCheck vermgaic in module.\n");
        fprintf(stderr, "-w -c\tCheck crc value in module.\n");
        fprintf(stderr, "-s -v <new_vermagic>\tSet vermagic in module.\n");
        fprintf(stderr, "-s -c\tSet crc value in module.\n");

        exit(0);
}

int init_load_elf(char *elf_file)
{
        char buff[1024];

        elf_fd = open(elf_file, O_RDWR);
        if (elf_fd == -1) {
                perror("open");
                return 0;
        }
        fprintf(stderr, "[+] Open %s ok.\n", elf_file);

        if (fstat(elf_fd, &f_stat) == -1) {
                perror("fstat");
                return 0;
        }

        ehdr = (Elf32_Ehdr *)mmap(NULL, f_stat.st_size, PROT_WRITE|PROT_READ, MAP_SHARED, elf_fd, 0);
        if(ehdr == MAP_FAILED) {
                perror("mmap");
                return 0;
        }

        phdr = (Elf32_Phdr *)((unsigned long)ehdr + ehdr->e_phoff);
        shdr = (Elf32_Shdr *)((unsigned long)ehdr + ehdr->e_shoff);

        shstrtab = &shdr[ehdr->e_shstrndx];
        shstrtab_off = (unsigned int)shstrtab->sh_offset;
        shstrtab_len = shstrtab->sh_size;

        real_strtab = (char *)( (unsigned long)ehdr + shstrtab_off );

        printf("[+] .Shstrtab Size :0x%x,%d\n", shstrtab->sh_size, shstrtab->sh_name);
        printf("[+] .Shstrtab Off: 0x%x\n", shstrtab_off);

        return 1;
}

int display_module_crc_info(void)
{
        struct modversion_info *versions;
        char *buff = NULL;
        unsigned int version_off, version_size, num_versions;
        int i, j;

        buff = (char *)malloc(shstrtab_len + 2);
        if (!buff) {
                fprintf(stderr, "[-] Malloc failed.\n");
                return 0;
        }

        memcpy(buff, real_strtab, shstrtab_len + 1);
        for (i = 0 ; i < (int)ehdr->e_shnum ; i++){
                if (!strcmp(buff + shdr[i].sh_name,"__versions")){
                        printf("[+] found section %s.\n", buff + shdr[i].sh_name);
                        version_off = (unsigned int)shdr[i].sh_offset;
                        version_size = (unsigned int)shdr[i].sh_size;
                        printf("[+] version off: 0x%x\n", version_off);
                        printf("[+] version size: 0x%x\n", version_size);
                        break;
                }
        }

        printf("[+] %x,%x\n", (unsigned long)ehdr + version_off, shdr[i].sh_addr);
        versions = (void *)((unsigned long)ehdr + version_off);
        num_versions = version_size / sizeof(struct modversion_info);
        printf("[+] num_versions: %d\n", num_versions);

        for (j = 0; j < num_versions; j++) {
                printf("[+] %s:0x%08x.\n", versions[j].name, versions[j].crc);
        }

        free(buff);
        return 1;
}

int set_module_crc_info(void)
{
        struct modversion_info *versions;
        char *buff = NULL;
        unsigned int version_off, version_size, num_versions;
        int i, j;

        buff = (char *)malloc(shstrtab_len + 2);
        if (!buff) {
                fprintf(stderr, "[-] Malloc failed.\n");
                return 0;
        }

        memcpy(buff, real_strtab, shstrtab_len + 1);
        for (i = 0 ; i < (int)ehdr->e_shnum ; i++){
                if (!strcmp(buff + shdr[i].sh_name,"__versions")){
                        printf("[+] found section %s.\n", buff + shdr[i].sh_name);
                        version_off = (unsigned int)shdr[i].sh_offset;
                        version_size = (unsigned int)shdr[i].sh_size;
                        printf("[+] version off: 0x%x\n", version_off);
                        printf("[+] version size: 0x%x\n", version_size);
                        break;
                }
        }

        printf("[+] %x,%x\n", (unsigned long)ehdr + version_off, shdr[i].sh_addr);
        versions = (void *)((unsigned long)ehdr + version_off);
        num_versions = version_size / sizeof(struct modversion_info);
        printf("[+] num_versions: %d\n", num_versions);

        for (j = 0; j < num_versions; j++) {
                printf("[+] %s:0x%08x.\n", versions[j].name, versions[j].crc);
                if (!strcmp(versions[j].name, "struct_module")) {
                        fprintf(stderr, "[+] Found symbol struct_module.\n");
                        versions[j].name[0] = 'T';
                        break;
                }
        }

        for (j = 0; j < num_versions; j++) {
                printf("[+] %s:0x%08x.\n", versions[j].name, versions[j].crc);
        }

        free(buff);
        return 1;
}

static char *next_string(char *string, unsigned long *secsize)
{
        /* Skip non-zero chars */
        while (string[0]) {
                string++;
                if ((*secsize)-- <= 1)
                        return NULL;
        }

        /* Skip any zero padding. */
        while (!string[0]) {
                string++;
                if ((*secsize)-- <= 1)
                        return NULL;
        }
        return string;
}

static char *get_modinfo(Elf32_Shdr *sechdrs, unsigned int info, const char *tag)
{
        char *p;
        unsigned int taglen = strlen(tag);
        unsigned long size = sechdrs[info].sh_size;

        for (p = (char *)(ehdr +sechdrs[info].sh_offset); p; p = next_string(p, &size)) {
                if (strncmp(p, tag, taglen) == 0 && p[taglen] == '=')
                        return p + taglen + 1;
        }
        return NULL;
}

int display_module_vermagic_info(void)
{
        char *buff, *p;
        char *ver = "vermagic";
        unsigned int taglen = strlen(ver);
        int size, i;

        buff = (char *)malloc(shstrtab_len + 2);
        if (!buff) {
                fprintf(stderr, "[-] Malloc failed.\n");
                return 0;
        }

        memcpy(buff, real_strtab, shstrtab_len + 1);
        for (i = 0 ; i < (int)ehdr->e_shnum ; i++){
                if (!strcmp(buff + shdr[i].sh_name,".modinfo")){
                        printf("[+] found section %s.\n", buff + shdr[i].sh_name);
                        break;
                }
        }

        size = shdr[i].sh_size;
        printf("[+] size: 0x%x.\n", size);

        p = (char *)((unsigned long)ehdr + shdr[i].sh_offset);
        printf("[+] 0x%08x\n", p);

        for (; p; p = next_string(p, &size)) {
                printf("[+] %s\n", p);
                if (strncmp(p, "vermagic", taglen) == 0 && p[taglen] == '=') {
                        printf("[+] %s\n", p + taglen + 1);
                        //memset(p + taglen + 1, 'A', 30);
                }
        }

        return 1;
}

int set_module_vermagic_info(char *new_vermagic)
{
        char *buff, *p;
        char *ver = "vermagic";
        unsigned int taglen = strlen(ver);
        int size, i;

        buff = (char *)malloc(shstrtab_len + 2);
        if (!buff) {
                fprintf(stderr, "[-] Malloc failed.\n");
                return 0;
        }

        memcpy(buff, real_strtab, shstrtab_len + 1);
        for (i = 0 ; i < (int)ehdr->e_shnum ; i++){
                if (!strcmp(buff + shdr[i].sh_name,".modinfo")){
                        printf("[+] found section %s.\n", buff + shdr[i].sh_name);
                        break;
                }
        }

        size = shdr[i].sh_size;
        printf("[+] size: 0x%x.\n", size);

        p = (char *)((unsigned long)ehdr + shdr[i].sh_offset);
        printf("[+] 0x%08x\n", p);

        for (; p; p = next_string(p, &size)) {
                printf("[+] %s\n", p);
                if (strncmp(p, "vermagic", taglen) == 0 && p[taglen] == '=') {
                        printf("[+] %s\n", p + taglen + 1);
                        if (strlen(p + taglen + 1) < strlen(new_vermagic)) {
                                printf("[-] New vermagic len must < current magic len.\n");
                                return 0;
                        }
                        memset(p + taglen + 1, '\0', strlen(new_vermagic));
                        memcpy(p + taglen + 1, new_vermagic, strlen(new_vermagic));
                }
        }

        return 1;
}

int exit_elf_load(void)
{
        close(elf_fd);
        if (munmap(ehdr, f_stat.st_size) == -1) {
                return 0;
        }

        return 1;
}

int main(int argc, char **argv)
{
        if (argc == 1) {
                usage(argv[0]);
        }

        if (!strcmp(argv[1], "-w") && !strcmp(argv[2], "-c")) {
                fprintf(stderr, "[+] Display %s module crc value.\n", argv[3]);
                if (!init_load_elf(argv[3])) {
                        fprintf(stderr, "[-] Init elf load failed.\n");
                        return 0;
                }
                display_module_crc_info();
                exit_elf_load();
        }
        else if (!strcmp(argv[1], "-s") && !strcmp(argv[2], "-c")) {
                fprintf(stderr, "[+] Set %s module crc value.\n", argv[3]);
                if (!init_load_elf(argv[3])) {
                        fprintf(stderr, "[-] Init elf load failed.\n");
                        return 0;
                }
                set_module_crc_info();
                exit_elf_load();
        }
        if (!strcmp(argv[1], "-w") && !strcmp(argv[2], "-v")) {
                fprintf(stderr, "[+] Display %s module crc value.\n", argv[3]);
                if (!init_load_elf(argv[3])) {
                        fprintf(stderr, "[-] Init elf load failed.\n");
                        return 0;
                }
                display_module_vermagic_info();
                exit_elf_load();
        }
        if (!strcmp(argv[1], "-s") && !strcmp(argv[2], "-v")) {
                fprintf(stderr, "[+] Display %s module crc value.\n", argv[4]);
                if (!init_load_elf(argv[4])) {
                        fprintf(stderr, "[-] Init elf load failed.\n");
                        return 0;
                }
                set_module_vermagic_info(argv[3]);
                exit_elf_load();
        }
        else {
                return 0;
        }

}
