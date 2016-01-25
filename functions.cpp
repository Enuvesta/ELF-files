
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <dlfcn.h>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <elf.h>
#include <cstring>
#include <stab.h>
#include <vector>
#include <algorithm>

using namespace std;

struct Stab
{
    uint32_t n_strx;  
    uint8_t n_type;
    uint8_t n_other;
    uint16_t n_desc;
    uintptr_t n_value;
};

struct funcs{
	string name_it;
	unsigned int id;
	unsigned int start;
	unsigned int end;
	char* source;
	bool end_so;
	bool was_sline;
};

bool compare(funcs &a, funcs &b){
	if(a.name_it < b.name_it){
		return true;
	} else if(a.name_it == b.name_it){
		if (a.start < b.start){
			return true;
		} else {
			return false;
		}

	} else {
		return false;
	}
}

int main(int argc, char const *argv[])
{
	int fd = open(argv[1], O_RDONLY);

	struct stat buf;
	fstat(fd, &buf);

	char* p = (char*)mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	char* beginning = p;


	Elf32_Ehdr* elfHdr;
	elfHdr = (Elf32_Ehdr*)p;

	int off = elfHdr->e_shoff + elfHdr->e_shstrndx * elfHdr->e_shentsize;

	p += off;
	Elf32_Shdr* sectHdr = (Elf32_Shdr*)p;

	char* p2 = beginning;
	p2 += sectHdr->sh_offset;

	char* SectNames = (char*) p2;

	int index;

	const char* stab = ".stab";
	const char* stabstr = ".stabstr";
	unsigned int stab_off = 0;
	unsigned int stab_size;
	unsigned int stab_entsize = 0;
	unsigned int stabstr_off = 0;
	bool find_stabs = false;


	for(index = 1; index < elfHdr->e_shnum; index++){
		const char* name = "";
		size_t offset_2 = elfHdr->e_shoff + index * sizeof(Elf32_Shdr);
		char* p3 = beginning;
		p3 += offset_2;
		Elf32_Shdr* sectHdr = (Elf32_Shdr*)p3;
		name = SectNames + sectHdr->sh_name;
		if(strcmp(name,stab) == 0){
			find_stabs = true;
			stab_off = sectHdr->sh_offset;
			stab_size = sectHdr->sh_size;
			stab_entsize = sectHdr->sh_entsize;
		}
		if(strcmp(name,stabstr) == 0){
			find_stabs = true;
			stabstr_off = sectHdr->sh_offset;
		}
	}
	if (find_stabs){
		char* stab_begin = beginning;
		stab_begin += stab_off + stab_entsize;

		char* stabstr_begin = beginning;
		stabstr_begin += stabstr_off;
		std::vector<funcs> v;
		v.resize(0);
		unsigned int i = stab_entsize;
		int count = 0;
		char* cur_sol = (char*)"";
		//char* help_func = "";
		while(i < stab_size){
			count++;
			Stab* current = (Stab*)stab_begin;
			if(current->n_type == N_SOL || current->n_type == N_SO){
				cur_sol = stabstr_begin + current->n_strx;
				if((current->n_type == N_SO) && (strcmp(cur_sol,"") == 0)){
					if ((v.size() > 0) && (!v[v.size() - 1].end_so)) {
						v[v.size() - 1].end_so = true;
						v[v.size() - 1].end = current->n_value;
					}
				}
			}
			if(current->n_type == N_SLINE){
				if(v.size() > 0){
					if(!v[v.size()-1].was_sline){
						v[v.size()-1].was_sline = true;
						v[v.size()-1].source = cur_sol;
					}
				}
			}
			if(current->n_type == N_FUN){
				funcs cur_func;
				cur_func.id = count;

				char* fun_source = stabstr_begin + current->n_strx;
				string func_name(fun_source);
				string::size_type last_colon = func_name.find_first_of(':');
				if(last_colon != std::string::npos)
					func_name = func_name.substr(0, last_colon);
				cur_func.name_it = func_name;

				cur_func.was_sline = false;
				cur_func.end_so = false;	
				cur_func.start = current->n_value; 
				if(v.size() > 0){
					if (!v[v.size() - 1].end_so) {
	  					v[v.size() - 1].end = current->n_value;
	  				}
				}
				v.push_back(cur_func);
			}

			stab_begin += stab_entsize;
			i += stab_entsize;
		}
		sort(v.begin(), v.end(), compare);
		for(size_t  in = 0; in < v.size(); in++){
			//printf("%ld\n", in);
			printf("%s %d 0x%08x 0x%08x %s\n", v[in].name_it.c_str(), v[in].id, v[in].start, v[in].end, v[in].source);
		}
	} else {
		printf("%s\n", "No debug info");
	}
	return 0;
}
