#include <stdio.h>
#include "bmalloc.h"

int 
main ()
{
	void *p1, *p2, *p3, *p4 ;

	bmprint() ;

    

	p1 = bmalloc(2000) ; 
	printf("bmalloc(2000):%p\n", p1) ; 
	bmprint() ;

	p2 = bmalloc(2500) ; 
	printf("bmalloc(2500):%p\n", p2) ; 
	bmprint() ;

	p3 = bmalloc(4081) ; 
	printf("bmalloc(4081):%p\n", p3) ; 
	bmprint() ;

	bfree(p1) ; 
	printf("bfree(%p)\n", p1) ; 
	bmprint() ;


    p4 = (void*) p1 - 5;
    
   
    bfree(p4);
	printf("bfree(%p)\n", p4) ; 
	bmprint() ;

	bfree(p2);
	printf("bfree(%p)\n", p2) ; 
	bmprint() ;

}
