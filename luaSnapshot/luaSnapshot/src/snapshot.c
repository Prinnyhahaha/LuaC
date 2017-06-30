#include <lua.h>
#include <lauxlib.h>


#if FALSE
static void mark_object(lua_State *L, lua_State *dL, const void * parent, const char * desc);

#if LUA_VERSION_NUM == 501

/** 保证只在主线程内加载该库
*/
static void
luaL_checkversion(lua_State *L) {
	if (lua_pushthread(L) == 0) {
		luaL_error(L, "Must require in main thread");
	}
	lua_setfield(L, LUA_REGISTRYINDEX, "mainthread");
}

/** t[p] = v 其中v是栈顶对象，结束后出栈
	@param idx t所在的位置
	@param p 插入的key
*/
static void
lua_rawsetp(lua_State *L, int idx, const void *p) {
	if (idx < 0) {
		idx += lua_gettop(L) + 1;
	}
	lua_pushlightuserdata(L, (void *)p);
	lua_insert(L, -2);
	lua_rawset(L, idx);
}

/** 入栈t[p]
	@param idx t所在的位置
	@param p 查询key
*/
static void
lua_rawgetp(lua_State *L, int idx, const void *p) {
	if (idx < 0) {
		idx += lua_gettop(L) + 1;
	}
	lua_pushlightuserdata(L, (void *)p);
	lua_rawget(L, idx);
}

/** 入栈userdata的值，一张table
	@param idx userdata所在的位置
*/
static void
lua_getuservalue(lua_State *L, int idx) {
	lua_getfenv(L, idx);
}

static void
mark_function_env(lua_State *L, lua_State *dL, const void * t) {
	lua_getfenv(L,-1);
	if (lua_istable(L,-1)) {
		mark_object(L, dL, t, "[environment]");
	} else {
		lua_pop(L,1);
	}
}

#else

#define mark_function_env(L,dL,t)

#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TABLE 1
#define FUNCTION 2
#define SOURCE 3
#define THREAD 4
#define USERDATA 5
//MARK表防止重复引用
#define MARK 6

/** 查询指定裸指针是否已经被标记过，防止对同一内存重复标记
	@param p 要查询的裸指针
*/
static bool
ismarked(lua_State *dL, const void *p) {
	lua_rawgetp(dL, MARK, p);
	if (lua_isnil(dL,-1)) {
		lua_pop(dL,1);
		lua_pushboolean(dL,1);
		lua_rawsetp(dL, MARK, p);
		return false;
	}
	lua_pop(dL,1);
	return true;
}

/** 将一个裸指针为key，一张表为value，加入制定类型的表中，记录其父对象的地址和描述
	如果不是table, function, thread, userdata类型的或是已经被标记过则返回NULL，否则返回栈顶对象的地址
	@param parent 父对象地址
	@param desc 父对象描述
*/
static const void *
readobject(lua_State *L, lua_State *dL, const void *parent, const char *desc) {
	int t = lua_type(L, -1);
	int tidx = 0;
	switch (t) {
	case LUA_TTABLE:
		tidx = TABLE;
		break;
	case LUA_TFUNCTION:
		tidx = FUNCTION;
		break;
	case LUA_TTHREAD:
		tidx = THREAD;
		break;
	case LUA_TUSERDATA:
		tidx = USERDATA;
		break;
	default:
		return NULL;
	}

	const void * p = lua_topointer(L, -1);
	if (ismarked(dL, p)) {
		lua_rawgetp(dL, tidx, p);
		if (!lua_isnil(dL,-1)) {
			lua_pushstring(dL,desc);
			lua_rawsetp(dL, -2, parent);
		}
		lua_pop(dL,1);
		lua_pop(L,1);
		return NULL;
	}

	lua_newtable(dL);
	lua_pushstring(dL,desc);
	lua_rawsetp(dL, -2, parent);
	lua_rawsetp(dL, tidx, p);

	return p;
}

/** 构造key的描述字符串
	@param index key在lua栈中的位置
	@param buffer 返回的字符串
*/
static const char *
keystring(lua_State *L, int index, char * buffer) {
	int t = lua_type(L,index);
	switch (t) {
	case LUA_TSTRING:
		return lua_tostring(L,index);
	case LUA_TNUMBER:
		sprintf(buffer,"[%lg]",lua_tonumber(L,index));
		break;
	case LUA_TBOOLEAN:
		sprintf(buffer,"[%s]",lua_toboolean(L,index) ? "true" : "false");
		break;
	case LUA_TNIL:
		sprintf(buffer,"[nil]");
		break;
	default:
		sprintf(buffer,"[%s:%p]",lua_typename(L,t),lua_topointer(L,index));
		break;
	}
	return buffer;
}

