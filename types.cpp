#include <fcntl.h>
#include <iostream>
#include <dlfcn.h>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <elf.h>
#include <stdlib.h>

using namespace std;


int main(int argc, char const *argv[])
{
	int fd = open(argv[1], O_RDONLY);

	if(fd == -1){
		fprintf(stderr, "Couldn't open input file\n");
		exit(1);
	}

	struct stat buf;
	fstat(fd, &buf);

	char* p = (char*)mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	char* beginning = p; //seg fault buf.st_size wrong header


	Elf32_Ehdr* elfHdr;
	elfHdr = (Elf32_Ehdr*)p;
	unsigned int bytes = buf.st_size;
	if (bytes < sizeof(Elf32_Ehdr)){
		fprintf(stderr, "Error while reading ELF Header\n");
		exit(1);
	}

	if((elfHdr->e_ident[EI_MAG0] != ELFMAG0) ||
       (elfHdr->e_ident[EI_MAG1] != ELFMAG1) ||
       (elfHdr->e_ident[EI_MAG2] != ELFMAG2) ||
       (elfHdr->e_ident[EI_MAG3] != ELFMAG3)){
		fprintf(stderr, "Not an ELF file\n");
		exit(1);
	}

	if ((elfHdr->e_ident[EI_CLASS] != ELFCLASS32) ||
		(elfHdr->e_ident[EI_VERSION] != EV_CURRENT) ||
		(elfHdr->e_version != EV_CURRENT) ||
		(elfHdr->e_ident[EI_DATA] != ELFDATA2LSB)) {
		fprintf(stderr, "Not supported ELF file\n");
		exit(1);
	}

	int c = elfHdr->e_type;

	int off = elfHdr->e_shoff + elfHdr->e_shstrndx * elfHdr->e_shentsize;

	p += off;
	Elf32_Shdr* sectHdr = (Elf32_Shdr*)p;

	char* p2 = beginning;
	p2 += sectHdr->sh_offset;

	char* SectNames = (char*) p2;

	int index;
	switch(c){
		case 0:
		{
			printf("TYPE: NONE\n");
			break;
		}
		case 1:
		{
			printf("TYPE: REL\n");
			break;
		}
		case 2:
		{
			printf("TYPE: EXEC\n");
			break;
		}
		case 3:
		{
			printf("TYPE: DYN\n");
			break;
		}
		case 4:
		{
			printf("TYPE: CORE\n");
			break;
		}
	}
	printf("%16sNAME       ADDR     OFFSET       SIZE   ALGN\n", "");
	for(index = 1; index < elfHdr->e_shnum; index++){
		printf("%d\n", index);
		const char* name = "";
		size_t offset_2 = elfHdr->e_shoff + index * sizeof(Elf32_Shdr);
		char* p3 = beginning;
		p3 += offset_2;
		Elf32_Shdr* sectHdr = (Elf32_Shdr*)p3;
		name = SectNames + sectHdr->sh_name;
		printf("%20s 0x%08x %10d %10d 0x%04x\n", 
			name, (unsigned int)sectHdr->sh_addr, (int)sectHdr->sh_offset, 
			(int)sectHdr->sh_size, (unsigned int)sectHdr->sh_addralign);
	}
	return 0;
}
