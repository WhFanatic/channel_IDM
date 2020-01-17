# include <iostream>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>

# include "Basic.h"

using namespace std;

typedef fftw_complex fcmplx;


Bulk::Bulk(int n1, int n2, int n3):
nx(n1),
ny(n2),
nz(n3),
nxz(n1*n3),
nxc(nx/2+1),
nxr(2*nxc),
nxzc(nz*nxc),
nxzr(nz*nxr)
{
	q = new double [nxzr * ny + nxz]; // q and fq share most memory, but staggered for 1 layer to avoid fft interfering
	frcs = new fftw_plan [ny];
	fcrs = new fftw_plan [ny];

	for (int j=0; j<ny; j++) {
		double *r = (double*) lyrGet (j);
		fcmplx *c = (fcmplx*) lyrGetF(j);
		frcs[j] = fftw_plan_dft_r2c_2d(nz, nx, r, c, FFTW_MEASURE);
		fcrs[j] = fftw_plan_dft_c2r_2d(nz, nx, c, r, FFTW_MEASURE); // note: the inverse transform (complex to real) has the side-effect of overwriting its input array
	}
}

Bulk::~Bulk()
{
	for (int j=0; j<ny; j++) {
		fftw_destroy_plan(frcs[j]);
		fftw_destroy_plan(fcrs[j]);
	}
	delete [] frcs;
	delete [] fcrs;
	delete [] q;
}


/***** FFT *****/

//  q: [===][===][===][===][-------]
// fq: [---][====][====][====][====]

void Bulk::fft() // order must be bottom->top to avoid interfering. No parallel can be performed here !
{ for (int j=ny-1; j>=0; j--) fftw_execute(frcs[j]); }

void Bulk::ifft() // order must be top->bottom to avoid interfering. No parallel can be performed here !
{ for (int j=0; j<ny; j++) fftw_execute(fcrs[j]); blkMlt(1./nxz); }


/***** convinent operations for whole arrays *****/

void Bulk::layerTraverse(double a, int j, void (*pfun)(double &b, double a))
{
	double *ql = this->lyrGet(j);
	for (int i=0; i<nxz; i++) pfun(ql[i], a);
}
void Bulk::layerTraverse(double *src, int j, void (*pfun)(double &b, double a))
{
	double *ql = this->lyrGet(j);
	for (int i=0; i<nxz; i++) pfun(ql[i], src[i]);
}

void Bulk::bulkTraverse(double a, void (*pfun)(double &b, double a))
{ for (int i=0; i<nxz*ny; i++) pfun(q[i], a); }

void Bulk::bulkTraverse(double *src, void (*pfun)(double &b, double a))
{ for (int i=0; i<nxz*ny; i++) pfun(q[i], src[i]); }



/***** file IO *****/

void Bulk::fileIO(const char *path, const char *name, char mode) const
/* read & write field from & to binary files */
{
	FILE *fp;
	char str[1024];

	sprintf(str, "%s%s.bin", path, name);

	fp = fopen( str, (mode == 'w' ? "wb" : "rb") );

		// write domain information at the beginning
		if (mode == 'w') {
			fwrite(& nx, sizeof(int), 1, fp);
			fwrite(& ny, sizeof(int), 1, fp);
			fwrite(& nz, sizeof(int), 1, fp);
		}
		// data begin after the info section
		fseek(fp, sizeof(double) * nxz, SEEK_SET);
		if (mode == 'w') fwrite(q, sizeof(double) * nxz, ny, fp);
		if (mode == 'r') fread (q, sizeof(double) * nxz, ny, fp);

	fclose(fp);
}

void Bulk::debug_AsciiOutput(const char *path, const char *name, int j1, int j2, bool ifreal) const
/* write the fields in ascii files for check */
{
	FILE *fp; char str[1024]; int i, j, k;

	sprintf(str, "%s%s.txt", path, name);
	fp = fopen(str, "w");
	for (j=j1;j<j2; j++) { fputc('\n', fp);
	for (k=0; k<nz; k++) { fputc('\n', fp);
	for (i=0; i<nx; i++) {
		sprintf(str, "%.6f\t", this->id(i,j,k));
		fputs(str, fp);
	}}}
	fclose(fp);

	if (ifreal) return;

	sprintf(str, "%s%s_FR.txt", path, name);
	fp = fopen(str, "w");
	for (j=j1;j<j2; j++) { fputc('\n', fp);
	for (k=0; k<nz; k++) { fputc('\n', fp);
	for (i=0; i<nxc;i++) {
		sprintf(str, "%.6f\t", this->idf(2*i,j,k));
		fputs(str, fp);
	}}}
	fclose(fp);

	sprintf(str, "%s%s_FI.txt", path, name);
	fp = fopen(str, "w");
	for (j=j1;j<j2; j++) { fputc('\n', fp);
	for (k=0; k<nz; k++) { fputc('\n', fp);
	for (i=0; i<nxc;i++) {
		sprintf(str, "%.6f\t", this->idf(2*i+1,j,k));
		fputs(str, fp);
	}}}
	fclose(fp);
}


// void Bulk::fileIO(char *path, char *name, char mode)
// /* read & write field from & to binary files */
// {
// 	FILE *fp;
// 	char str[1024];
// 	float *buf = new float [nxz*ny];
// 	int i, j, k, idx;

// 	sprintf(str, "%s%s.bin", path, name);

// 	fp = fopen( str, (mode == 'w' ? "wb" : "rb") );

// 		// write domain information at the beginning
// 		if (mode == 'w') {
// 			fwrite(& nx, sizeof(int), 1, fp);
// 			fwrite(& ny, sizeof(int), 1, fp);
// 			fwrite(& nz, sizeof(int), 1, fp);
// 		}
// 		// data begin after the info section
// 		fseek(fp, sizeof(float) * nxz, SEEK_SET);
// 		if (mode == 'w') {
// 			for (j=0; j<ny; j++) {
// 			for (k=0; k<nz; k++) {
// 			for (i=0; i<nx; i++) {
// 				idx = nxz * j + nx * k + i;
// 				buf[idx] = (float)q[idx];
// 			}}}
// 			fwrite(buf, sizeof(float) * nxz, ny, fp);
// 		}
// 		if (mode == 'r') {
// 			fread (buf, sizeof(float) * nxz, ny, fp);
// 			for (j=0; j<ny; j++) {
// 			for (k=0; k<nz; k++) {
// 			for (i=0; i<nx; i++) {
// 				idx = nxz * j + nx * k + i;
// 				q[idx] = (double)buf[idx];
// 			}}}
// 		}

// 	fclose(fp);
// 	delete [] buf;
// }





// #define DEBUG

// sample compiling commands for test on Mac
// export CPLUS_INCLUDE_PATH=/usr/local/include
// export LIBRARY_PATH=/usr/local/lib
// g++-9 ffttest.cpp -lfftw3 -lm -fopenmp

# ifdef DEBUG

int main()
{
	int i, j, k;
	Bulk blk(5,4,3);

	for (j=0; j<4; j++) {
	for (k=0; k<3; k++) {
	for (i=0; i<5; i++) {
		blk.id(i,j,k) = i*j + j*k + k*i;
	}}}

	blk.debug_AsciiOutput("", "test0", 0, 4);
	blk.fft();
	blk.debug_AsciiOutput("", "test1", 0, 4);
	blk.ifft();
	blk.debug_AsciiOutput("", "test2", 0, 4);
}

# endif
