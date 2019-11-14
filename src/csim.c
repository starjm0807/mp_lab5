/* loginID : starjm0807 */

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

//#define DEBUG
#ifdef DEBUG
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define DEGMSG(msg,...) fprintf(stderr, ANSI_COLOR_RED msg "\n" ANSI_COLOR_RESET, ##__VA_ARGS__)
#else
#define DEGMSG(...)
#endif


typedef struct {
    char valid;
    size_t tag;
    size_t counter; 
} Line;

Line** cache;
size_t S=0, E=0, B=0, s=0, b=0;
size_t h=0, m=0, e=0;

char mode_v=0;

void help();
void simulate(size_t addr);
int main(int argc, char** argv)
{
    char flag_s=0, flag_E=0, flag_b=0, flag_t=0;
    FILE* fp = NULL;
    
    //parse arguments
    int opt;
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch(opt){
            case 'h':
                help();
                return 0;
            case 'v':
                mode_v=1;
                break;
            case 's':
                flag_s=1; 
                s = atoi(optarg);
                S = 2 << s;
                break;
            case 'E':
                flag_E=1;
                E = atoi(optarg);
                break;
            case 'b':
                flag_b=1;
                b = atoi(optarg);
                B = 2 << b;
                break;
            case 't':
                flag_t=1;
                if (!(fp = fopen(optarg, "r")))  return 3;
                break;
            default:
                return 1; 
        }
    }

    //check if all flags are used
    if(!(flag_s && flag_E && flag_b && flag_t)) return 2; 

    //prepare cache
    cache = malloc(sizeof(Line*) * S);
    for(int i=0;i<S;i++)
        cache[i] = calloc(sizeof(Line), E);

    //simulate tracefile line by line
    char oper;
    size_t addr;
    size_t size;
    while(fscanf(fp, " %c %lx%*c%ld", &oper, &addr, &size) != EOF)
    {
        
        //decide miss/hit/evic
        if (oper == 'I') continue;
        
        for(int i=0;i<s;i++){
            for(int j=0;j<E;j++){
                DEGMSG("set%d line%d : %d %lx %ld", i, j,
                    cache[i][j].valid, cache[i][j].tag, cache[i][j].counter);
            }
        }

        if(mode_v)
            printf("%c %lx,%ld ", oper, addr, size);
             
        if (oper == 'M') simulate(addr);
        simulate(addr);
        
        if(mode_v)
            printf("\n"); 
    }

    //clean memory
    for(int i=0;i<S;i++)
        free(cache[i]);
    free(cache);
    fclose(fp);

    //print the answer
    printSummary(h, m, e);

    return 0;
}

void help(){
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n\n");
    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");

    return;
}

void refresh_counter(Line* set, size_t n);
void simulate(size_t addr){
    size_t tag = addr >> (s+b);
    size_t set_index = (addr >> b) & (0x7fffffffffffffff >> (sizeof(size_t)*8-1-s));

    DEGMSG("tag : %lx", tag);
    DEGMSG("set : %lx", set_index);
   
    Line* set = cache[set_index];
   
    //check hit  
    for(int i=0;i<E;i++){
        Line* line = &set[i];

        if(!line->valid)
            continue;
        if(line->tag != tag)
            continue;
        
        h++;
        refresh_counter(set, i);
        if(mode_v)
            printf("hit ");
        return; 
    }
    
    //miss 
    m++;
    if(mode_v)
        printf("miss ");

    //find empty line
    for(int i=0;i<E;i++){
        Line* line = &set[i];
        
        if(!line->valid){
            line->tag = tag;
            line->valid = 1;

            refresh_counter(set, i);

            return;
        }
    }

    //eviction
    e++;
    if(mode_v)
        printf("eviction ");

    //LRU eviction
    for(int i=0;i<E;i++){
        Line* line = &set[i];
        
        if(!line->counter){
            line->tag = tag;
            line->valid = 1;
            
            refresh_counter(set, i);

            return;
        }
    }

    return;
}

void refresh_counter(Line* set, size_t n){
    Line* target = &set[n];
    
    for(int i=0;i<E;i++){
        Line* line = &set[i];
        if(!line->valid) continue;
        if(line->counter > target->counter)
            line->counter--;
    }

    target->counter = E-1;
    
    return;
}

