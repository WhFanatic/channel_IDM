#pragma once

#include <iostream>
#include <algorithm>
#include <cmath>
#include <fftw3.h>

#include "Geometry.h"


class Scla
{
public:
	const Mesh ms;

	Scla(const Mesh &ms);
	~Scla();
	Scla(const Scla &src);
	Scla& operator=(const Scla &src);

	// memory access
	double& operator()(int i, int j, int k)       { return q_[ms.idx(i,j,k)]; };
	double  operator()(int i, int j, int k) const { return q_[ms.idx(i,j,k)]; };
	
	double& operator[](int id)       { return q_[id]; };
	double  operator[](int id) const { return q_[id]; };

	double*       GetLyr(int j=0)       { return &q_[ms.idx(0,j,0)]; };
	const double* SeeLyr(int j=0) const { return &q_[ms.idx(0,j,0)]; };
	
	double*       GetBlk()       { return &q_[ms.idx(0,0,0)]; };
	const double* SeeBlk() const { return &q_[ms.idx(0,0,0)]; };

	// fft
	void fftx (int j0=0, int jn=0);
	void ifftx(int j0=0, int jn=0);
	void fftz (int j0=0, int jn=0); // FORWARD (exponent -1), real to complex, without normalization
	void ifftz(int j0=0, int jn=0); // BACKWARD (exponent 1), complex to real, with normalization
	void dctx (int j0=0, int jn=0);
	void idctx(int j0=0, int jn=0);

	void fftxz (int j0=0, int jn=0);
	void ifftxz(int j0=0, int jn=0);
	void dctxz (int j0=0, int jn=0);
	void idctxz(int j0=0, int jn=0);

	// arithmetic mean (simple averaging)
	double meanz(int i, int j) const;
	double meanxz(int j=0) const;

	// weighed average (with integration)
	double MeanUx(int j, int k) const;
	double MeanVy(int i, int k) const;
	double MeanWz(int i, int j) const;
	double MeanAy(int i, int k) const;
	double MeanAz(int i, int j) const;

	double MeanUyz(int i) const;
	double MeanVxz(int j) const;
	double MeanWxy(int k) const;

	double MeanU() const;
	double MeanV() const;
	double MeanW() const;
	double MeanA() const;

	// interpolation (among faces, edges and cell-centers)
	Scla& Ugrid2CellCenter(const Scla &src);
	Scla& Vgrid2CellCenter(const Scla &src);
	Scla& Wgrid2CellCenter(const Scla &src);
	void CellCenter2Edges(Scla &dst1, Scla &dst2, Scla &dst3) const;

	// differential operators (operating on cell-centered quantities)
	std::vector<double> Gradient(int i, int j, int k) const;

	// arithmetic
	Scla& SetLyr(double a, int j=0) { TravLyr(a, j, set); return *this; }; // set to a
	Scla& AddLyr(double a, int j=0) { TravLyr(a, j, add); return *this; }; // add a
	Scla& MltLyr(double a, int j=0) { TravLyr(a, j, mlt); return *this; }; // multiplied by a

	Scla& SetLyr(const double *src, int j=0) { TravLyr(src, j, set); return *this; };
	Scla& AddLyr(const double *src, int j=0) { TravLyr(src, j, add); return *this; };
	Scla& MnsLyr(const double *src, int j=0) { TravLyr(src, j, mns); return *this; };
	Scla& MltLyr(const double *src, int j=0) { TravLyr(src, j, mlt); return *this; };
	Scla& DvdLyr(const double *src, int j=0) { TravLyr(src, j, dvd); return *this; };
	// note: bulk functions will change the boundary
	Scla& Set       (double a) { TravBlk(a, set); return *this; };
	Scla& operator+=(double a) { TravBlk(a, add); return *this; };
	Scla& operator*=(double a) { TravBlk(a, mlt); return *this; };

	Scla& Set       (const Scla &src) { TravBlk(src.SeeBlk(), set); return *this; };
	Scla& operator+=(const Scla &src) { TravBlk(src.SeeBlk(), add); return *this; };
	Scla& operator-=(const Scla &src) { TravBlk(src.SeeBlk(), mns); return *this; };
	Scla& operator*=(const Scla &src) { TravBlk(src.SeeBlk(), mlt); return *this; };
	Scla& operator/=(const Scla &src) { TravBlk(src.SeeBlk(), dvd); return *this; };

