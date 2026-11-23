-- Dimensions in metres. Each call emits a bounded, repeatable room.
-- box(x,y,z, width,height,depth, colour, material?, yaw?, roll?, lightRadius?) uses centred boxes.
local oak, pine, trim = 0x986543, 0xc38d59, 0x69462f
local wood = {0xa57650,0xa2734d,0xac7c52,0xb18057}
local cloth = {0xad5847,0x477773,0x737e50,0x657887,0xb17c50}
-- Half widths and depths are shared by movement, doorways, previews and the room builder.
cabin_layouts = {
  {name="Hearth Room", width=4.4, depth=4, height=3.72},
  {name="The spring baths", width=10, depth=14, height=6.6, balcony=true},
  {name="The winter garden", width=12, depth=9, height=7.1, balcony=true},
  {name="The reading room", width=4.8, depth=4.6, height=3.7},
  {name="The map gallery", width=5.4, depth=8, height=4.6, balcony=true},
  {name="The guest room", width=5, depth=4.4, height=3.8},
  {name="The sunroom", width=6.5, depth=5.4, height=4.5, balcony=true},
  {name="The cedar library", width=9, depth=13, height=7.6, balcony=true},
  {name="The observatory", width=8, depth=9, height=7.6, balcony=true},
  {name="The tea pavilion", width=7, depth=7.5, height=4.5, balcony=true},
  {name="The pottery studio", width=6.6, depth=7.2, height=4.6},
  {name="The music room", width=8.5, depth=11, height=5.9},
  {name="The seed archive", width=6.6, depth=10, height=5.4},
  {name="The fern court", width=15, depth=19, height=9.8, balcony=true},
  {name="The weaving loft", width=6.8, depth=8.4, height=4.4},
  {name="The print room", width=6.2, depth=8.6, height=4.6},
  {name="The rain hall", width=13.5, depth=21, height=9.8},
  {name="The bunkhouse", width=7.4, depth=11, height=5.2},
  {name="The stone sauna", width=5.8, depth=7, height=3.9},
  {name="The cabinet of curiosities", width=8, depth=12, height=5.5},
  {name="The linen room", width=5.8, depth=7, height=4.1},
  {name="The luthier's workshop", width=7.5, depth=8.5, height=4.8},
  {name="The dawn terrace room", width=8, depth=9, height=5.5, balcony=true},
  {name="The long hall", width=12, depth=25, height=10, balcony=true}
}

local function table_at(b,x,z,w,d)
  b(x,.84,z,w,.14,d,pine,3)
  for _,a in ipairs({-.43,.43}) do for _,c in ipairs({-.38,.38}) do
    b(x+w*a,.4,z+d*c,.11,.8,.11,trim,3)
  end end
end
local function books(b,x,y,z,n)
  local covers={0x486e69,0xa05b42,0xd1ad68,0x77768a,0x6a784e}
  for j=1,n do
    local h=.22+(j*7%5)*.04
    b(x+j*.115,y+h/2,z,.09,h,.22,covers[j%5+1])
    b(x+j*.115,y+h*.73,z+.116,.072,.021,.012,0xd7b983)
  end
end
local function shelf(b,x,z)
  b(x,1.0,z,1.46,2.0,.12,trim,3)
  for _,side in ipairs({-.74,.74}) do b(x+side,1.0,z+.14,.10,2,.42,oak,3) end
  for i=0,3 do
    local y=.12+i*.53; b(x,y,z+.14,1.52,.085,.44,pine,3)
    books(b,x-.68,y+.05,z+.17,6+(i%3))
  end
end
local function bed(b,x,z,colour)
  b(x,.37,z,2.23,.38,2.75,trim,3); b(x,.64,z,2.18,.28,2.69,0xe1c9a4)
  b(x,.82,z+.38,2.20,.24,1.94,colour)
  for j=-2,2 do b(x+j*.42,.95,z+.34,.038,.024,1.85,0xd7aa77) end
  b(x,.89,z-1.44,2.35,1.42,.15,oak,3)
  for _,d in ipairs({-.54,.54}) do b(x+d,.87,z-.87,.89,.24,.48,0xf2dfbb) end
