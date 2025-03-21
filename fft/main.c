#include<stdio.h>
#include<math.h>
#include<complex.h>
#define pi 3.141593
#define N 1024
#define fs 100
int main()
{
   double complex X[1024];
   int k,i;
   for(k = 0; k < N; k++)
   {
      X[k] = 0;
      for(i = 0; i < N; i++)
      {
         double n = 1.0 * i / fs;
         double complex tmp = (0.5*sin(2*pi*15*n)+2*sin(2*pi*40*n)) * cexp(-I*(2*pi/N)*i*k);
         X[k] = X[k] + tmp;
      }
      printf("%f\t%f\n", 1.0*k*fs*N, cabs(X[k]));
   }
}
