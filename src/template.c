#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bs.h"
#include "hash_map.h"
#include "template.h"

struct Template {
	const char* filename;
	Hash_map* context;
};

Template* templateNew(const char* filename) {
	Template* template = malloc(sizeof(Template));

	template->filename = filename;
	template->context = Hash_map_new();

	return template;
}

void templateDel(Template* template) {
	Hash_map_delete(template->context);

	free(template);
}

void templateSet(Template* template, const char* key, const char* value) {
	SPair* p = malloc(sizeof(SPair));
	p->key = strdup(key);
	p->value = strdup(value);

	Hash_map_insert_move(template->context, p);
}

char* templateRender(const Template* template) {
	FILE* file = fopen(template->filename, "r");

	if(!file) {
		fprintf(stderr, "error: template '%s' not found\n", template->filename);
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	size_t len = ftell(file);
	rewind(file);

	// For cross platform or we can use mmap
	char* buff = malloc(sizeof(char) * (len + 2));
	(void)!fread(buff + 1, sizeof(char), len, file);
	fclose(file);

	buff[0] = ' ';
	buff[len + 1] = '\0';

	char* res = bsNew("");
	char* pos = NULL;
	// VARIABLES
	char* segment = strtok_r(buff, "{\0", &pos);
	assert(segment);
	bsLCat(&res, segment + 1);

	bool rep = false;
	for(;;) {
		segment = strtok_r(NULL, "}\0", &pos);

		if(!segment)
			break;

		if(*segment == '{') {
			rep = true;
			segment += 1;
			char* val = Hash_map_get(template->context, segment);
			if(val)
				bsLCat(&res, val);
		}
		else if(*segment == '%') {
			rep = true;
			segment += 1;
			if(!strncmp(segment, "include", 7)) {
				segment += 8;
				segment[strlen(segment) - 1] = '\0';

				Template* inc = templateNew(segment);
				Hash_map_delete(inc->context);
				inc->context = template->context;
				char* incBs = templateRender(inc);

				bsLCat(&res, incBs);
				bsDel(incBs);
				free(inc);
			}
			else if(!strncmp(segment, "when", 4)) {
				segment += 5;
				char* spc = strchr(segment, ' ');
				*spc = '\0';
				char* incBs = Hash_map_get(template->context, segment);

				if(incBs) {	 // Found
					segment = spc + 1;
					spc = strchr(segment, ' ');
					*spc = '\0';

					if(!strcmp(incBs, segment)) {
						segment = spc + 1;
						segment[strlen(segment) - 1] = '\0';

						bsLCat(&res, segment);
					}
				}
			}
			else {
				fprintf(stderr, "error: unknown exp {%%%s} in '%s'\n", segment, template->filename);
				exit(1);
			}
		}
		else {
			rep = false;

			bsLCat(&res, "{");
		}

		segment = strtok_r(NULL, "{\0", &pos);

		if(!segment)
			break;

		if(rep) {
			rep = false;
			segment += 1;
		}
		else {
			bsLCat(&res, "}");
		}

		bsLCat(&res, segment);
	}

	free(buff);
	return res;
}
