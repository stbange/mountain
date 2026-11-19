varying vec3 v_pos;
varying vec3 v_colour;
varying vec3 v_uvm;
varying float v_dist;
uniform sampler2D u_atlas;
uniform sampler2D u_lake;
uniform sampler2D u_trail;
uniform float u_season;
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
float hash(vec2 p) {
    vec3 a=fract(vec3(p.xyx)*vec3(.1031,.1030,.0973));
    a+=dot(a,a.yzx+33.33); return fract((a.x+a.y)*a.z);
}
float noise(vec2 p) {
    vec2 a=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
    return mix(mix(hash(a),hash(a+vec2(1,0)),f.x),mix(hash(a+vec2(0,1)),hash(a+1.0),f.x),f.y);
}
float clouds(vec2 p) {
    return noise(p)*.55+noise(p*2.04+7.2)*.27+noise(p*4.17-3.8)*.12+noise(p*8.2)*.06;
}
void main() {
    float day=u_air.y, warmth=u_air.z, weather=u_air.w;
    float grain=(hash(floor(gl_FragCoord.xy))-.5)/130.0;
    if(u_layer==3) { gl_FragColor=texture2D(u_atlas,v_uvm.xy); return; }
    if(u_layer==2) {
        vec4 ink=v_uvm.z>.5?texture2D(u_atlas,v_uvm.xy):vec4(1);
        if(ink.a<.45) discard;
        gl_FragColor=vec4(v_colour*ink.rgb,1); return;
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
        sky=mix(sky,vec3(.91,.93,.83)*face,smoothstep(.99945,.99969,md)*(1.0-day));
        vec2 stars=vec2(atan(ray.z,ray.x),asin(clamp(ray.y,-1.0,1.0)))*170.0;
        float star=step(.985,hash(floor(stars)))*(1.0-smoothstep(.07,.32,length(fract(stars)-.5)));
        sky+=vec3(.79,.83,.84)*star*(1.0-day)*smoothstep(-.04,.2,h);
        vec2 uv=ray.xz/(h+.115)*vec2(1.22,2.32)+vec2(u_clock*.002,-.49);
        float cloud=clouds(uv), threshold=weather==0.0?.56:(weather==3.0?.52:.36);
        float cover=smoothstep(threshold,threshold+.115,cloud)*smoothstep(-.03,.11,ray.y);
        vec3 shadow=mix(vec3(.12,.15,.22),mix(vec3(.71,.76,.78),vec3(.85,.73,.68),warmth),day);
        vec3 highlight=mix(vec3(.30,.33,.39),vec3(1.0,.95,.849),day);
        if(weather==2.0) highlight=mix(highlight,shadow,.54);
        sky=mix(sky,mix(shadow,highlight,smoothstep(threshold+.005,threshold+.16,cloud)),cover*.93);
        gl_FragColor=vec4(sky+grain,1); return;
    }
    float dist=length(v_pos-u_eye);
    float mat=v_uvm.z;
    if(u_reflection==1 && (v_pos.y<u_water || (mat>1.5 && mat<2.5))) discard;
    vec3 col=v_colour;
    if(mat>.5 && mat<1.5) {
        vec4 spr=texture2D(u_atlas,v_uvm.xy); if(spr.a<.48) discard;
        col*=spr.rgb;
    } else if(mat>1.5 && mat<2.5) {
        float ribbon=noise(vec2(v_pos.x*.14+u_clock*.16,v_pos.z*1.65+u_clock*.085));
        float remote=smoothstep(45.0,360.0,dist);
        col=mix(vec3(.16,.44,.48),u_horizon*.88,.15+remote*.40);
        vec2 screen=(gl_FragCoord.xy-u_viewport.xy)/u_viewport.zw;
        float wave=noise(vec2(v_pos.x*.07,v_pos.z*.68+u_clock*.07))-.5;
        vec3 reflection=texture2D(u_lake,clamp(screen+vec2(wave*.009,(ribbon-.5)*.019),.002,.998)).rgb;
        col=mix(col*(.29+.71*day),reflection,.48+.22*remote);
        col+=vec3(.11,.14,.13)*smoothstep(.72,.92,ribbon)*(.30+.70*day);
        gl_FragColor=vec4(col+grain,1);return;
    } else if(mat>2.5 && mat<3.5) {
        float rings=noise(vec2((v_pos.x+v_pos.z)*3.1,v_pos.y*34.0));
        col*=.92+.15*rings;
    } else {
        float detail=noise(v_pos.xz*.87+v_pos.y*.2);
        col*=.963+.08*detail;
        if(mat>3.5 && mat<4.5) {
            vec3 normal=vec3(v_uvm.x,sqrt(max(.001,1.0-dot(v_uvm.xy,v_uvm.xy))),v_uvm.y);
            float direct=max(0.0,dot(normal,u_sun));
            float sunlight=(.87+.24*direct);
            col*=mix(1.0,sunlight,weather==0.0?day:.25*day);
            float crag=smoothstep(49.0,155.0,v_pos.y);
            float seams=noise(vec2(v_pos.x*.065+v_pos.z*.036+v_pos.y*.023,v_pos.y*.027))+detail*.18;
            col*=1.0-crag*smoothstep(.48,.83,seams)*.22;
            col+=vec3(.16,.07,.028)*direct*warmth*crag*day;
            float distanceToTrail=6.0*texture2D(u_trail,v_pos.xz/512.0+.5).r;
            float edge=distanceToTrail+(noise(v_pos.xz*1.35)-.5)*.56;
            float tread=1.0-smoothstep(1.15,2.12,edge);
            vec3 grit=mix(vec3(.49,.37,.25),vec3(.76,.64,.45),detail);
            grit=mix(grit,vec3(.64,.70,.70),step(2.5,u_season)*.86);
            float gravel=noise(v_pos.xz*7.0);
            grit*=.90+.13*gravel+.09*smoothstep(.71,.88,gravel);
            col=mix(col,grit*sunlight,tread*.97);
        }
    }
    vec3 tint=mix(vec3(.23,.31,.48),vec3(1.0,.99,.94),day);
    tint+=vec3(.09,.005,-.065)*warmth*day;
    col*=tint;
    if(mat>4.5) col=mix(col,v_colour, .81);
    float fograte=weather==3.0?.0065:(weather==2.0?.0037:(weather==1.0?.0019:.00078));
    float fog=1.0-exp(-dist*fograte);
    vec3 fogcol=mix(u_horizon,mix(vec3(.43,.53,.63),vec3(.62,.69,.72),day),.48*day);
    col=mix(col,fogcol,fog*.91);
    gl_FragColor=vec4(col+grain,1);
}
