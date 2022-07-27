#define _DEFAULT_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "bs.h"
#include "db.h"
#include "facebooc_server.h"
#include "http/cookies.h"
#include "models/account.h"
#include "models/connection.h"
#include "models/like.h"
#include "models/post.h"
#include "models/session.h"
#include "server.h"
#include "template.h"
#include "utility.h"

// Facebooc is an directive class of Server
struct Facebooc {
	Server* server;
};

struct Buf_size {
	void* addr;
	size_t len;
};

static Response* home(Request*);
static Response* dashboard(Request*);
static Response* profile(Request*);
static Response* post(Request*);
static Response* like(Request*);
static Response* unlike(Request*);
static Response* connect(Request*);
static Response* search(Request*);
static Response* login(Request*);
static Response* logout(Request*);
static Response* signup(Request*);
static Response* static_handler(Request* req);
static Response* notFound(Request*);  // default route

static inline int get_id(const char* uri);
static const Account* get_account(const Cookies* c);

Facebooc* FB_new(const uint16_t port) {
	Facebooc* s = malloc(sizeof(Facebooc));
	s->server = serverNew(port);
	serverAddHandler(s->server, "signup", signup);
	serverAddHandler(s->server, "logout", logout);
	serverAddHandler(s->server, "login", login);
	serverAddHandler(s->server, "search", search);
	serverAddHandler(s->server, "connect", connect);
	serverAddHandler(s->server, "like", like);
	serverAddHandler(s->server, "unlike", unlike);
	serverAddHandler(s->server, "post", post);
	serverAddHandler(s->server, "profile", profile);
	serverAddHandler(s->server, "dashboard", dashboard);
	serverAddHandler(s->server, "", home);
	serverAddHandler(s->server, "static", static_handler);

	set_callback(s->server, notFound);

	initDB();

	return s;
}

int FB_run(Facebooc* s) {
	serverServe(s->server);
	return 1;
}

void FB_delete(Facebooc* s) {
	if(s->server)
		serverDel(s->server);
	void db_close();
	free(s);
	s = NULL;
}

static inline int invalid(Template* template, const char* key, const char* value) {
	templateSet(template, key, value);
	return false;
}

// compare str1 and str2 from tail
static inline int rev_cmp(const char* str1, size_t len1, const char* str2, size_t len2) {
	const long smaller = min(len1, len2);
	return strncmp(str1 + len1 - smaller, str2 + len2 - smaller, smaller);
}

static inline const char* mimeType_mux(const char* path) {
	static const char* file_type[] = {
		"html", "json", "jpeg", "jpg", "gif", "png", "css", "js",
	};
	static const size_t file_type_len = ARR_LEN(file_type);

	static const char* mime_type[] = {
		"text/html", "application/json",	   "image/jpeg", "image/jpeg", "image/gif", "image/png",
		"text/css",	 "application/javascript", "text/plain",
	};

	size_t i = 0;
	for(; i < file_type_len; i++) {
		if(rev_cmp(path, strlen(path), file_type[i], strlen(file_type[i])) == 0)
			break;
	}

	return mime_type[i];
}

// Each entry of this Hash_map is referenced by mmap
// Don't use free to munmap it
// Also, not expected to munmap it
static Hash_map* static_file_table;

static struct Buf_size get_static_file(const char* filename) {
	struct Buf_size fm = { .addr = NULL, .len = 0 };
	// Check if it is already in the static_file_table
	const struct Buf_size* entry = Hash_map_get(static_file_table, filename);
	if(entry) {	 // Found
		fm.addr = entry->addr;
		fm.len = entry->len;
		goto found;
	}

	// EXIT ON DIRS
	struct stat stat_file;
	if(stat(filename, &stat_file) < 0 || S_ISDIR(stat_file.st_mode))
		goto fail;

	// EXIT ON NOT FOUND
	int file = open(filename, O_RDONLY);
	if(unlikely(file == -1)) {
		// Not found or error
		fprintf(stderr, "%s not found\n", filename);
		goto fail;
	};

