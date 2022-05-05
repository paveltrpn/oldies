// cube.cpp
// by Glenn G. Chappell and Chris Hartman
// Begin: 16 May 2001
// Current: 22 Feb 2002

#include "cube.h"
#include <GL/glut.h>
#include <assert.h>
#include <math.h>

#include <algorithm>
using std::sort;


// bitsout()
// ???xxxx -> x00x00x00x
// Number of bits (x's) = DEPTH
template <int D>
int Cube<D>::bitsout(int in)
{
int out = 0;

for (int ii=0,inmask=1,outmask=1; ii<DEPTH; ++ii,(inmask<<=1),(outmask<<=3))
   {
   if (in & inmask) out |= outmask;
   }
return out;
}


// bitsin()
// ???x??x??x??x -> xxxx
// Number of bits (x's) = DEPTH
template <int D>
int Cube<D>::bitsin(int out)
{
int in = 0;

for (int ii=0,inmask=1,outmask=1; ii<DEPTH; ++ii,(inmask<<=1),(outmask<<=3))
   {
   if (out & outmask) in |= inmask;
   }
return in;
}


template <int D>
Cube<D>::Cube(Potentialtype startthresh,  // =0
              Potentialtype startpot,     // =0
              Vertcoordtype bigcubewidth, // =1
              Vertcoordtype originx,      // =0
              Vertcoordtype originy,      // =0
              Vertcoordtype originz)      // =0
   :threshold(startthresh)
{
cubiewidth = bigcubewidth/(POTSIDE-1);
origin[0] = originx;
origin[1] = originy;
origin[2] = originz;

modifiedleaves.reserve(CUBIES); // CUBIES = number of leaf nodes
modifiedinternals.reserve(FIRSTLEAF); // FIRSTLEAF = number of internal nodes

for (int n=0; n<NODES; ++n)
   {
   data[n].minv = startpot;
   data[n].maxv = startpot;
   }

// Set up nodenumtable, potentials
int xsub, ysub, zsub;
int xoutn, xyoutn;
for (xsub=0; xsub<SIDE; ++xsub)
   {
   xoutn = FIRSTLEAF + bitsout(xsub);
   for (ysub=0; ysub<SIDE; ++ysub)
      {
      xyoutn = xoutn + (bitsout(ysub)<<1);
      for (zsub=0; zsub<SIDE; ++zsub)
         {
         potentials[xsub][ysub][zsub] = startpot;
         nodenumtable[xsub][ysub][zsub] = xyoutn + (bitsout(zsub)<<2);
         }
      }
   }

// Set up vertsubstable, leafmodified
for (int ii=0; ii<CUBIES; ++ii)
   {
   int cubien = FIRSTLEAF+ii;
   vertsubstable[cubien][0] = bitsin(ii);
   vertsubstable[cubien][1] = bitsin(ii>>1);
   vertsubstable[cubien][2] = bitsin(ii>>2);
   leafmodified[cubien] = false;
   }
}


template <int D>
void Cube<D>::surfspit(void (* handletri)(Triangle &))
{
if(!modifiedleaves.empty()) update();

ssrecurse(0,handletri);
}


template <int D>
void Cube<D>::ssrecurse(int n, void (* handletri)(Triangle &))
{
if (data[n].minv < threshold && data[n].maxv >= threshold)
   {
   if(isleaf(n))
      {
      vector_Triangle::iterator jj;
      vector_Triangle & mytris = data[n].ld.tris;
      for(jj=mytris.begin(); jj!=mytris.end(); ++jj)
         handletri(*jj);
      }
   else for(int ii=1;ii<=8;++ii) ssrecurse(8*n+ii,handletri);
   }
}