end
local function plant(b,x,z,s)
  b(x,.22*s,z,.42*s,.44*s,.43*s,0xa5674b)
  for j=1,9 do
    local a=j*2.4
    local reach=(.18+.05*(j%3))*s
    b(x+math.cos(a)*reach,(.52+(j%4)*.095)*s,z+math.sin(a)*reach,.10*s,.69*s,.17*s,
      0x557d52+j*0x020202,0,a,-.45-(j%3)*.20)
  end
end
local function kitchen(b)
  b(3.48,.52,-1.55,1.32,1.04,3.65,0x687a65,3)
  b(3.45,1.10,-1.55,1.48,.16,3.81,0xc7ae88)
  for j=0,2 do
    b(2.81,.57,-2.86+j*1.22,.05,.83,1.09,0x789079,3)
    b(2.76,.81,-2.55+j*1.22,.07,.08,.20,0xd6b77c)
  end
  b(3.44,1.205,-2.43,.78,.04,.80,0x465551)
  b(3.53,1.39,-2.86,.055,.38,.06,0xc1c5ae)
  b(3.43,1.56,-2.86,.23,.05,.06,0xc1c5ae)
  b(3.42,1.25,-.44,.47,.22,.49,0xb96542); b(3.42,1.40,-.44,.37,.08,.40,0x454a42)
end
local function stove(b)
  b(2.70,.12,2.45,1.5,.2,1.45,0x77766a)
  for row=0,3 do for col=0,3 do
    local n=(row*7+col*5)%4
    b(2.24+col*.30,.32+row*.25,2.78,.29,.24,.62,0x776c60+n*0x080704)
  end end
  b(2.70,.72,2.44,.94,1.13,.62,0x414744)
  b(2.70,.71,2.105,.60,.54,.04,0xdd8942,7,0,0,3.7)
  for j=1,4 do b(2.42+j*.12,.69,2.07,.035,.51,.045,0x614835) end
  b(2.70,2.18,2.60,.29,2.0,.29,0x484a43)
end

-- The tangent on each wall agrees with the engine's window numbering.
local function wall_box(b,side,w,d)
  return function(t,y,n,sw,h,th,c,mat,turn)
    local x,z,yaw
    if side==0 then x,z,yaw=t,d+n,0
    elseif side==1 then x,z,yaw=t,-d-n,0
    elseif side==2 then x,z,yaw=w+n,t,math.pi/2
    else x,z,yaw=-w-n,t,math.pi/2 end
    b(x,y,z,sw,h,th,c,mat,yaw+(turn or 0))
  end
end

local function window(wall,side,t,half,top,opened,lowered)
  local base=.92
  local centre=(base+top)/2
  for _,y in ipairs({base,top}) do wall(t,y,0,2*half+.17,.13,.31,pine,3) end
  for _,a in ipairs({-half,half}) do wall(t+a,centre,0,.11,top-base,.26,trim,3) end
  -- The casement swings out on its left jamb. The panes and handle rotate with the sash.
  local a=opened and 1.22 or 0
  local outward=(side==1 or side==2) and -1 or 1
  local function sash(u,y,w,h,th,c,mat)
    wall(t-half+.06+u*math.cos(a),y,.02+u*math.sin(a),w,h,th,c,mat,outward*a)
  end
  local span=2*half-.13
  for _,y in ipairs({base+.105,top-.105}) do sash(span/2,y,span,.065,.072,oak,3) end
  for _,u in ipairs({0,span*.5,span}) do sash(u,centre,.055,top-base-.16,.072,oak,3) end
  if top>3.1 then sash(span/2,top-.83,span,.075,.085,trim,3) end
  sash(span/2,centre,span-.04,top-base-.22,.025,0xc6e0d9,8)
  sash(span-.08,1.63,.07,.16,.11,0xc59d62,0)
  wall(t,top+.12,-.14,half*2+.20,.17,.31,trim,3)
  if lowered then
    local rows=math.ceil((top-base)/.15)
    local step=(top-base)/rows
    for j=0,rows-1 do
      wall(t,top-(j+.5)*step,-.18,half*2+.04,step+.016,.065,j%2==0 and 0x9e794b or 0xbd9864,3)
    end
    for _,u in ipairs({-.6,.6}) do wall(t+u*half,centre,-.23,.027,top-base,.026,0x6e5d48) end
  else wall(t,top-.035,-.18,half*2+.04,.10,.11,0xb89566,3) end