/** 标记一个table
	@param parent 父对象地址
	@param desc 父对象描述
*/
static void
mark_table(lua_State *L, lua_State *dL, const void * parent, const char * desc) {
	const void * t = readobject(L, dL, parent, desc);
	if (t == NULL)
		return;

	/* 检查是否存在metatable，以及key和value是否为弱引用 */
	bool weakk = false;
	bool weakv = false;
	if (lua_getmetatable(L, -1)) {
		lua_pushliteral(L, "__mode");
		lua_rawget(L, -2);
		if (lua_isstring(L,-1)) {
			const char *mode = lua_tostring(L, -1);
			if (strchr(mode, 'k')) {
				weakk = true;
			}
			if (strchr(mode, 'v')) {
				weakv = true;
			}
		}
		lua_pop(L,1);

		luaL_checkstack(L, LUA_MINSTACK, NULL);
		mark_table(L, dL, t, "[metatable]");
	}

	/* 遍历整张表，忽略弱引用value，标记强引用value并用key的对应字符串描述该value，标记强引用key */
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		if (weakv) {
			lua_pop(L,1);
		} else {
			char temp[32];
			const char * desc = keystring(L, -2, temp);
			mark_object(L, dL, t , desc);
		}
		if (!weakk) {
			lua_pushvalue(L,-1);
			mark_object(L, dL, t , "[key]");
		}
	}

	lua_pop(L,1);
}

/** 标记一个userdata
	@param parent 父对象地址
	@param desc 父对象描述
*/
static void
mark_userdata(lua_State *L, lua_State *dL, const void * parent, const char *desc) {
	const void * t = readobject(L, dL, parent, desc);
	if (t == NULL)
		return;
	/* 检查并标记metatable */
	if (lua_getmetatable(L, -1)) {
		mark_table(L, dL, t, "[metatable]");
	}

	/* 认为uservalue是个table */
	lua_getuservalue(L,-1);
	if (lua_isnil(L,-1)) {
		lua_pop(L,2);
	} else {
		mark_table(L, dL, t, "[uservalue]");
		lua_pop(L,1);
	}
}

/** 标记一个function
	@param parent 父对象地址
	@param desc 父对象描述
*/
static void
mark_function(lua_State *L, lua_State *dL, const void * parent, const char *desc) {
	const void * t = readobject(L, dL, parent, desc);
	if (t == NULL)
		return;

	/* lua5.1 存在函数环境表 */
	mark_function_env(L,dL,t);
	/* 闭包变量 */
	int i;
	for (i=1;;i++) {
		const char *name = lua_getupvalue(L,-1,i);
		if (name == NULL)
			break;
		/* C函数用空串代表名字，改成[upvalue]作为名字，Lua函数为变量名 */
		mark_object(L, dL, t, name[0] ? name : "[upvalue]");
	}
	/* 轻量级C函数不计入函数统计，Lua函数记录其创建的chunk和行数，记录在SOURCE表中 */
	if (lua_iscfunction(L,-1)) {
		if (i==1) {
			// light c function
			lua_pushnil(dL);
			lua_rawsetp(dL, FUNCTION, t);
		}
		lua_pop(L,1);
	} else {
		lua_Debug ar;
		lua_getinfo(L, ">S", &ar);
		luaL_Buffer b;
		luaL_buffinit(dL, &b);
		luaL_addstring(&b, ar.short_src);
		char tmp[16];
		sprintf(tmp,":%d",ar.linedefined);
		luaL_addstring(&b, tmp);
		luaL_pushresult(&b);
		lua_rawsetp(dL, SOURCE, t);
	}
}

/** 标记一个thread
	@param parent 父对象地址
	@param desc 父对象描述
*/
static void
mark_thread(lua_State *L, lua_State *dL, const void * parent, const char *desc) {
	const void * t = readobject(L, dL, parent, desc);
	if (t == NULL)
		return;
	int level = 0;
	lua_State *cL = lua_tothread(L,-1);
	if (cL == L) {
		/* 主线程level1 */
		level = 1;
	} else {
		/* 子线程标记栈内元素 */
		int top = lua_gettop(cL);
		luaL_checkstack(cL, 1, NULL);
		int i;
		char tmp[16];
		for (i=0;i<top;i++) {
			lua_pushvalue(cL, i+1);
			sprintf(tmp, "[%d]", i+1);
			mark_object(cL, dL, cL, tmp);
		}
	}
	/* 循环标记每层调用的局部变量 */
	lua_Debug ar;
	luaL_Buffer b;
	luaL_buffinit(dL, &b);
	while (lua_getstack(cL, level, &ar)) {
		char tmp[128];
		lua_getinfo(cL, "Sl", &ar);
		luaL_addstring(&b, ar.short_src);
		if (ar.currentline >=0) {
			char tmp[16];
			sprintf(tmp,":%d ",ar.currentline);
			luaL_addstring(&b, tmp);
		}

		int i,j;
		for (j=1;j>-1;j-=2) {
			for (i=j;;i+=j) {
				const char * name = lua_getlocal(cL, &ar, i);
				if (name == NULL)
					break;
				snprintf(tmp, sizeof(tmp), "%s : %s:%d",name,ar.short_src,ar.currentline);
				mark_object(cL, dL, t, tmp);
			}
		}

		++level;
	}
	luaL_pushresult(&b);
	lua_rawsetp(dL, SOURCE, t);
	lua_pop(L,1);
}

