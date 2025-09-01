#include "iirfilter.h"

const int CHANNELNUM=16;

IIRFILTER::IIRFILTER()
{
    initW();

}
//初始化w矩阵  每次开始采集初始化一次
void IIRFILTER::initW(){

    //    for (int i = 0; i < 5; i++)//初始化
    //    {
    //        for (int j = 0; j < 3; j++)
    //        {wh[i][j] = 0;}
    //        for (int k = 0; k < 3; k++)
    //        {wl[i][k] = 0;}
    //    }
    for(int x=0;x<CHANNELNUM;x++)
    {


        for (int i = 0; i < 5; i++)//初始化
        {
            for (int j = 0; j < 3; j++)
            {
                matrix_band[x][i][j] = 0;
            }
        }
    }
    for(int a=0;a<CHANNELNUM;a++)
    {
        for(int i=0;i<21;i++)
        {
            x[a][i]=0;
            y[a][i]=0;
        }
    }

}
/*******************   N阶梳状滤波:去除工频   ************************/
double IIRFILTER::B_filter(int i,double temp)
{
    double yn=0;
    double r=0.8508348019071;
    double b=(1+r)/2;


    /*y[n] = bx[n]-bx[n-N]+ry[n-N]*/
    yn = b*(temp-x[i%CHANNELNUM][n_N])+r*y[i%CHANNELNUM][n_N];


    /*存储yn为下次迭代准备*/
    y[i%CHANNELNUM][n_N] = yn;
    x[i%CHANNELNUM][n_N] = temp;


    if(n_N==0)
    {
        n_N = 20;
    }
    else
    {
        if(i%CHANNELNUM==7)
        {
            n_N --;
        }
       
    }
    return yn;
}

double IIRFILTER::iir(double *a, double *b, double *w, double xin,int N_IIR){

    int k;

    double temp = xin;



    for (k = 0; k < N_IIR; k++)

    {
//        *(w + k * 3) = temp - *(a + 3 + 6 * k + 1) * (*(w + k * 3 + 1)) - *(a + 3 + 6 * k + 2) * (*(w + k * 3 + 2));

//        //这里temp为本二阶节的输入，也是上一个二阶节的输出

//        temp = *(b + 3 + 6 * k) * (*(w + k * 3)) + *(b + 3 + 6 * k + 1) * (*(w + k * 3 + 1)) + *(b + 3 + 6 * k + 2) * (*(w + k * 3 + 2));

//        //这里temp为本二阶节的输出，也是下一个二阶节的输入

//        *(w + k * 3 + 2) = *(w + k * 3 + 1);

//        *(w + k * 3 + 1) = *(w + k * 3);

//        temp = temp * (*(b + 6 * k));//放大倍数，稳定信号
        w[k*3]=temp -a[3 + 6 * k + 1] * w[k * 3 + 1] - a[3 + 6 * k + 2] * w[+ k * 3 + 2];
        temp =b[3 + 6 * k] *w[k * 3] + b[3 + 6 * k + 1] *w[k * 3 + 1] +b[ + 3 + 6 * k + 2] *w[k * 3 + 2];
        w[k * 3 + 2] =w[k * 3 + 1];
        w[k * 3 + 1] =w[k * 3];
        temp=temp*b[6 * k];




    }
    return temp;
}
