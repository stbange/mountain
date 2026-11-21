// Shared by the camera and sunlight passes so foliage casts its actual silhouette.
uniform sampler2D u_atlas;
uniform sampler2D u_canopy;
uniform vec3 u_pattern;
float hash(vec2 p) {
    vec3 a=fract(vec3(p.xyx)*vec3(.1031,.1030,.0973));
    a+=dot(a,a.yzx+33.33); return fract((a.x+a.y)*a.z);
}
float noise(vec2 p) {
    vec2 a=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
    return mix(mix(hash(a),hash(a+vec2(1,0)),f.x),mix(hash(a+vec2(0,1)),hash(a+1.0),f.x),f.y);
}
vec3 foliage(vec3 point, vec3 material) {
    if(material.z>.5 && material.z<1.5) {
        vec4 spr=material.z>1.4?SAMPLE(u_canopy,material.xy):SAMPLE(u_atlas,material.xy);
        if(spr.a<.48) discard;
        return spr.rgb;
    }
    if(material.z>4.5 && material.z<5.5) {
        vec2 edge=abs(fract(material.xy)-.5);
        vec2 uv=mod(point.xz+u_pattern.xz,4096.0)*.90+vec2(.37*point.y,point.y*1.32);
        float clump=noise(uv*8.0), needles=noise(uv*vec2(17.0,34.0));
        if(max(edge.x,edge.y)>.37 && clump+.34*needles<.39) discard;
        return vec3(.77+.32*clump+.22*needles);
    }
    return vec3(1);
}
