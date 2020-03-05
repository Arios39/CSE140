#include "tips.h"
typedef int bool;
enum { false, true };
   CacheAction horm;

/* The following two functions are defined in util.c */
typedef struct {
    bool run;
    unsigned int addr;
} assistant;


typedef struct {
    int lrused;
    unsigned int addr;
    unsigned int block;
    assistant arr[MAX_SETS][MAX_ASSOC];
  unsigned int  lruvalue[MAX_SETS][MAX_ASSOC];
} LRUARRAY;
/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w);
TransferUnit transfer_unit;
LRUARRAY lruarr;
/* return random int from 0..x-1 */
int randomint( int x );

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index){
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index){
  cache[assoc_index].block[block_index].lru.value = 0;
}


void hitormiss(int index, int tag, int offset){
               horm= MISS;
    for(int i =0; i <assoc;i++){
        if(cache[index].block[i].tag==tag&&cache[index].block[i].valid==VALID){
            lruarr.block=i;
               highlight_offset(index, i, offset, HIT);
            horm= HIT;
            break;
        }
       
    }

}
int findlru(int index, int tag, int offset){
    for(int i =0; i <assoc;i++){
          if(cache[index].block[i].lru.value==0){
              return i;
          }
      }
    return 0;
}
void write_through(address addr, word* data, WriteEnable we,int index, int block,int tag,int offset){
    highlight_offset(index, block, offset, MISS);
    cache[index].block[block].tag= tag;
    cache[index].block[block].valid=VALID;
    accessDRAM(addr, cache[index].block[block].data, transfer_unit, READ);

    switch (we) {
            case READ:
            memcpy(data,cache[index].block[block].data+offset, 4);

            break;
            case WRITE:
            memcpy(cache[index].block[block].data+offset,data, 4);
            accessDRAM(addr, cache[index].block[block].data, transfer_unit, WRITE);
            break;
        default:
            break;
    }
    
}

void write_back(address addr, word* data, WriteEnable we,int index, int block,int tag,int offset){

    if (cache[index].block[block].dirty==DIRTY){
        accessDRAM(lruarr.arr[index][block].addr, cache[index].block[block].data, transfer_unit, WRITE);
    }
    cache[index].block[block].tag= tag;
    cache[index].block[block].valid=VALID;
    accessDRAM(addr, cache[index].block[block].data, transfer_unit, READ);

    switch (we) {
            case READ:
            memcpy(data,cache[index].block[block].data+offset, 4);
            cache[index].block[block].dirty = VIRGIN;
                
            break;
            case WRITE:
            memcpy(cache[index].block[block].data+offset,data, 4);
            lruarr.arr[index][block].addr=addr;
             cache[index].block[block].dirty = DIRTY;
                       break;
        default:
            break;
    }
}

void lrualgorithm(address addr, word* data, WriteEnable we, int block,int index, int tag, int offset){
    if(lruarr.lrused==0){
    for(int j = 0; j<set_count;j++){
      for(int i = 0; i<assoc;i++){
          lruarr.lruvalue[j][i] = i;
      }
      }
    }
    ///////////////
    if (lruarr.lruvalue[index][block]==0){
        lruarr.lruvalue[index][block] = assoc;
        lruarr.lrused=lruarr.lrused +1;
        cache[index].block[block].lru.value = lruarr.lruvalue[index][block];
               
    }
       
           for(int i = 0; i<assoc;i++){
            if (lruarr.lruvalue[index][i]!=0){
            lruarr.lruvalue[index][i] =  lruarr.lruvalue[index][i] -1;
            }
               if( cache[index].block[i].lru.value!=0){

                   cache[index].block[i].lru.value = lruarr.lruvalue[index][i];

               }

        }
}
void hitalgo(address addr, word* data, WriteEnable we, int block,int index, int tag, int offset){
    switch (we) {
        case READ:
  
        memcpy(data,cache[index].block[block].data+offset, 4);
            //4 bytes in one word
            break;
        case WRITE:
            switch (memory_sync_policy) {
                   case WRITE_BACK:
               
                    memcpy(cache[index].block[block].data+offset,data, 4);
                              lruarr.arr[index][block].addr=addr;
                               cache[index].block[block].dirty = DIRTY;
 
            
                    break;
                       
                       case WRITE_THROUGH:
                    memcpy(cache[index].block[block].data+offset,data, 4);
                    accessDRAM(addr, cache[index].block[block].data, transfer_unit, WRITE);
                       break;
                       
                   default:
                       break;
               }
            break;
        default:
            break;
    }
}

