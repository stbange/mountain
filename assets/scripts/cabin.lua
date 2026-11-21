-- Dimensions in metres. Each call emits a bounded, repeatable room.
-- box(x,y,z, width,height,depth, colour, material?, yaw?, roll?, lightRadius?) uses centred boxes.
local oak, pine, trim = 0x986543, 0xc38d59, 0x69462f
local wood = {0xa57650,0xa2734d,0xac7c52,0xb18057}
local cloth = {0xad5847,0x477773,0x737e50,0x657887,0xb17c50}
local titles = {"The reading room","The glasshouse","The map room","The guest room","The sunroom"}
function room_title(i) return i==0 and "Hearth Room" or titles[(i-1)%#titles+1] end

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
    b(x+math.cos(a)*.15*s,(.45+(j%4)*.11)*s,z+math.sin(a)*.15*s,.15*s,.55*s,.15*s,0x507850+j*0x020200,0,a)
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

function build_room(i,open,front,b)
  local colour=cloth[i%#cloth+1]
  for j=0,15 do b(-4.14+j*.55,-.09,0,.544,.18,8.13,wood[(j*3)%4+1],3) end
  -- Planks stop around real window and doorway openings.
  for j=0,13 do
    local y=.14+j*.265; local c=wood[j%4+1]
    for _,x in ipairs({-4.4,4.4}) do
      if y<.88 or y>2.75 then b(x,y,0,.20,.26,8.15,c,3)
      else for _,z in ipairs({-3.24,3.24}) do b(x,y,z,.20,.26,1.65,c,3) end end
    end
    for _,z in ipairs({-4.0,4.0}) do
      if y>2.49 then b(0,y,z,8.8,.26,.22,c,3)
      elseif y<.87 then
        for _,x in ipairs({-2.67,2.67}) do b(x,y,z,3.42,.26,.22,c,3) end
      else
        for _,x in ipairs({-4.02,-1.23,1.23,4.02}) do b(x,y,z,.68,.26,.22,c,3) end
      end
    end
  end
  for _,x in ipairs({-4.38,4.38}) do
    for _,z in ipairs({-3.94,3.94}) do b(x,1.87,z,.28,3.85,.32,trim,3) end
    b(x,.94,0,.33,.15,4.96,pine,3); b(x,2.72,0,.30,.15,4.96,trim,3)
    for _,z in ipairs({-2.42,0,2.42}) do b(x,1.83,z,.16,1.82,.10,trim,3) end
  end
  for _,z in ipairs({-4.0,4.0}) do
    for _,x in ipairs({-2.65,2.65}) do
      b(x,.92,z,2.21,.15,.34,pine,3); b(x,2.49,z,2.21,.14,.32,trim,3)
      b(x,1.67,z,.07,1.52,.11,trim,3)
    end
    for _,x in ipairs({-1.0,1.0}) do b(x,1.26,z,.19,2.53,.34,trim,3) end
    b(0,2.51,z,2.18,.18,.34,pine,3)
  end
  -- A pitched roof assembled from overlapping cedar courses.
  for j=0,15 do
    local d=(j+.5)*.322
    for _,side in ipairs({-1,1}) do
      b(side*d,5.74-d*.425,0,.373,.12,9.36,wood[(j+2)%4+1]-0x344337,3,0,-side*.402)
      for _,z in ipairs({-4.63,4.63}) do b(side*d,5.63-d*.425,z,.38,.22,.16,trim,3,0,-side*.402) end
      if d<4.42 then
        for _,z in ipairs({-4,4}) do
          local h=1.98-d*.43
          b(side*d,3.74+h/2,z,.33,h,.19,oak,3)
        end
      end
    end
  end
  for _,z in ipairs({-2.8,0,2.8}) do b(0,3.67,z,8.85,.24,.23,trim,3) end
  if front then
    if open then
      b(-.89,1.20,3.10,.15,2.37,1.80,oak,3); b(-.79,1.05,2.40,.12,.12,.12,0xd1ad6c)
    else
      b(0,1.20,4.02,1.82,2.38,.18,oak,3)
      for j=-3,3 do b(j*.235,1.18,4.12,.20,2.28,.045,wood[(j+4)%4+1],3) end
      b(.67,1.0,4.19,.14,.14,.13,0xd1ad6c)
    end
    b(0,-.13,4.53,3.4,.25,1.0,oak,3); b(0,-.28,5.15,2.62,.25,.39,0x817c67)
    b(-1.34,1.98,4.26,.24,.43,.25,0xf6c27a,7); b(-1.34,2.22,4.26,.37,.09,.34,trim)
  end
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
  elseif i%5==2 then
    for j=-2,2 do plant(b,-3.3,j*1.43,1.35+(j%2)*.6); plant(b,3.2,j*1.45,1+j%3*.3) end
    table_at(b,0,-1.2,2.1,.8)
  elseif i%5==4 then bed(b,-2.76,-1.6,colour); stove(b); shelf(b,2.85,-3.66)
  else
    shelf(b,-2.67,-3.6); shelf(b,2.68,-3.6); table_at(b,-2.8,.4,1.75,1.05)
    b(-2.8,.94,.4,.85,.035,.6,0xe1cba0,0,-.12)
    b(2.9,.48,.55,1.44,.65,1.4,colour); b(3.43,.93,.55,.36,.78,1.44,colour)
    plant(b,-3.5,2.9,1.6)
  end
end
