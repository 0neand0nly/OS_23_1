# Operating Systems Homework3

* 21800637 Jooyoung Jang
* 21900699 Seongjun Cho

[Assignment PDF](https://github.com/hongshin/OperatingSystem/blob/master/assignments/homework3.pdf)

---

# bmalloc library: Buddy memory allocator

bmalloc is a library provided in c that provides efficient memory allocations by using the buddy memory allocation strategy. The library function bmalloc() will allocate memory space that will fit the requested size with minimal size, minimalizing internal fragmentation.

# How to use this library

### Preprocess

1. clone this repository using:

```
$ git clone https://github.com/0neand0nly/OS_bmalloc
```

2. Include the bmalloc library by adding this header in your c program

```
#include "bmalloc.h"
```

3. Compile and run
   This repository provides example test programs. To run them, use ``$ make`` to compile and run.

If you want to remove executable files, use
```
$ make clean
```

# Library functions

### void * bmalloc (size_t s)

Allocates a buffer of s-bytes and returns its starting address.

### void bfree (void * p)

Free the allocated buffer starting at pointer p.

### void * brealloc (void * p, size_t s)

Resize the allocated memory buffer into s bytes.

### void bmconfig (bm_option opt)

Set the space management scheme as BestFit, or FirstFit.

### void bmprint ()

Print out the internal status of the block

---

* Example usage: test1.c ($ sh ./test1)

```
#include <stdio.h>
#include "bmalloc.h"

int 
main ()
{
	void *p1, *p2, *p3, *p4 ;

	p1 = bmalloc(2000) ; 
	printf("bmalloc(2000):%p\n", p1) ; 
	bmprint() ;

	p2 = bmalloc(2500) ; 
	printf("bmalloc(2500):%p\n", p2) ; 
	bmprint() ;

	bfree(p1) ; 
	printf("bfree(%p)\n", p1) ; 
	bmprint() ;

	p3 = bmalloc(1000) ; 
	printf("bmalloc(1000):%p\n", p3) ; 
	bmprint() ;

	p4 = bmalloc(1000) ; 
	printf("bmalloc(1000):%p\n", p4) ; 
	bmprint() ;

}


...
```

1. Check the results with bmprint()

```
bmalloc(2000):0x7f3359060010
==================== bm_list ====================
  0:0x7f3359060010:1       11     2032:00 00 00 00 00 00 00 00
  1:0x7f3359060810:0       11     2032:00 00 00 00 00 00 00 00
=================================================
=================================================
===================== stats =====================
total given memory:             4096
total given memory to user:     2048
total available memory:         2048
=================================================
bmalloc(2500):0x7f3359026010
==================== bm_list ====================
  0:0x7f3359060010:1       11     2032:00 00 00 00 00 00 00 00
  1:0x7f3359060810:0       11     2032:00 00 00 00 00 00 00 00
  2:0x7f3359026010:1       12     4080:00 00 00 00 00 00 00 00
=================================================
=================================================
===================== stats =====================
total given memory:             8192
total given memory to user:     6144
total available memory:         2048
=================================================
bfree(0x7f3359060010)
==================== bm_list ====================
  0:0x7f3359026010:1       12     4080:00 00 00 00 00 00 00 00
...
```
