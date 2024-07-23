/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer. A block is pure payload. There are no headers or
 * footers. Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "GODSONS",
    /* First member's full name */
    "Enkhmunkh Nyamdorj",
    /* First member's email address */
    "20B1NUM2118",
    /* Second member's full name (leave blank if none) */
    "Saruul Ganbold",
    /* Second member's email address (leave blank if none) */
    "20B1NUM2095"
};
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
//Figure 9.43-s avch hereglej bga macronuud
#define WSIZE 4            
#define DSIZE 8
#define CHUNKSIZE (1 << 12) //4mb
#define MINBLOCKSIZE 16 //ALIGN(2*WSIZE) header footer niileed 8 payload bagadaa 8 niit 16

#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7) /* suuliin 3 bit size todorhoilohod hereggui */
#define GET_ALLOC(p) (GET(p) & 0x1) /*Suuliin neg bit ni allocated statusiig zaana*/
#define HDRP(bp) ((char *)(bp)-WSIZE)/*payloadiin omnoh block in header */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)


#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))) 
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))
#define get_size_by_header(bp) (GET_SIZE(HDRP(bp)))
#define get_size_by_footer(bp) (GET_SIZE(FTRP(bp)))
#define PUT_HEADER(bp, size, alloc) (PUT(HDRP(bp), PACK(size, alloc)))
#define PUT_FOOTER(bp, size, alloc) (PUT(FTRP(bp), PACK(size, alloc)))


static char *heap_listp, *prev_listp;

static void *merge(void *);
static void *extend_heap(size_t );
static void *find_fit(size_t );
static inline void allocate(void *, size_t );

static inline void set_free(void *bp, size_t size){
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
} 
static inline void set_alloc(void *bp, size_t size){
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
}
static inline int is_allocated(void *bp){
    return GET_ALLOC(HDRP(bp));
}
static inline int is_free(void *bp){
    return !GET_ALLOC(HDRP(bp));
}


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    //nomnii Figure 9.44-s harsan
    heap_listp = mem_sbrk(4 * WSIZE);
    if (heap_listp == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); 
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); 
    PUT(heap_listp + (3 * WSIZE), PACK(DSIZE, 1)); 
    heap_listp += (2 * WSIZE);                     
    prev_listp = heap_listp;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 * Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t alloc_size,size_need; 
    char *ret;

    if (size == 0) 
    {
        return NULL;
    }
    
    alloc_size = (size<DSIZE)?MINBLOCKSIZE:ALIGN(size+DSIZE);
    //odoo bga heapees free heseg haigad uzeh
    ret = find_fit(alloc_size);
    if(ret != NULL)
    {
        allocate(ret, alloc_size);
        return ret; //odoo bga heapees olood return hiine
    }
    //odoo bga heap dr size hemjeetei hooson zai bhgu
    //dahij heapee tomruulna payload size in chunk sizes tom baih tohioldol baij bolno.
    size_need = MAX(alloc_size, CHUNKSIZE);
    if ((ret = extend_heap(size_need / WSIZE)) == NULL)
        return NULL;
    allocate(ret, alloc_size);
    return ret;
}

/*
 * mm_free - Freeing a block and coalesce if necessary.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    set_free(bp, size);
    merge(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    size = get_size_by_header(oldptr);
    copySize = get_size_by_header(newptr);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize - WSIZE);
    mm_free(oldptr);
    return newptr;
}

static void *merge(void *bp)
{
    size_t prev_alloc = is_allocated(PREV_BLKP(bp));
    size_t next_alloc = is_allocated(NEXT_BLKP(bp));
    size_t size = get_size_by_header(bp);

    if(prev_alloc && next_alloc) // 2la allocated, merge hiihgui
        return bp;
    if(prev_alloc && !next_alloc) // daraagin heseg ni free bol neg tom bolgono
    { 
        if (NEXT_BLKP(bp) == prev_listp)
            prev_listp = bp;
        size += get_size_by_header(NEXT_BLKP(bp));
        set_free(bp, size);
        prev_listp = bp;
    }
    else if (!prev_alloc && next_alloc) //omnoh ni free
    {
        if (PREV_BLKP(bp) == prev_listp)
            prev_listp = bp;
        size += get_size_by_header(PREV_BLKP(bp));
        //omnoh blockiin header bolon odoogin blockiin footer dr size-g ni oruulj ogno 
        PUT_FOOTER(bp, size, 0);
        PUT_HEADER(PREV_BLKP(bp), size, 0);
        bp = PREV_BLKP(bp);
        prev_listp = bp;
    }
    else //2la free gher 3lang ni niiluulne
    {
        if (PREV_BLKP(bp) == prev_listp)
            prev_listp = bp;
        size += get_size_by_header(PREV_BLKP(bp)) +
                get_size_by_footer(NEXT_BLKP(bp));
        //sul uchraas niit size-g ni omnoh blockiin header daraagin blockiin footer dr oruulad boloo
        PUT_HEADER(PREV_BLKP(bp), size, 0);
        PUT_FOOTER(NEXT_BLKP(bp), size, 0);
        bp = PREV_BLKP(bp);
        prev_listp = bp;
    }
    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; //list togsgol oo 1 wordd bagtaah

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    set_free(bp, size);
    //0hemjeetei allocated block in listnii togsgoliig zaana.
    PUT_HEADER(NEXT_BLKP(bp), 0, 1);
    return merge(bp);
}

static void *find_fit(size_t s) /*First-fit algorithm ashiglasan*/
{
    for (char *bp = prev_listp; get_size_by_header(bp) > 0; bp = NEXT_BLKP(bp))
    {
        if (is_free(bp) && get_size_by_header(bp) >= s)
        {
            prev_listp = bp;
            return bp;
        }
    }

    for (char *bp = heap_listp; bp != prev_listp; bp = NEXT_BLKP(bp))
    {
        if (is_free(bp) && get_size_by_header(bp) >= s)
        {
            prev_listp = bp;
            return bp;
        }
    }
    return NULL;
}

static inline void allocate(void *bp, size_t s)
{
    size_t blk_size = GET_SIZE(HDRP(bp));
    size_t remain_size = blk_size - s;
    if (remain_size >= MINBLOCKSIZE) //2 bolgoj huvaaj bolohoo shiidne
    {
        //tusdaa 2 heseg bolgoh
        set_alloc(bp, s);
        set_free(NEXT_BLKP(bp), remain_size);
    }
    else
    {
        //internal fragmentation huleen zovshoorno.
        set_alloc(bp, blk_size);
    }
    prev_listp = bp;
}