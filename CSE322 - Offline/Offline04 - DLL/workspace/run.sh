g++ main.cpp -c && g++ -w -g main.o -fsanitize=address -o main.out && ./main.out <input/test7.txt
