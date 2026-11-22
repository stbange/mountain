IN vec3 v_pos;
IN vec3 v_colour;
IN vec3 v_uvm;
IN float v_dist;
uniform sampler2D u_lake;
uniform sampler2D u_trail;
uniform float u_season;
uniform int u_profile;
uniform vec3 u_soil;
uniform vec3 u_liquid;
uniform vec3 u_trail_center;
uniform int u_reflection;
uniform float u_water;
uniform vec4 u_viewport;
uniform int u_layer;
uniform vec3 u_eye;
uniform vec3 u_sun;
uniform vec3 u_horizon;
uniform vec3 u_zenith;
uniform vec4 u_air;
uniform float u_clock;
uniform vec3 u_sky_fill;
uniform vec3 u_bounce;
uniform vec4 u_light_params; // cloud strength, shadow softness, inverse near/far texture sizes
uniform vec3 u_cloud_origin;
uniform sampler2D u_near_depth;
uniform sampler2D u_far_depth;
uniform mat4 u_near_light;
uniform mat4 u_far_light;
uniform int u_house;
uniform vec3 u_room_origin;
uniform vec4 u_room_axes; // cosine, sine, half width and half depth
uniform int u_lookout; // 0 calm, 1 event, 2 lightning, 3 heat shimmer
uniform vec4 u_event_air; // sunlight tint and extinction per metre
vec3 roomLocal(vec3 p) {
    vec3 d=p-u_room_origin;
    return vec3(dot(d.xz,u_room_axes.xy),d.y,d.z*u_room_axes.x-d.x*u_room_axes.y);
}
bool throughDoor(float group) {
    vec3 a=roomLocal(u_eye), b=roomLocal(v_pos);
    bool side=group>2.5;
    float wall=side?u_room_axes.z:(group<1.5?u_room_axes.w:-u_room_axes.w);
    float delta=side?b.x-a.x:b.z-a.z;
    if(abs(delta)<.00001) return false;
    float t=(wall-(side?a.x:a.z))/delta;
    vec3 p=mix(a,b,t);
    return t>=0.0 && t<=1.002 && abs(side?p.z:p.x)<.916 && p.y>=-.05 && p.y<2.39;
}
float distantFlash() {
    float t=mod(u_clock,19.0);
    return smoothstep(2.6,2.78,t)*(1.0-smoothstep(2.78,3.35,t));
}
uniform vec4 u_lamps[4]; // world position and radius, zero for unused slots
uniform vec3 u_lamp_colour[4];

