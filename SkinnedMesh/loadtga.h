/*  load & save TGA file
 *
 *      written by Alexander Zaprjagaev
 *      frustum@public.tsu.ru
 *      2:5005/93.15@FidoNet
 */

#ifndef __LOADTGA_H__
#define __LOADTGA_H__

unsigned char *LoadTGA(char*,int*,int*);
int SaveTGA(char*,unsigned char*,int,int);

#endif /* __LOADTGA_H__ */
