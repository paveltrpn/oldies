// cube.h
// by Glenn G. Chappell and Chris Hartman
// Begin: 16 May 2001
// Current: 22 Feb 2002

#ifndef CUBE_H
#define CUBE_H

#include <assert.h>
#include <vector>

// **************************************************************
// Global types
// **************************************************************

// (Put these inside class Cube sometime)

typedef float Potentialtype;
typedef float Vertcoordtype;
typedef float Normcoordtype;

struct Triangle
{
   Vertcoordtype * verts[3];  // acts like Vertcoordtype verts[3][3];
   Normcoordtype * norms[3];  // acts like Vertcoordtype enorms[3][3];
   Normcoordtype facenorm[3];
};

// **************************************************************
// class Cube<int>
// **************************************************************

template <int D>
class Cube
{

// ***** Cube<int> public interface *****
public:

   enum {DEPTH = D};         // huge Cube is 2^depth cubies on a side
   enum {SIDE = 1<<DEPTH};
   enum {POTSIDE = 1+SIDE};  // cube of pot's has 1+2^depth pot's on a side

   Cube(Potentialtype startthresh = 0,
        Potentialtype startpot = 0,
        Vertcoordtype bigcubewidth = 1,
        Vertcoordtype originx = 0,
        Vertcoordtype originy = 0,
        Vertcoordtype originz = 0);
   void surfspit(void (* handletri)(Triangle &));
   void setpotential(int xsub, int ysub, int zsub, Potentialtype newpot);
   void setpotentialcoord(Vertcoordtype x,
                          Vertcoordtype y,
                          Vertcoordtype z,
                          Potentialtype newpot)
   {
   setpotential(x*(POTSIDE-1)+0.5,
                y*(POTSIDE-1)+0.5,
                z*(POTSIDE-1)+0.5,
                newpot);
   }

   Potentialtype getpotentialcoord(Vertcoordtype x,
                                   Vertcoordtype y,
                                   Vertcoordtype z)
   {
   return getpotential(x*(POTSIDE-1)+0.5,
                       y*(POTSIDE-1)+0.5,
                       z*(POTSIDE-1)+0.5);
   }

   Potentialtype getpotential(int xsub, int ysub, int zsub)
   {
   if (xsub<0 || xsub>=POTSIDE ||
       ysub<0 || ysub>=POTSIDE ||
       zsub<0 || zsub>=POTSIDE)
      return 0;
   return potentials[xsub][ysub][zsub];
   }

   void setthreshold(Potentialtype t);

   Potentialtype getthreshold(void)
   { return threshold; }

   void update();

// ***** Cube<int> internal-use types *****
protected:

   typedef std::vector<int> vector_int;
   typedef std::vector<Triangle> vector_Triangle;

   struct Leafdata
   {
      vector_Triangle tris;
   };

   class Node
   {
   public:
      Node():minv(0),maxv(0){}
      Leafdata ld;
      Potentialtype minv;
      Potentialtype maxv;
   };

// ***** Cube<int> internal-use constants & functions *****
protected:
   enum {CUBIES = 1<<(3*DEPTH)};  // There are 8^depth cubies & depth+1 levels
   enum {NODES = (CUBIES*8-1)/7}; // 11...11 in base 8 is (8^(depth+1)-1)/(8-1)
   enum {FIRSTLEAF = (CUBIES-1)/7}; 

   void stdowndate(Potentialtype oldt, int n);
   void ssrecurse(int n, void (* handletri)(Triangle &));
   Node data[NODES];
   Potentialtype potentials[POTSIDE][POTSIDE][POTSIDE];
   vector_int modifiedleaves;
   vector_int modifiedinternals;
   bool leafmodified[NODES];
   Potentialtype threshold;
   void recalccubie(int n); 
   bool isleaf(int n) { return n>=FIRSTLEAF; }

// ***** Cube<int> data members *****
protected:

   Vertcoordtype xeverts[SIDE][POTSIDE][POTSIDE][3];
   Vertcoordtype yeverts[POTSIDE][SIDE][POTSIDE][3];
   Vertcoordtype zeverts[POTSIDE][POTSIDE][SIDE][3];

   Normcoordtype xenorms[SIDE][POTSIDE][POTSIDE][3];
   Normcoordtype yenorms[POTSIDE][SIDE][POTSIDE][3];
   Normcoordtype zenorms[POTSIDE][POTSIDE][SIDE][3];

   Vertcoordtype cubiewidth;
   Vertcoordtype origin[3];

   int bitsout(int in);
   int bitsin(int out);
   inline void calcXintersection(int xs,int ys,int zs);
   inline void calcYintersection(int xs,int ys,int zs);
   inline void calcZintersection(int xs,int ys,int zs);
   inline void markleafasmodified(int xsub, int ysub, int zsub);
   inline void getcubieinfo(const Potentialtype potvals[8],
                            int & edgebitlist,
                            const int * & trilist, int & trilistmaxsize);

   static const int edgetable[256];
   static const int tritable[256][16];
   static int nodenumtable[SIDE][SIDE][SIDE];
   static int vertsubstable[NODES][3];  // 1/8 of space is wasted ... so what?
};

// **************************************************************
// END class Cube<int>
// **************************************************************

#endif  // #ifndef CUBE_H
