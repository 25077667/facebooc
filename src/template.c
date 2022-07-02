#define _GNU_SOURCE	 // For the mremap
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bs.h"
#include "hash_map.h"
#include "http/helper.h"
#include "template.h"
#include "utility.h"

int DEFAULT_FRONEN_ENTRIES = 48;
int DEFAULT_PLACEHOLDER_ENTRIES = 64;
int DEFAULT_PH_PAGE_NUM = 2;
int DEFAULT_CTX_PAGE_NUM = 4;

static bool BO_NOTHING(void* p) {
	(void)p;
	return true;
}

struct F_mem {
	size_t buf_len;
	char* buf;
};

typedef struct Mmapped_buffer {
	char* begin;
	char* end;
	size_t page_num;
} Mmapped_buffer;

static Mmapped_buffer* MB_new(size_t page_num__);
void MB_delete(Mmapped_buffer* mb);
void MB_concat_resize(Mmapped_buffer* dst, const Mmapped_buffer* src);

// As calculated: "index" has 11581 chars, login has 4549 chars
// I would like to mmap 4 pages for a template, s.t. we could reduce the remmap rate
typedef struct Template_internal {
	struct string_view* frozen_ctx;
	size_t frozen_ctx_len;
	Hash_map* placeholder_map;	// cosnt char* key : string_view value

	Mmapped_buffer* ctx;  // default 4 pages

	Node* combine_recorder;	 // Recording the concatenating relationship of placeholder and
							 // frozen_ctx, each node is a reference of struct string_view
} Template_internal;

// Each Template_internal looks like:
//
//     ctx_page                     ph_page
// |--------------|             |--------------|
// |xxxxxxxxxxxxxx|             |aaaaaaaaaaaaaa|
// |xxxxxyyyyyyyyy|             |aaaaabbbbbbbbb|
// |yyyyyyyyyyyyyy|             |bbbbbbbbbbbbbb|
// |yyyyyyy...    |             |bbbbbbbbbbbbbb|
// |              |             |bbccccc...    |
//        .                             .
//        .                             .
//        .                             .
//
// frozen_ctx[0]->begin points to first byte of "xxxxx..." ctx in ctx_page
// frozen_ctx[0]->end points to the one pass the end of "...xxxxx" ctx in ctx_page
//
// Becaue the frozen_ctx is no needed to searchable, therefore, we use an array to store
// all frozen_ctx pointers.
//
// However, the place holder needs to search quickly. Hence, we use a hash map

static void TI_new__(Template_internal* t);
static struct F_mem open_file(const char* filename);

// Merge nested template_internal together
static Template_internal* TI_merge(const char* filename);
static void TI_merge_node(Template_internal* dst, const Template_internal* src);

// Return NULL if file is not exist
static Template_internal* TI_gen(const char* filename);
static void TI_recorder_gen(Template_internal* t);
static void recorder_insert_head(Template_internal* restrict t,
								 const struct string_view* restrict seg);
static SPair* ph_new(const char* restrict begin, const char* restrict end);

static Hash_map* T_ph_map_gen(const Template_internal* ti);
static const struct string_view* reassociate_recorder(const Template_internal* restrict ti,
													  const Hash_map* ph_map);

// Share those static templates together
typedef struct Template_manager {
	Hash_map* templates;  // Hash map of template internal

} Template_manager;

// Would not be freed
// Expected this manager whould ve collected by OS
static Template_manager* manager = NULL;
static Template_manager* TM_new();
// Suppose the manager is initialized
static void TM_insert(const char* filename);
static Template_internal* TM_find(const char* filename);

// Return NULL if not found, for Templte to return
static Template* TM_get(const char* filename);

struct Template {
	// const members refer to template internal, read only, don't free
	const struct string_view* const frozen_ctx;
	const size_t frozen_ctx_len;
	const struct string_view* const recorder;  // Array of string_view
	const size_t recorder_len;

	Hash_map* ph_map;	 // cosnt char* key : string_view value, value is stored in ph
	Mmapped_buffer* ph;	 // default 2 pages
};

Template* templateNew(const char* filename) {
	if(unlikely(!manager))
		manager = TM_new();

	// Do nothing if it is already exist.
	TM_insert(filename);

	// Return NULL if not found
	return TM_get(filename);
}

void templateDel(Template* t) {
	Hash_map_delete(t->ph_map);
	MB_delete(t->ph);
	free(t);
}

void templateSet(Template* t, const char* key, const char* value) {
	struct string_view* sv = Hash_map_get(t->ph_map, key);

	size_t len = strlen(value);

	if(sv->size >= len) {
		strncpy(sv->begin, value, len);
		return;
	}

	//! Reuire a new area to write
}

char* templateRender(const Template* t) {
	size_t len = 0;
	for(size_t i = 0; i < t->recorder_len; i++)
		len += (t->recorder[i]).size;

	char* ret = malloc(len);

	for(size_t i = 0; i < t->recorder_len; i++) {
		memcpy(ret, t->recorder[i].begin, t->recorder[i].size);
		ret += t->recorder[i].size;
	}

	return ret;
}