	// IO functions
	void FileIO(const char *path, const char *name, char mode) const;
	void debug_AsciiOutput(const char *path, const char *name, int j1, int j2) const;

private:
	const int Nx, Ny, Nz;
	// pointer to the bulk memory
	double *q_;
	// array of x-direction fft plans
	fftw_plan *frc_x;
	fftw_plan *fcr_x;
	// array of z-direction fft plans
	fftw_plan *frc_z;
	fftw_plan *fcr_z;
	double **fft_temp;
	// array of 2D fft plans
	fftw_plan *fcr_xz;
	fftw_plan *frc_xz;
	// array of x-direction dct plans
	fftw_plan *frR_x;
	fftw_plan *fRr_x;

	static void set(double &b, double a) {b =  a;};
	static void add(double &b, double a) {b += a;};
	static void mns(double &b, double a) {b -= a;};
	static void mlt(double &b, double a) {b *= a;};
	static void dvd(double &b, double a) {b /= a;};

	void TravLyr(double a,          int j, void (*pfun)(double&, double));
	void TravLyr(const double *src, int j, void (*pfun)(double&, double));

	void TravBlk(double a,          void (*pfun)(double&, double));
	void TravBlk(const double *src, void (*pfun)(double&, double));
};


class Vctr
{
public:
	const Mesh ms;
	
	Vctr(const Mesh &ms);

	Vctr& Set(double a) { v1_.Set(a); v2_.Set(a); v3_.Set(a); return *this; };

	Scla&       operator[](int n)       { return n==1 ? v1_ : n==2 ? v2_ : v3_; };
	const Scla& operator[](int n) const { return n==1 ? v1_ : n==2 ? v2_ : v3_; };

	Vctr& operator+=(double a) { v1_+=a; v2_+=a; v3_+=a; return *this; };
	Vctr& operator*=(double a) { v1_*=a; v2_*=a; v3_*=a; return *this; };
	Vctr& operator+=(const Vctr &src) { v1_+=src[1]; v2_+=src[2]; v3_+=src[3]; return *this; };
	Vctr& operator-=(const Vctr &src) { v1_-=src[1]; v2_-=src[2]; v3_-=src[3]; return *this; };
	Vctr& operator*=(const Vctr &src) { v1_*=src[1]; v2_*=src[2]; v3_*=src[3]; return *this; };
	Vctr& operator/=(const Vctr &src) { v1_/=src[1]; v2_/=src[2]; v3_/=src[3]; return *this; };

	// vector operators
	double  Module    (int i, int j, int k) const;
	double  Divergence(int i, int j, int k) const;
	double  Convection(int i, int j, int k) const;
	std::vector<double> ShearStrain(int i, int j, int k) const;
	std::vector<double> Strainrate (int i, int j, int k) const;
	std::vector<double> Gradient   (int i, int j, int k) const;

	Vctr& Sgrid2CellCenter(const Vctr &src);

private:
	Scla v1_;
	Scla v2_;
	Scla v3_;
};


class Flow
{
public:
	const Mesh ms;

	Flow(const Mesh &ms);

	Flow& Set(double a) { v_.Set(a); s_.Set(a); return *this; };

	Flow& operator+=(double a) { v_+=a; s_+=a; return *this; };
	Flow& operator*=(double a) { v_*=a; s_*=a; return *this; };
	Flow& operator+=(const Flow &src) { v_+=src.SeeVec(); s_+=src.SeeScl(); return *this; };
	Flow& operator-=(const Flow &src) { v_-=src.SeeVec(); s_-=src.SeeScl(); return *this; };
	Flow& operator*=(const Flow &src) { v_*=src.SeeVec(); s_*=src.SeeScl(); return *this; };
	Flow& operator/=(const Flow &src) { v_/=src.SeeVec(); s_/=src.SeeScl(); return *this; };

	void InitRand(double energy);

	Scla& GetScl() { return s_; };
	Vctr& GetVec() { return v_; };
	Scla& GetVec(int n) { return GetVec()[n]; };

	const Scla& SeeScl() const { return s_; };
	const Vctr& SeeVec() const { return v_; };
	const Scla& SeeVec(int n) const { return SeeVec()[n]; };

	// interpolate
	void CellCenter2Edges() { s_.CellCenter2Edges(v_[1], v_[2], v_[3]); };

	void CleanBoundary();
	void CombineBoundary(const Flow &fld, double a, double b);

	// IO functions
	Flow& ReadField(const char *path, int tstep, const char *suffix);
	void WriteField(const char *path, int tstep, const char *suffix) const;
	void WriteTecplot(const char *path, int tstep, double time) const;

private:
	const int Nx, Ny, Nz;
	Vctr v_;
	Scla s_;
};
