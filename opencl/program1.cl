#define TILE_WIDTH 16
#define DATA_TYPE unsigned char

#define TS 3

///function to calculate minimum and maximum of 3 numbers
float max31(float r,float g,float b)
{
    float cmp;
    float m, n;
    cmp = b<=g;
    m = bitselect(b,g,cmp);
    cmp = m<=r;
    m = bitselect(m,r,cmp);
    return m;
}

float min31(float r,float g,float b)
{
    float cmp;
    float m,n;
    cmp = b<=g;
    m = bitselect(g,b,cmp);
    cmp = m<=r;
    m = bitselect(r,m,cmp);
    return m;
}

int min311(int r,int b,int g)
{
    int maxx;
    int max1;
    maxx = b<g ? b :g;
    max1 = maxx;
    maxx = maxx<r ? maxx :r;
    return maxx;
}

int max311(int r,int b,int g)
{
    int maxx;
    int max1;
    maxx = b>g ? b :g;
    max1 = maxx;
    maxx = maxx>r ? maxx :r;
    return maxx;
}

//BGR 2 GRAY conversion
__kernel void BGR2GRAY(__global DATA_TYPE * src, int rows, int cols, int src_step, int channel)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    if(x < cols && y < rows )
    {
        int idx =(y*src_step)+(x*channel);
        int b = ((DATA_TYPE)src[idx]);
        int g = ((DATA_TYPE)src[idx+1]);
        int r = ((DATA_TYPE)src[idx+2]);
        float val=(0.299f*r)+(0.587f*g)+(0.114f*b);

        src[idx] = (unsigned char)val;
        src[idx+1] = (unsigned char)val;
        src[idx+2] = (unsigned char)val;
    }
}

// BGR 2 HSV
__kernel void BGR2HSV(__global DATA_TYPE * src, int rows, int cols, int src_step, int channel)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    if(x < cols && y < rows )
    {
        float fR, fG, fB;
        float fH, fS, fV;
        const float FLOAT_TO_BYTE = 255.0f;
        const float BYTE_TO_FLOAT = 1.0f / FLOAT_TO_BYTE;

        int idx = (y*src_step)+(x*channel);
        //converting to float
        int bB = ((DATA_TYPE)src[idx]);
        int bG = ((DATA_TYPE)src[idx+1]);
        int bR = ((DATA_TYPE)src[idx+2]);

        // Convert from 8-bit integers to floats.
        fR = bR * BYTE_TO_FLOAT;
        fG = bG * BYTE_TO_FLOAT;
        fB = bB * BYTE_TO_FLOAT;

        // Convert from RGB to HSV, using float ranges 0.0 to 1.0.
        float fDelta;
        float fMin, fMax;
        int iMax;
        // Get the min and max, but use integer comparisons for slight speedup.
        if (bB < bG)
        {
            if (bB < bR)
            {
                fMin = fB;
                if (bR > bG)
                {
                    iMax = bR;
                    fMax = fR;
                }
                else
                {
                    iMax = bG;
                    fMax = fG;
                }
            }
            else {
                fMin = fR;
                fMax = fG;
                iMax = bG;
            }
        }
        else {
            if (bG < bR)
            {
                fMin = fG;
                if (bB > bR)
                {
                    fMax = fB;
                    iMax = bB;
                }
                else
                {
                    fMax = fR;
                    iMax = bR;
                }
            }
            else
            {
                fMin = fR;
                fMax = fB;
                iMax = bB;
            }
        }

        fDelta = fMax - fMin;
        fV = fMax;				// Value (Brightness).
        if (iMax != 0)
        {			// Make sure its not pure black.
            fS = fDelta / fMax;		// Saturation.
            float ANGLE_TO_UNIT = 1.0f / (6.0f * fDelta);	// Make the Hues between 0.0 to 1.0 instead of 6.0
            if (iMax == bR)
            {		// between yellow and magenta.
                fH = (fG - fB) * ANGLE_TO_UNIT;
            }
            else if (iMax == bG)
            {		// between cyan and yellow.
                fH = (2.0f/6.0f) + ( fB - fR ) * ANGLE_TO_UNIT;
            }
            else
            {				// between magenta and cyan.
                fH = (4.0f/6.0f) + ( fR - fG ) * ANGLE_TO_UNIT;
            }
            // Wrap outlier Hues around the circle.
            if (fH < 0.0f)
                fH += 1.0f;
            if (fH >= 1.0f)
                fH -= 1.0f;
        }
        else
        {
            // color is pure Black.
            fS = 0;
            fH = 0;	// undefined hue
        }

        // Convert from floats to 8-bit integers.
        int bH = (int)(0.5f + fH * 255.0f);
        int bS = (int)(0.5f + fS * 255.0f);
        int bV = (int)(0.5f + fV * 255.0f);

        // Clip the values to make sure it fits within the 8bits.
        if (bH > 255) bH = 255;
        if (bH < 0) bH = 0;
        if (bS > 255) bS = 255;
        if (bS < 0) bS = 0;
        if (bV > 255) bV = 255;
        if (bV < 0) bV = 0;

        src[idx]=(unsigned char)bH;
        src[idx+1]=(unsigned char)bS;
        src[idx+2]=(unsigned char)bV;
    }
}


