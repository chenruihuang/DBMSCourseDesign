SRC=src/b_node.cpp src/b_tree.cpp src/block_file.cpp \
src/count.cpp src/LItem.cpp src/main.cpp src/medrank.cpp 

OBJS=$(SRC:.cpp=.o)
DEBUG=-g
CXX=g++ -std=c++11

all: $(OBJS)
	$(CXX) $(DEBUG) -o medrank $(OBJS)

b_node.o: src/b_node.h

b_tree.o: src/b_tree.h

block_file.o: block_file.h

LItem.o: LItem.h

count.o:

medrank.o: medrank.h

main.o:

clean:
	-rm $(OBJS) medrank
