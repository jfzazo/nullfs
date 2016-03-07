LDLIBS="-lfuse"
T=nullfs
CXXFLAGS=-g #-DCREATE_AS_FILE_IF_NOT_EXIST

all: $(T)
nullfs: nullfs.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LDLIBS) -o $@
clean:
	rm -f $(T) *.o