__kernel void BGR2HSV1(__global DATA_TYPE * src,int rows,int cols,int src_step,int channel)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    if(x < cols && y < rows )
    {
        float fR, fG, fB;
        float fH, fS, fV;
        const float FLOAT_TO_BYTE = 255.0f;
        const float BYTE_TO_FLOAT = 1.0f / FLOAT_TO_BYTE;

        //index of pixel
        int idx=(y*src_step)+(x*channel);
        //int idx1=(y*dst_step)+(x*channel);

        //converting to float
        int bB=((DATA_TYPE)src[idx]);
        int bG=((DATA_TYPE)src[idx+1]);
        int bR=((DATA_TYPE)src[idx+2]);

        // Convert from 8-bit integers to floats.
        fR = bR * BYTE_TO_FLOAT;
        fG = bG * BYTE_TO_FLOAT;
        fB = bB * BYTE_TO_FLOAT;

        // Convert from RGB to HSV, using float ranges 0.0 to 1.0.
        float fDelta;
        float fMin, fMax;
        int iMax=0,iMin=0;
        // Get the min and max, but use integer comparisons for slight speedup.

        iMax=max31(bB,bG,bR);
        iMin=min31(bB,bG,bR);
        fMax=(iMax / FLOAT_TO_BYTE);
        fMin=(iMin / FLOAT_TO_BYTE);
        fDelta = fMax - fMin;
        fV = fMax;
        int flag_image=(iMax!=0);
        int fr,fg,fb,flag_image2;
        fS=(flag_image*(fDelta/fMax));
        float ANGLE_TO_UNIT = 1.0f / (6.0f * fDelta);
        fH=0.0f;
        fr=(iMax==bR);
        fg=(iMax==bG);
        fb=(iMax==bB);

        flag_image2=(fr)*flag_image;
        fH = flag_image2*(((fG - fB) * ANGLE_TO_UNIT ));

        flag_image2=(fg * (fr==false) * flag_image);
        fH = flag_image2*((((2.0f*fDelta) + ( fB - fR ))*ANGLE_TO_UNIT))+fH;

        flag_image2=(fb * (fr==false) * (fg==false)* flag_image);
        fH = flag_image2*((((4.0f*fDelta) + ( fR - fG ))*ANGLE_TO_UNIT))+fH;

        flag_image = (fH<0.0f);
        fH=fH+(flag_image*1.0f);
        flag_image = (fH>=1.0f);
        fH=fH+(flag_image*-1.0f);


        // Convert from floats to 8-bit integers.
        int bH = (0.5f + fH * 255.0f);
        int bS = (0.5f + fS * 255.0f);
        int bV = (0.5f + fV * 255.0f);

        fr= bH>255;bH=(fr==true)*255+((fr==false)*bH);
        fr= bS>255;bS=(fr==true)*255+((fr==false)*bS);
        fr= bV>255;bV=(fr==true)*255+((fr==false)*bV);
        fr= bH<0;bH=(fr==true)*0+((fr==false)*bH);
        fr= bS<0;bS=(fr==true)*0+((fr==false)*bS);
        fr= bV<0;bV=(fr==true)*0+((fr==false)*bV);

        src[idx]=(unsigned char)bH;
        src[idx+1]=(unsigned char)bS;
        src[idx+2]=(unsigned char)bV;
    }
}