// A periodic cloud deck has one coordinate system for the sky and its shadow, even after rebasing.
uniform sampler2D u_clouds;
float clouds(vec3 from,vec3 ray) {
    vec2 scale=vec2(1.22,2.32);
    vec2 uv=u_cloud_origin.xz+from.xz*scale/950.0
            +ray.xz/max(.055,ray.y)*max(.015,1.0-from.y/950.0)*scale+vec2(u_clock*.006,-.49);
    return SAMPLE(u_clouds,uv/16.0).r;
}
float cloudThreshold(float weather) {
    return weather==0.0?.59:(weather==3.0?.49:.38);
}
float readShadow(sampler2D depthMap,vec3 pos,float texel,float bias) {
    if(min(pos.x,pos.y)<texel || max(pos.x,pos.y)>1.0-texel || pos.z<=0.0 || pos.z>=1.0) return 1.0;
    float z=pos.z-bias;
    if(u_light_params.y<.05) return step(z,SAMPLE(depthMap,pos.xy).r);
    vec2 pixel=pos.xy/texel-.5;
    vec2 uv=(floor(pixel)+.5)*texel, f=fract(pixel);
    f=clamp((f-.5)/u_light_params.y+.5,0.0,1.0);
    return mix(mix(step(z,SAMPLE(depthMap,uv).r),step(z,SAMPLE(depthMap,uv+vec2(texel,0)).r),f.x),
               mix(step(z,SAMPLE(depthMap,uv+vec2(0,texel)).r),step(z,SAMPLE(depthMap,uv+texel).r),f.x),f.y);
}
float visibility(vec3 world,float incidence) {
    vec3 nearPos=(u_near_light*vec4(world,1.0)).xyz*.5+.5;
    float edge=max(abs(nearPos.x-.5),abs(nearPos.y-.5));
    float bias=(.025+.10*(1.0-incidence))/5200.0;
    float close=1.0;
    if(edge<.49) close=readShadow(u_near_depth,nearPos,u_light_params.z,bias);
    if(edge<=.43) return close;
    vec3 farPos=(u_far_light*vec4(world,1.0)).xyz*.5+.5;
    float farLight=readShadow(u_far_depth,farPos,u_light_params.w,bias+.00019);
    return mix(close,farLight,smoothstep(.43,.49,edge));
}
vec3 irradiance(vec3 normal,float direct,vec3 point) {
    float day=u_air.y, weather=u_air.w;
    float cloudiness=(weather==2.0?.87:(weather==3.0?.68:(weather==1.0?.38:0.0)))*u_light_params.x;
    float skyView=.64+.36*max(0.0,normal.y);
    vec3 ambient=u_sky_fill*skyView*mix(.115,1.0,day)
                +u_bounce*(.35+.65*(1.0-max(0.0,normal.y)))*mix(.10,1.0,day);
    ambient+=vec3(.18,.17,.15)*cloudiness*day;
    if(u_house==1) ambient*=.68;
    vec3 light=ambient;
    if(u_sun.y>0.0 && direct>0.0) {
        float visible=visibility(point,direct);
        if(visible>0.0 && u_light_params.x>0.0) {
            float t=cloudThreshold(weather);
            float cover=smoothstep(t,t+.115,clouds(point,u_sun));
            visible*=1.0-u_light_params.x*cover*.76;
        }
        vec3 sun=mix(vec3(1.04,1.02,.92),vec3(1.54,.83,.43),u_air.z);
        light+=sun*visible*direct*(1.0-cloudiness)*smoothstep(0.0,.15,u_sun.y);
    }
    if(u_house==1) {
        for(int i=0;i<4;i++) {
            vec3 delta=u_lamps[i].xyz-point;
            float dist=length(delta),radius=u_lamps[i].w;
            float falloff=max(0.0,1.0-dist/max(radius,.001));
            float facing=.26+.74*max(0.0,dot(normal,delta/max(dist,.001)));
            light+=u_lamp_colour[i]*falloff*falloff*facing*mix(2.6,.10,day);
        }
    }
    return light;
}
void main() {
    float day=u_air.y, warmth=u_air.z, weather=u_air.w;
    float grain=(hash(floor(gl_FragCoord.xy))-.5)/130.0;
    if(u_layer==3) { FRAGMENT=SAMPLE(u_atlas,v_uvm.xy); return; }
    if(u_layer==2) {
        vec4 ink=v_uvm.z>.5?SAMPLE(u_atlas,v_uvm.xy):vec4(1);
        if(ink.a<.45) discard;
        FRAGMENT=vec4(v_colour*ink.rgb,1); return;
    }
    if(u_layer==1) {
        vec3 ray=normalize(v_pos);
        float h=max(0.0,ray.y), sd=dot(ray,u_sun);
        vec3 sky=mix(u_horizon,u_zenith,pow(clamp(h*1.38,0.0,1.0),.56));
        vec3 night=vec3(.069,.099,.163);
        sky+=vec3(.32,.18,.074)*pow(max(sd,0.0),18.0)*day;
        sky=mix(sky,vec3(1.0,.92,.69),smoothstep(.99941,.99969,sd)*day);
        vec3 moon=normalize(vec3(-u_sun.x,max(.12,-u_sun.y),-u_sun.z));
        float md=dot(ray,moon);
        float face=noise(ray.xz*445.0)*.19+.77;
        if(u_profile!=7) sky=mix(sky,vec3(.91,.93,.83)*face,smoothstep(.99945,.99969,md)*(1.0-day));
        vec2 stars=vec2(atan(ray.z,ray.x),asin(clamp(ray.y,-1.0,1.0)))*170.0;
        float star=step(.985,hash(floor(stars)))*(1.0-smoothstep(.07,.32,length(fract(stars)-.5)));
        sky+=vec3(.79,.83,.84)*star*(u_profile==7?1.0:(1.0-day))*smoothstep(-.04,.2,h);
        float cloud=clouds(u_eye,ray), threshold=cloudThreshold(weather);
        float cover=smoothstep(threshold,threshold+.115,cloud)*smoothstep(-.03,.11,ray.y)*u_light_params.x;
        vec3 shadow=mix(vec3(.12,.15,.22),mix(vec3(.71,.76,.78),vec3(.85,.73,.68),warmth),day);
        vec3 highlight=mix(vec3(.30,.33,.39),vec3(1.0,.95,.849),day);
        if(weather==2.0) highlight=mix(highlight,shadow,.54);
        sky=mix(sky,mix(shadow,highlight,smoothstep(threshold+.005,threshold+.16,cloud)),cover*.93);
        if(u_profile==7) {
            sky=mix(u_horizon,u_zenith,h)+vec3(.72,.78,.89)*star+vec3(1.0,.96,.84)*smoothstep(.9995,.9997,sd);
        }
        if(u_profile>=7) {
            vec3 centre=normalize(vec3(-.36,.37,-1.0));
            vec3 ex=normalize(cross(centre,vec3(0,1,0))),ey=cross(ex,centre);
            vec2 orb=vec2(dot(ray,ex),dot(ray,ey))/(u_profile==7?.072:.115);
            float rr=dot(orb,orb);
            if(dot(ray,centre)>.8) {
                if(u_profile==10) {
                    vec2 tilt=vec2(orb.x+.36*orb.y,orb.y-.36*orb.x);
                    float ring=length(tilt*vec2(.62,2.40));
                    if(ring>1.05 && ring<1.65)sky=mix(sky,vec3(.9,.77,.61),.46+.20*cos(ring*52.0));
                }
                if(rr<1.0) {
                    float depth=sqrt(1.0-rr),land=noise(orb*3.5+2.3)+.33*noise(orb*9.1);
                    vec3 disk=u_profile==7?mix(vec3(.14,.38,.63),vec3(.35,.53,.37),smoothstep(.62,.77,land)):mix(u_liquid,u_soil,.48+.24*sin(orb.y*14.0+land));
                    if(u_profile==7)disk=mix(disk,vec3(.90,.92,.89),smoothstep(.58,.78,noise(orb*8.0+2.0*land)));
                    sky=disk*(.28+.75*max(0.0,.58*orb.x+.23*orb.y+.65*depth));
                }
            }
            if(u_profile==8 || u_profile==9 || u_profile==11) {
                float small=dot(ray,normalize(vec3(.30,.21,-1.0)));
                sky=mix(sky,mix(u_soil,vec3(1),.50),smoothstep(.9995,.99965,small));
            }
        }
        if(u_lookout!=0) {
            vec3 veil=u_event_air.rgb*(.25+.75*day);
            sky=mix(sky,veil,min(.87,u_event_air.w*17.0)*(.65+.35*(1.0-h)));
            if(u_lookout==2) sky+=vec3(.22,.25,.32)*distantFlash()*(.25+.75*cloud);
            if(u_lookout==3) sky+=vec3(.16,.11,.046)*(1.0-smoothstep(0.0,.14,h))*day;
        }
        FRAGMENT=vec4(sky+grain,1); return;
    }
    float dist=length(v_pos-u_eye);
    float group=floor((v_uvm.z+.10)/16.0);
    if(group>0.0 && !throughDoor(group)) discard;
    float mat=v_uvm.z-16.0*group;
    vec2 groundUV=mod(v_pos.xz+u_pattern.xz,4096.0);
    if(u_reflection==1 && (v_pos.y<u_water || (mat>1.5 && mat<2.5))) discard;
    vec3 geometric=cross(dFdx(v_pos),dFdy(v_pos));
    vec3 normal=geometric*inversesqrt(max(dot(geometric,geometric),1.e-16));
    if(dot(normal,u_eye-v_pos)<0.0) normal=-normal;
    vec3 col=v_colour;
    if(mat>7.5 && mat<8.5) {
        // Sparse glass glints keep the outdoor colour and depth visible at the game's pixel scale.
        if(mod(floor(gl_FragCoord.x)+3.0*floor(gl_FragCoord.y),11.0)>0.5) discard;
        col=mix(u_horizon,vec3(.66,.79,.79),.25)*(.42+.58*day);
    } else if(mat>8.5 && mat<9.5) {
        float ripple=noise(vec2(groundUV.x*.6+u_clock*.22,groundUV.y*2.8-u_clock*.14));
        col+=vec3(.14,.25,.22)*smoothstep(.52,.86,ripple);
    } else if(mat>.5 && mat<1.5) {
        col*=foliage(v_pos,vec3(v_uvm.xy,mat));
    } else if(mat>1.5 && mat<2.5) {
        float ribbon=noise(vec2(groundUV.x*.14+u_clock*.16,groundUV.y*1.65+u_clock*.085));
        float remote=smoothstep(45.0,360.0,dist);
        col=mix(u_liquid,u_horizon*.88,.12+remote*.33);
        if(u_profile==11){float cracks=noise(groundUV*.39+u_clock*.07);FRAGMENT=vec4(mix(vec3(.28,.085,.07),vec3(1.0,.55,.12),smoothstep(.31,.66,cracks)),1);return;}
        vec2 screen=(gl_FragCoord.xy-u_viewport.xy)/u_viewport.zw;
        float wave=noise(vec2(groundUV.x*.07,groundUV.y*.68+u_clock*.07))-.5;
        vec3 reflection=SAMPLE(u_lake,clamp(screen+vec2(wave*.009,(ribbon-.5)*.019),.002,.998)).rgb;
        col=mix(col*(.29+.71*day),reflection,.48+.22*remote);
        col+=vec3(.11,.14,.13)*smoothstep(.72,.92,ribbon)*(.30+.70*day);
        FRAGMENT=vec4(col+grain,1);return;
    } else if(mat>4.5 && mat<5.5) {
        col*=foliage(v_pos,vec3(v_uvm.xy,mat));
    } else if(mat>2.5 && mat<3.5) {
        float rings=noise(vec2((groundUV.x+groundUV.y)*3.1,v_pos.y*34.0));
        col*=.92+.15*rings;
    } else {
        float detail=noise(groundUV*.87+v_pos.y*.2);
        col*=.963+.08*detail;
        if(mat>3.5 && mat<4.5) {
            normal=vec3(v_uvm.x,sqrt(max(.001,1.0-dot(v_uvm.xy,v_uvm.xy))),v_uvm.y);
            float crag=smoothstep(49.0,155.0,v_pos.y);
            float seams=noise(vec2(groundUV.x*.065+groundUV.y*.036+v_pos.y*.023,v_pos.y*.027))+detail*.18;
            col*=1.0-crag*smoothstep(.48,.83,seams)*.22;
            vec2 uvTrail=(v_pos.xz-u_trail_center.xz)/512.0+.5;
            float distanceToTrail=6.0;
            if(all(greaterThan(uvTrail,vec2(.002))) && all(lessThan(uvTrail,vec2(.998))))
                distanceToTrail=6.0*SAMPLE(u_trail,uvTrail).r;
            float edge=distanceToTrail+(noise(groundUV*1.35)-.5)*.56;
            float tread=1.0-smoothstep(1.15,2.12,edge);
            vec3 grit=mix(vec3(.49,.37,.25),vec3(.76,.64,.45),detail);
            if(u_profile<=1) grit=mix(grit,vec3(.76,.82,.82),max(step(2.5,u_season)*.86,smoothstep(135.0,190.0,v_pos.y)*.96));
            else grit=mix(u_soil*.70,u_soil*1.06,detail);
            if(u_profile==3 || u_profile==2)col*=.95+.07*sin(groundUV.x*4.1+groundUV.y*1.6+4.0*detail);
            if(u_profile==7)col*=.81+.28*noise(groundUV*5.0);
            float gravel=noise(groundUV*7.0);
            grit*=.90+.13*gravel+.09*smoothstep(.71,.88,gravel);
            col=mix(col,grit,tread*.97);
        }
    }
    float direct=max(0.0,dot(normal,u_sun));
    if((mat>.5 && mat<1.5) || (mat>4.5 && mat<5.5)) {
        // Needles and leaves scatter light through both sides of their silhouettes.
        direct=.36+.64*abs(dot(normal,u_sun));
    }
    if(group>.5) direct*=.08; // The folded room is enclosed by its own roof.
    col*=pow(max(vec3(.001),irradiance(normal,direct,v_pos)),vec3(1.0/2.2));
    if(mat>6.5 && mat<7.5) col=mix(col,v_colour,.82*(1.0-day));
    float fograte=weather==3.0?.0065:(weather==2.0?.0037:(weather==1.0?.0019:.00078));
    if(mat>5.5 && mat<6.5)col=v_colour*(1.0+.11*sin(u_clock*1.8+v_pos.y));
    if(u_profile==7)fograte=0.0;
    vec3 fogcol=mix(u_horizon,mix(vec3(.43,.53,.63),vec3(.62,.69,.72),day),.48*day);
    if(u_house==1) fograte=.0005;
    else if(u_lookout!=0) {
        fograte=u_event_air.w;
        fogcol=u_event_air.rgb*(.25+.75*day);
        if(u_lookout==2) col+=vec3(.12,.14,.17)*distantFlash();
    }
    float fog=1.0-exp(-dist*fograte);
    col=mix(col,fogcol,fog*.91);
    FRAGMENT=vec4(col+grain,1);
}
