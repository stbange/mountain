-- Enumerate executable lines of the chunks and their Lua closures before
-- exercising them, so an uncalled helper cannot silently inflate coverage.
local root = assert(arg[1])
local included, hit, seen = {}, {}, {}
local function record(fn)
  if type(fn) ~= "function" or seen[fn] then return end
  seen[fn] = true
  local info = debug.getinfo(fn, "SL")
  if not info.source:find(root, 1, true) then return end
  local t = included[info.source] or {}
  included[info.source] = t
  for line in pairs(info.activelines) do t[line] = true end
  for i=1,255 do
    local name, up = debug.getupvalue(fn,i)
    if not name then break end
    if type(up)=="function" then record(up) end
  end
end
local function trace(_,line)
  local src = debug.getinfo(2,"S").source
  if included[src] then
    hit[src]=hit[src] or {}
    hit[src][line]=true
  end
end
local function load(name)
  local chunk=assert(loadfile(root.."/scripts/"..name..".lua"))
  record(chunk)
  return chunk()
end
debug.sethook(trace,"l")
local world=load("world")
assert(#world.seasons==4 and #world.skies==4 and #world.trails==8)
assert(#world.tree_shapes==4 and world.tree_shapes[3].deciduous and world.tree_shapes[4].form=="forked")
load("wildlife")
record(build_animal)
for k=1,#world.fauna_names do
  for season=0,3 do
    local count=0
    build_animal(k,season,.8,.65,20,function(x,y,z,u,v,w,width,depth,c)
      count=count+1
      assert((x-u)^2+(y-v)^2+(z-w)^2>0 and width>0 and depth>0 and math.type(c)=="integer")
    end)
    assert(count>=6)
  end
end
load("cabin")
record(build_room);record(room_title)
local function shapes(i,open,front)
  local n, blocked, ink=0,false,0
  build_room(i,open,front,function(x,y,z,w,h,d,colour,material,yaw,roll)
    assert(w>0 and h>0 and d>0 and math.type(colour)=="integer")
    n=n+1;ink=(ink+colour+math.floor((x+y+z)*200))%1000000007
    -- Test the centre of the entrance aperture at eye height.
    if math.abs(x)<w/2 and math.abs(y-1.65)<h/2 and math.abs(z-4.02)<d/2 then blocked=true end
  end)
  return n,blocked,ink
end
local closed,blocked=shapes(0,false,true)
assert(closed>200 and blocked)
local open,blocked2=shapes(0,true,true)
assert(open>200 and not blocked2)
local seen_layout={}
for i=1,52 do
  local n,gap,hash=shapes(i,true,false)
  assert(n>150 and not gap and room_title(i)~=room_title(0))
  local _,_,same=shapes(i,true,false)
  assert(same==hash);seen_layout[hash]=true
end
local unique=0;for _ in pairs(seen_layout) do unique=unique+1 end;assert(unique>20)
debug.sethook()
local total,covered=0,0
for src,lines in pairs(included) do
  local all,reached=0,0
  for line in pairs(lines) do
    all=all+1
    if (hit[src] or {})[line] then reached=reached+1 end
  end
  print(string.format("%s: %d/%d executable lines",src,reached,all))
  total=total+all;covered=covered+reached
end
local percent=100*covered/total
print(string.format("Lua gameplay: %d/%d executable lines (%.2f%%). Required: 95%%.",covered,total,percent))
assert(percent>=95)
