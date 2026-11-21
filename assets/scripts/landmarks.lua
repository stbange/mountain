-- A tube joins two points, with independently tapered ends and an elliptical cross section.
-- Local forward is -Z. Material 6 glows in the dark.
local sin, cos, pi = math.sin, math.cos, math.pi
local function limb(draw,x,y,z,X,Y,Z,r,R,colour,flat,mat,sides)
  draw(x,y,z,X,Y,Z,r,R,sides or 7,flat or 1,colour,mat or 0)
end
local function ring(draw,x,y,z,r,wire,colour,vertical,mat)
  for i=0,15 do
    local a,b=i*pi/8,(i+1)*pi/8
    local c,d=r*cos(a),r*cos(b)
    local e,f=r*sin(a),r*sin(b)
    if vertical then limb(draw,x+c,y+e,z,x+d,y+f,z,wire,wire,colour,1,mat,5)
    else limb(draw,x+c,y,z+e,x+d,y,z+f,wire,wire,colour,1,mat,5) end
  end
end
local function arch(draw,v,col)
  for i=0,10 do
    local a,b=i*pi/11,(i+1)*pi/11
    limb(draw,7*cos(a),6.7*sin(a)-.35,0,7*cos(b),6.7*sin(b)-.35,0,
         1.25+.28*sin(i*2+v),1.45, col,1.4,0,7)
  end
end
local function mushroom(draw,v)
  local h=4.4+.65*v
  limb(draw,0,-.2,0,.35,h,0,.44,.25,0x9ca9a0)
  limb(draw,.35,h-.4,0,.35,h+.35,0,.5,3.1,0x83cda2,.98,6,12)
  limb(draw,.35,h+.35,0,.35,h+1.7,0,3.14,.4,0x9977a1,1,0,12)
  for i=0,9 do
    local a=i*.628
    limb(draw,.35,h-.34,0,.35+2.8*cos(a),h+.23,2.8*sin(a),.05,.12,0xe1e7a6,1,6,4)
  end
end
local function palm(draw,v)
  local h=5+v*.7
  for i=0,5 do
    limb(draw,.024*i*i,i*h/6,0,.024*(i+1)^2,(i+1)*h/6,0,.25-i*.014,.22-i*.014,0x96714d)
  end
  for i=0,10 do
    local a=i*.572+v*.3
    local x,z=cos(a),sin(a)
    limb(draw,.86,h,0,.86+1.7*x,h+1.1,1.7*z,.14,.32,0x788d4c,.25)
    limb(draw,.86+1.7*x,h+1.1,1.7*z,.86+3.8*x,h-.65,3.8*z,.36,.012,0x708447,.23)
    for j=1,4 do
      local t=j*.46
      for _,side in ipairs({-1,1}) do
        limb(draw,.86+(1+t)*x,h+.95-.38*t,(1+t)*z,
          .86+(1.4+t)*x-side*.6*z,h+.15-.56*t,(1.4+t)*z+side*.6*x,.115,.008,0x829552,.27,0,4)
      end
    end
  end
end
local function cluster(draw,v,col)
  for i=0,6 do
    local a=i*2.4+v*.35
    local x,z=cos(a)*.6,sin(a)*.6
    local h=1.3+(i%3)*.54
    limb(draw,x,-.12,z,x*1.6,h,z*1.6,.27,.32,col,1,0,5)
    limb(draw,x*1.6,h,z*1.6,x*2,h+.8,z*2,.32,0,col,1,6,5)
  end