end

local function doorway(wall,is_open,omit,style)
  for _,u in ipairs({-.99,.99}) do wall(u,1.27,0,.17,2.54,.32,trim,3) end
  wall(0,2.49,0,2.14,.23,.33,pine,3)
  if omit then return end
  if is_open then
    if style.roof=="tent" then
      for _,s in ipairs({-1,1}) do
        wall(s*.97,1.23,-.12,.16,2.36,.25,style.cover)
        wall(s*.97,1.29,-.12,.19,.095,.29,style.accent)
      end
    else
      wall(-.89,1.20,-.91,.15,2.38,1.82,oak,3)
      wall(-.79,1.04,-1.55,.13,.13,.13,0xd4ad6e)
    end
  else
    wall(0,1.20,0,1.82,2.38,.17,oak,3)
    if style.roof=="capsule" then
      for _,n in ipairs({-.10,.10}) do
        wall(0,1.24,n,1.48,1.99,.04,style.wall)
        wall(0,1.60,n*1.29,.52,.11,.045,style.accent,7)
      end
    elseif style.roof=="tent" then
      for _,n in ipairs({-.10,.10}) do
        for j=-2,2 do wall(j*.35,1.20,n,.35,2.34,.03,j%2==0 and style.cover or style.frame) end
        wall(0,1.20,n*1.22,.035,2.32,.035,style.accent)
      end
    else
      for j=-3,3 do
        for _,n in ipairs({-.10,.10}) do wall(j*.238,1.20,n,.218,2.30,.034,wood[(j+4)%4+1],3) end
      end
    end
    for _,n in ipairs({-.16,.16}) do
      wall(0,.46,n,1.62,.075,.05,trim,3)
      wall(.68,1.03,n,.15,.12,.12,0xd6b77c)
    end
  end
end

-- Roof courses meet edge to edge. The gable infill ends above the operable windows.
local function roof(b,style,w,d,h)
  if style.roof=="flat" then
    b(0,h+.08,0,2*w+.55,.28,2*d+.66,style.cover)
    for _,sign in ipairs({-1,1}) do
      b(sign*w,h+.29,0,.22,.46,2*d+.27,style.wall)
      b(0,h+.29,sign*d,2*w,.46,.23,style.wall)
    end
    return
  end
  local span=w+(style.roof=="tent" and 1.15 or .42)
  local function rise(x)
    local u=math.min(1,math.abs(x)/span)
    if style.roof=="capsule" then return .22+w*.48*math.sqrt(math.max(0,1-u*u)) end
    if style.roof=="tent" then return -.38+w*.62*(1-u)^1.55 end
    if style.roof=="bamboo" then return .15+w*(.54*(1-u)^1.5+.12*u^7) end
    return (span-math.abs(x))*(style.roof=="turf" and .55 or .41)
  end
  local n=math.ceil(span/.46)
  for j=0,n-1 do
    local a,bx=j*span/n,(j+1)*span/n
    local x=(a+bx)/2
    local high,low=rise(a),rise(bx)
    local length=math.sqrt((bx-a)^2+(high-low)^2)
    local thickness=style.roof=="turf" and .36 or .14
    local cover=style.cover+(j%3)*0x020202
    for _,side in ipairs({-1,1}) do
      local angle=side*math.atan(low-high,bx-a)
      b(side*x,h+(high+low)/2,0,length+.045,thickness,2*d+1,cover,0,0,angle)
      if x<w and (high+low)>0 then
        for _,endz in ipairs({-d,d}) do
          local fill=(high+low)/2
          b(side*x,h+fill/2,endz,bx-a+.012,fill,.20,style.wall)
        end
      end
      if style.roof=="tent" then
        -- A striped fly projects past the entrance. Its open sides frame the view.
        local fly=1.15+1.55*(1-x/span)
        b(side*x,h+(high+low)/2+.06,d+fly*.5,length+.04,.105,fly,
          j%4==1 and style.accent or cover,0,0,angle)
        if j%4==1 then
          b(side*x,h+(high+low)/2+.085,0,length+.015,.045,2*d+.9,style.accent,0,0,angle)
        end
      end
      if style.roof=="capsule" then
        for _,z in ipairs({-d,-d*.45,d*.45,d}) do
          b(side*x,h+(high+low)/2+.09,z,length+.04,.10,.12,style.frame,0,0,angle)
        end
      end
    end
  end
