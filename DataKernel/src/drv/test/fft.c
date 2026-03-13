/* fft.c - FFT realizaition and interface */

/* Copyright (c) 2005 SNAC(Guodian Nanjing Automation Co., Ltd.), All Rights Reserved. */

/*
modification history
--------------------------------------------
01a, 21oct07, dy first created.
*/

/*
DESCRIPTION
This module includes  FFT realizaition and interface.
*/

/* includes */

#include "stdio_compat.h"		/* Standard input/output functions. */
#include "stdlib_compat.h"			/* Dynamic memory allocation. */
#include "time_compat.h"	/* time */
#include "math_compat.h"
#include "datetime.h"
#include "dsp_asst.h"

/* defines */

#define PROCESSDATANUM 128
#define abs_error 1.e-10

#define FFT_N 128
#define FFT_M 7

/* typedefs */

typedef struct
{
    float real,imag;
} complex;

/* globals */

complex fData[PROCESSDATANUM];
float fRealData[PROCESSDATANUM];
float RealCos[64];
float RealSin[64];

float w[128];    /* FFT test */
float cos_tab[128];
float sin_tab[128];
float dataR[128];
float dataI[128];
float dataRTemp[64];
float dataITemp[64];

float dataRTemp1[64];
float dataITemp1[64];

float dataRTemp2[64];
float dataITemp2[64];

float dataROut[128];
float dataIOut[128];

float dataAltOut[128];
float dataAngOut[128];

/* real data FFT */

float fft_r[FFT_N];  /* FFT输入实数序列，保存变换后的频域实部 */
float fft_i[FFT_N]; 	/* 保存变换后的频域虚部 */