template <int D>
void Cube<D>::update() //set maxv-minv's from bottom up
{
vector_int::iterator iter;
assert(modifiedinternals.empty());
if(modifiedleaves.empty()) return;

sort(modifiedleaves.begin(),modifiedleaves.end());
  // cubies must be re-calc'd in lo-to-hi order

int lastputon = -1; //nonexistent value - nothing was put on most recently
for (iter=modifiedleaves.begin(); iter!=modifiedleaves.end(); ++iter)
   {
   int curnode = *iter;
   leafmodified[curnode] = false;
   recalccubie(curnode);

   int parent = (curnode-1)/8;
   if(parent != lastputon)
      {
      modifiedinternals.push_back(parent);
      lastputon = parent;
      }
   }
modifiedleaves.erase(modifiedleaves.begin(),modifiedleaves.end());

for (iter=modifiedinternals.begin(); iter!=modifiedinternals.end(); ++iter)
// Note: end() changes, but we have reserved space, so iterator remains valid.
   {
   int curnode = *iter;
   int childbase = curnode*8+1;
   Potentialtype maxval = data[childbase].maxv;
   Potentialtype minval = data[childbase].minv;
   for (int jj=1; jj<=7; ++jj)
      {
      if (data[childbase+jj].maxv > maxval) maxval = data[childbase+jj].maxv;
      if (data[childbase+jj].minv < minval) minval = data[childbase+jj].minv;
      }

   data[curnode].maxv = maxval;
   data[curnode].minv = minval;

   int parent = (curnode-1)/8;
   if((curnode != 0) && parent != lastputon)
      {
      modifiedinternals.push_back(parent);
      lastputon = parent;
      }
   }
modifiedinternals.erase(modifiedinternals.begin(),modifiedinternals.end());
}


template <int D>
void Cube<D>::setthreshold(Potentialtype t)
{
if(!modifiedleaves.empty()) update();

Potentialtype oldt=threshold;
threshold=t;
stdowndate(oldt,0);
}


template <int D>
void Cube<D>::stdowndate(Potentialtype oldt, int n)
{
if( (data[n].minv<oldt && data[n].maxv>=oldt) ||
    (data[n].minv<threshold && data[n].maxv>=threshold))
   {
   if(isleaf(n)) recalccubie(n);  // cubies must be re-calc'd in lo-to-hi order
   else for(int ii=1;ii<=8;++ii) stdowndate(oldt,8*n+ii);
   }
}


template <int D>
inline void Cube<D>::markleafasmodified(int xsub, int ysub, int zsub)
{
int n = nodenumtable[xsub][ysub][zsub];
if (!leafmodified[n])
   {
   modifiedleaves.push_back(n);
   leafmodified[n] = true;
   }
}