	// Init local fm
	fm.addr = mmap(NULL, stat_file.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	fm.len = stat_file.st_size;
	madvise(fm.addr, stat_file.st_size, MADV_SEQUENTIAL | MADV_WILLNEED | MADV_DONTFORK);
	close(file);

	// Dup a fm into static_file_table
	struct Buf_size* copy_fm = malloc(sizeof(struct Buf_size));
	copy_fm->addr = fm.addr;
	copy_fm->len = fm.len;
	SPair p = { .key = (char*)filename, .value = copy_fm };
	Hash_map_insert_move(static_file_table, &p);

fail:
found:
	return fm;
}

static Response* static_handler(Request* req) {
	// EXIT ON SHENANIGANS
	if(strstr(req->uri, "../"))
		return NULL;

	const char* filename = req->uri + 1;

	// Prepare static_file_table
	if(unlikely(!static_file_table))
		static_file_table = Hash_map_new();

	struct Buf_size fm = get_static_file(filename);
	if(fm.addr == NULL)	 // file not found
		return NULL;

	Response* response = responseNew();
	responseSetBody_data(response, fm.addr, fm.len);

	// MIME TYPE
	// It is impossible to not found any matched mimetype, because we had check static file is exist
	const char* mimeType = mimeType_mux(req->uri);
	char content_len[11];
	snprintf(content_len, 11, "%ld", fm.len);

	// RESPOND
	responseSetStatus(response, OK);
	responseAddHeader(response, eXpire_pair("Content-Type", (char*)mimeType, SSPair));
	responseAddHeader(response, eXpire_pair("Content-Length", content_len, SSPair));
	responseAddHeader(response, eXpire_pair("Cache-Control", "max-age=2592000", SSPair));
	return response;
}

// Get sid string from cookies
static const Account* get_account(const Cookies* c) {
	// TODO: Use an object pool to reduce malloc times
	if(unlikely(c == NULL))
		return NULL;
	Cookie* cookie = Cookies_get(c, "sid");
	return accountGetBySId(get_db(), Cookie_get_attr(cookie, VALUE));
}

static Response* home(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	if(my_acc) {
		accountDel((Account*)my_acc);
		return responseNewRedirect("/dashboard/");
	}

	Response* response = responseNew();
	Template* template = templateNew("templates/index.html");
	responseSetStatus(response, OK);
	templateSet(template, "active", "home");
	templateSet(template, "subtitle", "Home");
	responseSetBody(response, templateRender(template));
	templateDel(template);
	return response;
}

static Response* dashboard(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	if(unlikely(my_acc == NULL))
		return NULL;

	Posts* posts = postGetLatestGraph(get_db(), my_acc->id, 0);
	char* res = NULL;
	if(!Posts_is_empty(posts))
		res = bsNew("<ul class=\"posts\">");

	List* cur = NULL;
	list_for_each(cur, &posts->list) {
		Post* p = container_of(cur, Posts, list)->p;

		Account* account = accountGetById(get_db(), p->authorId);
		const size_t acc_len = strlen(account->name);
		const size_t post_len = bsGetLen(p->body);
		bool liked = likeLiked(get_db(), my_acc->id, p->id);
		char* bbuff = bsNewLen("", bsGetLen(p->body) + 256);
		snprintf(bbuff, 86 + acc_len + post_len,
				 "<li class=\"post-item\">"
				 "<div class=\"post-author\">%s</div>"
				 "<div class=\"post-content\">"
				 "%s"
				 "</div>",
				 account->name, p->body);
		accountDel(account);
		bsLCat(&res, bbuff);

		char sbuff[128];
		if(liked) {
			snprintf(sbuff, 55, "<a class=\"btn\" href=\"/unlike/%d/\">Liked</a> - ", p->id);
			bsLCat(&res, sbuff);
		}
		else {
			snprintf(sbuff, 52, "<a class=\"btn\" href=\"/like/%d/\">Like</a> - ", p->id);
			bsLCat(&res, sbuff);
		}

		time_t t = p->createdAt;
		struct tm* info = gmtime(&t);
		info->tm_hour = info->tm_hour + 8;
		strftime(sbuff, 128, "%c GMT+8", info);
		bsLCat(&res, sbuff);
		bsLCat(&res, "</li>");

		bsDel(bbuff);
	}
	Posts_delete(posts);

	Template* template = templateNew("templates/dashboard.html");
	if(res) {
		bsLCat(&res, "</ul>");
		templateSet(template, "graph", res);
		bsDel(res);
	}
	else {
		templateSet(template, "graph",
					"<ul class=\"posts\"><div class=\"not-found\">Nothing "
					"here.</div></ul>");
	}

	templateSet(template, "active", "dashboard");
	templateSet(template, "loggedIn", "t");
	templateSet(template, "subtitle", "Dashboard");
	templateSet(template, "accountName", my_acc->name);
	Response* response = responseNew();
	responseSetStatus(response, OK);
	responseSetBody(response, templateRender(template));
	templateDel(template);
	accountDel((Account*)my_acc);
	return response;
}

// Preview someone's profile page
static Response* profile(Request* req) {
	// Check I'm logged-in
	const Account* my_acc = get_account(req->cookies);
	if(unlikely(my_acc == NULL))
		return NULL;

	const Account* acc2 = accountGetById(get_db(), get_id(req->uri));

	if(!acc2) {
		accountDel((Account*)my_acc);
		return NULL;
	}
	if(acc2->id == my_acc->id) {
		accountDel((Account*)my_acc);
		accountDel((Account*)acc2);
		return responseNewRedirect("/dashboard/");
	}

	// Max length of uid is len(INT_MAX), in other words, the maxlen of uid is 10
	char acc2_id_str[11] = { 0 };
	snprintf(acc2_id_str, 11, "%d", acc2->id);

	Connection* connection = connectionGetByAccountIds(get_db(), my_acc->id, acc2->id);
	char connectStr[512] = { 0 };
	if(connection) {
		snprintf(connectStr, 26 + strlen(acc2->name), "You and %s are connected!", acc2->name);
	}
	else {
		const size_t name_len = strlen(acc2->name);
		snprintf(connectStr, 76 + name_len + 10,
				 "You and %s are not connected."
				 " <a href=\"/connect/%d/\">Click here</a> to connect!",
				 acc2->name, acc2->id);
	}
	connectionDel(connection);

	Posts* posts = postGetLatest(get_db(), acc2->id, 0);

	char* res = NULL;
	if(!Posts_is_empty(posts))
		res = bsNew("<ul class=\"posts\">");

	List* cur = NULL;
	list_for_each(cur, &posts->list) {
		Post* p = container_of(cur, Posts, list)->p;

		const size_t body_len = bsGetLen(p->body);
		char* bbuff = bsNewLen("", body_len + 256);
		snprintf(bbuff, 54 + body_len,
				 "<li class=\"post-item\"><div class=\"post-author\">%s</div>", p->body);
		bsLCat(&res, bbuff);

		char sbuff[128];
		bool liked = likeLiked(get_db(), my_acc->id, p->id);
		if(liked) {
			bsLCat(&res, "Liked - ");
		}
		else {
			snprintf(sbuff, 52, "<a class=\"btn\" href=\"/like/%d/\">Like</a> - ", p->id);
			bsLCat(&res, sbuff);
		}

		time_t t__ = p->createdAt;
		strftime(sbuff, 128, "%c GMT", gmtime(&t__));
		bsLCat(&res, sbuff);
		bsLCat(&res, "</li>");

		bsDel(bbuff);
	}
	Posts_delete(posts);

	Template* template = templateNew("templates/profile.html");
	if(res) {
		bsLCat(&res, "</ul>");
		templateSet(template, "profilePosts", res);
		bsDel(res);
	}
	else {
		templateSet(template, "profilePosts",
					"<h4 class=\"not-found\">This person has not posted "
					"anything yet!</h4>");
	}

	templateSet(template, "active", "profile");
	templateSet(template, "loggedIn", "t");
	templateSet(template, "subtitle", acc2->name);
	templateSet(template, "profileId", acc2_id_str);
	templateSet(template, "profileName", acc2->name);
	templateSet(template, "profileEmail", acc2->email);
	templateSet(template, "profileConnect", connectStr);
	templateSet(template, "accountName", my_acc->name);
	Response* response = responseNew();
	responseSetStatus(response, OK);
	responseSetBody(response, templateRender(template));
	accountDel((Account*)my_acc);
	accountDel((Account*)acc2);
	templateDel(template);
	return response;
}

// I posted a post via HTTP POST
static Response* post(Request* req) {
	const Account* acc = get_account(req->cookies);
	if(unlikely(acc == NULL))
		goto fail;

	if(unlikely(req->method != POST))
		goto fail;

	const char* postStr = body_get(req->postBody, "post");

	if(strlen(postStr) == 0)
		goto fail;
	else if(strlen(postStr) < MAX_BODY_LEN)
		postDel(postCreate(get_db(), acc->id, postStr));

fail:
	accountDel((Account*)acc);
	return responseNewRedirect("/dashboard/");
}

static Response* unlike(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	char redir_to[32] = { "/dashboard/" };
	if(unlikely(my_acc == NULL))
		goto fail;

	const Post* post = postGetById(get_db(), get_id(req->uri));
	if(!post)
		goto fail;

	likeDel(likeDelete(get_db(), my_acc->id, post->authorId, post->id));

	if(query_get(req->queryString, "r"))
		snprintf(redir_to, 32, "/profile/%d/", post->authorId);

	postDel((Post*)post);

fail:
	accountDel((Account*)my_acc);
	return responseNewRedirect(redir_to);
}

static Response* like(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	char redir_to[32] = { "/dashboard/" };
	if(unlikely(my_acc == NULL))
		goto fail;

	const Post* post = postGetById(get_db(), get_id(req->uri));
	if(!post)
		goto fail;

	likeDel(likeCreate(get_db(), my_acc->id, post->authorId, post->id));

	if(query_get(req->queryString, "r"))
		snprintf(redir_to, 32, "/profile/%d/", post->authorId);

	postDel((Post*)post);

fail:
	accountDel((Account*)my_acc);
	return responseNewRedirect(redir_to);
}

static Response* connect(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	char redir_to[32] = { "/dashboard/" };
	if(unlikely(my_acc == NULL))
		goto fail;

	const Account* account = accountGetById(get_db(), get_id(req->uri));
	if(!account)
		goto fail;

	connectionDel(connectionCreate(get_db(), my_acc->id, account->id));

	snprintf(redir_to, 32, "/profile/%d/", account->id);

	accountDel((Account*)account);

fail:
	accountDel((Account*)my_acc);
	return responseNewRedirect(redir_to);
}

static Response* search(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	if(unlikely(my_acc == NULL))
		return responseNewRedirect("/login/");
	accountDel((Account*)my_acc);

	const char* query = query_get(req->queryString, "q");

	if(!query)
		return NULL;

	char* res = NULL;
	Accounts* as = accountSearch(get_db(), query, 0);
	if(!accounts_is_empty(as))
		res = bsNew("<ul class=\"search-results\">");

	List* cur = NULL;
	list_for_each(cur, &as->list) {
		Account* a = container_of(cur, Accounts, list)->a;
		const size_t name_len = strlen(a->name);
		const size_t email_len = strlen(a->email);

		char sbuff[1024];
		snprintf(sbuff, 62 + name_len + email_len,
				 "<li><a href=\"/profile/%d/\">%s</a> (<span>%s</span>)</li>\n", a->id, a->name,
				 a->email);
		bsLCat(&res, sbuff);
	}
	accounts_delete(as);

	if(res)
		bsLCat(&res, "</ul>");

	Response* response = responseNew();
	Template* template = templateNew("templates/search.html");
	responseSetStatus(response, OK);

	if(!res) {
		templateSet(template, "results",
					"<h4 class=\"not-found\">There were no results "
					"for your query.</h4>");
	}
	else {
		templateSet(template, "results", res);
		bsDel(res);
	}

	templateSet(template, "searchQuery", query);
	templateSet(template, "active", "search");
	templateSet(template, "loggedIn", "t");
	templateSet(template, "subtitle", "Search");
	responseSetBody(response, templateRender(template));
	templateDel(template);
	return response;
}

static Response* login(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	if(unlikely(my_acc != NULL)) {	// It's not usually logined
		accountDel((Account*)my_acc);
		return responseNewRedirect("/dashboard/");
	}

	Response* response = responseNew();
	Template* template = templateNew("templates/login.html");
	responseSetStatus(response, OK);
	templateSet(template, "active", "login");
	templateSet(template, "subtitle", "Login");

	if(req->method == POST) {
		const char* username = body_get(req->postBody, "username");
		const char* password = body_get(req->postBody, "password");

		if(!username) {
			invalid(template, "usernameError", "Username missing!");
		}
		else {
			templateSet(template, "formUsername", username);
		}

		if(!password) {
			invalid(template, "passwordError", "Password missing!");
		}

		bool valid = account_auth(get_db(), username, password);

		if(valid) {
			Session* session = sessionCreate(get_db(), username, password);
			if(session) {
				char expire_string[64];
				Cookie_gen_expire(expire_string, 3600 * 24 * 30);

				// Setting cookie
				Cookie* cookie = Cookie_init(eXpire_pair("sid", session->sessionId, SSPair));
				Cookie_set_attr(cookie, EXPIRE, expire_string);
				responseAddCookie(response, cookie);

				sessionDel(session);
				Cookie_delete(cookie);

				responseSetStatus(response, FOUND);
				responseAddHeader(response, eXpire_pair("Location", "/dashboard/", SSPair));
				goto ret;
			}
			else {
				invalid(template, "usernameError", "Invalid username or password.");
			}
		}
		else {
			invalid(template, "usernameError", "Invalid username or password.");
		}
	}

	responseSetBody(response, templateRender(template));

ret:
	templateDel(template);
	return response;
}

static Response* logout(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	if(unlikely(!my_acc))  // It's usually logined
		return responseNewRedirect("/");

	accountDel((Account*)my_acc);

	Response* response = responseNewRedirect("/");

	Cookie* cookie = Cookie_init(eXpire_pair("sid", "", SSPair));
	char buf[64];
	Cookie_gen_expire(buf, -1);
	Cookie_set_attr(cookie, EXPIRE, buf);
	responseAddCookie(response, cookie);  // Copy
	Cookie_delete(cookie);				  // delete

	return response;
}

static Response* signup(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	if(unlikely(my_acc != NULL)) {
		// If you already logined, you should not regist
		accountDel((Account*)my_acc);
		return responseNewRedirect("/dashboard/");
	}

	Response* response = responseNew();
	Template* template = templateNew("templates/signup.html");
	templateSet(template, "active", "signup");
	templateSet(template, "subtitle", "Sign Up");
	responseSetStatus(response, OK);

	if(req->method == POST) {
		bool valid = true;
		const char* name = body_get(req->postBody, "name");
		const char* email = body_get(req->postBody, "email");
		const char* username = body_get(req->postBody, "username");
		const char* password = body_get(req->postBody, "password");
		const char* confirmPassword = body_get(req->postBody, "confirm-password");

		if(!name) {
			invalid(template, "nameError", "You must enter your name!");
		}
		else if(strlen(name) < 5 || strlen(name) > 50) {
			invalid(template, "nameError", "Your name must be between 5 and 50 characters long.");
		}
		else {
			templateSet(template, "formName", name);
		}

		if(!email) {
			invalid(template, "emailError", "You must enter an email!");
		}
		else if(strchr(email, '@') == NULL) {
			invalid(template, "emailError", "Invalid email.");
		}
		else if(strlen(email) < 3 || strlen(email) > 50) {
			invalid(template, "emailError", "Your email must be between 3 and 50 characters long.");
		}
		else if(!accountCheckEmail(get_db(), email)) {
			invalid(template, "emailError", "This email is taken.");
		}
		else {
			templateSet(template, "formEmail", email);
		}

		if(!username) {
			invalid(template, "usernameError", "You must enter a username!");
		}
		else if(strlen(username) < 3 || strlen(username) > 50) {
			invalid(template, "usernameError",
					"Your username must be between 3 and 50 characters long.");
		}
		else if(!accountCheckUsername(get_db(), username)) {
			invalid(template, "usernameError", "This username is taken.");
		}
		else {
			templateSet(template, "formUsername", username);
		}

		if(!password) {
			invalid(template, "passwordError", "You must enter a password!");
		}
		else if(strlen(password) < 8) {
			invalid(template, "passwordError", "Your password must be at least 8 characters long!");
		}

		if(!confirmPassword) {
			invalid(template, "confirmPasswordError", "You must confirm your password.");
		}
		else if(strcmp(password, confirmPassword) != 0) {
			invalid(template, "confirmPasswordError", "The two passwords must be the same.");
		}

		if(valid) {
			Account* account = accountCreate(get_db(), name, email, username, password);

			if(account) {
				accountDel(account);

				responseSetStatus(response, FOUND);
				responseAddHeader(response, eXpire_pair("Location", "/login/", SSPair));
				goto ret;
			}
			else {
				invalid(template, "nameError", "Unexpected error. Please try again later.");
			}
		}
	}

	responseSetBody(response, templateRender(template));
ret:
	templateDel(template);
	return response;
}

static Response* notFound(Request* req) {
	const Account* my_acc = get_account(req->cookies);
	const int is_loggedIn = !!my_acc;
	accountDel((Account*)my_acc);

	Response* response = responseNew();
	Template* template = templateNew("templates/404.html");
	templateSet(template, "loggedIn", is_loggedIn ? "t" : "");
	templateSet(template, "subtitle", "404 Not Found");
	responseSetStatus(response, NOT_FOUND);
	responseSetBody(response, templateRender(template));
	templateDel(template);
	return response;
}

static inline int get_id(const char* uri) {
	// format: "/%s/<id>/"

	char* begin = strchr(uri + 1, '/') + 1;
	char* end = strchr(begin, '/');
	const size_t len = end - begin;

	char new_str[10] = { 0 };
	memcpy(new_str, begin, len);

	return atoi(new_str);
}
