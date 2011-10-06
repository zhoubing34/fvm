#ifndef _KSPACE_H_
#define _KSPACE_H_

#include <vector>
#include <math.h>
#include "Vector.h"
#include "pmode.h"
#include "kvol.h"

template<class T>
class Kspace
{

 public:

  typedef Vector<T,3> Tvec;
  typedef pmode<T> Tmode;
  typedef shared_ptr<Tmode> Tmodeptr;
  typedef kvol<T> Tkvol;
  typedef shared_ptr<Tkvol> Kvolptr;
  typedef vector<Kvolptr> Volvec;
  typedef typename Tmode::Reflection Reflection;
  typedef typename Tmode::Reflptr Reflptr;
  typedef typename Tmode::Refl_pair Refl_pair;
  typedef typename Tmode::Refl_Map Refl_Map;

 Kspace(T a, T tau, T vgmag, T omega, int ntheta, int nphi) :
  _length(ntheta*nphi),
    _Kmesh(),
    _totvol(0.)
      { //makes gray, isotropic kspace  
	
	const double pi=3.141592653;
	T theta;
	T phi;
	T dtheta=pi/ntheta;
	T dphi=2.*pi/nphi;
	T dk3;
	Tvec vg;
	for(int t=0;t<ntheta;t++)
	  {
	    theta=dtheta*(t+.5);
	    for(int p=0;p<nphi;p++)
	      {
		phi=dphi*(p+.5);
		vg[0]=vgmag*sin(theta)*sin(phi);
		vg[1]=vgmag*sin(theta)*cos(phi);
		vg[2]=vgmag*cos(theta);
		dk3=2.*sin(theta)*sin(dtheta/2.)*dphi;
		Tmodeptr modeptr=shared_ptr<Tmode>(new Tmode(vg,omega,tau));
		Kvolptr volptr=shared_ptr<Tkvol>(new Tkvol(modeptr,dk3));
		_Kmesh.push_back(volptr);
		_totvol+=dk3;
	      }
	  }
      }
  
  //void setvol(int n,Tkvol k) {*_Kmesh[n]=k;}
  Tkvol& getkvol(int n) const {return *_Kmesh[n];}
  int getlength() const {return _length;}
  int gettotmodes()
  {
    return (_Kmesh[0]->getmodenum())*_length;
  }
  T getDK3() const {return _totvol;}
  T calcTauTot()
  {   // returns sum(dk3/tau)
    T tauTot=0.;
    for(int k=0;k<_length;k++)
      {
	Tkvol& kv=getkvol(k);
	const int modenum=kv.getmodenum();
	T dk3=kv.getdk3();
	for(int m=0;m<modenum;m++)
	  {
	    Tmode& mode=kv.getmode(m);
	    tauTot+=dk3/mode.gettau();
	  }
      }
    return tauTot;
  }
  void NewtonSolve(T& guess, const T e_sum)
  {
    T e0_tau;
    T de0_taudT;
    T deltaT=1.;
    T newguess;
    int iters=0;
    while(deltaT>1e-6)
      {
	gete0_tau(guess,e0_tau,de0_taudT);
	deltaT=(e0_tau-e_sum)/de0_taudT;
	newguess=guess-deltaT;
	deltaT=deltaT/guess;
	guess=newguess;
	iters++;
      }	
  }

  void gete0_tau(T& Tguess, T& e0tau, T& de0taudT)
  {
    e0tau=0.;
    de0taudT=0.;
    for(int k=0;k<_length;k++)
      {
	Tkvol& kv=getkvol(k);
	const int modenum=kv.getmodenum();
	T dk3=kv.getdk3();
	for(int m=0;m<modenum;m++)
	  {
	    Tmode& mode=kv.getmode(m);
	    e0tau+=mode.calce0tau(Tguess)*dk3;
	    de0taudT+=mode.calcde0taudT(Tguess)*dk3;
	  }
      }
  }

  T calcSpecificHeat(T Tl)
  {
    T r(0.0);
    for(int k=0;k<_length;k++)
      {
	Tkvol& kv=getkvol(k);
	const int modenum=kv.getmodenum();
	T dk3=kv.getdk3();
	for(int m=0;m<modenum;m++)
	  {
	    Tmode& mode=kv.getmode(m);
	    r+=mode.calcde0dT(Tl)*dk3;
	  }
      }
    return r;
  }

  void findSpecs(T dk3, T vo, int m, Tvec so, Refl_pair& refls)
  {
    Tmode& mode1=getkvol(0).getmode(m);
    Tmode& mode2=getkvol(1).getmode(m);
    int m1=0;
    int m2=1;
    Tvec vg1=mode1.getv();
    Tvec vg2=mode2.getv();
    Tvec sn1=vg1/sqrt(pow(vg1[0],2)+pow(vg1[1],2)+pow(vg1[2],2));
    Tvec sn2=vg2/sqrt(pow(vg2[0],2)+pow(vg2[1],2)+pow(vg2[2],2));
    T sn1dotso=sn1[0]*so[0]+sn1[1]*so[1]+sn1[2]*so[2];
    T sn2dotso=sn2[0]*so[0]+sn2[1]*so[1]+sn2[2]*so[2];
    
    if (sn2dotso>sn1dotso)
      {
	T temp=sn1dotso;
	sn1dotso=sn2dotso;
	sn2dotso=temp;
	m1=1;
	m2=0;
      }

    for(int k=2;k<_length;k++)
      {
	Tmode& temp_mode=getkvol(k).getmode(m);
	Tvec vgt=temp_mode.getv();
	const Tvec sn=vgt/sqrt(pow(vgt[0],2)+pow(vgt[1],2)+pow(vgt[2],2));
	const T sndotso=sn[0]*so[0]+sn[1]*so[1]+sn[2]*so[2];
	
	if(sndotso>sn1dotso)
	  {
	    sn2dotso=sn1dotso;
	    sn1dotso=sndotso;
	    m2=m1;
	    m1=k;
	  }
	else if (sndotso>sn2dotso)
	  {
	    sn2dotso=sndotso;
	    m2=k;
	  }
      }
    
    T w1=sn1dotso/(sn1dotso+sn2dotso);
    T w2=sn2dotso/(sn1dotso+sn2dotso);

    if(sn1dotso>.99)
      {
	w1=1;
	w2=0;
      }

    T dk31=getkvol(m1).getdk3();
    T dk32=getkvol(m2).getdk3();

    vg1=getkvol(m1).getmode(m).getv();
    vg2=getkvol(m2).getmode(m).getv();

    T v1mag=sqrt(pow(vg1[0],2)+pow(vg1[1],2)+pow(vg1[2],2));
    T v2mag=sqrt(pow(vg2[0],2)+pow(vg2[1],2)+pow(vg2[2],2));

    refls.first.first=w1*vo*dk3/v1mag/dk31;
    refls.second.first=w2*vo*dk3/v2mag/dk32;
    refls.first.second=m1;
    refls.second.second=m2;  
  }
  
 private:

  Kspace(const Kspace&);
  //num volumes
  int _length;
  Volvec _Kmesh;
  T _totvol;    //total Kspace volume
  
};


#endif
