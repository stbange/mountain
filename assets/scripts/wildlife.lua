-- Each stroke joins two points using an elliptical cross-section.
-- Local forward is -Z. Distances, widths and depths are in metres.
local sin, cos = math.sin, math.cos
local function legs(draw, x, hip, fore, hind, stride, fur, foot)
  for _, side in ipairs({-1, 1}) do
    for n, z in ipairs({fore, hind}) do
      local swing = side * (n == 1 and stride or -stride)
      draw(side*x,hip,z, side*x,hip*.46,z+swing, x*.44,x*.48,fur)
      draw(side*x,hip*.49,z+swing, side*x,.022,z+swing*1.40-.034, x*.25,x*.29,foot)
    end
  end
end

local function deer(draw, phase, move, id)
  local fur, dark = 0xa4825d, 0x614f40
  local nod=(1-move)*(.32+.12*cos(phase*.5))
  draw(0,.95,-.60, 0,.97,.69, .235,.32,fur)
  draw(0,.99,-.42, 0,1.51-nod,-.66-nod*.45, .135,.17,fur)
  draw(0,1.52-nod,-.60-nod*.45, 0,1.39-nod,-.99-nod*.45, .113,.126,fur)
  draw(0,1.41-nod,-.90-nod*.45, 0,1.38-nod,-1.02-nod*.45, .065,.06,dark)
  for _, s in ipairs({-1,1}) do
    draw(s*.065,1.51-nod,-.66-nod*.45, s*.20,1.80-nod,-.60-nod*.45,.060,.038,fur)
    if id%4==0 then
      draw(s*.06,1.58-nod,-.72-nod*.45,s*.17,1.89-nod,-.65-nod*.45,.027,.024,0x917a5e)
      draw(s*.13,1.78-nod,-.68-nod*.45,s*.23,1.90-nod,-.78-nod*.45,.019,.019,0xb29a72)
    end
  end
  draw(0,1.05,.49,0,.87,.73,.10,.13,0xe2ccab)
  legs(draw,.15,.88,-.42,.43,.14*sin(phase)*move,fur,dark)
end
local function fox(draw, phase, move)
  local red, dark=0xb65c33,0x453b32
  draw(0,.34,-.40,0,.39,.42,.12,.16,red)
  draw(0,.43,-.25,0,.55,-.55,.112,.13,red)
  draw(0,.55,-.46,0,.48,-.72,.094,.077,red)
  draw(0,.50,-.65,0,.47,-.74,.040,.040,dark)
  for _,s in ipairs({-1,1}) do
    draw(s*.065,.56,-.48,s*.10,.72,-.45,.040,.051,dark)
  end
  local sway=.16*sin(phase*.55)
  draw(0,.39,.27,sway,.22,.86,.15,.14,red)
  draw(sway*.72,.26,.70,sway,.22,.96,.095,.092,0xe8dbbf)
  legs(draw,.082,.32,-.26,.26,.085*sin(phase)*move,red,dark)
end
local function hare(draw, phase, move, winter)
  local fur=winter and 0xe4e3d9 or 0xa79d83
  draw(0,.18,-.18,0,.22,.23,.12,.17,fur)
  draw(0,.27,-.10,0,.34,-.29,.078,.085,fur)
  for _,s in ipairs({-1,1}) do
    draw(s*.037,.36,-.20,s*.065,.56,-.16+.045*move,.029,.036,fur)
    draw(s*.083,.20,.15,s*.099,.051,.11-.06*sin(phase),.063,.073,fur)
    draw(s*.073,.18,-.14,s*.071,.035,-.19,.030,.038,fur)
  end
  draw(0,.23,.16,0,.24,.28,.052,.059,0xe7dfc9)
end
local function marmot(draw, phase, move)
  local fur=0x987a54
  local lift=.18*(1-move)
  draw(0,.16,.22,0,.27+lift,-.12,.15,.17,fur)
  draw(0,.26+lift,-.095,0,.32+lift,-.28,.104,.094,0xb39265)
  draw(0,.31+lift,-.23,0,.30+lift,-.32,.043,.04,0x575041)
  draw(0,.15,.19,.10,.065,.43,.068,.061,fur)
  for _,s in ipairs({-1,1}) do
    draw(s*.085,.20+lift,-.07,s*.10,.06+lift,-.15,.037,.045,0x66543f)
  end
end
local function eagle(draw, phase, move)
  draw(0,0,-.40,0,0,.46,.105,.12,0x534735)
  draw(0,.07,-.25,0,.07,-.51,.07,.078,0x9b8252)
  draw(0,-.01,.26,0,-.025,.65,.15,.033,0x514737)
  for _,s in ipairs({-1,1}) do
    local wing=.10+.19*sin(phase)*move
    draw(s*.055,.02,0,s*.62,wing,-.044,.22,.045,0x64543d)
    draw(s*.54,wing,-.04,s*1.10,wing-.07,.15,.18,.034,0x453f34)
    for i=0,2 do draw(s*.91,wing-.042,.04+i*.067,s*(1.14-i*.05),wing-.09,.08+i*.12,.042,.019,0x403c33) end
  end