template <int D>
void Cube<D>::setpotential(int xsub, int ysub, int zsub,
                           Potentialtype newpot)
{
if (xsub<0 || xsub>=POTSIDE ||
    ysub<0 || ysub>=POTSIDE ||
    zsub<0 || zsub>=POTSIDE) return;

potentials[xsub][ysub][zsub] = newpot;

switch( (xsub==0)     + ((xsub==POTSIDE-1)<<1) +
       ((ysub==0)<<2) + ((ysub==POTSIDE-1)<<3) +
       ((zsub==0)<<4) + ((zsub==POTSIDE-1)<<5)) {
   case  0: // (000 base 4) boundaries: x --  y --  z --
      markleafasmodified(xsub-1,ysub-1,zsub-1);
      markleafasmodified(xsub-1,ysub-1,zsub  );
      markleafasmodified(xsub-1,ysub  ,zsub-1);
      markleafasmodified(xsub-1,ysub  ,zsub  );
      markleafasmodified(xsub  ,ysub-1,zsub-1);
      markleafasmodified(xsub  ,ysub-1,zsub  );
      markleafasmodified(xsub  ,ysub  ,zsub-1);
      markleafasmodified(xsub  ,ysub  ,zsub  );
      break;
   case  1: // (001 base 4) boundaries: x lo  y --  z --
      markleafasmodified(xsub  ,ysub-1,zsub-1);
      markleafasmodified(xsub  ,ysub-1,zsub  );
      markleafasmodified(xsub  ,ysub  ,zsub-1);
      markleafasmodified(xsub  ,ysub  ,zsub  );
      break;
   case  2: // (002 base 4) boundaries: x HI  y --  z --
      markleafasmodified(xsub-1,ysub-1,zsub-1);
      markleafasmodified(xsub-1,ysub-1,zsub  );
      markleafasmodified(xsub-1,ysub  ,zsub-1);
      markleafasmodified(xsub-1,ysub  ,zsub  );
      break;
   case  4: // (010 base 4) boundaries: x --  y lo  z --
      markleafasmodified(xsub-1,ysub  ,zsub-1);
      markleafasmodified(xsub-1,ysub  ,zsub  );
      markleafasmodified(xsub  ,ysub  ,zsub-1);
      markleafasmodified(xsub  ,ysub  ,zsub  );
      break;
   case  5: // (011 base 4) boundaries: x lo  y lo  z --
      markleafasmodified(xsub  ,ysub  ,zsub-1);
      markleafasmodified(xsub  ,ysub  ,zsub  );
      break;
   case  6: // (012 base 4) boundaries: x HI  y lo  z --
      markleafasmodified(xsub-1,ysub  ,zsub-1);
      markleafasmodified(xsub-1,ysub  ,zsub  );
      break;
   case  8: // (020 base 4) boundaries: x --  y HI  z --
      markleafasmodified(xsub-1,ysub-1,zsub-1);
      markleafasmodified(xsub-1,ysub-1,zsub  );
      markleafasmodified(xsub  ,ysub-1,zsub-1);
      markleafasmodified(xsub  ,ysub-1,zsub  );
      break;
   case  9: // (021 base 4) boundaries: x lo  y HI  z --
      markleafasmodified(xsub  ,ysub-1,zsub-1);
      markleafasmodified(xsub  ,ysub-1,zsub  );
      break;
   case 10: // (022 base 4) boundaries: x HI  y HI  z --
      markleafasmodified(xsub-1,ysub-1,zsub-1);
      markleafasmodified(xsub-1,ysub-1,zsub  );
      break;
   case 16: // (100 base 4) boundaries: x --  y --  z lo
      markleafasmodified(xsub-1,ysub-1,zsub  );
      markleafasmodified(xsub-1,ysub  ,zsub  );
      markleafasmodified(xsub  ,ysub-1,zsub  );
      markleafasmodified(xsub  ,ysub  ,zsub  );
      break;
   case 17: // (101 base 4) boundaries: x lo  y --  z lo
      markleafasmodified(xsub  ,ysub-1,zsub  );
      markleafasmodified(xsub  ,ysub  ,zsub  );
      break;
   case 18: // (102 base 4) boundaries: x HI  y --  z lo
      markleafasmodified(xsub-1,ysub-1,zsub  );
      markleafasmodified(xsub-1,ysub  ,zsub  );
      break;
   case 20: // (110 base 4) boundaries: x --  y lo  z lo
      markleafasmodified(xsub-1,ysub  ,zsub  );
      markleafasmodified(xsub  ,ysub  ,zsub  );
      break;
   case 21: // (111 base 4) boundaries: x lo  y lo  z lo
      markleafasmodified(xsub  ,ysub  ,zsub  );
      break;
   case 22: // (112 base 4) boundaries: x HI  y lo  z lo
      markleafasmodified(xsub-1,ysub  ,zsub  );
      break;
   case 24: // (120 base 4) boundaries: x --  y HI  z lo
      markleafasmodified(xsub-1,ysub-1,zsub  );
      markleafasmodified(xsub  ,ysub-1,zsub  );
      break;
   case 25: // (121 base 4) boundaries: x lo  y HI  z lo
      markleafasmodified(xsub  ,ysub-1,zsub  );
      break;
   case 26: // (122 base 4) boundaries: x HI  y HI  z lo
      markleafasmodified(xsub-1,ysub-1,zsub  );
      break;
   case 32: // (200 base 4) boundaries: x --  y --  z HI
      markleafasmodified(xsub-1,ysub-1,zsub-1);
      markleafasmodified(xsub-1,ysub  ,zsub-1);
      markleafasmodified(xsub  ,ysub-1,zsub-1);
      markleafasmodified(xsub  ,ysub  ,zsub-1);
      break;
   case 33: // (201 base 4) boundaries: x lo  y --  z HI
      markleafasmodified(xsub  ,ysub-1,zsub-1);
      markleafasmodified(xsub  ,ysub  ,zsub-1);
      break;
   case 34: // (202 base 4) boundaries: x HI  y --  z HI
      markleafasmodified(xsub-1,ysub-1,zsub-1);
      markleafasmodified(xsub-1,ysub  ,zsub-1);
      break;
   case 36: // (210 base 4) boundaries: x --  y lo  z HI
      markleafasmodified(xsub-1,ysub  ,zsub-1);
      markleafasmodified(xsub  ,ysub  ,zsub-1);
      break;
   case 37: // (211 base 4) boundaries: x lo  y lo  z HI
      markleafasmodified(xsub  ,ysub  ,zsub-1);
      break;
   case 38: // (212 base 4) boundaries: x HI  y lo  z HI
      markleafasmodified(xsub-1,ysub  ,zsub-1);
      break;
   case 40: // (220 base 4) boundaries: x --  y HI  z HI
      markleafasmodified(xsub-1,ysub-1,zsub-1);
      markleafasmodified(xsub  ,ysub-1,zsub-1);
      break;
   case 41: // (221 base 4) boundaries: x lo  y HI  z HI
      markleafasmodified(xsub  ,ysub-1,zsub-1);
      break;
   case 42: // (222 base 4) boundaries: x HI  y HI  z HI
      markleafasmodified(xsub-1,ysub-1,zsub-1);
      break;
   default:
      assert(0);
   }
}


