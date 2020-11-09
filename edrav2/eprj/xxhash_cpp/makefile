OBJ = test_main.o xxhash.o

%.o: %.cpp 
	$(CXX) -c -o $@ $< $(CPPVERFLAG) $(EXTRAARGS)

cpplinqmake: $(OBJ)
	$(CXX) -o test $^ -I. -Wall -Wextra $(CPPVERFLAG) $(EXTRAARGS) $(LIBS)