end
function build_animal(kind, season, phase, motion, id, draw)
  if kind==1 then deer(draw,phase,motion,id)
  elseif kind==2 then fox(draw,phase,motion)
  elseif kind==3 then hare(draw,phase,motion,season==3)
  elseif kind==4 then marmot(draw,phase,motion)
  else eagle(draw,phase,motion) end
end

-- All sightings are animated in their own local frame, independently of the hiker.
local function canine(draw,phase,cinder)
  local step=sin(phase)
  local coat,ruff,feet=0x777568,0x9a9582,0x4e504b
  if cinder then coat,ruff,feet=0x524552,0xb56c4d,0x34333a end
    draw(0,.72,.73,0,.90,-.61,.25,.32,coat)
    draw(0,.90,-.41,0,1.16,-.79,.24,.29,ruff)
    draw(0,1.17,-.65,0,1.06,-1.23,.16,.15,coat)
    draw(0,1.07,-1.09,0,1.04,-1.25,.078,.075,feet)
    for _,s in ipairs({-1,1}) do draw(s*.10,1.21,-.79,s*.15,1.47,-.72,.064,.076,feet) end
    draw(0,.82,.51,.12*cos(phase*.45),.31,1.29,.15,.17,coat)
    legs(draw,.16,.70,-.43,.44,.18*step,coat,feet)
  if cinder then for j=0,4 do
    draw(0,.96,.39-j*.17,0,1.18+j*.045,.50-j*.17,.075,.10,0xe59a59)
  end end
end

local function colossus(draw,phase,cinder)
  local step=sin(phase)
  local stone,moss,lichen=0x5e685e,0x778367,0xa69b7d
  if cinder then stone,moss,lichen=0x50434b,0x715347,0xee9959 end
  draw(0,7.1,.85,0,14.6,-.42,2.9,2.2,stone)
  draw(0,13.0,-.2,0,16.7,-1.36,1.56,1.65,moss)
  for _,s in ipairs({-1,1}) do
    draw(s*1.34,8,.56,s*1.76,4.1,.3+s*step,1.22,1.38,stone)
    draw(s*1.75,4.4,.3+s*step,s*1.91,.30,s*step*1.55,.91,1.04,stone)
    draw(s*2.22,13.5,0,s*3.40,9.2,-s*step,.95,1.07,moss)
    draw(s*3.4,9.5,-s*step,s*3.65,6.92,-s*step-.6,.63,.74,stone)
    if cinder then
      for i=1,4 do
        draw(s*(.55+i*.23),10+i*.57,-1.65,s*(.9+i*.26),10.7+i*.57,-1.86,.14,.18,lichen)
        draw(s*(1.0+i*.2),14.2,-.45,s*(1.4+i*.39),14.6+i*.47,-.25,.32,.33,stone)
      end
    else
    draw(s*.93,15.7,-1.1,s*2.18,18.55,-.45,.36,.38,lichen)
    draw(s*2.18,18.4,-.48,s*3.54,19.8,.3,.17,.21,lichen)
    draw(s*1.97,18.1,-.6,s*2.04,19.72,-1.32,.18,.23,lichen)
    end
    draw(s*.85,15.1,-2.46,s*.41,15.02,-2.68,.14,.11,0xc7b987)
  end
end

local function bear(draw,p)
  local fur,face=0x65483a,0x9b7959
  draw(0,.81,.91,0,1.08,-.79,.43,.55,fur)
  draw(0,1.14,-.62,0,1.35,-1.07,.37,.45,fur)
  draw(0,1.28,-.92,0,1.04,-1.51,.25,.23,face)
  for _,s in ipairs({-1,1}) do draw(s*.22,1.43,-1.0,s*.25,1.62,-.99,.10,.095,fur) end
  legs(draw,.31,.81,-.55,.57,.19*sin(p),fur,0x473a32)
end
local function moose(draw,p)
  local fur=0x695446
  draw(0,1.32,1.08,0,1.66,-.91,.40,.51,fur)
  draw(0,1.61,-.66,0,2.13,-1.26,.30,.36,0x5b4b41)
  draw(0,2.12,-1.12,0,1.82,-1.85,.22,.25,fur)
  draw(0,1.83,-1.28,0,1.41,-1.36,.10,.12,fur)
  legs(draw,.24,1.35,-.61,.66,.24*sin(p),fur,0x968473)
  for _,s in ipairs({-1,1}) do
    draw(s*.10,2.12,-1.2,s*.89,2.55,-1.05,.17,.12,0xb39a78)
    for j=1,4 do draw(s*(.31+.14*j),2.25+.06*j,-1.07,s*(.43+.16*j),2.57+.08*j,-1.23,.052,.055,0xc4ae8c) end
  end
