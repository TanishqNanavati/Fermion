#include "common.h"

void putchar(char ch);

void printf(const char *fmt,...){
    va_list vargs;
    va_start(vargs,fmt);

    while(*fmt){
        if(*fmt=='%'){
            fmt++;
            switch(*fmt){
                case '\0':{
                    putchar('%');
                    goto end;
                }
                case '%':{
                    putchar('%');
                    break;
                }
                case 's':{
                    const char * s=va_arg(vargs,const char*);
                    while(*s){
                        putchar(*s);
                        s++;
                    }
                    break;
                }
                case 'd':{
                    int value = va_arg(vargs,int);
                    unsigned magnitude =value;
                    if(value<0){
                        putchar('-');
                        magnitude=-magnitude;
                    } 
                    unsigned divisor = 1;
                    while(magnitude/divisor>0) divisor*=10;
                    while(divisor>0){
                        putchar('0'+ magnitude/divisor);
                        magnitude%=divisor;
                        divisor/=10;
                    }
                    break;
                }
                // lowercase hexadecimal
                case 'x':{
                    unsigned value = va_arg(vargs,unsigned);
                    for(int i=7;i>=0;i--){
                        unsigned nibble = (value >>(i*4)) & 0xf;
                        putchar("0123456789abcdef"[nibble]);
                    }
                }

                case 'c':{
                    char ch = va_arg(vargs,int);
                    putchar(ch);
                    break;
                }

                case 'u':{
                    unsigned value = va_arg(vargs,unsigned);
                    unsigned divisor =1;
                    while(value/divisor>=10 && divisor<=UINT_MAX/10) divisor*=10;
                    while(divisor>0){
                        putchar('0'+value/divisor);
                        value%=divisor;
                        divisor/=10;
                    }
                    break;
                }
                // uppercase hexadecimal
                case 'X':{
                    unsigned value=va_arg(vargs,unsigned);
                    for(int i=7;i>=0;i--){
                        unsigned nibble=(value>>(i*4)) & 0xf;
                        putchar("0123456789ABCDEF"[nibble]);
                    }
                    break;
                }
            }
        }else{
            putchar(*fmt);
        }
        fmt++;
    }

    end:
    va_end(vargs);
}

void *memcpy(void *dst , const void *src, size_t n){
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while(n--){
        *d++=*s++;
    }
    return dst;
}

void *memset(void *buf,char c,size_t n){
    uint8_t *p=(uint8_t *)buf;
    while(n--){
        *p++=c;
    }
    return buf;
}

void*strcpy(char *dst,const char *src){
    char *d=(char *)dst;
    char *s=src;
    while(*s){
        *d++=*s++;
    }
    *d='\0';
    return dst;
}

int strcmp(const char *s1 ,const char *s2){
    char *d=(const char *)s1;
    char *s=s2;
    while(*s && *d){
        if(*s != *d) break;
        s++;
        d++;
    }
    return *(unsigned char*)s - *(unsigned char*)d;
}
