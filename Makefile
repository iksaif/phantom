VERSION=3.0

CXXFLAGS+=-O3 -pedantic -Wall -Wno-unused-variable -DVERSION=\"$(VERSION)\" $(DEBUG_FLAGS)
LDFLAGS=$(DEBUG_FLAGS)

OBJS=error.o p.o

all: phantom

phantom: $(OBJS)
	$(CXX) -Wall -W $(OBJS) $(LDFLAGS) -o phantom

install: phantom
	cp phantom $(DESTDIR)/usr/local/sbin

uninstall: clean
	rm -f $(DESTDIR)/usr/local/bin/phantom

clean:
	rm -f $(OBJS) phantom core gmon.out

package: clean
  # source package
	rm -rf phantom-$(VERSION)
	mkdir phantom-$(VERSION)
	cp *.cpp *.h Makefile readme.txt license.txt phantom-$(VERSION)
	tar czf phantom-$(VERSION).tgz phantom-$(VERSION)
	rm -rf phantom-$(VERSION)

check:
	cppcheck -v --enable=all --std=c++11 --inconclusive -I. . 2> err.txt
