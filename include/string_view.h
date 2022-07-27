#ifndef __STRING_VIEW_H__
#define __STRING_VIEW_H__

// Simulate C++'s string_view
typedef struct string_view {
	const char* begin;
	const char* end;
	const long size;
} string_view;

string_view string_view_ctor(const char* buf, const char* delim);
#ifdef DEBUG
void string_view_show(const string_view* const);
#endif

char* string_view_dup(const string_view* const sv);

#endif