end

local function spar(b,x,y,z,X,Y,Z,width,colour)
  local dx,dy,dz=X-x,Y-y,Z-z
  local along=math.sqrt(dx*dx+dz*dz)
  b((x+X)/2,(y+Y)/2,(z+Z)/2,width,math.sqrt(along^2+dy^2),width,
    colour,0,math.atan(dz,dx),-math.atan(along,dy))
end

local function equipment(b,key,t,w,d,h)
  local accent,frame=t.accent,t.frame
  if key=="survey" then
    b(-w*.79,h+.36,-d*.62,.78,2.35,.91,0x6c685b)
    b(-w*.79,h+1.55,-d*.62,.95,.15,1.08,0x4e5350)
    b(0,h-.74,d+.70,3.5,.18,1.68,t.cover,3)
    for _,s in ipairs({-1,1}) do b(s*1.60,1.48,d+1.30,.15,2.96,.15,frame,3) end
  elseif key=="coast" then
    for j=-5,5 do b(j*.29,2.84,d+.53,.29,.10,1.34,j%2==0 and accent or 0xf1e6ce) end
    for _,s in ipairs({-1,1}) do
      b(s*1.11,1.29,d+.10,.17,2.58,.24,accent)
      b(s*(w-.20),h+.48,d-.5,.61,.61,.76,t.wall)
    end
    b(-w+.82,h+.73,-d+.9,.61,1.09,.65,t.wall)
  elseif key=="dunes" then
    for _,x in ipairs({-1,1}) do for _,z in ipairs({-1,1}) do
      spar(b,x*(w+.55),h+.04,z*(d+.24),x*(w+2.15),.12,z*(d+1.15),.047,0xdfc99e)
      b(x*(w+2.15),.14,z*(d+1.15),.12,.48,.12,0x70503c,0,0,x*.20)
    end end
    for _,s in ipairs({-1,1}) do
      spar(b,s*(w+.41),.03,d+1.28,s*(w+.41),h-.16,d+1.28,.115,frame)
      spar(b,s*(w+.41),h-.16,d+1.28,s*(w+2.55),.10,d+2.42,.065,0xe7cf9b)
      -- The loose lower canvas flares to the ground below the openable casements.
      for j=-3,3 do
        b(s*(w+.49),.49,j*d*.294,1.37,.08,d*.30,
          j%3==0 and accent or t.cover,0,0,-s*.66)
      end
      for j=0,2 do
        b(s*2.66,.20+j*.23,d+.113,3.46,.075,.038,j==1 and accent or t.cover)
      end
    end
    for j=-3,3 do b(0,.024,d+.82+j*.14,1.45,.025,.12,j%3==0 and accent or 0x975f48) end
  elseif key=="basalt" then
    b(w*.6,h+.34,-d*.48,.74,2.53,.76,0x545751)
    for _,s in ipairs({-1,1}) do
      for j=0,3 do b(s*(w-.04),.20+j*.38,d+.20,.68-j*.06,.36,.72,0x636259+j*0x040402) end
    end
    b(0,2.84,d+.38,2.84,.34,.96,t.cover,0)
  elseif key=="mesa" then
    for j=-4,4 do b(j*w/5,h-.25,0,.19,.21,2*d+1.03,frame,3) end
    for _,s in ipairs({-1,1}) do b(s*1.67,1.35,d+1.57,.16,2.70,.17,frame,3) end
    for j=-6,6 do b(j*.28,2.76,d+.78,.21,.095,1.82,0xa07c55,3) end
  elseif key=="karst" then
    for j=-4,4 do b(j*.49,.005,d+1.00,.47,.15,2.02,0xa28857,3) end
    for _,s in ipairs({-1,1}) do
      b(s*2.1,1.56,d+1.58,.11,3.13,.11,t.frame,3)
      b(s*2.1,.83,d+.90,.10,.10,1.63,t.accent)
      b(s*1.49,2.22,d+.47,.35,.54,.33,0xdc9c66,7)
    end
    b(0,h+2.08,0,.24,.27,2*d+1.14,t.frame,3)
  elseif t.roof=="capsule" then
    -- The collar frames a clear entrance with room to see through the open hatch.
    for _,s in ipairs({-1,1}) do
      b(s*1.16,1.30,d+.19,.30,2.60,.42,frame)
      b(s*1.16,1.38,d+.41,.14,.34,.045,accent,7)
      b(s*1.27,1.55,d+.32,.18,1.81,.62,t.cover)
      b(s*1.045,2.57,d+.32,.70,.20,.61,t.cover,0,0,-s*.73)
      b(s*1.045,.28,d+.32,.67,.18,.61,t.cover,0,0,s*.73)
      b(s*1.30,1.85,d+.64,.14,.54,.055,accent,6)
      -- External pressure bottles sit clear of the side windows and hatch.
      local tank=s*(w+.77)
      b(tank,1.37,d*.66,.82,1.76,.85,t.wall)
      for _,y in ipairs({.48,2.26}) do
        b(tank,y,d*.66,.70,.22,.72,frame)
        b(tank,y+(y>1 and .16 or -.16),d*.66,.47,.13,.50,accent)
      end
      b(s*(w+.02),.64,0,.31,.26,2*d+.13,accent)
      for j=-1,1 do b(s*(w+.12),.82,j*d*.71,.24,.35,.62,frame) end
    end
    b(0,2.65,d+.17,2.61,.27,.47,frame)
    b(0,2.66,d+.42,1.13,.055,.035,accent,7)
    b(0,2.80,d+.32,1.74,.21,.61,t.cover)
    b(0,2.78,d+.636,.64,.07,.04,accent,6)
    if key=="lunar" then
      spar(b,-w*.70,h+.89,d*.39,-w*.70,h+3.29,d*.39,.13,frame)
      for j=-3,3 do b(-w*.70+j*.25,h+3.21+j*j*.050,d*.39,.29,.15,1.19,t.wall,0,0,j*.18) end
      spar(b,-w*.70,h+3.20,d*.39,-w*.87,h+4.01,d*.39,.09,accent)
      for _,s in ipairs({-1,1}) do
        spar(b,s*(w-.45),h+.14,-d*.26,s*(w+2.15),h+.59,-d*.26,.17,frame)
        for j=0,3 do for k=0,2 do
          b(s*(w+.62+j*.57),h+.84+j*.17,-d*.26+(k-1)*.93,.56,.09,.88,0x54759b,0,0,s*.28)
        end end
      end
    elseif key=="prism" then
      for j=-2,2 do for _,s in ipairs({-1,1}) do
        spar(b,s*w*.62,h+1.06,j*1.48,s*w*.85,h+2.49-(math.abs(j)*.21),j*1.48,.18,accent)
      end end
    elseif key=="fungal" then
      for j=-1,1 do
        b(w*.6,h+1.48,j*2,.70,.78,.76,frame)
        for k=0,3 do b(w*.6,h+1.53+k*.14,j*2,1.02-k*.08,.08,.96-k*.07,k==2 and accent or t.cover) end
      end
    elseif key=="reefs" then
      for _,s in ipairs({-1,1}) do
        b(s*(w+.43),.12,0,.94,.69,2*d+.42,frame)
        for j=-2,2 do b(s*(w+.42),.14,j*d*.46,1.0,.71,.14,accent) end
        spar(b,s*w*.51,h+1.45,-d*.53,s*w*.91,h+2.43,-d*.76,.15,accent)
      end
    elseif key=="cinder" then
      for j=-4,4 do for _,s in ipairs({-1,1}) do
        b(s*w*.62,h+1.69,j*.76,1.65,.36,.16,j%2==0 and accent or frame,0,0,-s*.42)
      end end
    end
  end