end
local function cougar(draw,p)
  local fur=0xb58a60
  draw(0,.67,.81,0,.79,-.73,.23,.29,fur)
  draw(0,.78,-.56,0,.96,-1.04,.18,.19,fur)
  for _,s in ipairs({-1,1}) do draw(s*.09,.98,-.89,s*.15,1.09,-.84,.059,.059,0x57493e) end
  draw(0,.74,.6,.16,.54,1.29,.10,.092,fur)
  draw(.16,.54,1.28,.33*sin(p*.5),.71,1.91,.064,.065,0x705646)
  legs(draw,.17,.66,-.45,.50,.19*sin(p),fur,0x9c7857)
end
local function snake(draw,p,green)
  local skin=green and 0x718a49 or 0xbb935f
  local lastx,lastz=0,0
  for j=1,16 do
    local z=j*.15
    local x=.23*(sin(p+z*3.7)-sin(p))
    local width=.075+.045*sin(math.pi*j/17)
    draw(lastx,.10,lastz,x,.10,z,width,width*.72,j%3==0 and skin-0x151006 or skin)
    lastx,lastz=x,z
  end
  draw(0,.13,.17,0,.14,-.27,.145,.064,skin)
  for _,s in ipairs({-1,1}) do draw(s*.06,.16,-.13,s*.073,.22,-.16,.023,.025,skin) end
end
local function swimmer(draw,p,orca)
  local swim=sin(p)
  if orca then
    draw(0,-.34,-3.31,0,-.41,3.2,.91,.98,0x323d45)
    draw(0,-.24,-2.86,0,-.34,-3.65,.55,.44,0x35414a)
    draw(0,.26,-.31,0,1.93,.56,.10,.50,0x333a41)
    for _,s in ipairs({-1,1}) do
      draw(s*.69,-.19,-2.35,s*.72,.04,-1.71,.24,.17,0xe5e5d9)
      draw(s*.62,-.36,-.65,s*1.74,-.66,.08,.36,.15,0x36424a)
      draw(0,-.44,2.71,s*1.39,-.41+swim*.15,3.53,.40,.13,0x37404a)
    end
  else
    local leap=math.max(0,sin(p*.46))*.64
    draw(0,leap-.1,-1.03,0,leap-.08,1.14,.17,.23,0x729297)
    draw(0,leap-.08,-.72,0,leap-.19,-1.47,.098,.10,0xaebfba)
    for _,s in ipairs({-1,1}) do
      draw(0,leap-.08,.99,swim*.14,leap+s*.35,1.43,.09,.11,0x687f82)
      draw(s*.12,leap,-.24,s*.35,leap-.12,.14,.073,.056,0x7c9697)
    end
  end
end
local function spiral(draw,p)
  for j=0,33 do
    local u=j/34
    local a=u*17+p*.37
    local r=.65+u*u*4.1
    draw(r*cos(a),u*17,r*sin(a),r*cos(a+.67),(u+.035)*17,r*sin(a+.67),
         .27+u*.34,.38+u*.36,j%3==0 and 0xc6d6cd or 0x8caaa9)
  end
end
local function debris(draw,p,model)
  local sand=model=="sandfall"
  for j=1,32 do
    local t=(p*.095+j*.323)%1
    local x=sin(j*2.4)*(sand and 6 or 3.8)
    local y=.40+7*(1-t)^2+math.abs(sin(t*11))*.9
    local z=t*9-4
    local size=sand and .55+1.6*t or .34+(j%6)*.20
    local c=sand and (0xc29360+j%3*0x060400) or (0x69676b+j%5*0x060505)
    draw(x,y,z,x+.47*size,y+size,z+.75*size,size*.79,size*.73,c)
  end
end
local function raft(draw,p)
  for j=1,12 do
    local x,z=sin(j*2.4)*2.9,cos(j*2.4)*3.3
    local y=.13+.11*sin(p+j)
    draw(x,y,z,x+sin(j)*2.1,y+.30,z+cos(j)*3.5,.19,.21,0x76654e+j%4*0x070603)
    draw(x+.12,y+.08,z+.12,x-.55,y+.73,z+.63,.092,.084,0x827761)
  end
