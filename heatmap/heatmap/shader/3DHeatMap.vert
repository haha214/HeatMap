#extension GL_EXT_gpu_shader4 : enable
uniform float parSpacing;
uniform int parSimpleNum;
uniform sampler2D heightMapID;
varying float heightlevel;
uniform float parMinlon;
uniform float parMaxlon;
uniform float parMinlat;
uniform float parMaxlat;
float PI=3.1415926f;
float _radiusEquator=6378137f;
float _radiusPolar = 6356752.3142;
double flattening = (_radiusEquator-_radiusPolar)/_radiusEquator;
double _eccentricitySquared = 2*flattening - flattening*flattening;
	vec3 convertLatLongHeightToXYZ(vec3 llh)
	{
       double sin_latitude = sin(llh.y/180.0*PI);
       double cos_latitude = cos(llh.y/180.0*PI);
       double N = _radiusEquator / sqrt( 1.0 - _eccentricitySquared*sin_latitude*sin_latitude);
		vec3 XYZ;
		XYZ.x = (N+llh.z)*cos_latitude*cos(llh.x/180.0*PI);
		XYZ.y = (N+llh.z)*cos_latitude*sin(llh.x/180.0*PI);
		XYZ.z = (N*(1-_eccentricitySquared)+llh.z)*sin_latitude;
		return XYZ;
	}
void main(void)
{
    vec4 pos=gl_Vertex;
    pos.xy=pos.xy*parSpacing;
    vec2 textcoord;
    textcoord.xy=pos.xy/parSimpleNum;
    heightlevel = texture2D(heightMapID,textcoord).r;
   // heightlevel = heightlevel>1.0?1:heightlevel;
	pos.z=heightlevel*160000.0;
	pos.w=1;
	pos.x=(pos.x/parSimpleNum)*(parMaxlon - parMinlon)+parMinlon;
    pos.y=(pos.y/parSimpleNum)*(parMaxlat - parMinlat)+parMinlat;
    pos.xyz = convertLatLongHeightToXYZ(pos.xyz);

	
		
	gl_Position = gl_ModelViewProjectionMatrix * pos;
}



           