// calcXintersection
// Calculate vertex & normal coords for vertex on an edge in the x direction.
// Parameters are x,y,z subscripts of low end of edge --
//  same as subscripts of edge in xeverts array.
template <int D>
inline void Cube<D>::calcXintersection(int xs, int ys, int zs)
{
int xsm1 = (xs==0) ? xs : xs-1;
int xsp2 = (xs==POTSIDE-2) ? xs+1 : xs+2;
int ysm1 = (ys==0) ? ys : ys-1;
int ysp1 = (ys==POTSIDE-1) ? ys : ys+1;
int zsm1 = (zs==0) ? zs : zs-1;
int zsp1 = (zs==POTSIDE-1) ? zs : zs+1;
Potentialtype p011 = potentials[xsm1][ys  ][zs  ];
Potentialtype p111 = potentials[xs  ][ys  ][zs  ];
Potentialtype p211 = potentials[xs+1][ys  ][zs  ];
Potentialtype p311 = potentials[xsp2][ys  ][zs  ];
Potentialtype p101 = potentials[xs  ][ysm1][zs  ];
Potentialtype p121 = potentials[xs  ][ysp1][zs  ];
Potentialtype p110 = potentials[xs  ][ys  ][zsm1];
Potentialtype p112 = potentials[xs  ][ys  ][zsp1];
Potentialtype p201 = potentials[xs+1][ysm1][zs  ];
Potentialtype p221 = potentials[xs+1][ysp1][zs  ];
Potentialtype p210 = potentials[xs+1][ys  ][zsm1];
Potentialtype p212 = potentials[xs+1][ys  ][zsp1];
double lirpfactor = ((double)threshold-p111)/(p211-p111);

xeverts[xs][ys][zs][0] = (lirpfactor + xs)*cubiewidth+origin[0];
xeverts[xs][ys][zs][1] = (             ys)*cubiewidth+origin[1];
xeverts[xs][ys][zs][2] = (             zs)*cubiewidth+origin[2];

Normcoordtype nn[3];
nn[0] = (1-lirpfactor)*(p211-p011)+lirpfactor*(p311-p111);
nn[1] = (1-lirpfactor)*(p121-p101)+lirpfactor*(p221-p201);
nn[2] = (1-lirpfactor)*(p112-p110)+lirpfactor*(p212-p210);
Normcoordtype len = sqrt(nn[0]*nn[0]+nn[1]*nn[1]+nn[2]*nn[2]);
if (len > 0)
   {
   xenorms[xs][ys][zs][0] = -nn[0]/len;
   xenorms[xs][ys][zs][1] = -nn[1]/len;
   xenorms[xs][ys][zs][2] = -nn[2]/len;
   }
else
   {
   xenorms[xs][ys][zs][0] = 1;
   xenorms[xs][ys][zs][1] = 0;
   xenorms[xs][ys][zs][2] = 0;
   }
}


