/*  load & save TGA file
 *
 *      written by Alexander Zaprjagaev
 *      frustum@public.tsu.ru
 *      2:5005/93.15@FidoNet
 */

#include <stdio.h>
#include <malloc.h>

unsigned char *LoadTGA(char *name,int *width,int *height) {
    unsigned char   rep,*data,*buffer,*ptr,info[18];
    int Width,Height,Components,Size,i,j,k,l;
    FILE    *FileTGA = fopen(name,"rb");
    if(!FileTGA) return 0;
    fread(&info,1,18,FileTGA);  // Read header
    Width = info[12] + info[13] * 256;
    Height = info[14] + info[15] * 256;
    switch(info[16]) {      // Read only 32 && 24 bit per pixel
        case 32:
            Components = 4; // 32 bit per pixel (RGBA)
            break;
        case 24:
            Components = 3; // 24 bit per pixel (RGB)
            break;
        default:
            fclose(FileTGA);
            return 0;
    }
    Size = Width * Height * Components;
    buffer = (unsigned char*)malloc(Size);  // Buffer for RGB or RGBA image
    data = (unsigned char*)malloc(Width * Height * 4);  // Output RGBA image
    if(!data || !buffer) {
        fclose(FileTGA);
        return 0;
    }
    fseek(FileTGA,info[0],SEEK_CUR);
    i = 0;
    ptr = buffer;
    switch(info[2]) {
        case 2:     // Unmapped RGB image
            fread(buffer,1,Size,FileTGA);
            break;
        case 10:    // Run length encoded
            while(i < Size) {
                fread(&rep,1,1,FileTGA);
                if(rep & 0x80) {
                    rep ^= 0x80;
                    fread(ptr,1,Components,FileTGA);
                    ptr += Components;
                    for(j = 0; j < rep * Components; j++) {
                        *ptr = *(ptr - Components);
                        ptr ++;
                    }
                    i += Components * (rep + 1);
                }
                else {
                    k = Components * (rep + 1);
                    fread(ptr,1,k,FileTGA);
                    ptr += k;
                    i += k;
                }
            }
            break;
        default:
            fclose(FileTGA);
            free(buffer);
            free(data);
            return 0;
    }
    for(i = 0, j = 0; i < Size; i += Components, j += 4) {  // BGR -> RGBA
        data[j] = buffer[i + 2];
        data[j + 1] = buffer[i + 1];
        data[j + 2] = buffer[i];
        if(Components == 4) data[j + 3] = buffer[i + 3];
        else data[j + 3] = 255;
    }
    if(!(info[17] & 0x20))
        for(j = 0, k = Width * 4; j < Height / 2; j ++)
            for(i = 0; i < Width * 4; i ++) {
                l = data[j * k + i];
                data[j * k + i] = data[(Height - j - 1) * k + i];
                data[(Height - j - 1) * k + i] = l;
            }
    fclose(FileTGA);
    free(buffer);
    *width = Width;
    *height = Height;
    return data;
}

int SaveTGA(char *name,unsigned char *data,int width,int height) {
    int i,j;
    unsigned char *buffer;
    FILE    *FileTGA = fopen(name,"wb");
    if(!FileTGA) return 0;
    buffer = (unsigned char*)malloc(18 + width * height * 4);
    memset(buffer,0,18);
    buffer[2] = 2;
    buffer[12] = width & 0xFF;
    buffer[13] = width >> 8;
    buffer[14] = height & 0xFF;
    buffer[15] = height >> 8;
    buffer[16] = 32;
    buffer[17] = 0x20;
    memcpy(buffer + 18,data,width * height * 4);
    for(i = 18; i < 18 + width * height * 4; i += 4) {
        j = buffer[i];
        buffer[i] = buffer[i + 2];
        buffer[i + 2] = j;
    }
    fwrite(buffer,1,18 + width * height * 4,FileTGA);
    fclose(FileTGA);
    free(buffer);
    return 1;
}
