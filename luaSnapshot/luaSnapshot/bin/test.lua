f = require "snapshot"

res1 = f()

function bar()
    print(2)
end

function foo()
    local a = 1
    bar()
    print(1)
end

co = coroutine.create(foo)

frame = {}
for i = 1, 4 do
    frame[i] = {}
    frame[i].time = 0
    frame[i].position = {0, 0, 0, 0}
end

t = {}
tt = t
sss = "lalal"

res2 = f()

file = io.open("log.log", "w")
for k, v in pairs(res2) do
    if res1[k] == nil then
        file:write(tostring(k) .. " >>> " .. v .. "\n")
    end
end
file:close()