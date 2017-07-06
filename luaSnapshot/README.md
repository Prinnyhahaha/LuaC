# LuaSnapshot

Lua内存快照工具，VS下的生成配置没有配好，不能直接生成，可以自行配置或使用MinGW

`make win64`

可生成win64的动态库

`make exe`

可生成可执行程序(源码中存在main函数)

## LuaJIT

编译LuaSnapshot需要链接LuaJIT(至少之前项目里用JIT)

把LuaJIT编译成库(动态，静态随意，动态库省几百K的内存)

MinGW环境下执行 `luaSnapshot/luajit/buildwin64.sh` 后在 `luaSnapshot/luajit/lib` 目录下找到静态库 `libluajit.a` ，把这个静态库链接进去