end
local function growth(draw,p,v,col)
  if p==3 then palm(draw,v)
  elseif p==6 then
    for i=0,6 do
      local a=i*2.4
      local x,z=cos(a)*.9,sin(a)*.9
      local h=4.2+(i%3)+v*.4
      limb(draw,x,0,z,x*1.4,h,z*1.4,.11,.065,0x807e45)
      for j=1,3 do
        limb(draw,x*1.3,h*j/4,z*1.3,x*(2+j*.23),h*j/4+.65,z*(2+j*.23),.16,.01,0x568354,.24)
      end
    end
  elseif p==9 then mushroom(draw,v)
  elseif p==8 then cluster(draw,v,col)
  elseif p==11 then
    for i=0,3 do
      local a=i*1.6+v
      limb(draw,.3*cos(a),-.08,.3*sin(a),1.03*cos(a),1.2+.2*i,1.03*sin(a),.46,.012,0x4e424f,.48,0,5)
      limb(draw,.23*cos(a),.05,.23*sin(a),.62*cos(a),.57,.62*sin(a),.055,.018,col,.6,6,5)
    end
  elseif p==10 then
    for i=0,5 do
      local a=i*1.05
      limb(draw,0,0,0,cos(a)*.8,2.5+(i%2),sin(a)*.8,.21,.09,0xc18486)
      limb(draw,cos(a)*.5,1.5,sin(a)*.5,cos(a)*1.45,2.2,sin(a)*1.45,.12,.03,col,1,6)
    end
  elseif p==5 then
    for i=0,8 do
      local a=i*.698+v
      limb(draw,0,0,0,cos(a)*.85,1.25-(i%3)*.27,sin(a)*.85,.19,0,0x87937a,.3)
    end
  elseif p==2 then
    for i=0,5 do
      local a=i*2.4+v
      limb(draw,0,0,0,cos(a)*.8,1,sin(a)*.8,.16,.07,0x8f806a)
      limb(draw,cos(a)*.8,.91,sin(a)*.8,cos(a)*1.23,1.65,sin(a)*1.23,.66,.22,0x818861,.78)
    end
  else limb(draw,0,-.1,0,.3,.75,0,.72,.45,col,.78) end
end
local rocks={0x858784,0x858784,0xe4dbcb,0xad7852,0x424954,0xb96142,0x87958c,0x888990,0x84709b,0x726a79,0x9b9296,0x403849}
local function feature(draw,p,v,col)
  local stone=rocks[p+1]
  if p==2 or p==5 then arch(draw,v,stone)
  elseif p==3 then palm(draw,v)
  elseif p==4 then
    for i=0,12 do
      local a=i*2.4
      local x,z=2.7*cos(a)*math.sqrt(i/12),2.7*sin(a)*math.sqrt(i/12)
      limb(draw,x,-1,z,x,3.2+(i*7+v)%9,z,.85,.8,stone,1,0,6)
    end
  elseif p==7 then
    for i=0,8 do
      local a=i*2.4+v
      limb(draw,cos(a)*i*.56,-.20,sin(a)*i*.56,cos(a)*i*.6,1.3+(i%3),sin(a)*i*.6,.9,.25,stone,1,0,6)
    end
  elseif p==10 then
    limb(draw,0,6,0,.5,15,0,.25,7.8,0x736c88,1,0,9)
    limb(draw,.5,15,0,-.3,16.1,0,7.8,5.3,0x94c1a6,1,0,9)
    for i=1,3 do limb(draw,i,15.6,-4+i,i-.4,1.7+i*.9,-4+i,.20,.04,0x9bddde,.65,6) end
  elseif p==11 then
    limb(draw,0,.05,0,0,.14,0,2.1,1.8,col,1,6,13)
    for i=0,6 do
      local a=i*.898
      limb(draw,cos(a)*2.2,-1,sin(a)*2.2,cos(a)*.9,2.8,sin(a)*.9,1.1,.48,stone)
    end
    ring(draw,0,2.7,0,1,.14,col,false,6)
  else growth(draw,p,v,col) end
end
local function relic(draw,p,v,col)
  limb(draw,0,-.15,0,0,.32,0,.84,.71,0x69666a,1,0,9)
  if p==0 or p==7 or p==2 or p==5 then
    local r=p==2 and .73 or 1.05
    ring(draw,0,1.52,0,r,.095,col,true,6)
    ring(draw,0,1.49,0,r*.74,.052,0xf3dca6,false,6)
    if p==2 then
      for i=1,12 do
        local a=i*.55
        limb(draw,cos(a)*i*.044,1.42+sin(a)*i*.044,0,cos(a+.55)*(i+1)*.044,1.42+sin(a+.55)*(i+1)*.044,0,.065,.082,0xf4c49e)
      end
    else limb(draw,0,.31,0,0,1.9,0,.14,.045,col) end
  elseif p==1 or p==8 or p==11 then
    cluster(draw,v,col)
    ring(draw,0,1.60,0,1.24,.065,col,false,6)
  elseif p==3 or p==4 then
    limb(draw,0,.37,0,0,1.28,0,.71,.29,col,1,6,10)
    ring(draw,0,1.45,0,.72,.08,0xe7c580,true)
  elseif p==9 then mushroom(draw,v)
  else
    limb(draw,0,.42,0,0,1.18,0,.12,.68,col,1,6,9)
    limb(draw,0,1.18,0,0,1.82,0,.68,.11,col,1,6,9)
    ring(draw,0,1.16,0,.93,.06,0xe8d2a2,false)
  end
