-- Original cabin furnishings and enamel wayfinding plaques. Box dimensions are in metres.
local cedar, oak, brass = 0x7c4a33, 0xad7a4d, 0xcbab70
local cream, stone, jade = 0xe7d7b3, 0x71817c, 0x477f74
local signs = {-1, 1}
local pigments = {0x964e45, 0x557d78, 0xbe8e51, 0x747690, 0x779365}
local letters = {
  A={14,17,17,31,17,17,17}, B={30,17,17,30,17,17,30}, C={14,17,16,16,16,17,14},
  D={30,17,17,17,17,17,30}, E={31,16,16,30,16,16,31}, G={14,17,16,23,17,17,15},
  H={17,17,17,31,17,17,17}, I={31,4,4,4,4,4,31}, L={16,16,16,16,16,16,31},
  N={17,25,21,19,17,17,17}, O={14,17,17,17,17,17,14}, P={30,17,17,30,16,16,16},
  R={30,17,17,30,20,18,17}, S={15,16,16,14,1,1,30}, T={31,4,4,4,4,4,4},
  W={17,17,17,21,21,27,17}, Y={17,17,10,4,4,4,4}, [" "]={0,0,0,0,0,0,0}
}
function cabin_plaque(wall, direction, title, y, px)
  px=px or .034
  local span=(#title*6-1)*px
  wall(0,y,-.159,span+.24,7*px+.041,.065,cedar,3)
  wall(0,y,-.197,span+.18,7*px+.012,.018,0x324f48)
  for _,s in ipairs(signs) do wall(s*(span/2+.06),y,-.211,.027,.039,.014,brass) end
  for i=1,#title do
    for row,bits in ipairs(letters[title:sub(i,i)]) do
      for k=0,4 do
        if bits & (16 >> k) ~= 0 then
          local x=((i-1)*6+k+.5)*px-span/2
          wall(direction*x, y+(4-row)*px,-.212,px,px,.014,cream)
        end
      end
    end
  end
end

local function lamp(b,x,z,y,colour)
  b(x,y+.32,z,.047,.64,.047,cedar,3)
  b(x,y-.13,z,.33,.26,.33,colour,7,0,0,5)
  b(x,y-.28,z,.41,.067,.41,brass)
end
local function lamps(b,l,colour)
  for _,s in ipairs(signs) do
    for _,z in ipairs(signs) do lamp(b,s*l.width*.44,z*l.depth*.54,math.min(l.height-.6,3.95),colour) end
  end
end
local function runner(b,l,colour)
  b(0,.025,0,2.38,.038,l.depth*1.45,colour)
  for _,s in ipairs(signs) do b(s*1.12,.048,0,.041,.012,l.depth*1.42,brass) end
end
local function drawers(b,x,z,w,d,n,front)
  b(x,.53,z,w,1.06,d,cedar,3)
  for row=0,2 do for i=1,n do
    local dx=(i-.5-n/2)*(w/n)
    local y=.19+.32*row
    b(x+dx,y,z+front*d*.50,w/n-.045,.274,.047,oak,3)
    b(x+dx,y+.03,z+front*(d*.50+.039),.134,.034,.048,brass)
  end end
end
local function pots(b,x,y,z,scale,col)
  b(x,y+.19*scale,z,.48*scale,.38*scale,.51*scale,col,0,.13)
  b(x,y+.43*scale,z,.36*scale,.13*scale,.36*scale,col+0x13100b,0,.13)
  b(x,y+.515*scale,z,.39*scale,.043*scale,.39*scale,col,0,.13)
end

cabin_room_furniture = {
  [7]=function(b,l,key,f)
    runner(b,l,jade); lamps(b,l,0xffd39c)
    for _,s in ipairs(signs) do
      for j=-4,4 do
        f.shelf(b,s*6.1,j*2.11)
        f.shelf(function(x,y,z,...) b(x,y+2.25,z,...) end,s*6.1,j*2.11)
      end
      b(s*6.14,2.22,0,2.40,.17,l.depth*1.52,cedar,3)
      b(s*4.95,3.18,0,.13,.14,l.depth*1.50,oak,3)
      for j=-8,8 do b(s*4.95,2.75,j*1.08,.095,.92,.097,cedar,3) end
      for j=0,12 do b(s*5.01,2.30-j*.17,-8.7-j*.27,1.26,.18,.29,oak,3) end
      f.table(b,s*2.73,4.4,1.3,1.55)
      b(s*2.73,.94,4.4,.83,.044,.66,cream,0,s*.2)
    end
  end,
  [8]=function(b,l)
    runner(b,l,0x4f5d7b); lamps(b,l,0xe8bd90)
    local x,z=-2.7,-2.2
    b(x,.21,z,3.1,.42,3.4,stone)
    for j=0,2 do
      local a=j*2.094
      b(x+.56*math.cos(a),.94,z+.56*math.sin(a),.105,1.63,.144,brass,0,a,.41)
    end
    b(x,1.66,z,.45,.27,.50,cedar)
    b(x,2.39,z,3.04,.42,.44,0x576c84,0,-.59,.52)
    b(x+1.19,3.05,z+.79,.35,.58,.57,brass,0,-.59,.52)
    b(x+1.35,3.13,z+.89,.33,.45,.44,0x84aaa7,6,-.59,.52)
    for _,s in ipairs(signs) do
      drawers(b,s*4.6,6.7,3.1,1.1,4,-1)
      b(s*4.6,1.09,6.7,2.3,.038,.85,0x445676)
      for j=-4,4 do b(s*4.6+j*.21,1.12,6.7+.35*math.sin(j),.043,.024,.043,cream,7) end
    end
    for j=-3,3 do b(j*1.54,l.height-.33,-l.depth*.77,.51,.12,1.44,0x83919e) end
  end,
  [9]=function(b,l,key,f)
    runner(b,l,0x6f8d72); lamps(b,l,0xffdcab)
    for _,s in ipairs(signs) do for j=-1,1 do
      local x,z=s*3.8,j*4.0+.45
      f.table(b,x,z,1.67,1.13)
      b(x,.93,z,1.01,.037,.60,0x889f91)
      pots(b,x,.95,z,.47,0x96604b)
      for _,t in ipairs(signs) do
        b(x+t*.70,.30,z,.64,.44,.62,cedar,3)
        b(x+t*.70,.56,z,.68,.10,.66,pigments[(j+key)%#pigments+1])
        b(x+t*.36,.99,z,.168,.083,.165,cream)
      end
    end end
  end,
  [10]=function(b,l,key,f)
    runner(b,l,0x8e6555); lamps(b,l,0xffc698)
    for _,s in ipairs(signs) do
      f.table(b,s*3.66,-2.47,2.44,3.52)
      for j=-1,1 do pots(b,s*3.66,.92,-2.47+j*.89,.61+(j+1)*.21,pigments[(j+key)%#pigments+1]) end
      f.shelf(b,s*3.45,-l.depth+.65)
    end
    for j=0,9 do
      local a=j*math.pi/5
      b(-3.58+math.cos(a)*.94,1.20,2.65+math.sin(a)*.94,.72,2.4,.30,0xae7254,0,-a-math.pi/2)
    end
    b(-3.58,2.4,2.65,2.26,.32,2.18,0x806052)
    b(-3.58,1.06,3.70,.80,.78,.06,0x423e38)
    b(-3.58,1.06,3.745,.56,.56,.035,0xcc7945,7,0,0,2.8)
  end,
  [11]=function(b,l,key,f)
    runner(b,l,0x665a77); lamps(b,l,0xfac48d)
    b(-3.86,.16,-1.9,5.28,.32,5.47,cedar,3)
    b(-4.1,1.14,-2.65,2.72,.61,1.36,0x473c3a)
    b(-4.1,.93,-1.94,2.50,.22,.37,0x4a3a35)
    for j=0,23 do
      b(-5.25+j*.10,1.079,-1.91,.095,.047,.28,cream)
      if j%7~=2 and j%7~=6 then b(-5.20+j*.10,1.11,-1.99,.055,.044,.16,0x44393a) end
    end
    b(-4.1,.56,-1.28,1.35,.48,.48,cedar,3)
    b(-5.4,.90,-3.3,.19,1.24,.24,oak,3)
    b(-5.3,2.62,-3.3,.15,1.65,.15,brass,0,0,-.64)
    for j=0,7 do b(-5.47+j*.18,1.69+j*.055,-3.3,.022,1.32+j*.092,.021,brass) end
    for _,s in ipairs(signs) do for j=1,3 do
      local z=1.9+j*1.47
      b(s*4,.47,z,2.35,.46,.67,pigments[(key+j)%#pigments+1])
      b(s*4,.88,z+.30,2.35,.52,.14,oak,3)
    end end
  end,
  [12]=function(b,l)
    runner(b,l,0x668160); lamps(b,l,0xefc88c)
    for _,s in ipairs(signs) do for j=-2,2 do
      local x,z=s*3.81,j*2.76
      drawers(b,x,z,3.25,.85,6,1)
      for k=-3,3 do
        pots(b,x+k*.40,1.06,z,.43,0x997c53+(k+3)*0x060804)
        b(x+k*.40,1.20,z+.11,.168,.07,.014,cream)
      end
    end end
  end,
  [13]=function(b,l,key,f)
    runner(b,l,0x6b8b69); lamps(b,l,0xf2d1a1)
    for _,s in ipairs(signs) do for j=-3,3 do
      local x,z=s*7.4,j*4.45+1.8
      b(x,.24,z,5.45,.48,3.21,stone)
      b(x,.49,z,5.26,.072,3.02,0x5e5640)
      for k=-2,2 do f.plant(b,x+k*.96,z+.53*math.sin(k+j),1.62+(key+k+j)%5*.26) end
      for k=0,8 do
        local a=k*.698
        b(x+1.40*math.cos(a),1.56+k%3*.21,z+.91*math.sin(a),.30,.44,2.84,
          0x4e784f+k*0x020300,5,-a,-.33)
      end
    end end
    for j=-3,3 do b(0,l.height-.28,j*4.96,.20,.38,.19,brass) end
  end,
  [14]=function(b,l,key)
    runner(b,l,0x7d645e); lamps(b,l,0xffceaa)
    for _,s in ipairs(signs) do for j=-1,1 do
      local x,z=s*3.9,j*4.2
      for _,t in ipairs(signs) do b(x+t*.87,1.12,z,.18,2.24,.34,oak,3) end
      for _,y in ipairs({.35,2.15}) do b(x,y,z,1.98,.16,.31,cedar,3) end
      for k=-8,8 do b(x+k*.095,1.26,z,.018,1.62,.021,cream) end
      for k=0,7 do
        b(x,.50+k*.125,z+.039,1.53,.124,.038,pigments[(k+j+key)%#pigments+1])
      end
      b(x,.33,z+1.26,1.37,.66,.47,oak,3)
    end end
  end,
  [15]=function(b,l,key,f)
    runner(b,l,0x7c856a); lamps(b,l,0xf3c899)
    for _,s in ipairs(signs) do for j=-1,1 do
      local x,z=s*3.72,j*4.3+.45
      f.table(b,x,z,1.76,1.64)
      b(x,.94,z,1.39,.056,1.25,0x8d7771)
      for k=0,2 do b(x+.21*k,.982+k*.009,z-.25+k*.18,.69,.016,.43,cream,0,.11*k) end
      b(x,.966,z-.61,1.42,.07,.12,cedar,3)
    end end
    for j=0,2 do b(-3.31+j*.50,.75,3.01,.10,1.49,.11,cedar,3,0,(j-1)*.33) end
    b(-2.81,1.63,3.02,.77,.53,.42,0x55504c)
    b(-2.81,1.64,3.30,.38,.34,.24,brass)
    b(-2.81,1.64,3.43,.25,.23,.041,0x768f87,6)
  end,
  [16]=function(b,l)
    runner(b,l,0x626f79); lamps(b,l,0xe1dbac)
    for _,s in ipairs(signs) do for j=-2,2 do
      local x,z=s*6.34,j*6.53
      b(x,.23,z,4.08,.46,4.28,stone)
      b(x,.47,z,3.55,.042,3.82,0x53858a,9)
      for n=-2,2 do
        b(x+n*.71,.51,z+.28*math.sin(n+j),.45,.04,.40,0xbeb8a7,0,n*.18)
      end
      for _,t in ipairs(signs) do b(x+t*1.90,.48,z,.18,.16,4.28,0xacb6a9) end
    end end
    for j=-4,4 do b(0,1.58,j*3.54,5.8,.052,.032,0xb7c8bf,6,0,.22+j*.032) end
  end,
  [17]=function(b,l,key,f)
    runner(b,l,0x847351); lamps(b,l,0xffcd93)
    for _,s in ipairs(signs) do for j=-1,1 do
      local x,z=s*4.7,j*5.7+.15
      f.bed(b,x,z,pigments[(j+key)%#pigments+1])
      f.bed(function(X,y,Z,...) b(X,y+1.66,Z,...) end,x,z,pigments[(j+key+1)%#pigments+1])
      for _,u in ipairs(signs) do b(x+u*1.04,1.35,z-1.25,.126,2.7,.15,oak,3) end
      for n=0,5 do b(x+.84,.27+.31*n,z+1.16,.49,.091,.059,oak,3) end
    end end
  end,
  [18]=function(b,l)
    runner(b,l,0x987052); lamps(b,l,0xf3be86)
    for _,s in ipairs(signs) do for _,t in ipairs(signs) do
      local x,z=s*3.69,t*3.42
      for j=-4,4 do b(x+j*.162,.56,z,.149,.12,2.4,oak,3) end
      for j=-4,4 do b(x+j*.152,1.09,z+t*.29,.142,.13,1.67,oak,3) end
      b(x,.36,z,.25,.68,2.18,cedar,3)
    end end
    for j=-2,2 do b(2.72+j*.21,.93,-4.2,.19,.18,.38,stone,0,j*.34) end
    b(2.72,.46,-4.2,1.27,.88,1.02,0x625c55)
    b(2.72,.46,-3.66,.75,.56,.049,0xe29a54,7,0,0,3)
  end,
  [19]=function(b,l,key,f)
    runner(b,l,0x756887); lamps(b,l,0xe8c0a6)
    for _,s in ipairs(signs) do for j=-2,2 do
      local x,z=s*4.72,j*3.62+.40
      f.table(b,x,z,1.59,1.45)
      b(x,.936,z,1.25,.047,1.02,stone)
      for k=0,2 do
        b(x-.37+k*.37,1.18+(k%2)*.11,z,.25,.48+(k%2)*.22,.21,
          pigments[(j+k+key)%#pigments+1],k==1 and 6 or 0,.29*j,.21*k)
      end
      b(x,1.30,z+.51,1.24,.72,.026,0x94b9ac,8)
      b(x,1.68,z,1.27,.06,1.05,oak,3)
    end end
  end,
  [20]=function(b,l,key,f)
    runner(b,l,0x8a947e); lamps(b,l,0xf1d6a0)
    for _,s in ipairs(signs) do for j=-1,1 do
      local x,z=s*3.73,j*3.86
      drawers(b,x,z,2.25,1.03,3,1)
      for k=0,3 do
        b(x,.99+(k+1)*.11,z+.031*k,1.66-.115*k,.107,.83,
          pigments[(j+k+key)%#pigments+1])
      end
      pots(b,x-s*1.23,0,z+.72,.95,0x9c8056)
    end end
    f.plant(b,-3.70,5.1,1.3)
  end,
  [21]=function(b,l,key,f)
    runner(b,l,0x88724f); lamps(b,l,0xffc99a)
    for _,s in ipairs(signs) do for j=-1,1 do
      local x,z=s*4.08,j*3.98
      f.table(b,x,z,2.18,1.45)
      b(x,.96,z,1.45,.034,.84,cedar,3)
      for n=-1,1 do b(x+n*.28,1.02,z,.45,.075,.64,oak,3,0,n*.37) end
      b(x+.42,1.07,z,.88,.037,.14,brass,0,-.21)
      for n=0,3 do b(x+.76,1.096,z-.14+n*.061,.68,.012,.015,cream,0,-.21) end
      b(x,.33,z+1.13,.67,.56,.55,stone)
    end end
  end,
  [22]=function(b,l,key,f)
    runner(b,l,0x77948b); lamps(b,l,0xffdbad)
    for _,s in ipairs(signs) do for j=-1,1 do
      local x,z=s*4.50,j*4.7+.73
      b(x,.43,z,2.49,.52,.87,cedar,3)
      b(x,.74,z,2.43,.13,.82,pigments[(key+j)%#pigments+1])
      b(x,1.01,z+.43,2.55,.51,.14,oak,3)
      f.plant(b,x+s*1.22,z-1.13,1.42)
    end end
    for j=-3,3 do b(j*.34,2.82,-5.8,.07,.46+(j+3)*.075,.07,brass) end
  end,
  [23]=function(b,l,key,f)
    runner(b,l,0x966756); lamps(b,l,0xffd394)
    for j=-4,4 do
      local z=j*5.17+.66
      for _,s in ipairs(signs) do
        b(s*7.8,.235,z,.57,.47,.59,stone)
        b(s*7.8,(l.height-.5)*.5,z,.32,l.height-.5,.34,oak,3)
        b(s*7.8,l.height-.63,z,.57,.29,.59,cedar,3)
        b(s*4.5,.49,z,2.20,.47,.62,oak,3)
        f.plant(b,s*8.47,z+1.44,1.21+(key+j)%5*.16)
      end
      b(0,l.height-.50,z,16.0,.31,.27,cedar,3)
    end
  end
}