// calcYintersection
// See calcXintersection, above
template <int D>
inline void Cube<D>::calcYintersection(int xs, int ys, int zs)
{
int ysm1 = (ys==0) ? ys : ys-1;
int ysp2 = (ys==POTSIDE-2) ? ys+1 : ys+2;
int xsm1 = (xs==0) ? xs : xs-1;
int xsp1 = (xs==POTSIDE-1) ? xs : xs+1;
int zsm1 = (zs==0) ? zs : zs-1;
int zsp1 = (zs==POTSIDE-1) ? zs : zs+1;
Potentialtype p011 = potentials[xs  ][ysm1][zs  ];
Potentialtype p111 = potentials[xs  ][ys  ][zs  ];
Potentialtype p211 = potentials[xs  ][ys+1][zs  ];
Potentialtype p311 = potentials[xs  ][ysp2][zs  ];
Potentialtype p101 = potentials[xsm1][ys  ][zs  ];
Potentialtype p121 = potentials[xsp1][ys  ][zs  ];
Potentialtype p110 = potentials[xs  ][ys  ][zsm1];
Potentialtype p112 = potentials[xs  ][ys  ][zsp1];
Potentialtype p201 = potentials[xsm1][ys+1][zs  ];
Potentialtype p221 = potentials[xsp1][ys+1][zs  ];
Potentialtype p210 = potentials[xs  ][ys+1][zsm1];
Potentialtype p212 = potentials[xs  ][ys+1][zsp1];
double lirpfactor = ((double)threshold-p111)/(p211-p111);

yeverts[xs][ys][zs][0] = (             xs)*cubiewidth+origin[0];
yeverts[xs][ys][zs][1] = (lirpfactor + ys)*cubiewidth+origin[1];
yeverts[xs][ys][zs][2] = (             zs)*cubiewidth+origin[2];

Normcoordtype nn[3];
nn[1] = (1-lirpfactor)*(p211-p011)+lirpfactor*(p311-p111);
nn[0] = (1-lirpfactor)*(p121-p101)+lirpfactor*(p221-p201);
nn[2] = (1-lirpfactor)*(p112-p110)+lirpfactor*(p212-p210);
Normcoordtype len = sqrt(nn[0]*nn[0]+nn[1]*nn[1]+nn[2]*nn[2]);
if (len > 0)
   {
   yenorms[xs][ys][zs][0] = -nn[0]/len;
   yenorms[xs][ys][zs][1] = -nn[1]/len;
   yenorms[xs][ys][zs][2] = -nn[2]/len;
   }
else
   {
   yenorms[xs][ys][zs][0] = 1;
   yenorms[xs][ys][zs][1] = 0;
   yenorms[xs][ys][zs][2] = 0;
   }
}