//    kernel function for HSV to BGR conversion
__kernel void HSV2BGR(__global DATA_TYPE * src,int rows,int cols,int src_step,int channel)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    if(x < cols && y < rows)
    {
        float fH, fS, fV;
        float fR, fG, fB;
        const float FLOAT_TO_BYTE = 255.0f;
        const float BYTE_TO_FLOAT = 1.0f / FLOAT_TO_BYTE;
        int idx=(y*src_step)+(x*channel);
        int bH=((DATA_TYPE)src[idx]);
        int bS=((DATA_TYPE)src[idx+1]);
        int bV=((DATA_TYPE)src[idx+2]);

        fH = (float)bH * BYTE_TO_FLOAT;
        fS = (float)bS * BYTE_TO_FLOAT;
        fV = (float)bV * BYTE_TO_FLOAT;

        int iI;
        float fI, fF, p, q, t;

        if( bS == 0 )
        {
            // achromatic (grey)
            fR = fG = fB = fV;
        }
        else
        {
            // If Hue == 1.0, then wrap it around the circle to 0.0
            if (fH >= 1.0f)
                fH = 0.0f;

            fH *= 6.0;			// sector 0 to 5
            fI = floor( fH );		// integer part of h (0,1,2,3,4,5 or 6)
            iI = (int) fH;			//		"		"		"		"
            fF = fH - fI;			// factorial part of h (0 to 1)

            p = fV * ( 1.0f - fS );
            q = fV * ( 1.0f - fS * fF );
            t = fV * ( 1.0f - fS * ( 1.0f - fF ) );

            switch( iI )
            {
                case 0:
                    fR = fV;
                    fG = t;
                    fB = p;
                    break;
                case 1:
                    fR = q;
                    fG = fV;
                    fB = p;
                    break;
                case 2:
                    fR = p;
                    fG = fV;
                    fB = t;
                    break;
                case 3:
                    fR = p;
                    fG = q;
                    fB = fV;
                    break;
                case 4:
                    fR = t;
                    fG = p;
                    fB = fV;
                    break;
                default:		// case 5 (or 6):
                    fR = fV;
                    fG = p;
                    fB = q;
                    break;
            }
        }

        // Convert from floats to 8-bit integers
        int bR = (int)(fR * FLOAT_TO_BYTE);
        int bG = (int)(fG * FLOAT_TO_BYTE);
        int bB = (int)(fB * FLOAT_TO_BYTE);

        // Clip the values to make sure it fits within the 8bits.
        if (bR > 255)
            bR = 255;
        if (bR < 0)
            bR = 0;
        if (bG > 255)
            bG = 255;
        if (bG < 0)
            bG = 0;
        if (bB > 255)
            bB = 255;
        if (bB < 0)
            bB = 0;

        src[idx]=(unsigned char)bB;
        src[idx+1]=(unsigned char)bG;
        src[idx+2]=(unsigned char)bR;
    }
}

__kernel void HSV2BGR1(__global DATA_TYPE * src,int rows,int cols,int src_step,int channel)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    if(x < cols && y < rows)
    {
        float fH, fS, fV;
        float fR, fG, fB;
        const float FLOAT_TO_BYTE = 255.0f;
        const float BYTE_TO_FLOAT = 1.0f / FLOAT_TO_BYTE;

        int idx = (y*src_step)+(x*channel);
        int bH = ((DATA_TYPE)src[idx]);
        int bS = ((DATA_TYPE)src[idx+1]);
        int bV = ((DATA_TYPE)src[idx+2]);

        fH = (float)bH * BYTE_TO_FLOAT;
        fS = (float)bS * BYTE_TO_FLOAT;
        fV = (float)bV * BYTE_TO_FLOAT;

        int iI;
        float fI, fF, p, q, t;
        // If Hue == 1.0, then wrap it around the circle to 0.0
        int flag1=(fH >= 1.0f);
        fH=(flag1==true)*0+(flag1==false)*(fH*6.0f);
        fI = floor( fH );		// integer part of h (0,1,2,3,4,5 or 6)
        iI=(int)fH;
        fF = fH - fI;			// factorial part of h (0 to 1)
        p = fV * ( 1.0f - fS );
        q = fV * ( 1.0f - fS * fF );
        t = fV * ( 1.0f - fS * ( 1.0f - fF ) );

        //iI=bS==0 ? -1:(int)fH;
        //fR=bS==0 ? fV :fV ;
        //fG=bS==0 ? fV :p ;
        //fB=bS==0 ? fV :q ;

        int f1=(iI==0)&&(bS!=0);
        int f2=(iI==1)&&(bS!=0);
        int f3=(iI==2)&&(bS!=0);
        int f4=(iI==3)&&(bS!=0);
        int f5=(iI==4)&&(bS!=0);

        fR=0;
        fR=(f1==true)*fV+fR;
        fR=(f2==true)*q+fR;
        fR=(f3==true)*p+fR;
        fR=(f4==true)*p+fR;
        fR=(f5==true)*t+fR;
        fR=(fR==0)*fV+fR;

        fG=0;
        fG=(f1==true)*t+fG;
        fG=(f2==true)*fV+fG;
        fG=(f3==true)*fV+fG;
        fG=(f4==true)*q+fG;
        fG=(f5==true)*p+fG;
        fR=(fG==0)*p+fR;

        fB=0;
        fB=(f1==true)*p+fB;
        fB=(f2==true)*p+fB;
        fB=(f3==true)*t+fB;
        fB=(f4==true)*fV+fB;
        fB=(f5==true)*fV+fB;
        fB=(fB==0)*q+fB;

        // Convert from floats to 8-bit integers
        int bR = (int)(fR * FLOAT_TO_BYTE);
        int bG = (int)(fG * FLOAT_TO_BYTE);
        int bB = (int)(fB * FLOAT_TO_BYTE);

        // Clip the values to make sure it fits within the 8bits.
        bR= bR>255 ? 255:bR;
        bG= bG>255 ? 255:bG;
        bB= bB>255 ? 255:bB;
        bR= bR<0 ? 0:bR;
        bG= bG<0 ? 0:bG;
        bB= bB<0 ? 0:bB;

        src[idx]=(unsigned char)bB;
        src[idx+1]=(unsigned char)bG;
        src[idx+2]=(unsigned char)bR;
    }
}
