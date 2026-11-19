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