// calcZintersection
// See calcXintersection, above
template <int D>
inline void Cube<D>::calcZintersection(int xs, int ys, int zs)
{
int zsm1 = (zs==0) ? zs : zs-1;
int zsp2 = (zs==POTSIDE-2) ? zs+1 : zs+2;
int xsm1 = (xs==0) ? xs : xs-1;
int xsp1 = (xs==POTSIDE-1) ? xs : xs+1;
int ysm1 = (ys==0) ? ys : ys-1;
int ysp1 = (ys==POTSIDE-1) ? ys : ys+1;
Potentialtype p011 = potentials[xs  ][ys  ][zsm1];
Potentialtype p111 = potentials[xs  ][ys  ][zs  ];
Potentialtype p211 = potentials[xs  ][ys  ][zs+1];
Potentialtype p311 = potentials[xs  ][ys  ][zsp2];
Potentialtype p101 = potentials[xsm1][ys  ][zs  ];
Potentialtype p121 = potentials[xsp1][ys  ][zs  ];
Potentialtype p110 = potentials[xs  ][ysm1][zs  ];
Potentialtype p112 = potentials[xs  ][ysp1][zs  ];
Potentialtype p201 = potentials[xsm1][ys  ][zs+1];
Potentialtype p221 = potentials[xsp1][ys  ][zs+1];
Potentialtype p210 = potentials[xs  ][ysm1][zs+1];
Potentialtype p212 = potentials[xs  ][ysp1][zs+1];
double lirpfactor = ((double)threshold-p111)/(p211-p111);

zeverts[xs][ys][zs][0] = (             xs)*cubiewidth+origin[0];
zeverts[xs][ys][zs][1] = (             ys)*cubiewidth+origin[1];
zeverts[xs][ys][zs][2] = (lirpfactor + zs)*cubiewidth+origin[2];

Normcoordtype nn[3];
nn[2] = (1-lirpfactor)*(p211-p011)+lirpfactor*(p311-p111);
nn[0] = (1-lirpfactor)*(p121-p101)+lirpfactor*(p221-p201);
nn[1] = (1-lirpfactor)*(p112-p110)+lirpfactor*(p212-p210);
Normcoordtype len = sqrt(nn[0]*nn[0]+nn[1]*nn[1]+nn[2]*nn[2]);
if (len > 0)
   {
   zenorms[xs][ys][zs][0] = -nn[0]/len;
   zenorms[xs][ys][zs][1] = -nn[1]/len;
   zenorms[xs][ys][zs][2] = -nn[2]/len;
   }
else
   {
   zenorms[xs][ys][zs][0] = 1;
   zenorms[xs][ys][zs][1] = 0;
   zenorms[xs][ys][zs][2] = 0;
   }
}


template <int D>
inline void Cube<D>::getcubieinfo(const Potentialtype potvals[8],
                                  int & edgebitlist,
                                  const int * & trilist, int & trilistmaxsize)
{
int potbits = 0;
if (potvals[0] < threshold) potbits |=   1;
if (potvals[1] < threshold) potbits |=   2;
if (potvals[2] < threshold) potbits |=   4;
if (potvals[3] < threshold) potbits |=   8;
if (potvals[4] < threshold) potbits |=  16;
if (potvals[5] < threshold) potbits |=  32;
if (potvals[6] < threshold) potbits |=  64;
if (potvals[7] < threshold) potbits |= 128;

edgebitlist = edgetable[potbits];
trilist = tritable[potbits];
trilistmaxsize = 15;
}


