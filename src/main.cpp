#include "b_tree.h"
#include "medrank.h"
#include "def.h"
#include "LItem.h"

//#include <unistd.h>
//#include <getopt.h>

float result[100];

class Litem;

int a[60000][784];
LItem Litems[50][60000];
float q[100][50];
int queries[100][784];
void generate();

// void parseArgs(int argc, char** argv);

int main(int argc, char** argv) {
    //  Test Parse
    //printf("Test ParseArgs\n");
    //parseArgs(argc, argv);
     int dataSize = -1;

     BTree* tree;
     medrank solution;
     generate();
     printf("generated!");
     char fileName[50];
     int ans[100];
     for (int i = 0; i < 50; ++i) {
         solution.generateFileName(i, fileName);
		 tree = new BTree;
         tree->init(fileName, BLOCKLENGTH);
         tree->bulkload(Litems[i], 60000);
         printf("bulkload finish:%d\n", i);
		 delete tree;
     }
     for (int i = 0; i < 100; ++i)
     {
		 printf("run start:%d\n", i);
         ans[i] = solution.runAlgorithm(q[i]);
         for (int j = 0; j < 784; ++j)
         {
             result[i] += queries[i][j] * a[ans[i]][j];
         }
         printf("%lf\n", result[i]);
     }
    return 0;
}


//void parseArgs(int argc, char** argv) {
//    int opt;  
//    int digit_optind = 0;  
//    int option_index = 0;  
//    char *optstring = "n:d:";  
//    static struct option long_options[] = {
//        {"qn", required_argument, NULL, '1'},  
//        {"ds", required_argument, NULL, '2'},  
//        {"qs", required_argument, NULL, '3'},  
//        {0, 0, 0, 0}  
//    };  
//  
//    while ( (opt = getopt_long_only(argc, argv, optstring, long_options, &option_index)) != -1)  
//    {  
//        printf("optarg = %s\n", (char*) optarg);  
//    }  
//}