end

local function shell(b,i,l,doors,blinds,panes,omit,key,style)
  local w,d,h=l.width,l.depth,l.height
  for side=0,3 do
    local emit=wall_box(b,side,w,d)
    local function wall(t,y,n,sw,ht,th,c,mat,turn)
      if key~="alpine" and mat==3 then
        if c==trim or c==oak then c=style.frame
        elseif c==pine then c=style.accent
        else c=style.wall end
        mat=(style.base=="timber" or style.base=="stilts") and 3 or 0
      end
      emit(t,y,n,sw,ht,th,c,mat,turn)
    end
    local half=side<2 and w or d
    local mid=.60*half
    local wh=math.min(3.6,half*.26)
    local top=2.62+math.max(0,h-3.72)*.86
    local hasdoor=side<3 or l.balcony
    local function timber(lo,hi,bottom,top,c)
      local finish=key=="alpine" and c or style.wall
      wall((lo+hi)/2,(bottom+top)/2,0,hi-lo,top-bottom,.20,finish,key=="alpine" and 3 or 0)
    end
    for _,sign in ipairs({-1,1}) do
      local function section(lo,hi,bottom,top,c)
        if sign==1 then timber(lo,hi,bottom,top,c) else timber(-hi,-lo,bottom,top,c) end
      end
      section(hasdoor and .90 or 0,mid-wh,0,top+.05,wood[2])
      section(mid+wh,half,0,top+.05,wood[3])
      section(mid-wh,mid+wh,0,.92,wood[4])
      local n=2*side+(sign==1 and 1 or 0)
      window(wall,side,sign*mid,wh,top,panes & (1<<n)~=0,blinds & (1<<n)~=0)
    end
    timber(-half,half,top+.02,h,wood[2])
    if hasdoor then
      timber(-.91,.91,2.44,top+.05,wood[1])
      doorway(wall,doors & (1<<side)~=0,omit==side,style)
    end
    for _,u in ipairs({-half,half}) do wall(u,h/2,0,.26,h,.31,trim,3) end
    wall(0,h-.17,0,2*half,.22,.28,pine,3)
  end
  -- Continuous boarded floors surround a recessed pool.
  local n=math.ceil(2*w/.63)
  for j=1,n do
    local x=-w+(j-.5)*2*w/n
    if i==1 and math.abs(x)<w*.55 then
      for _,side in ipairs({-1,1}) do b(x,-.10,side*d*.775,2*w/n,.2,d*.45,0xbaaa8d) end
    else b(x,-.09,0,2*w/n-.004,.18,2*d+.15,i==1 and 0xb9a990 or wood[j%4+1],3) end
  end
  if key~="alpine" then
    roof(b,style,w,d,h)
  else
  -- A pitched, timber-lined roof. The gable ends close the triangle above each wall.
  local courses=math.ceil(w/.55)
  for j=0,courses do
    local x=(j+.5)*w/courses
    local rise=math.max(.02,(w-x)*.41)
    for _,sign in ipairs({-1,1}) do
      b(sign*x,h+(w-x)*.41,0,w/courses*1.12,.15,2*d+.9,wood[(j+2)%4+1]-0x344337,3,0,-sign*.389)
      if x<w then for _,z in ipairs({-d,d}) do b(sign*x,h+rise/2,z,w/courses+.01,rise,.17,oak,3) end end
    end
  end
  end
  for j=-1,1 do b(0,h-.11,j*d*.66,2*w,.23,.23,trim,3) end
  if i==0 then
    equipment(b,key,style,w,d,h)
    b(0,-.13,d+.53,3.4,.25,1,oak,3); b(0,-.28,d+1.15,2.62,.25,.39,0x817c67)
    b(-1.34,1.98,d+.26,.24,.43,.25,0xf6c27a,7); b(-1.34,2.22,d+.26,.37,.09,.34,trim)
  end
  if l.balcony then
    -- An outward-facing chair and a low tea table leave the door and railing clear.
    local seat,z=-w-.92,1.78
    for _,u in ipairs({-.29,.29}) do for _,v in ipairs({-.26,.26}) do
      b(seat+u,.23,z+v,.075,.46,.075,trim,3)
      b(-w-2.13+u,.35,z+v,.065,.70,.065,trim,3)
    end end
    for j=-2,2 do b(seat,.49,z+j*.122,.73,.09,.115,pine,3) end
    b(seat+.32,.76,z,.09,.57,.69,oak,3)
    for _,v in ipairs({-.33,.33}) do b(seat,.71,z+v,.67,.07,.09,oak,3) end
    b(-w-2.13,.71,z,.72,.075,1.10,pine,3)
    local depth=d*.60
    for j=0,7 do b(-w-(j+.5)*.4,-.09,0,.39,.18,2*depth,pine,3) end
    for j=-4,4 do b(-w-3.12,.53,j*depth/4,.095,1.06,.095,trim,3) end
    for _,y in ipairs({.40,1.04}) do
      b(-w-3.12,y,0,.13,.10,2*depth+.09,oak,3)
      for _,s in ipairs({-1,1}) do b(-w-1.57,y,s*depth,3.15,.10,.13,oak,3) end
    end
  end