static Mmapped_buffer* MB_new(size_t page_num__) {
	Mmapped_buffer* mb = malloc(sizeof(Mmapped_buffer));
	char* beg = mmap(NULL, getpagesize() * page_num__, PROT_READ | PROT_WRITE,
					 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	mb->begin = beg;
	mb->end = beg;
	mb->page_num = page_num__;
}

void MB_delete(Mmapped_buffer* mb) {
	munmap(mb->begin, mb->page_num);
	free(mb);
	mb = NULL;
}

void MB_concat_resize(Mmapped_buffer* dst, const Mmapped_buffer* src) {
	if(src == NULL)
		return;

	size_t used = dst->end - dst->begin;
	size_t to_be_append = src->end - src->begin;
	if(used + to_be_append >= dst->page_num * getpagesize()) {
		char* new_page =
			mremap(dst->begin, dst->page_num * getpagesize(),
				   (dst->page_num + src->page_num) * getpagesize(), MREMAP_MAYMOVE | MREMAP_FIXED);
		// update members
		dst->page_num = dst->page_num + src->page_num;
		dst->end = new_page + (dst->end - dst->begin);
		dst->begin = new_page;
	}
}

static void TI_new__(Template_internal* t) {
	t = malloc(sizeof(Template_internal));

	t->frozen_ctx_len = DEFAULT_FRONEN_ENTRIES;
	t->frozen_ctx = calloc(t->frozen_ctx_len, sizeof(struct string_view));

	t->placeholder_map = Hash_map_new();

	t->ctx = MB_new(DEFAULT_CTX_PAGE_NUM);

	t->combine_recorder = NULL;
}

static struct F_mem open_file(const char* filename) {
	struct F_mem f_mem = { .buf = NULL, .buf_len = 0 };
	// Jail setting, check prefix is "templates", and no "../" in filename
	if(strstr(filename, "../") || strncmp(filename, "templates/", 10))
		goto ret;

	int fd = open(filename, O_RDONLY);
	if(fd == -1)
		goto ret;

	struct stat f_stat;
	int _ = fstat(fd, &f_stat);
	if(_ == -1 || ((f_stat.st_mode & S_IFMT) != S_IFREG))  // Open failed or not regular file
		goto close_fd;

	f_mem.buf = mmap(NULL, f_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	f_mem.buf_len = f_stat.st_size;

close_fd:
	close(fd);
ret:
	return f_mem;
}

// Merge nested template_internal together
static Template_internal* TI_merge(const char* filename) {
	// If it is already parsed, fast pass
	Template_internal* existed_TI = TM_find(filename);
	if(existed_TI)
		return existed_TI;

	Template_internal* t = NULL;
	const struct F_mem fm = open_file(filename);
	if(fm.buf == NULL)
		return NULL;
	struct F_mem fm_copy = fm;

	TI_new__(t);

	char* nested = find_first_of(fm_copy.buf, "{%%include ");
	while(nested != NULL) {
		memcpy(t->ctx->end, fm_copy.buf, nested - fm_copy.buf);
		// Move t's ctx end to the begin of the place which is going to be overwittern
		t->ctx->end += nested - fm_copy.buf;

		char* filename_end = find_first_of(nested + 1, "%%}");
		char* filename_begin = find_first_of(nested + 1, " ") + 1;	// The next char after the SPACE
		size_t filename_len = filename_end - filename_begin;
		char* inner_filename[filename_len];	 // VLA
		memcpy(inner_filename, filename_begin, filename_len);

		Template_internal* inner_TI = TI_merge(inner_filename);

		// Merge
		TI_merge_node(t, inner_TI);

		fm_copy.buf_len -= filename_end + 2 - fm_copy.buf;
		fm_copy.buf = filename_end + 2;
		nested = find_first_of(fm_copy.buf, "{%%include ");
	}

	// Concatnate remain fm_copy
	memcpy(t->ctx->end, fm_copy.buf, fm_copy.buf_len);
	t->ctx->end += fm_copy.buf_len;

	// Close fm
	munmap(fm.buf, fm.buf_len);

	if(t != NULL) {
		SPair* p = malloc(sizeof(SPair));
		p->key = strdup(filename);
		p->value = t;
		Hash_map_insert_move(manager->templates, p);
	}
	return t;
}

// Concatenate ctx, in the same time, ph is not ready, we have nothing to do with ph
static void TI_merge_node(Template_internal* dst, const Template_internal* src) {
	if(src == NULL)
		return;

	// merge ctx
	// Resize if it needs
	MB_concat_resize(dst->ctx, src->ctx);
	memcpy(dst->ctx->end, src->ctx->begin, src->ctx->end - src->ctx->begin);
	dst->ctx->end += src->ctx->end - src->ctx->begin;  // Move to tail
}

// Return NULL if file is not exist
static Template_internal* TI_gen(const char* filename) {
	// Recursive include file
	Template_internal* t = TI_merge(filename);
	if(t = NULL)
		return NULL;

	// Mark down placeholders to record and placeholder_map
	// Chain all ctx an ph_map into combine_recorder
	TI_recorder_gen(t);

	return t;
}

static Template_manager* TM_new() {
	Template_manager* m = malloc(sizeof(Template_manager));
	m->templates = Hash_map_new();
	return m;
}

// Suppose the manager is initialized
static void TM_insert(const char* filename) {
	Template_internal* t = TM_find(filename);
	if(t)  // Already have the template
		return;

	// Not found in manager, t = NULL
	t = TI_gen(filename);
	if(!t)
		return NULL;  // File not exist

	SPair* p = malloc(sizeof(SPair));
	p->key = strdup(filename);
	p->value = t;
	Hash_map_insert_move(manager->templates, p);
}

static Template_internal* TM_find(const char* filename) {
	return Hash_map_get(manager->templates, filename);
}

// Return NULL if not found
static Template* TM_get(const char* filename) {
	const Template_internal* ti = TM_find(filename);
	if(ti == NULL)
		return NULL;

	Template* t = malloc(sizeof(Template));

	CONST_INIT(t->frozen_ctx, ti->frozen_ctx);
	CONST_INIT(t->frozen_ctx_len, ti->frozen_ctx_len);

	t->ph = MB_new(DEFAULT_PH_PAGE_NUM);
	t->ph_map = T_ph_map_gen(ti);

	// Re Associate recorder
	const struct string_view* new_rec = reassociate_recorder(ti, t->ph_map);
	CONST_INIT(t->recorder, new_rec);

	size_t rec_len = 0;
	for(Node* cur = ti->combine_recorder; cur; cur = cur->next)
		rec_len++;
	CONST_INIT(t->recorder_len, rec_len);

	return t;
}

#ifdef DEBUG
static void MB_print(const Mmapped_buffer* mb) {
	if(!mb)
		return;

	for(char* iter = mb->begin; iter != mb->end; iter++)
		fprintf(stderr, "%c", *iter);
}
#endif

static void TI_recorder_gen(Template_internal* t) {
	char* buf = t->ctx->begin;
	while(1) {
		const char* ph_begin = find_first_of(buf, "{");

		// Copy frozen ctx
		const struct string_view ctx_seg = {
			.begin = buf,
			.end = (ph_begin) ? ph_begin : t->ctx->end,
			.size = (ph_begin) ? (ph_begin - buf) : (t->ctx->end - buf),
		};
		recorder_insert_head(t, &ctx_seg);
		buf += ctx_seg.size;  // Nove to ctx_seg.end

		if(ph_begin) {	// Have a placeholder
			const char* ph_end = find_first_of(buf, "}");
			// Gen pat and move to recorder
			SPair* ph = ph_new(ph_begin, ph_end);  // char* key : struct string_view*
			const struct string_view* ph_value = ph->value;
			t->placeholder_map = Hash_map_insert_move(t->placeholder_map, ph);

			// Must refer to ph's vlaue
			recorder_insert_head(t, ph_value);

			// Move buf to pat's end
			buf = ph_end + 1;
		}

		if(ph_begin == NULL)
			break;
	}
}

static void recorder_insert_head(Template_internal* restrict t,
								 const struct string_view* restrict seg) {
	// Suppose those are not NULL ptr
	t->combine_recorder = insert(t->combine_recorder, sizeof(struct string_view), seg);
}

// bagin points to '{', and end points to '}'
// dynamic langth placeholder should not be duplicated
static SPair* ph_new(const char* restrict begin, const char* restrict end) {
	char* key = NULL;

	struct string_view* value = malloc(sizeof(struct string_view));
	value->begin = "";	// this sv must movable
	value->end = "";
	CONST_INIT(value->size, (size_t)0);

	const char type = begin[1];
	if(type == '%') {  // "when" specifier
		// Suppose key and value are static immutable data
		//
		// @key will store strdup("<key> <expected_value>") and
		// @value will store string_view_new("<to be showed>")
		const char* key_start = begin + 7;							// strlen("{%when ") + 1
		const char* major_key_end = find_first_of(key_start, " ");	// start from expexcted value
		const char* key_end = find_first_of(major_key_end + 1, " ");
		size_t len = key_end - key_start;

		key = malloc(len + 1);	// Add the '\0'
		strncpy(key, key_start, len);
		CONST_INIT(value->size, len);

		// const char* expected_value = find_first_of(key, " ") + 1;
		value->begin = key_end + 1;
		value->end = end - 1;  // "%}" point to '%'
		size_t val_lan = value->end - value->begin;
		CONST_INIT(value->size, val_lan);
	}
	else if(type == '{') {	// dynamic length
		size_t len = end - (begin + 2);
		key = malloc(len + 1);	// "{{" and Add the '\0'
		strncpy(key, begin + 2, len);
	}

	SPair* p = malloc(sizeof(SPair));
	p->key = key;
	p->value = value;

	return p;
}