void lrupolicy(address addr, word* data, WriteEnable we, int index, int tag, int offset){
       transfer_unit = uint_log2(block_size);
    int block = findlru(index, tag, offset);
    highlight_block(index, block);
    highlight_offset(index, block, offset, MISS);

    
    switch (memory_sync_policy) {
        case WRITE_BACK:
            write_back(addr,data,we,index,block,tag,offset);
            break;
            
            case WRITE_THROUGH:
            write_through(addr,data,we,index,block,tag,offset);
            break;
            
        default:
            break;
    }
    
    
    lrualgorithm(addr,data, we, block, index, tag, offset);
}



void randompolicy(address addr, word* data, WriteEnable we, int index, int tag, int offset){
    int block = randomint(assoc);
    transfer_unit = uint_log2(block_size);
    highlight_block(index, block);
    highlight_offset(index, block, offset, MISS);

    switch (memory_sync_policy) {
         case WRITE_BACK:
             write_back(addr,data,we,index,block,tag,offset);
             break;
             
             case WRITE_THROUGH:
             write_through(addr,data,we,index,block,tag,offset);
             break;
             
         default:
             break;
     }
}

    

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/

void accessMemory(address addr, word* data, WriteEnable we){
  /* Declare variables here */
    unsigned int offset_bit = uint_log2(block_size);
    unsigned int index_bit =uint_log2(set_count);
  //  unsigned int tag_bit = 32 - (offset_bit+ index_bit);
    unsigned int offset_value,shift;
    unsigned int index_value;
    unsigned int tag_value;
  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */

  /* Start adding code here */
     unsigned int alteraddr = addr;
   // unsigned int not_offset =~ offset_bit;
    //offset_bit = offset_bit|not_offset;
    //offset_value= addr & offset_bit;
    
// get offset vcalue
    shift = 32 - offset_bit;
    offset_value = addr<<shift;
    offset_value = offset_value>>shift;
    // shit addr to get new addr
    alteraddr = alteraddr>>offset_bit;
    //get index value
     shift = shift+offset_bit-index_bit;
    index_value = alteraddr<<shift;
    index_value =index_value>>shift;
    // alter address to give us tag
    alteraddr = alteraddr>>index_bit;
    tag_value = alteraddr;
    // we didnt need tag bit


//initial test shows that we are getting the correct value of tag

  //  if(lruarr.arr[0][0].run==false){
        
    //for(int j = 0; j<set_count;j++){
      //for(int i = 0; i<assoc;i++){
        //  lruarr.arr[j][i].addr = i;
      //}
      //}
       // lruarr.arr[0][0].run=true;
   // }
 
    hitormiss(index_value, tag_value, offset_value);
    switch (horm) {
        case MISS:
            switch (policy){
                case LRU:
            lrupolicy(addr, data, we, index_value, tag_value, offset_value);
                    break;
                    
                case LFU:
                    //removed from project
                    break;
                case RANDOM:
                    randompolicy(addr, data, we, index_value, tag_value, offset_value);
                    break;
                    
            default:
            break;
            }
            break;
        case HIT:
            hitalgo (addr,data,we, lruarr.block,index_value, tag_value, offset_value);
            break;
        default:
            break;
    }

  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
//  accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}

