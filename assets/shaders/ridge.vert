IN vec3 a_pos;
IN vec3 a_colour;
IN vec3 a_uvm;
uniform mat4 u_vp;
uniform mat4 u_inverse;
uniform vec3 u_eye;
uniform vec3 u_pattern;
uniform float u_clock;
uniform int u_layer;
uniform int u_lookout;
uniform int u_house;
OUT vec3 v_pos;
OUT vec3 v_colour;
OUT vec3 v_uvm;
OUT float v_dist;
void main() {
    v_colour=a_colour; v_uvm=a_uvm; v_pos=a_pos;
    if(u_layer==0) {
        if(a_uvm.z>1.2 && a_uvm.z<1.3) {
            float tip=1.0-fract(a_uvm.y*4.0);
            vec2 wind=mod(a_pos.xz+u_pattern.xz,4096.0);
            v_pos.x+=tip*tip*(.055*sin(u_clock*1.7+wind.y*.35)+.03*sin(u_clock*2.1+wind.x*.42));
        }
        if(u_lookout==3 && u_house==0) {
            float dist=length(a_pos-u_eye);
            v_pos.y+=sin(a_pos.x*.29+u_clock*2.1+sin(a_pos.z*.17))*smoothstep(24.0,195.0,dist)*.51;
        }
        gl_Position=u_vp*vec4(v_pos,1.0);
        v_dist=length(a_pos-u_eye);
    } else {
        gl_Position=vec4(a_pos.xy,0.995,1.0);
        vec4 world=u_inverse*vec4(a_pos.xy,0.55,1.0);
        v_pos=world.xyz/world.w-u_eye; v_dist=0.0;
    }
}
