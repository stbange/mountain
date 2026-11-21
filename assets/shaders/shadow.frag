IN vec3 v_pos;
IN vec3 v_uvm;
void main() {
    if(v_uvm.z>=16.0 || (v_uvm.z>7.5 && v_uvm.z<8.5)) discard; // Portal views and clear glass.
    if(v_uvm.z>1.5 && v_uvm.z<2.5) discard; // The water surface transmits sunlight.
    FRAGMENT=vec4(foliage(v_pos,v_uvm),1.0);
}