end

local function wayfinding(b,i,l)
  local w,d=l.width,l.depth
  if i==0 then
    cabin_plaque(wall_box(b,1,w,d),1,"SPRING BATHS",3.02)
    local garden_door=wall_box(b,2,w,d)
    cabin_plaque(garden_door,1,"WINTER GARDEN",3.10)
    cabin_plaque(garden_door,1,"BALCONY",2.79)
    for j=1,5 do
      b(w-j*.41,.008,0,.24,.016,.13,0x60877c,0,math.pi/4)
    end
  end
  if l.balcony then
    cabin_plaque(wall_box(b,3,w,d),-1,"BALCONY",2.94)
    for j=1,5 do
      b(-w+j*.44,.012,0,.26,.018,.13,0x60877c,0,math.pi/4)
    end
  end
end

local function baths(b,w,d)
  b(0,-1.47,0,w*1.12,.20,d*1.13,0x548984)
  for _,s in ipairs({-1,1}) do
    b(s*w*.55,-.69,0,.22,1.48,d*1.12,0x91ac9e)
    b(0,-.69,s*d*.55,w*1.12,1.48,.22,0x91ac9e)
    b(s*w*.55,.01,0,.42,.12,d*1.14,0xf1d9a9)
    b(0,.01,s*d*.55,w*1.14,.12,.42,0xf1d9a9)
    for j=-1,1 do
      local x,z=s*w*.78,j*4
      b(x,.30,z,1.10,.34,2.28,0x99794e,3)
      b(x,.51,z,1.07,.15,2.20,0x737e6e)
      for k=0,4 do b(x,.61+k*.11,z-.59-k*.086,1.04,.15,.16,0xb1b49a) end
      b(x,.61,z+.54,.75,.075,.46,0xefdcaf)
      b(x,.66,z+.54,.75,.025,.06,0x70918c)
    end
  end
  for j=-7,7 do
    for _,s in ipairs({-1,1}) do
      b(s*w*.55,.079,j*d*.073,.22,.024,.46,j%2==0 and 0x558a88 or 0x779e98)
      b(j*w*.073,.079,s*d*.55,.33,.024,.21,j%2==0 and 0x558a88 or 0x779e98)
    end
  end
  b(0,-.30,0,w*1.1,.035,d*1.1,0x438e8e,9)
  for j=0,3 do b(0,-.16-j*.28,d*.55-.4-j*.48,2.4,.22,.54,0xb0bcab) end
  for _,s in ipairs({-1,1}) do b(s*1.35,.28,d*.55-.6,.065,.78,.065,0xad9872) end