end
local function rover(draw,p)
  draw(0,.83,-1.45,0,.83,1.40,.87,.37,0xc9c0a5)
  draw(0,1.07,-.37,0,1.98,-.38,.12,.12,0x85888a)
  draw(-.31,1.93,-.50,.39,1.98,-.52,.18,.17,0x676b72)
  for _,s in ipairs({-1,1}) do for j=-1,1 do
    local z=j*.96
    draw(s*.52,.76,z,s*.96,.41,z+.16,.08,.09,0x999a94)
    draw(s*.84,.38,z,s*1.13,.38,z,.38,.38,0x565962)
    draw(s*1.12,.38,z,s*1.17,.38+.15*cos(p),z+.15*sin(p),.10,.095,0xc2b491)
  end end
end
local function crawler(draw,p,style)
  local facet=style=="facet"
  local fungus=style=="stalker"
  local scale=facet and 4.0 or 1
  local c=fungus and 0x708266 or 0x83b2bc
  local glow=fungus and 0xbad89a or 0xc4e5df
  local function part(x,y,z,X,Y,Z,w,d,col)
    draw(x*scale,y*scale,z*scale,X*scale,Y*scale,Z*scale,w*scale,d*scale,col)
  end
  part(0,1.0,1.2,0,1.36,-.83,.42,.39,c)
  part(0,1.32,-.69,0,1.82,-1.2,.20,.20,glow)
  for _,s in ipairs({-1,1}) do
    part(s*.12,1.77,-1.15,s*.49,2.35,-1.34,.042,.042,glow)
    for j=-1,1 do
      local stride=sin(p+j*1.8)*s*.25
      part(s*.25,1.18,j*.74,s*1.10,1.68,j*.84+stride,.11,.14,c)
      part(s*1.10,1.68,j*.84+stride,s*1.54,.05,j*.95+stride,.065,.085,glow)
    end
    if style=="mantis" then part(s*.17,1.65,-.96,s*.58,2.1,-1.61,.12,.11,c) end
  end
  if facet then for j=-2,2 do part(0,1.43,j*.39,.24,2.43-.13*math.abs(j),j*.45,.22,.26,glow) end end
end
local function grazer(draw,p)
  draw(0,3.75,3.3,0,4.20,-2.3,2.08,1.89,0x687f70)
  draw(0,4.0,-1.9,0,6.2,-3.21,.81,.85,0x88977b)
  for _,s in ipairs({-1,1}) do for j=-1,1 do
    local z,swing=j*1.97,s*sin(p)*.52
    draw(s*1.04,3.49,z,s*1.41,.08,z+swing,.43,.56,0x616e66)
  end end
  for j=0,8 do
    local x,z=cos(j*2.4)*1.33,sin(j*2.4)*2.1
    draw(x,4.62,z,x,5.65,z,.13,.16,0xc2bf8a)
    draw(x,5.23,z,x,5.95,z,.67,.71,0xb1ce91)
  end
end
local function skyfish(draw,p,large)
  local s=large and 4.0 or 1.0
  local wing=sin(p*.77)*s
  local c=large and 0x698ca2 or 0x86b3bb
  draw(0,0,-2*s,0,0,2.2*s,.54*s,.47*s,c)
  for _,k in ipairs({-1,1}) do
    draw(k*.22*s,0,-.45*s,k*2.27*s,wing*.55,.34*s,.73*s,.15*s,0xa3c0ba)
    draw(k*2.02*s,wing*.5,.28*s,k*3.0*s,wing*.79,.9*s,.33*s,.09*s,0xe0b7aa)
  end
  draw(0,0,1.71*s,sin(p*.5)*.46*s,.3*s,3.8*s,.14*s,.15*s,c)
  if large then for j=1,6 do
    draw(.22*s,-.16*s,j*.49*s-s,.37*s,-1.08*s,j*.59*s-s,.084*s,.07*s,0xcfb7ad)
  end end
end

function build_visitor(model,phase,draw)
  if model=="wolf" or model=="hound" then canine(draw,phase,model=="hound")
  elseif model=="giant" or model=="colossus" then colossus(draw,phase,model=="colossus")
  elseif model=="bear" then bear(draw,phase)
  elseif model=="moose" then moose(draw,phase)
  elseif model=="cougar" then cougar(draw,phase)
  elseif model=="viper" or model=="green_viper" then snake(draw,phase,model=="green_viper")
  elseif model=="orca" or model=="barracuda" then swimmer(draw,phase,model=="orca")
  elseif model=="waterspout" then spiral(draw,phase)
  elseif model=="rockfall" or model=="sandfall" then debris(draw,phase,model)
  elseif model=="driftwood" then raft(draw,phase)
  elseif model=="rover" then rover(draw,phase)
  elseif model=="mantis" or model=="facet" or model=="stalker" then crawler(draw,phase,model)
  elseif model=="grazer" then grazer(draw,phase)
  elseif model=="ray" or model=="leviathan" then skyfish(draw,phase,model=="leviathan")
  else error("Unknown lookout sighting: "..model) end
end
