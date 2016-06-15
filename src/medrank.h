#ifndef __MEDRANK_H
#define __MEDRANK_H

class medrank {
public:
    medrank();
    ~medrank();
    int runAlgorithm(float* q);
	static void generateFileName(int id, char* fileName);
};

#endif