end

local function garden(b,w,d,key)
  for _,s in ipairs({-1,1}) do
    for j=-1,1 do
      local x,z=s*w*.58,d*j*.56
      b(x,.22,z,3.4,.44,2.85,trim,3); b(x,.46,z,3.26,.12,2.70,0x544d38)
      for k=1,9 do
        local a=k*2.4+key%6
        plant(b,x+math.cos(a)*1.1,z+math.sin(a)*.9,.95+k%4*.28)
        local fx,fz=x+math.cos(a)*1.1,z+math.sin(a)*.9
        local fy=1.33+k%3*.14
        b(fx,fy-.22,fz,.035,.46,.035,0x698251)
        for n=1,4 do b(fx+.095*math.cos(n*1.57),fy,fz+.095*math.sin(n*1.57),.17,.066,.14,cloth[k%5+1],0,n) end
        b(fx,fy+.04,fz,.08,.055,.08,0xeccb77)
      end
      b(x,2.4,z,.24,4.8,.24,oak,3)
      for k=1,6 do
        local a=k*2.4
        b(x+math.cos(a)*.65,3.75+k%3*.32,z+math.sin(a)*.65,.11,1.68,.12,trim,3,a,-.72)
        for n=0,2 do
          local r=.83+n*.31
          b(x+math.cos(a+n*.31)*r,4.3+k%3*.32-n*.14,z+math.sin(a+n*.31)*r,
            1.45-n*.23,.72-n*.13,1.07-n*.14,0x617f52+k*0x030300,5,a+n*.75,-.15)
        end
      end
    end
  end
  for j=-6,6 do
    for k=-1,1 do b(k*.88,.017,j*1.11,.84,.034,1.065,wood[(j+k)%4+1]+0x212620,0) end
  end
  for j=-2,2 do b(j*1.1,5.55,0,.12,.21,d*1.6,oak,3) end
