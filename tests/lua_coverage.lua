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
load("shelters")
local places=load("destinations")
assert(#places==12)
load("landmarks")
record(build_landmark)
for id,place in ipairs(places) do
  assert(place.lighting.clouds>=0 and place.lighting.softness>=0 and math.type(place.lighting.sky)=="integer")
  assert(#place.colours==6 and #place.stops==2 and #place.discovery==7)
  for kind=0,6 do
    for variant=0,2 do
      local strokes=0
      build_landmark(id-1,kind,variant,place.colours[4],function(x,y,z,X,Y,Z,r,R,sides,flat,col,material)
        assert((X-x)^2+(Y-y)^2+(Z-z)^2>0 and r>=0 and R>=0 and sides>=3 and flat>0 and math.type(col)=="integer")
        strokes=strokes+1
      end)
      assert(strokes>0)
    end
  end
end
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
record(build_visitor)
for _,place in ipairs(places) do
  local style=shelter_styles[place.shelter or place.profile]
  assert(#style.events==3 and #style.sights==2 and #style.name>5)
  for _,sight in ipairs(style.sights) do
    local n=0
    build_visitor(sight[2],1.3,function(x,y,z,X,Y,Z,w,d,col)
      assert(w>0 and d>0 and math.type(col)=="integer" and (x-X)^2+(y-Y)^2+(z-Z)^2>0)
      n=n+1
    end)
    assert(n>=5)
  end
end
assert(not pcall(build_visitor,"missing",0,function() end))
load("cabin-rooms")
record(cabin_plaque)
for _,room in pairs(cabin_room_furniture) do record(room) end
load("cabin")
record(build_room)
assert(#cabin_layouts==24 and cabin_layouts[14].width*2==30 and cabin_layouts[24].depth*2==50)
local function shapes(i,doors,blinds,panes,omit,skin)
  local l=cabin_layouts[i+1]
  local n,lights,glass,water,ink=0,0,0,0,0
  local openings={{0,l.depth},{0,-l.depth},{l.width,0},{-l.width,0}}
  local blocked={}
  build_room(i,31+i,doors,blinds,panes,omit,function(x,y,z,w,h,d,c,mat,yaw,roll,radius)
    assert(w>0 and h>0 and d>0 and math.type(c)=="integer")
    if radius then assert(radius>0 and mat==7); lights=lights+1 end
    if mat==8 then glass=glass+1 end
    if mat==9 then water=water+1 end
    ink=ink+c+math.floor((x*13+y*17+z*19+(yaw or 0)*31)*100)
    n=n+1
    for k,o in ipairs(openings) do
      local dx,dz=o[1]-x,o[2]-z
      local cs,sn=math.cos(yaw or 0),math.sin(yaw or 0)
      if math.abs(dx*cs+dz*sn)<w/2 and math.abs(dz*cs-dx*sn)<d/2 and math.abs(y-1.65)<h/2 then blocked[k]=true end
    end
  end,skin)
  assert(n>140 and glass==(i==19 and 18 or 8))
  assert(water==(i==1 and 1 or i==16 and 10 or 0))
  if i==0 then assert(lights==2) end
  return blocked,ink,n
end
local room_shapes={}
for i=0,#cabin_layouts-1 do
  local closed,ink,count=shapes(i,0,0,0,-1)
  assert(not room_shapes[ink]); room_shapes[ink]=true
  local opened=shapes(i,15,0,0,0)
  for wall=1,3 do assert(closed[wall] and not opened[wall]) end
  if cabin_layouts[i+1].balcony then assert(closed[4] and not opened[4]) end
  local _,lower,more=shapes(i,0,255,0,-1);assert(lower~=ink and more>count)
  local _,glass=shapes(i,0,0,255,-1);assert(glass~=ink)
  local _,again=shapes(i,0,0,255,-1);assert(again==glass)
end
local skins={}
for _,place in ipairs(places) do
  local key=place.shelter or place.profile
  local closed,ink=shapes(0,0,0,0,-1,key)
  local open=shapes(0,15,255,255,-1,key)
  assert(not skins[ink] and closed[1] and closed[2] and closed[3])
  assert(not open[1] and not open[2] and not open[3])
  skins[ink]=key
end
local cookbook=load("cookbook")
local seen, used, fingerprints, local_count, drinks, travelling = {}, {}, {}, {}, 0, 0
assert(#cookbook.ingredients==240 and #cookbook.recipes==312 and #cookbook.house==12)
for i,item in ipairs(cookbook.ingredients) do
  assert(item[1]==i and item[2]==(i-1)//20 and #item[3]>2 and #item[4]>2 and item[6]>=-1 and item[6]<4 and (item[7]=="open" or item[7]=="shelter" or item[7]=="water"))
end
local glass={cup=true,tea=true,glass=true}
for i,r in ipairs(cookbook.recipes) do
  assert(r[1]==i and not seen[r[3]] and #r[4]>2)
  seen[r[3]]=true
  drinks=drinks+(glass[r[5]] and 1 or 0)
  local regions, parts={},{}
  for j=7,#r do
    local key=r[j]
    assert(cookbook.ingredients[key] and not parts[key])
    used[key]=true;parts[key]=true;regions[cookbook.ingredients[key][2]]=true
  end
  local fingerprint={r[4],r[6]}
  for n=1,#cookbook.ingredients do if parts[n] then fingerprint[#fingerprint+1]=tostring(n) end end
  local key=table.concat(fingerprint,"|")
  assert(not fingerprints[key],r[3]);fingerprints[key]=true
  if r[2]<0 then
    local n=0;for _ in pairs(regions) do n=n+1 end;assert(n>=2)
    travelling=travelling+1
  else
    for region in pairs(regions) do assert(region==r[2]) end
    local_count[r[2]]=(local_count[r[2]] or 0)+1
  end
end
assert(travelling==24 and drinks==54)
for i=1,240 do assert(used[i]) end
for i,house in ipairs(cookbook.house) do assert(local_count[i-1]==24 and cookbook.recipes[house][2]==i-1) end
assert(#cookbook.recipes[cookbook.coffee]==6 and #cookbook.recipes[cookbook.cookies]==6)
load("kitchen-art");record(kitchen_model)
for form=0,14 do
  local n=0
  kitchen_model(form,function(x,y,z,X,Y,Z,r,R,sides,flat,colour,mat)
    assert((X-x)^2+(Y-y)^2+(Z-z)^2>0 and math.min(r,R)>0 and sides>=3 and flat>0 and math.type(colour)=="integer" and mat>=0)
    n=n+1
  end)
  assert(n>=5)
end

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
