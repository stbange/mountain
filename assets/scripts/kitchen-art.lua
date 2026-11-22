-- Harvest patches, a glazed mug and a plate of biscuits, in metres.
-- The engine sweeps tapered, faceted sections between each pair of endpoints.
function kitchen_model(kind, limb)
  local function stalk(x,y,z,X,Y,Z,a,b,col,n,flat,mat)
    limb(x,y,z,X,Y,Z,a,b,n or 7,flat or 1,col,mat or 0)
  end
  local function pot(x,z,y,h,bottom,top,col)
    stalk(x,y,z,x,y+h,z,bottom,top,col,14)
  end
  local function leaf(x,y,z,a,col)
    stalk(x,y,z,x+.27*math.cos(a),y+.14,z+.27*math.sin(a),.081,.014,col,5,.42)
  end
  if kind==14 then
    pot(0,0,0,.034,.074,.074,0xbcc6bd)
    pot(0,0,.034,.15,.068,.089,0xc78d69)
    pot(0,0,.184,.044,.09,.098,0xd4ded0)
    pot(0,0,.226,.004,.095,.095,0xdca377)
    pot(-.030,.01,.230,.013,.026,.025,0xe1ded1)
    leaf(.064,.217,0,.4,0x84935b)
    return
  elseif kind==9 or kind==13 then
    pot(0,0,0,.025,.085,.095,0x68756a)
    pot(0,0,.024,.170,.085,.116,0xe0c79b)
    pot(0,0,.054,.019,.091,.095,0x6e9e98)
    pot(0,0,.079,.006,.097,.098,0x70968b)
    pot(0,0,.192,.013,.116,.116,0xf5e6c8)
    pot(0,0,.204,.003,.100,.100,kind==13 and 0xaf793f or 0x563927)
    pot(.006,.002,.207,.002,.062,.055,kind==13 and 0xc49151 or 0x77563c)
    for i=0,6 do
      local a,b=(-.5+i/7)*math.pi,(-.5+(i+1)/7)*math.pi
      stalk(.102+.083*math.cos(a),.116+.060*math.sin(a),0,
            .102+.083*math.cos(b),.116+.060*math.sin(b),0,.013,.013,0xd2b486)
    end
    return
  elseif kind>=10 then
    pot(0,0,0,.019,.15,.198,0x87988b)
    pot(0,0,.019,.011,.197,.202,0xe3dcc3)
    if kind==10 then
      for j=0,2 do
        local x,z=math.cos(j*2.1)*.077,math.sin(j*2.1)*.066
        pot(x,z,.03+j*.007,.030,.076,.070,0xc3874e+j*0x080601)
        for k=0,4 do
          local a=k*2.4+j
          pot(x+math.cos(a)*.042,z+math.sin(a)*.042,.061+j*.007,.006,.008,.007,0x734a31)
        end
      end
    elseif kind==11 then
      pot(0,0,.03,.065,.15,.13,0xc18c58)
      for k=0,5 do pot(math.cos(k)*.08,math.sin(k)*.08,.094,.012,.012,.01,0x916348) end
    else
      pot(0,0,.03,.09,.10,.16,0x849d8c)
      pot(0,0,.12,.003,.15,.15,0xba8052)
      leaf(0,.124,0,.5,0x778852)
    end
    return
  elseif kind==8 then
    pot(0,0,0,.18,.64,.65,0x838e95)
    pot(0,0,.18,.55,.60,.49,0xd1d3bd)
    pot(0,0,.73,.05,.50,.48,0x497386)
    pot(0,0,.78,.31,.46,.22,0x97b7ae)
    for j=-1,1 do stalk(j*.11,.43,-.48,j*.11,.57,-.50,.044,.044,0xa7cd86,5,1,7) end
    stalk(.49,.40,0,.8,1.06,0,.041,.031,0x788492)
    stalk(.62,1.01,0,1.10,1.12,0,.24,.24,0x637b91,4,.22)
    return
  end
  for j=0,5 do
    local a=j*2.4
    local x,z=math.cos(a)*(.16+j*.055),math.sin(a)*(.16+j*.055)
    local h=.29+.12*(j%3)
    if kind==2 then
      stalk(x,0,z,x+.023,h,z,.055,.055,0xe3c899)
      stalk(x+.023,h-.045,z,x+.023,h+.09,z,.20,.057,0xba8048,10,.82)
    elseif kind==5 then
      pot(x,z,0,.15,.11,.08,0xae8c63)
      for k=0,2 do leaf(x,.14,z,a+2.1*k,0x789258) end
    elseif kind==6 then
      stalk(x,0,z,x+.090,.38,z,.115,.060,0x678369,6,.26)
      stalk(x+.09,.36,z,x-.09,.67,z+.08,.062,.015,0x839756,6,.32)
    else
      local X,Z=x+math.cos(a)*.10,z+math.sin(a)*.10
      stalk(x,0,z,X,h+.15,Z,.027,.014,0x75804e)
      for k=0,2 do leaf(X,h*.35+k*.084,Z,a+k*2,0x71935b+k*0x050704) end
      if kind==0 or kind==3 then
        local r=kind==3 and .10 or .06
        for k=0,2 do pot(X+math.cos(k*2)*.062,Z+math.sin(k*2)*.062,h,.10,r,r*.67,
          kind==3 and 0xc2985a or 0x865675) end
      elseif kind==4 then
        stalk(X,h,Z,X+.07,h+.29,Z,.063,.020,0xc6aa65,7,.71)
      elseif kind==7 then
        stalk(X,h+.14,Z,X-.05,h-.06,Z+.09,.044,.023,0x9ea958)
      end
    end
  end
end
