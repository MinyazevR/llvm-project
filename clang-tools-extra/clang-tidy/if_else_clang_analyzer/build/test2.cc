#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void FUN_00101130(void *param_1,size_t param_2,size_t param_3,FILE *param_4)

{
  fwrite(param_1,param_2,param_3,param_4);
  return;
}



void * bmp_read(char *param_1,short *param_2,void *param_3)
{
  int iVar1;
  uint8_t bVar2;
  int iVar3;
  FILE *__stream;
  size_t sVar4;
  void *__ptr;
  
  __stream = fopen(param_1,"rb");
  if (__stream == (FILE *)0x0) {
    fwrite("Cannot open BMP file\n",1,0x15,stderr);
    __ptr = (void *)0x0;
  }
  else {
    sVar4 = fread(param_2,0xe,1,__stream);
    if (sVar4 == 1) {
      if ((*param_2 == 0x424d) || (*param_2 == 0x4d42)) {
        sVar4 = fread(param_3,0x28,1,__stream);
        if (sVar4 == 1) {
          if ((*(short *)((long)param_3 + 0xe) == 8) || (*(short *)((long)param_3 + 0xe) == 0x18)) {
            if (*(short *)((long)param_3 + 0xe) == 8) {
              if ((*(uint *)((long)param_3 + 4) & 3) == 0) {
                bVar2 = 0;
              }
              else {
                bVar2 = 4 - (char)(*(int *)((long)param_3 + 4) % 4);
              }
              iVar3 = *(int *)((long)param_3 + 8);
              iVar1 = -iVar3;
              if (0 < iVar3) {
                iVar1 = iVar3;
              }
              if ((ulong)(*(int *)(param_2 + 1) - 0x36) <
                  (long)(int)(((uint)bVar2 + *(int *)((long)param_3 + 4)) * iVar1) +
                  (ulong)(ushort)((short)*(uint8_t *)(param_2 + 5) - 0x36)) {
                fwrite("Incorrect BMP file\n",1,0x13,stderr);
                fclose(__stream);
                __ptr = (void *)0x0;
              }
              else {
                iVar3 = *(int *)(param_2 + 1);
                __ptr = malloc((ulong)(iVar3 - 0x36));
                if (__ptr == (void *)0x0) {
                  fwrite("Cannot allocate memory for pixel data\n",1,0x26,stderr);
                  fclose(__stream);
                }
                else {
                  sVar4 = fread(__ptr,(ulong)(iVar3 - 0x36),1,__stream);
                  if (sVar4 == 1) {
                    fclose(__stream);
                  }
                  else {
                    fwrite("Incorrect BMP file\n",1,0x13,stderr);
                    free(__ptr);
                    fclose(__stream);
                    __ptr = (void *)0x0;
                  }
                }
              }
            }
            else {
              if ((*(int *)((long)param_3 + 4) * 3 & 3U) == 0) {
                bVar2 = 0;
              }
              else {
                bVar2 = 4 - (char)((*(int *)((long)param_3 + 4) * 3) % 4);
              }
              iVar3 = *(int *)((long)param_3 + 8);
              if (iVar3 < 1) {
                iVar3 = -iVar3;
              }
              if ((ulong)(*(int *)(param_2 + 1) - 0x36) <
                  (long)(int)(((uint)bVar2 + *(int *)((long)param_3 + 4) * 3) * iVar3) +
                  (ulong)(ushort)((short)*(int *)(param_2 + 5) - 0x36)) {
                fwrite("Incorrect BMP file\n",1,0x13,	stderr);
                fclose(__stream);
                __ptr = (void *)0x0;
              }
              else {
                iVar3 = *(int *)(param_2 + 1);
                __ptr = malloc((ulong)(iVar3 - 0x36));
                if (__ptr == (void *)0x0) {
                  fwrite("Cannot allocate memory for pixel data\n",1,0x26,stderr);
                  fclose(__stream);
                }
                else {
                  sVar4 = fread(__ptr,(ulong)(iVar3 - 0x36),1,__stream);
                  if (sVar4 == 1) {
                    fclose(__stream);
                  }
                  else {
                    fwrite("Incorrect BMP file\n",1,0x13,stderr);
                    free(__ptr);
                    fclose(__stream);
                    __ptr = (void *)0x0;
                  }
                }
              }
            }
          }
          else {
            fwrite("Unsupported format of BMP file\n",1,0x1f,stderr);
            fclose(__stream);
            __ptr = (void *)0x0;
          }
        }
        else {
          fwrite("Cannot read BMP file2\n",1,0x16,stderr);
          fclose(__stream);
          __ptr = (void *)0x0;
        }
      }
      else {
        fwrite("Incorrect BMP file signature\n",1,0x1d,stderr);
        fclose(__stream);
        __ptr = (void *)0x0;
      }
    }
    else {
      fwrite("Cannot read BMP file1\n",1,0x16,stderr);
      fclose(__stream);
      __ptr = (void *)0x0;
    }
  }
  // This return is after an if-else
return __ptr;
}


