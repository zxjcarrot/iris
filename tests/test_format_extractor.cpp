#include "../src/format_extractor.h"
#include <assert.h>
#include <cstdio>
#include <string>
using namespace iris;
int main (){
    format_extractor ext("hhds,asd %ll %lf %d %q %4.5f %lld%,x %x %X %.f %.d %.lld %4.llf %dd\n");
    char *start, *end;
    int type;
    while (ext.extract(type, start, end)) {
        switch(type) {
            case TYPE_ILLFORMED:
                printf("ILLFORMED:[");
            break;
            case TYPE_OTHER:
                printf("OTHER:[");
            break;
            case TYPE_SPECIFIER:
                printf("SPECIFIER:[");
            break;
        }
        printf("%s]\n", std::string(start, end + 1).c_str());
    }
}