/* 供完成倒位序排列使用的位定义 */
const uint8_t FFT_BIT[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
float Sinf[FFT_N] = {0};

void mrelfft(
    float xr[],
    float xi[],
    int n,
    int isign
);

void fft(void);

/* functions */

/*-------------------------------------------------------------------*/
float mabs(
    complex a
)
{
    float m;

    m=a.real*a.real+a.imag*a.imag;
    m=sqrt(m);

    return(m);
}

/*-------------------------------------------------------------------*/
float msign(
    float a,
    float b
)
{
    float z;

    if(b >= 0)
        z=sqrt(pow(a,2));
    else
        z=-sqrt(pow(a,2));

    return(z);
}

/*-------------------------------------------------------------------*/
complex cexp(
    complex a
)
{
    complex z;

    z.real=exp(a.real)*cos(a.imag);
    z.imag=exp(a.real)*sin(a.imag);

    return(z);
}

/***********************************************************************
* RandDataCreat - Create rand data, seed rand() with the system time and display the first 10 numbers.
*
* RETURNS: None
*
*/
void RandDataCreate(void)
{
    int32_t i;
    uint32_t stime;
    time_t ltime;

    /* Get the current calendar time. */
    ltime=time(NULL);
    stime=(uint32_t)ltime/2;
    srand(stime);

    for(i=0; i<PROCESSDATANUM; i++)
    {
        fData[i].real=(float)rand()/RAND_MAX;
        fData[i].imag=(float)rand()/RAND_MAX;
        fRealData[i]=(float)rand()/RAND_MAX;
    }
}

/*---------------------------------------------------------------------
 Routine mcmpfft:to obtain the DFT of Complex Data x(n)
                               By Cooley-Tukey radix-2 DIT Algorithm .
 input parameters:
 x : complex array.input signal is stored in x(0) to x(n-1).
 n : the dimension of x and y.
 isign:if ISIGN=-1 For Forward Transform
       if ISIGN=+1 For Inverse Transform.
 output parameters:
 x : complex array. DFT result is stored in x(0) to x(n-1).
 Notes:
     n must be power of 2.
                                       in Chapter 5
--------------------------------------------------------------------*/
void mcmpfft(
    complex x[],
    int n,
    int isign
)
{
    complex t,z,ce;
    float pisign;
    int mr,m,l,j,i,nn;

    for(i=1; i<=16; i++)
    {
        nn=pow(2,i);
        if(n==nn)
            break;
    }

    if(i>16)
    {
        printf(" N is not a power of 2 ! \n");
        return;
    }

    z.real=0.0;
    pisign=4*isign*atan(1.);
    mr=0;
    for(m=1; m<n; m++)
    {
        l=n;
        while(mr+l >= n)
            l=l/2;
        mr=mr%l+l;
        if(mr<=m)
            continue;
        t.real=x[m].real;
        t.imag=x[m].imag;
        x[m].real=x[mr].real;
        x[m].imag=x[mr].imag;
        x[mr].real=t.real;
        x[mr].imag=t.imag;
    }
    /*-------------------------------------------------------------------*/
    l=1;
    while(1)
    {
        if(l>=n)
        {
            if(isign==-1)
                return;
            for(j=0; j<n; j++)
            {
                x[j].real=x[j].real/n;
                x[j].imag=x[j].imag/n;
            }
            return;
        }
        for(m=0; m<l; m++)
        {
            for(i=m; i<n; i=i+2*l)
            {
                z.imag=m*pisign/l;
                ce=cexp(z);
                t.real=x[i+l].real*ce.real-x[i+l].imag*ce.imag;
                t.imag=x[i+l].real*ce.imag+x[i+l].imag*ce.real;
                x[i+l].real=x[i].real-t.real;
                x[i+l].imag=x[i].imag-t.imag;
                x[i].real=x[i].real+t.real;
                x[i].imag=x[i].imag+t.imag;
            }
        }
        l=2*l;
    }
}

/***********************************************************************
* FFTTest - FFT test.
*
* RETURNS: None
*
*/
void FFTTest(void)
{
    int i;

    RandDataCreate();

    for(i=0; i<PROCESSDATANUM; i++)
    {
        /* Check if the data is OK. */
        printf("%d	%f\n", i, fRealData[i]);
    }

    fft();
    for(i=0; i<PROCESSDATANUM; i++)
    {
        printf("%d	%f\n", i, fRealData[i]);
    }
}

void fft(void)
{
    uint16_t i,j,k,L;
    uint16_t b,c,p,n;
    uint8_t x,xc;
    float Gr,Gi,Wr,Wi;

    /* 倒位序排列输入序列 */
    for(i=0; i<FFT_N; i++)
    {
        x =i;
        xc=0;
        for(j=0; j<FFT_M; j++)
        {
            if((x&0x01) != 0)
            {
                xc|=FFT_BIT[(FFT_M-1)-j];
            }
            x>>=1;
        }
        fft_i[xc]=fft_r[i];
    }
    for(i=0; i<FFT_N; i++)
    {
        fft_r[i]=fft_i[i];
        fft_i[i]=0;
    }

    /* FFT变换 */
    for(L=1; L<=FFT_M; L++)
    {
        b=1;
        p=FFT_N>>1;
        for(i=0; i<L-1; i++)
        {
            b<<=1; /* b=2^(L-1) */
            p>>=1; /* p=N/(2^L) */
        }
        c=b<<1; /* c=b*2 */
        for(j=0; j<b; j++)
        {
            n=p*j;
            for(k=j; k<FFT_N; k+=c)
            {
                Gr = fft_r[k];
                Gi = fft_i[k];
                Wr = Sinf[n+FFT_N/4]*fft_r[k+b] + Sinf[n]*fft_i[k+b];
                Wi = Sinf[n+FFT_N/4]*fft_i[k+b] - Sinf[n]*fft_r[k+b];
                fft_r[k] = Gr + Wr;
                fft_i[k] = Gi + Wi;
                fft_r[k+b] = Gr - Wr;
                fft_i[k+b] = Gi - Wi;
            }
        }
    }
}

/***********************************************************************
* FFT_Init - To perform  split-radix DIF fft algorithm.
*
* RETURNS: None
*
*/
void FFT_Init(void)
{
    int i;
    uint32_t ulTimeFst;
    uint32_t ulTimeSec;

    TM_Initialize();
    for(i=0; i<128; i++)
    {
        cos_tab[i]=cos(2*3.1415926*i/128);
        sin_tab[i]=sin(2*3.1415926*i/128);
        /* dataR[i]=(float)rand()/RAND_MAX; */
        dataR[i]=sin(2*3.1415926*i/128)+0.5*sin(2*3.1415926*i/64)+0.25*sin(2*3.1415926*i/32)+0.125*sin(2*3.1415926*i/16);
        dataI[i]=0.0;
    }

    ulTimeFst=TM_Get_usCnt();

    for(i=0; i<64; i++)
    {
        dataRTemp[i]=dataR[2*i];
        dataITemp[i]=dataR[2*i+1];
    }

    /* FFT(dataR, dataI); */
    mrelfft(dataRTemp, dataITemp, 64, -1);

    for(i=0; i<64; i++)
    {
        dataRTemp1[i]=0.5*(dataRTemp[i]+dataRTemp[64-i]);
        dataITemp1[i]=0.5*(dataITemp[i]-dataITemp[64-i]);

        dataRTemp2[i]=-0.5*(-dataITemp[i]-dataITemp[64-i]);
        dataITemp2[i]=-0.5*(dataRTemp[i]-dataRTemp[64-i]);

        dataROut[i]=dataRTemp1[i]+cos(3.1415926/64*i)*dataRTemp2[i]+sin(3.1415926/64*i)*dataITemp2[i];
        dataIOut[i]=dataITemp1[i]+cos(3.1415926/64*i)*dataITemp2[i]-sin(3.1415926/64*i)*dataRTemp2[i];
    }

    ulTimeSec=TM_Get_usCnt();

    printf("128 points FFT time: %d us\n", (int)(ulTimeSec-ulTimeFst));

    for(i=0; i<64; i++)
    {
        printf("%d	%f	%f\n", i, dataROut[i], dataIOut[i]);
    }
}

/* 采样来的数据放在dataR[ ]数组中，运算前dataI[ ]数组初始化为0 */
void FFT(
    float dataR[],
    float dataI[]
)
{
    int i;
    int xx;
    int x0, x1, x2, x3, x4, x5, x6;
    int L, j, k, b, p;
    float TR, TI, temp;

    /********** following code invert sequence ************/
    for(i=0; i<128; i++)
    {
        x0=x1=x2=x3=x4=x5=x6=0;
        x0=i&0x01;
        x1=(i/2)&0x01;
        x2=(i/4)&0x01;
        x3=(i/8)&0x01;
        x4=(i/16)&0x01;
        x5=(i/32)&0x01;
        x6=(i/64)&0x01;
        xx=x0*64+x1*32+x2*16+x3*8+x4*4+x5*2+x6;
        dataI[xx]=dataR[i];
    }

    for(i=0; i<128; i++)
    {
        dataR[i]=dataI[i];
        dataI[i]=0;
    }
    /************** following code FFT *******************/
    for(L=1; L<=7; L++)
    {
        /* for(1) */
        b=1;
        i=L-1;
        while(i>0)
        {
            b=b*2;
            i--;
        } 	/* b= 2^(L-1) */
        for(j=0; j<=b-1; j++) 	/* for (2) */
        {
            p=1;
            i=7-L;
            while(i>0) 	/* p=pow(2,7-L)*j; */
            {
                p=p*2;
                i--;
            }
            p=p*j;
            for(k=j; k<128; k=k+2*b) 		/* for (3) */
            {
                TR=dataR[k];
                TI=dataI[k];
                temp=dataR[k+b];
                dataR[k]=dataR[k]+dataR[k+b]*cos_tab[p]+dataI[k+b]*sin_tab[p];
                dataI[k]=dataI[k]-dataR[k+b]*sin_tab[p]+dataI[k+b]*cos_tab[p];
                dataR[k+b]=TR-dataR[k+b]*cos_tab[p]-dataI[k+b]*sin_tab[p];
                dataI[k+b]=TI+temp*sin_tab[p]-dataI[k+b]*cos_tab[p];
            } /* end for (3) */
        } 		/* end for (2) */
    } /* end for (1) */

    for(i=0; i<128; i++)
    {
        dataR[i]=dataR[i]/64;
        dataI[i]=dataI[i]/64;
        w[i]=sqrt(dataR[i]*dataR[i]+dataI[i]*dataI[i]);
    }

    w[0]=w[0]/2;
} /* end FFT */

/* real data FFT */

/***********************************************************************
* mrelfft - To perform  split-radix DIF fft algorithm.
*
* RETURNS: None
*
*/
void mrelfft(
    float xr[],		/* real part of complex data for DFT/IDFT,n=0,...,N-1. and real part of complex result of DFT/IDFT,n=0,...,N-1. */
    float xi[],					/* image part of complex data for DFT/IDFT,n=0,...,N-1. and image part of complex result of DFT/IDFT,n=0,...,N-1. */
    int n,					/* Data point number of DFT compute. must be a power of 2. */
    int isign							/* Transform direction disignator, isign=-1: For Forward Transform; isign=+1: For Inverse Transform. */
)
{
    float e, es, cc1, ss1, cc3, ss3, r1, s1, r2, s2, s3, xtr, xti, a, a3;
    int m, n2, n4, j, k, is, id, i0, i1, i2, i3, n1, i, nn;

    for(m=1; m<=16; m++)
    {
        nn=pow(2,m);
        if(n == nn)
            break;
    }

    if(m>16)
    {
        printf(" N is not a power of 2 ! \n");
        return;
    }

    n2=n*2;
    es=-isign*atan(1.0)*8.0;
    for(k=1; k<m; k++)
    {
        n2=n2/2;
        n4=n2/4;
        e=es/n2;
        a=0.0;
        for(j=0; j<n4; j++)
        {
            a3=3*a;
            cc1=cos(a);
            ss1=sin(a);
            cc3=cos(a3);
            ss3=sin(a3);
            a=(j+1)*e;
            is=j;
            id=2*n2;
            do
            {
                for(i0=is; i0<n; i0+=id)
                {
                    i1=i0+n4;
                    i2=i1+n4;
                    i3=i2+n4;
                    r1=xr[i0]-xr[i2];
                    s1=xi[i0]-xi[i2];
                    r2=xr[i1]-xr[i3];
                    s2=xi[i1]-xi[i3];
                    xr[i0]+=xr[i2];
                    xi[i0]+=xi[i2];
                    xr[i1]+=xr[i3];
                    xi[i1]+=xi[i3];
                    if(isign != 1)
                    {
                        s3=r1-s2;
                        r1=r1+s2;
                        s2=r2-s1;
                        r2=r2+s1;
                    }
                    else
                    {
                        s3=r1+s2;
                        r1=r1-s2;
                        s2=-r2-s1;
                        r2=-r2+s1;
                    }
                    xr[i2]=r1*cc1-s2*ss1;
                    xi[i2]=-s2*cc1-r1*ss1;
                    xr[i3]=s3*cc3+r2*ss3;
                    xi[i3]=r2*cc3-s3*ss3;
                }
                is=2*id-n2+j;
                id=4*id;
            }
            while(is<n-1);
        }
    }

    /* special last stage */
    is=0;
    id=4;
    do
    {
        for(i0=is; i0<n; i0+=id)
        {
            i1=i0+1;
            xtr=xr[i0];
            xti=xi[i0];
            xr[i0]=xtr+xr[i1];
            xi[i0]=xti+xi[i1];
            xr[i1]=xtr-xr[i1];
            xi[i1]=xti-xi[i1];
        }
        is=2*id-2;
        id=4*id;
    }
    while(is<n-1);
    j=1;
    n1=n-1;
    for(i=1; i<=n1; i++)
    {
        if(i<j)
        {
            xtr=xr[j-1];
            xti=xi[j-1];
            xr[j-1]=xr[i-1];
            xi[j-1]=xi[i-1];
            xr[i-1]=xtr;
            xi[i-1]=xti;
        }
        k=n/2;
        while(1)
        {
            if(k >= j)
                break;
            j=j-k;
            k=k/2;
        }
        j=j+k;
    }
    if(isign == -1)
        return;
    for(i=0; i<n; i++)
    {
        xr[i]/=n;
        xi[i]/=n;
    }

    return;
}

/***********************************************************************
* RealDataFFTTest - 实数序列FFT
*
* RETURNS: None
*
*/
void RealDataFFTTest(void)
{
    int i;
    uint32_t ulTimeFst;
    uint32_t ulTimeSec;

    TM_Initialize();
    for(i=0; i<128; i++)
    {
        cos_tab[i]=cos(2*3.1415926*i/128);
        sin_tab[i]=sin(2*3.1415926*i/128);
        /* dataR[i]=(float)rand()/RAND_MAX; */
        dataR[i]=sin(2*3.1415926*i/128)+0.5*sin(2*3.1415926*i/64)+0.25*sin(2*3.1415926*i/32)+0.125*sin(2*3.1415926*i/16);
        dataI[i]=0.0;
    }

    for(i=0; i<64; i++)
    {
        RealCos[i]=cos(3.1415926/64*i);
        RealSin[i]=sin(3.1415926/64*i);
    }

    ulTimeFst=TM_Get_usCnt();

    for(i=0; i<64; i++)
    {
        dataRTemp[i]=dataR[2*i];
        dataITemp[i]=dataR[2*i+1];
    }

    /* FFT(dataR, dataI); */
    mrelfft(dataRTemp, dataITemp, 64, -1);

    for(i=0; i<64; i++)
    {
        dataRTemp1[i]=0.5*(dataRTemp[i]+dataRTemp[64-i]);
        dataITemp1[i]=0.5*(dataITemp[i]-dataITemp[64-i]);

        dataRTemp2[i]=-0.5*(-dataITemp[i]-dataITemp[64-i]);
        dataITemp2[i]=-0.5*(dataRTemp[i]-dataRTemp[64-i]);

        dataROut[i]=dataRTemp1[i]+RealCos[i]*dataRTemp2[i]+RealSin[i]*dataITemp2[i];
        dataIOut[i]=dataITemp1[i]+RealCos[i]*dataITemp2[i]-RealSin[i]*dataRTemp2[i];
    }

    for(i=0; i<64; i++)
    {
        dataROut[i]/=64;
        dataIOut[i]/=64;
        RealImageTrAltAngle(dataROut[i], dataIOut[i], dataAltOut+i, dataAngOut+i);
    }
    ulTimeSec=TM_Get_usCnt();

    printf("128 points FFT time: %d us\n", (int)(ulTimeSec-ulTimeFst));

    for(i=0; i<64; i++)
    {
        printf("%d	%f	%f 	%f	%f\n", i, dataROut[i], dataIOut[i], dataAltOut[i], dataAngOut[i]);
    }
}

void mcmpdft(
    complex x[],
    complex y[],
    int n,
    int isign
)
{
    /*----------------------------------------------------------------------
      Routinue mcmpdft: Directly to Compute the DFT/IDFT of Complex Data
                       x(n) By DFT definition;  in chapter 3.
      If ISIGN=-1: For Forward Transform;
         ISIGN=1 : For Inverse Transform.
                                          in chapter 3
    ----------------------------------------------------------------------*/
    complex t,ts,z;
    float pi2;
    int m,k;

    pi2=8.*atan(1.);
    t.real=0.;
    t.imag=isign*pi2/n;
    ts.real=0.0;

    for(m=0; m<n; m++)
    {
        y[m]=x[0];
        for(k=1; k<n; k++)
        {
            ts.imag=t.imag*k*m;
            z=cexp(ts);
            y[m].real+=x[k].real*z.real-x[k].imag*z.imag;
            y[m].imag+=x[k].real*z.imag+x[k].imag*z.real;
        }

        if(isign==1)
        {
            y[m].real/=n;
            y[m].imag/=n;
        }
    }
}