template <int D>
void Cube<D>::recalccubie(int n)
{
int ii, jj;
Potentialtype mypots[8];
Vertcoordtype * everts[12];
Normcoordtype * enorms[12];
int xsub = vertsubstable[n][0];
int ysub = vertsubstable[n][1];
int zsub = vertsubstable[n][2];

mypots[0] = potentials[xsub  ][ysub  ][zsub  ];
mypots[1] = potentials[xsub  ][ysub+1][zsub  ];
mypots[2] = potentials[xsub+1][ysub+1][zsub  ];
mypots[3] = potentials[xsub+1][ysub  ][zsub  ];
mypots[4] = potentials[xsub  ][ysub  ][zsub+1];
mypots[5] = potentials[xsub  ][ysub+1][zsub+1];
mypots[6] = potentials[xsub+1][ysub+1][zsub+1];
mypots[7] = potentials[xsub+1][ysub  ][zsub+1];

int edgebitlist;
const int * trilist;
int trilistmaxsize;
getcubieinfo(mypots, edgebitlist, trilist, trilistmaxsize);

// Below, we assume that cubies are re-calc'd in lo-to-hi order.
// Thus, when re-calc'ing a cube, all but 3 of its edges have
// already been handled; we can assume that the correct intersection
// location has been stored in the [xyz]everts arrays.
// recalccubie() is called only by update() and stdowndate(), so
// these two need to be (and, in fact, are) written accordingly.
if (edgebitlist != 0)
   {
   // For efficiency: below only needed for small Y & Z
   if (edgebitlist &    8)  // edge  3: xyz(0)-Xyz(3)
      {
      everts[ 3] = xeverts[xsub  ][ysub  ][zsub  ];
      enorms[ 3] = xenorms[xsub  ][ysub  ][zsub  ];
      if (ysub == 0 && zsub == 0)
         calcXintersection(xsub  ,ysub  ,zsub  );
      }
   // For efficiency: below only needed for small X & Z
   if (edgebitlist &    1)  // edge  0: xyz(0)-xYz(1)
      {
      everts[ 0] = yeverts[xsub  ][ysub  ][zsub  ];
      enorms[ 0] = yenorms[xsub  ][ysub  ][zsub  ];
      if (xsub == 0 && zsub == 0)
         calcYintersection(xsub  ,ysub  ,zsub  );
      }
   // For efficiency: below only needed for small X & Y
   if (edgebitlist &  256)  // edge  8: xyz(0)-xyZ(4)
      {
      everts[ 8] = zeverts[xsub  ][ysub  ][zsub  ];
      enorms[ 8] = zenorms[xsub  ][ysub  ][zsub  ];
      if (xsub == 0 && ysub == 0)
         calcZintersection(xsub  ,ysub  ,zsub  );
      }

   // For efficiency: below only needed for small X
   if (edgebitlist &  512)  // edge  9: xYz(1)-xYZ(5)
      {
      everts[ 9] = zeverts[xsub  ][ysub+1][zsub  ];
      enorms[ 9] = zenorms[xsub  ][ysub+1][zsub  ];
      if (xsub == 0)
         calcZintersection(xsub  ,ysub+1,zsub  );
      }
   if (edgebitlist &   16)  // edge  4: xyZ(4)-xYZ(5)
      {
      everts[ 4] = yeverts[xsub  ][ysub  ][zsub+1];
      enorms[ 4] = yenorms[xsub  ][ysub  ][zsub+1];
      if (xsub == 0)
         calcYintersection(xsub  ,ysub  ,zsub+1);
      }

   // For efficiency: below only needed for small Y
   if (edgebitlist & 2048)  // edge 11: Xyz(3)-XyZ(7)
      {
      everts[11] = zeverts[xsub+1][ysub  ][zsub  ];
      enorms[11] = zenorms[xsub+1][ysub  ][zsub  ];
      if (ysub == 0)
         calcZintersection(xsub+1,ysub  ,zsub  );
      }
   if (edgebitlist &  128)  // edge  7: xyZ(4)-XyZ(7)
      {
      everts[ 7] = xeverts[xsub  ][ysub  ][zsub+1];
      enorms[ 7] = xenorms[xsub  ][ysub  ][zsub+1];
      if (ysub == 0)
         calcXintersection(xsub  ,ysub  ,zsub+1);
      }

   // For efficiency: below only needed for small Z
   if (edgebitlist &    4)  // edge  2: Xyz(3)-XYz(2)
      {
      everts[ 2] = yeverts[xsub+1][ysub  ][zsub  ];
      enorms[ 2] = yenorms[xsub+1][ysub  ][zsub  ];
      if (zsub == 0)
         calcYintersection(xsub+1,ysub  ,zsub  );
      }
   if (edgebitlist &    2)  // edge  1: xYz(1)-XYz(2)
      {
      everts[ 1] = xeverts[xsub  ][ysub+1][zsub  ];
      enorms[ 1] = xenorms[xsub  ][ysub+1][zsub  ];
      if (zsub == 0)
         calcXintersection(xsub  ,ysub+1,zsub  );
      }

   // Always need to calc edges below
   if (edgebitlist & 1024)  // edge 10: XYz(2)-XYZ(6)
      {
      everts[10] = zeverts[xsub+1][ysub+1][zsub  ];
      enorms[10] = zenorms[xsub+1][ysub+1][zsub  ];
      calcZintersection(xsub+1,ysub+1,zsub  );
      }

   if (edgebitlist &   64)  // edge  6: XyZ(7)-XYZ(6)
      {
      everts[ 6] = yeverts[xsub+1][ysub  ][zsub+1];
      enorms[ 6] = yenorms[xsub+1][ysub  ][zsub+1];
      calcYintersection(xsub+1,ysub  ,zsub+1);
      }

   if (edgebitlist &   32)  // edge  5: xYZ(5)-XYZ(6)
      {
      everts[ 5] = xeverts[xsub  ][ysub+1][zsub+1];
      enorms[ 5] = xenorms[xsub  ][ysub+1][zsub+1];
      calcXintersection(xsub  ,ysub+1,zsub+1);
      }

   vector_Triangle & mytris = data[n].ld.tris;
   mytris.erase(mytris.begin(), mytris.end());
   Triangle curtri;
   for (ii=0; ii!=trilistmaxsize && trilist[ii]!=-1; ii+=3)
      {
      for (jj=0; jj<3; ++jj)
         {
// We reverse vertex order below; normal & front faces toward lo pot's
         curtri.verts[jj] = everts[trilist[ii+2-jj]];
         curtri.norms[jj] = enorms[trilist[ii+2-jj]];
         }
      Vertcoordtype v1[3], v2[3];
      Normcoordtype nn[3], len;
      v1[0] = curtri.verts[1][0]-curtri.verts[0][0];
      v1[1] = curtri.verts[1][1]-curtri.verts[0][1];
      v1[2] = curtri.verts[1][2]-curtri.verts[0][2];
      v2[0] = curtri.verts[2][0]-curtri.verts[0][0];
      v2[1] = curtri.verts[2][1]-curtri.verts[0][1];
      v2[2] = curtri.verts[2][2]-curtri.verts[0][2];
      nn[0] = v1[1]*v2[2]-v1[2]*v2[1];
      nn[1] = v1[2]*v2[0]-v1[0]*v2[2];
      nn[2] = v1[0]*v2[1]-v1[1]*v2[0];
      len = sqrt(nn[0]*nn[0] + nn[1]*nn[1] + nn[2]*nn[2]);
      if (len > 0)
         {
         curtri.facenorm[0] = nn[0]/len;
         curtri.facenorm[1] = nn[1]/len;
         curtri.facenorm[2] = nn[2]/len;
         }
      else
         {
         curtri.facenorm[0] = 1.;
         curtri.facenorm[1] = 0.;
         curtri.facenorm[2] = 0.;
         }
      mytris.push_back(curtri);
      }
   }

Potentialtype maxval = mypots[0];
Potentialtype minval = mypots[0];
for (ii=1; ii<8; ++ii)
   {
   if (mypots[ii] > maxval) maxval = mypots[ii];
   if (mypots[ii] < minval) minval = mypots[ii];
   }
data[n].maxv = maxval;
data[n].minv = minval;
}


template <int D>
int Cube<D>::nodenumtable[Cube<D>::SIDE][Cube<D>::SIDE][Cube<D>::SIDE];


template <int D>
int Cube<D>::vertsubstable[Cube<D>::NODES][3];


// Two tables below taken from following:
//    File Name: CIsoSurface.cpp
//    Last Modified: 5/8/2000
//    Author: Raghavendra Chandrashekara (based on source code provided
//    by Paul Bourke and Cory Gene Bloyd)
//    Email: rc99@doc.ic.ac.uk, rchandrashekara@hotmail.com
// Original code by Cory Gene Bloyd marked as public domain.
// File found at http://astronomy.swin.edu.au/pbourke/modelling/polygonise

template <int D>
const int Cube<D>::edgetable[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};


template <int D>
const int Cube<D>::tritable[256][16] = {
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