end
local function shell(draw,x,y,z,r,col)
  for j=-4,4 do
    local a=j*.24
    limb(draw,x,y,z,x+r*sin(a),y+r*.64*cos(a),z-r*.48*cos(a),.09*r,.14*r,col,.55,0,5)
  end
end
local function track(draw,p,col)
  for i=0,2 do
    local x,z=(i%2-.5)*.38,-i*.77
    local h=.95-i*.19
    if p==2 then
      shell(draw,x,.10,z,h,col)
    elseif p==3 or p==6 or p==8 then
      for j=-1,1 do
        limb(draw,x+j*.09,-.04,z,x+j*.19,h+j*.11,z-.37,.065,.05,col,1,p==8 and 6 or 0,6)
      end
      ring(draw,x,.34,z-.14,.19,.034,0xa38156,false)
    elseif p==9 then
      limb(draw,x,-.03,z,x,h*.55,z-.10,.09,.06,0xaeb4b2)
      limb(draw,x,h*.55,z-.10,x,h*.83,z-.19,.075,.34,col,1,6,9)
    elseif p==7 or p==10 then
      limb(draw,x,0,z,x,h*.73,z-.19,.12,.08,rocks[p+1])
      limb(draw,x,h*.66,z-.19,x+.13,h,z-.43,.18,.05,col,.37,6,5)
    else
      limb(draw,x,-.08,z,x+.06,h,z-.29,.37-i*.06,.15,rocks[p+1],1,0,6)
      limb(draw,x+.06,h-.11,z-.29,x+.08,h+.13,z-.41,.18,.08,col,.80,p==11 and 6 or 0,6)
    end
  end
end
local function marker(draw,p,col)
  if p==1 or p==3 or p==9 then
    local base=p==9 and 0x838d90 or rocks[p+1]
    limb(draw,0,-.15,0,0,.75,0,.65,.53,base)
    limb(draw,0,.75,0,0,.90,0,1.06,1.02,base,1,0,12)
    ring(draw,0,.94,0,.85,.033,col,false,6)
    limb(draw,0,.90,0,0,1.75,-.19,.13,.022,col)
    for i=-3,3 do
      local a=i*.33
      limb(draw,.74*sin(a),.96,-.74*cos(a),.93*sin(a),.96,-.93*cos(a),.022,.033,col)
    end
  elseif p==2 then
    limb(draw,0,0,0,0,1.15,0,.25,.13,rocks[p+1])
    shell(draw,0,1.12,0,1.27,col)
  elseif p==4 or p==6 or p==8 or p==11 then
    local stem=p==6 and 0x879c62 or rocks[p+1]
    limb(draw,0,-.12,0,0,2.06,0,.32,.28,stem,1,0,p==4 and 6 or 8)
    ring(draw,0,2.08,0,.24,.07,col,false,6)
    limb(draw,0,2.084,0,0,2.09,0,.16,.16,0x292f38)
  else
    limb(draw,0,-.08,0,.04,1.8,-.12,.23,.19,rocks[p+1],1,0,7)
    limb(draw,.04,1.78,-.12,.06,2.14,-.55,.23,.034,col,1,6,7)
  end
end
function build_landmark(p,kind,v,col,draw)
  if kind==0 then
    local rock=rocks[p+1]
    limb(draw,0,-.25,0,.16,.38,0,.85,.90,rock,.73,0,7)
    limb(draw,.16,.38,0,.28,.79+.13*v,.07,.90,.28,rock,.73,0,7)
  elseif kind==1 then growth(draw,p,v,col)
  elseif kind==2 then feature(draw,p,v,col)
  elseif kind==3 then track(draw,p,col)
  elseif kind==4 then marker(draw,p,col)
  elseif kind==5 then relic(draw,p,v,col)
  else
    limb(draw,0,0,-.4,0,.05,.48,.12,.07,col,.7)
    for _,s in ipairs({-1,1}) do
      limb(draw,0,.04,0,s*.65,.26,0,.16,.16,col,.19,0,5)
      limb(draw,s*.65,.26,0,s*1.27,.10,.23,.16,.012,col,.19,0,5)
    end
  end
end
