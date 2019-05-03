#include <nsbase.h>

void benchmark(int dstalign, int srcalign, size_t size, int times)
{
    char *DATA1 = (char*)malloc(size + 64);
    char *DATA2 = (char*)malloc(size + 64);
    size_t LINEAR1 = ((size_t)DATA1);
    size_t LINEAR2 = ((size_t)DATA2);
    char *ALIGN1 = (char*)(((64 - (LINEAR1 & 63)) & 63) + LINEAR1);
    char *ALIGN2 = (char*)(((64 - (LINEAR2 & 63)) & 63) + LINEAR2);
    char *dst = (dstalign)? ALIGN1 : (ALIGN1 + 1);
    char *src = (srcalign)? ALIGN2 : (ALIGN2 + 3);
    DWORD t1, t2;
    int k;
     
    Sleep(100);
    t1 = timeGetTime();
    for (k = times; k > 0; k--) {
        memcpy(dst, src, size);
    }
    t1 = timeGetTime() - t1;
    Sleep(100);
    t2 = timeGetTime();
    for (k = times; k > 0; k--) {
        NSBase::NSFunction::memcpy_fast(dst, src, size);
    }
    t2 = timeGetTime() - t2;
 
    free(DATA1);
    free(DATA2);
 
    printf("result(dst %s, src %s): memcpy_fast=%dms memcpy=%d ms\n",  
        dstalign? "aligned" : "unalign", 
        srcalign? "aligned" : "unalign", (int)t2, (int)t1);
}
 
 
void bench(int copysize, int times)
{
    printf("benchmark(size=%d bytes, times=%d):\n", copysize, times);
    benchmark(1, 1, copysize, times);
    benchmark(1, 0, copysize, times);
    benchmark(0, 1, copysize, times);
    benchmark(0, 0, copysize, times);
    printf("\n");
}
 
 
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
 
int main(void)
{
    bench(12, 0x1000000);
	bench(64, 0x1000000);
    bench(512, 0x800000);
    bench(1024, 0x400000);
    bench(4096, 0x80000);
    bench(8192, 0x40000);
    bench(1024 * 1024 * 1, 0x800);
    bench(1024 * 1024 * 4, 0x200);
    bench(1024 * 1024 * 8, 0x100);
    return 0;
}
  