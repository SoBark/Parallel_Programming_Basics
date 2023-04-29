//original non parallel program was taken from https://rosettacode.org/wiki/Mandelbrot_set#PPM_non_interactive

/* 
 c program:
 --------------------------------
  1. draws Mandelbrot set for Fc(z)=z*z +c
  using Mandelbrot algorithm ( boolean escape time )
 -------------------------------         
 2. technique of creating ppm file is  based on the code of Claudio Rocchini
 http://en.wikipedia.org/wiki/Image:Color_complex_plot.jpg
 create 24 bit color graphic file ,  portable pixmap file = PPM 
 see http://en.wikipedia.org/wiki/Portable_pixmap
 to see the file use external application ( graphic viewer)
  */
 #include <stdio.h>
 #include <iostream>
 #include <math.h>
 #include <chrono>
 #include <omp.h>
 #include <fstream>

#define THREAD_COUNT 6
#define CHUNK_SIZE_FIRST 1
#define CHUNK_SIZE_SECOND 1

 int main()
 {
        omp_set_num_threads(THREAD_COUNT);
        auto start_s = std::chrono::steady_clock::now();
          /* screen ( integer) coordinate */
        //int iX,iY;
        const int iXmax = 800; 
        const int iYmax = 800;
        /* world ( double) coordinate = parameter plane*/
        // double Cx,Cy;
        const double CxMin=-2.5;
        const double CxMax=1.5;
        const double CyMin=-2.0;
        const double CyMax=2.0;
        /* */
        double PixelWidth=(CxMax-CxMin)/iXmax;
        double PixelHeight=(CyMax-CyMin)/iYmax;
        /* color component ( R or G or B) is coded from 0 to 255 */
        /* it is 24 bit color RGB file */
        const int MaxColorComponentValue=255; 
        FILE * fp;
        const char *filename="new1_parallel.ppm";
        const char *comment="# ";/* comment should start with # */
        static unsigned char colors[iYmax][iXmax][3];
        /* Z=Zx+Zy*i  ;   Z0 = 0 */
        //double Zx, Zy;
        //double Zx2, Zy2; /* Zx2=Zx*Zx;  Zy2=Zy*Zy  */
        /*  */
        //int Iteration;
        const int IterationMax=200;
        /* bail-out value , radius of circle ;  */
        const double EscapeRadius=2;
        const double ER2=EscapeRadius*EscapeRadius;
        /* compute image data */
        //#pragma omp parallel default(none) for schedule(dynamic) 
        #pragma omp parallel for schedule(dynamic, CHUNK_SIZE_FIRST) 
        for(int iY=0;iY<iYmax;iY++)
        {
             double Cy=CyMin + iY*PixelHeight;
             if (fabs(Cy)< PixelHeight/2) Cy=0.0; /* Main antenna */
             #pragma omp parallel for schedule(dynamic, CHUNK_SIZE_SECOND)
             for(int iX=0;iX<iXmax;iX++)
             {         
                        double Cx=CxMin + iX*PixelWidth;
                        /* initial value of orbit = critical point Z= 0 */
                        double Zx=0.0;
                        double Zy=0.0;
                        double Zx2=Zx*Zx;
                        double Zy2=Zy*Zy;
                        /* */
                        int Iteration;
                        for (Iteration=0;Iteration<IterationMax && ((Zx2+Zy2)<ER2);Iteration++)
                        {
                            Zy=2*Zx*Zy + Cy;
                            Zx=Zx2-Zy2 +Cx;
                            Zx2=Zx*Zx;
                            Zy2=Zy*Zy;
                        };
                        /* compute  pixel color (24 bit = 3 bytes) */
                        if (Iteration==IterationMax)
                        { /*  interior of Mandelbrot set = black */
                           colors[iY][iX][0]=0;
                           colors[iY][iX][1]=0;
                           colors[iY][iX][2]=0;                           
                        }
                     else 
                        { /* exterior of Mandelbrot set = white */
                             colors[iY][iX][0]=255; /* Red*/
                             colors[iY][iX][1]=255;  /* Green */ 
                             colors[iY][iX][2]=255;/* Blue */
                        };
                        /*write color to the file*/
                        //fwrite(color,1,3,fp);
                }
        }

        /*create new file,give it a name and open it in binary mode  */
        fp= fopen(filename,"wb"); /* b -  binary mode */
        /*write ASCII header to the file*/
        fprintf(fp,"P6\n %s\n %d\n %d\n %d\n",comment,iXmax,iYmax,MaxColorComponentValue);
        for(int iY=0;iY<iYmax;iY++)
            for (int iX=0;iX<iXmax;iX++)
                fwrite(colors[iY][iX],1,3,fp);

        fclose(fp);
        auto end_s = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_s-start_s);
        std::cout << "Time for "<< THREAD_COUNT <<" threads: " << elapsed_time.count() << " ms" << ";" << CHUNK_SIZE_FIRST << ";" << CHUNK_SIZE_SECOND << std::endl;
        std::ofstream myfile;
        myfile.open ("time.csv", std::ios_base::app);
        myfile << THREAD_COUNT << ";" << CHUNK_SIZE_FIRST << ";" << CHUNK_SIZE_SECOND << ";" << elapsed_time.count() << std::endl;
        myfile.close();
        return 0;
 }