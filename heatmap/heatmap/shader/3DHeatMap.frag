uniform sampler2D heightMapID;
uniform sampler2D textMapID;
varying float heightlevel;
void main (void)
{
    gl_FragColor =texture2D(textMapID, vec2(heightlevel,0)).rgba;
    gl_FragColor.a=heightlevel*1.2;
//  gl_FragColor = vec4(1.0,0.0,0.0,1.0);

 }
 