/** 标记一个object，基本类型不处理
	@param parent 父对象地址
	@param desc 父对象描述
*/
static void 
mark_object(lua_State *L, lua_State *dL, const void * parent, const char *desc) {
	luaL_checkstack(L, LUA_MINSTACK, NULL);
	int t = lua_type(L, -1);
	switch (t) {
	case LUA_TTABLE:
		mark_table(L, dL, parent, desc);
		break;
	case LUA_TUSERDATA:
		mark_userdata(L, dL, parent, desc);
		break;
	case LUA_TFUNCTION:
		mark_function(L, dL, parent, desc);
		break;
	case LUA_TTHREAD:
		mark_thread(L, dL, parent, desc);
		break;
	default:
		lua_pop(L,1);
		break;
	}
}

/** 统计table中kv对个数
	@param idx table的位置
*/
static int
count_table(lua_State *L, int idx) {
	int n = 0;
	lua_pushnil(L);
	while (lua_next(L, idx) != 0) {
		++n;
		lua_pop(L,1);
	}
	return n;
}

/**
*/
static void
gen_table_desc(lua_State *dL, luaL_Buffer *b, const void * parent, const char *desc) {
	char tmp[32];
	size_t l = sprintf(tmp,"%p : ",parent);
	luaL_addlstring(b, tmp, l);
	luaL_addstring(b, desc);
	luaL_addchar(b, '\n');
}

/**
*/
static void
pdesc(lua_State *L, lua_State *dL, int idx, const char * typename) {
	lua_pushnil(dL);
	while (lua_next(dL, idx) != 0) {
		luaL_Buffer b;
		luaL_buffinit(L, &b);
		const void * key = lua_touserdata(dL, -2);
		if (idx == FUNCTION) {
			lua_rawgetp(dL, SOURCE, key);
			if (lua_isnil(dL, -1)) {
				luaL_addstring(&b,"cfunction\n");
			} else {
				size_t l = 0;
				const char * s = lua_tolstring(dL, -1, &l);
				luaL_addlstring(&b,s,l);
				luaL_addchar(&b,'\n');
			}
			lua_pop(dL, 1);
		} else if (idx == THREAD) {
			lua_rawgetp(dL, SOURCE, key);
			size_t l = 0;
			const char * s = lua_tolstring(dL, -1, &l);
			luaL_addlstring(&b,s,l);
			luaL_addchar(&b,'\n');
			lua_pop(dL, 1);
		} else {
			luaL_addstring(&b, typename);
			luaL_addchar(&b,'\n');
		}
		lua_pushnil(dL);
		while (lua_next(dL, -2) != 0) {
			const void * parent = lua_touserdata(dL,-2);
			const char * desc = luaL_checkstring(dL,-1);
			gen_table_desc(dL, &b, parent, desc);
			lua_pop(dL,1);
		}
		luaL_pushresult(&b);
		lua_rawsetp(L, -2, key);
		lua_pop(dL,1);
	}
}

/** 输出结果，采用table
*/
static void
gen_result(lua_State *L, lua_State *dL) {
	int count = 0;
	count += count_table(dL, TABLE);
	count += count_table(dL, FUNCTION);
	count += count_table(dL, USERDATA);
	count += count_table(dL, THREAD);
	lua_createtable(L, 0, count);
	pdesc(L, dL, TABLE, "table");
	pdesc(L, dL, USERDATA, "userdata");
	pdesc(L, dL, FUNCTION, "function");
	pdesc(L, dL, THREAD, "thread");
}

static int
snapshot(lua_State *L) {
	int i;
	lua_State *dL = luaL_newstate();
	for (i=0;i<MARK;i++) {
		lua_newtable(dL);
	}
	lua_pushvalue(L, LUA_REGISTRYINDEX);
	mark_table(L, dL, NULL, "[registry]");
	gen_result(L, dL);
	lua_close(dL);
	return 1;
}

int
luaopen_snapshot(lua_State *L) {
	luaL_checkversion(L);
	lua_pushcfunction(L, snapshot);
	return 1;
}

ENDIF