end

local furnishings = {table=table_at, shelf=shelf, bed=bed, plant=plant}

function build_room(i,key,doors,blinds,panes,omit,b,skin)
  local l=cabin_layouts[i+1]
  local colour=cloth[key%#cloth+1]
  skin=skin or "alpine"
  shell(b,i,l,doors,blinds,panes,omit,skin,shelter_styles[skin])
  wayfinding(b,i,l)
  if cabin_room_furniture[i] then
    cabin_room_furniture[i](b,l,key,furnishings)
    return
  end
  if i==1 or i==2 then
    for _,x in ipairs({-1,1}) do for _,z in ipairs({-1,1}) do
      local px,pz=x*l.width*.36,z*(l.depth-.22)
      b(px,2.25,pz,.32,.09,.42,trim,3); b(px,2.53,pz,.22,.49,.28,0xffca83,7,0,0,7)
      b(px,2.82,pz,.33,.10,.39,trim,3)
    end end
  end
  if i==1 then baths(b,l.width,l.depth); return end
  if i==2 then garden(b,l.width,l.depth,key); return end
  b(0,.023,0,2.2,.045,3.6,colour)
  for _,z in ipairs({-1.55,-1.44,1.44,1.55}) do b(0,.051,z,2.12,.018,.05,0xe1bf89) end
  for j=-2,2 do
    for k=-1,1 do b(k*.54,.055,j*.49,.24,.025,.24,0xcba779,0,.785) end
  end
  if i>0 then
    local a=(i*37%101)/100
    plant(b,-3.53,1.2+a,1.1+a*.68)
    for j=1,2+i%4 do
      b(-3.70+j*.34,2.94,-3.80,.18,.22+(i*j%7)*.035,.14,cloth[(i+j)%5+1])
    end
  end
  if i==0 then
    bed(b,-2.75,-1.78,colour);
    table_at(b,-1.06,-2.72,.66,.60); b(-1.06,1.07,-2.72,.11,.38,.11,0x86603c);
    b(-1.06,1.36,-2.72,.41,.32,.42,0xf3c17f,7,0,0,3.2); kitchen(b); stove(b); shelf(b,-2.75,3.45)
    table_at(b,-3.53,1.28,1.1,1.7); b(-3.54,.94,1.30,.52,.032,.68,0xe9dbb2,0,.16)
    b(-3.57,1.03,.75,.14,.18,.14,0x45595b); b(-2.52,.38,1.30,.58,.76,.60,oak,3)
  elseif i==5 then bed(b,-2.76,-1.6,colour); stove(b); shelf(b,2.85,-3.66)
  elseif i==4 then
    for _,sign in ipairs({-1,1}) do
      for j=-1,1 do
        table_at(b,sign*3.4,j*3.8,1.65,1.4)
        b(sign*3.4,.935,j*3.8,1.30,.03,1.05,0xe7cf96,0,j*.13)
        b(sign*3.4,.96,j*3.8,.72,.024,.08,0x779792,0,.55)
      end
    end
  else
    shelf(b,-2.67,-3.6); shelf(b,2.68,-3.6); table_at(b,-2.8,.4,1.75,1.05)
    b(-2.8,.94,.4,.85,.035,.6,0xe1cba0,0,-.12)
    b(2.9,.48,.55,1.44,.65,1.4,colour); b(3.43,.93,.55,.36,.78,1.44,colour)
    plant(b,-3.5,2.9,1.6)
  end
end
