/* ................................................................. /
*	NAME : cacheSim
* 
*	SYNPOSIS : cacheSim [-s SIZE] [-a ASSOCIATIVITY]
*						[-l CACHE_LINE_SIZE] [-f FILEPATH]
*
*	DESCRIPTION : This program simulates a cache and returns the hit rate for a 
*				  given set of addresses.
*
*	ARGUMENTS : -s [Size]  
*					Size of the cache in bytes.
*				-a [ASSOCIATIVITY]
*					How many cache lines are in a set.
*				-l [CACHELINE]
*					Size of individual cache line in bytes.
*				-f [FILEPATH]
*					Path of file containing Addresses.
/ ................................................................. */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

/*
*	STRUCT : CacheLine
*
*	FIELDS : loadedAddress - The address in this particular cache line.
*			 lastUpdate - A number keeping track of the last time this line was updated.
*/

typedef struct CacheLine {
	unsigned int loadedAddress ;
	int lastUpdate ;
} CacheLine ;

/*
*	STRUCT : CacheSim
*
*	FIELDS : cache - Array containing the sets of cachelines (Dimensions numSet x associativity)
*			 cacheLines - Array containing the cache lines.
*			 lineSize - The size of a cache line in bytes.
*			 numLines - Number of cache lines.
*			 assoc - The associativity of the cache.
*			 numBitsOff - The number of bits dedicated to the offset.
*			 numBitsSet - The number of bits dedicated to determining the set.
*			 numBitsTag - The number of bits dedicated to determining the tag.
*			 setCompar - A number with all bits set as one with which one can & with the address 
*					and extract the set number.
*			 offCompar - A number with all bits set as one with which one can & with the address 
*					and extract the offset number.
*			 tagCompar - A number with all bits set as one with which one can & with the address 
*					and extract the tag number.
*/

typedef struct CacheSim {
	CacheLine ** cache ;
	CacheLine * cacheLines ;
	int lineSize ;
	int	numLines ;
	int numSets ;
	int assoc ;
	int numBitsOff ;
	int numBitsSet ;
	int numBitsTag ;
	int setCompar ;
	int offCompar ;
	int tagCompar ;
} CacheSim ;

/*
*	STRUCT : Addresses
*
*	FIELDS : size - The amount of addresses stored in adAr.
*			 adAR - Array storing the decimal addresses.
*			 hexAr - Array storing hex addresses.
*/

typedef struct Addresses {
	int size ;
	unsigned int * adAr ;
	char ** hexAr ;
} Addresses ;

CacheSim * makeCacheSim(int size, int linSize, int ass) ;
void freeCacheSim(CacheSim * cache) ;
void freeAddressStruct(Addresses * ad) ;
inline int getCacheSet(unsigned int address, CacheSim * test) ;
inline int getBitOffset(unsigned int address, CacheSim * test) ;
inline int getTag(unsigned int address, CacheSim * test) ;
Addresses * initAddressStruct(const char * filepath) ;
int getHighestBit(int num) ;
bool retrieveAddress(unsigned int address, CacheSim * test, int loadNum) ;

int main(int numArgs, char * args[]) {

	int i ;
	int cacheSize = 0 ;
	int lineSize = 0 ;
	int associativity = 0 ;
	char * filepath ;

	for (i = 0 ; i < numArgs ; ++i) {
		if(strcmp(args[i],"-s") == 0) {
			cacheSize = atoi(args[i+1]) ;
		}
		if(strcmp(args[i],"-a") == 0) {
			associativity = atoi(args[i+1]) ;
		}
		if(strcmp(args[i],"-l") == 0) {
			lineSize = atoi(args[i+1]) ;
		}
		if(strcmp(args[i],"-f") == 0) {
			filepath = args[i+1] ;
		}
	}

	Addresses * addresses = initAddressStruct(filepath) ;
	CacheSim * testCache = makeCacheSim(cacheSize,lineSize,associativity) ;

	// Loads each address up to cache sequentially and incements numHits if it's a hit. //
	int numHits = 0 ;
	for (i = 0 ; i < addresses->size ; ++i) {
		bool state = retrieveAddress(addresses->adAr[i], testCache, i) ;
		if (state) {
			printf("%s,%d,HIT\n" , addresses->hexAr[i], getCacheSet(addresses->adAr[i],testCache));
			++numHits ;
		} else {
			printf("%s,%d,MISS\n" , addresses->hexAr[i], getCacheSet(addresses->adAr[i],testCache));
		}
	}

	printf("NUM HITS %d\n", numHits) ;
	printf("NUM MISSES %d\n", addresses->size - numHits) ;
	printf("HIT RATE %f\n",(double) numHits/addresses->size) ;

	freeCacheSim(testCache) ;
	freeAddressStruct(addresses) ;
	return 0 ;
}

/*
*	FUNCTION : mackeCacheSim(int size, int linSize, int ass)
*
*	ARGUMENTS : size - The size of the cache in bytes.
*				linSize - The size of a cache line in bytes.
*				ass - The associativty of the cache.
*
*	PURPOSE : Intialises a virtual cache of with size amount of bytes divided into
*			  (size/lineSize)/ass number of sets consisting of cache lines of size
*			  linSize.
*/

CacheSim * makeCacheSim(int size, int linSize, int ass) {
	CacheSim * newCacheSim  = malloc(sizeof(CacheSim)) ;
	newCacheSim->numLines = size/linSize ;
	newCacheSim->lineSize = linSize ;
	newCacheSim->numSets = newCacheSim->numLines/ass ;
	newCacheSim->assoc = ass ;

	// Calculates the amount of bits needed to express the offset,sets and tag. //
	newCacheSim->numBitsOff = getHighestBit(linSize-1) ;
	newCacheSim->numBitsSet = getHighestBit((newCacheSim->numLines / ass)-1) ;
	newCacheSim->numBitsTag = 16 - newCacheSim->numBitsOff - newCacheSim->numBitsSet ;
	newCacheSim->cache = malloc(sizeof(CacheLine *)*(newCacheSim->numSets)) ;
	newCacheSim->cacheLines = malloc(sizeof(CacheLine)*newCacheSim->numLines) ;
	
	int i,j ;
	for (i = 0 ; i < newCacheSim->numSets ; ++i) {
		newCacheSim->cache[i] = &newCacheSim->cacheLines[i*ass] ;
	}
	// Intialises all the addresses to some arbitray address in this case 999999999 //
	for (i = 0 ; i < newCacheSim->numSets ; ++i){
		for (j = 0 ; j < newCacheSim->assoc ; ++j) {
			newCacheSim->cache[i][j].loadedAddress = 999999999 ;
			newCacheSim->cache[i][j].lastUpdate = 0 ;
		}
	}
	
	/*
	*	Generates a number of the form sum_{i=0}^{numBitsSet} 2^i. This number
	*	can be & with a shifted address in order to extract the set number. A
	*	similar process is performed for tag and offset.
	*/

	int compar = 0 ;
	for (i = 0 ; i < newCacheSim->numBitsSet ; ++i) {
		compar += pow(2,i) ;
	}
	newCacheSim->setCompar = compar ;
	
	compar = 0 ;
	for (i = 0 ; i < newCacheSim->numBitsOff ; ++i) {
		compar += pow(2,i) ;
	}
	newCacheSim->offCompar = compar ;
	
	compar = 0 ;
	for (i = 0 ; i < newCacheSim->numBitsTag  ; ++i) {
		compar += pow(2,16-i-1) ;
	}
	newCacheSim->tagCompar = compar ;
	return newCacheSim ;
}

/*
*	FUNCTION : getCacheSet(int address, CacheSim * test)
*
*	ARGUMENTS : address - The address to extract the information from.
*				test - The virtual cache we're determining this for.
*
*	PURPOSE : Shifts the address so that the bits determining the set are the first bits
*			  and then &s them with a comparing number of the form (000...111). The resulting
*			  number is the set of the address.
*/

inline int getCacheSet(unsigned int address, CacheSim * test) {
	return (address >> test->numBitsOff) & test->setCompar ;
}

/*
*	FUNCTION : getBitOffSet(int address, CacheSim * test)
*
*	ARGUMENTS : address - The address to extract the information from.
*				test - The virtual cache we're determining this for.
*
*	PURPOSE : &s the bits determining the offset with a comparing number of the form (111....). The resulting
*			  number is the offset of the address.
*/

inline int getBitOffset(unsigned int address, CacheSim * test) {
	return address & test->offCompar ;
}

/*
*	FUNCTION : getTag(int address, CacheSim * test)
*
*	ARGUMENTS : address - The address to extract the information from.
*				test - The virtual cache we're determining this for.
*
*	PURPOSE : &s the bits determining the tag with a comparing number of the form (1111...0000). The resulting
*			  number is the tag of the address.
*/

inline int getTag(unsigned int address, CacheSim * test) {
	return address & test->tagCompar ;
}

void freeCacheSim(CacheSim * sim) {
	free(sim->cacheLines) ;
	free(sim->cache) ;
	free(sim) ;
}

void freeAddressStruct(Addresses * ad) {
	free(ad->adAr) ;
	int i ;
	for (i = 0 ; i < ad->size ; ++i) {
		free(ad->hexAr[i]) ;
	}
	free(ad->hexAr) ;
	free(ad) ;
}

/*
*	FUNCTION : initAddressStruct(const char * filepath)
*
*	ARGUMENTS : filepath - Filepath of file containing addresses.
*
*	PURPOSE : Converts file containing hexadecimal addresses stored in each line to
*			  an array of integer addresses. Returns this struct.
*/

Addresses * initAddressStruct(const char * filepath) {
	Addresses * ad = malloc(sizeof(Addresses)) ;
	FILE * input ;
	input = fopen(filepath, "r") ;

	if (input == NULL) {
		fprintf(stderr,"Specified file does not exist\n") ;
		exit(-1) ;
	}

	// Finds the amount of lines in the files. //
	fseek(input, 0L, SEEK_END) ;
	int sz = ftell(input) ;
	int numAddresses = sz/5 ;
	// All dynamically allocated memory is freed using freeAdressStruct. //
	ad->adAr = malloc(sizeof(unsigned int)*numAddresses) ;
	ad->hexAr = malloc(sizeof(char*)*numAddresses) ;
	ad->size = numAddresses ;
	
	// Returns to start of file. //
	fseek(input, 0L, SEEK_SET) ;
	char buffer[5] ;

	// Retrieves the hex codes and stores the raw hex and decimal forms in Addresses struct. //
	int i ;
	for (i = 0 ; i < numAddresses ; ++i) {
		fscanf(input, "%s\n", buffer) ;
		ad->adAr[i] = strtol(buffer,NULL,16) ;
		ad->hexAr[i] = malloc(sizeof(char)*4) ;
		strcpy(ad->hexAr[i],buffer) ;
	}
	
	fclose(input) ;
	return ad ;
}

/*
*	FUNCTION : getHighestBit(int num)
*
*	ARGUMENTS  : num - The number you're trying to find the highest bit for.
*
*	PURPOSE : Returns the position of the highest non-zero bit in a number.
*/

int getHighestBit(int num) {
	if(num == 0){
		return 0 ;
	}
	int i = 1 ;
	while(num>>=1 != 0) {
		++i ;
	}
	return i ;
}

/*
*	FUNCTION : retrieveAddress(int address, CacheSim * test, int loadNum)
*
*	ARGUMENTS : address - The address you want to load.
*				test - The virtual cache.
*				loadNum - A number used to give the loaded address a relative timestamp.
*
*	PURPOSE : Attempts to retrieve data from the virtual cache at the passed address. Calculates
*			  the set of the address and checks these cache lines for a matching tag. If the tag is
*			  found then there is a cache hit. If the tag isn't found then the address in the oldest 
*			  cache line is replaced with the address.
*/

bool retrieveAddress(unsigned int address, CacheSim * test, int loadNum) {
	int set = getCacheSet(address, test) ;
	int i ;

	// Gets the tags of the cachelines in the set and compares with address tag. //
	int adrTag = getTag(address, test) ;
	for (i = 0 ; i < test->assoc ; ++i) {
		unsigned int prevAddress = test->cache[set][i].loadedAddress ;
		int cacheTag = getTag(prevAddress, test) ;
		if (cacheTag == adrTag) {
			// The cache was used so its last used time is updated. //
			test->cache[set][i].lastUpdate = loadNum ;
			return true ;
		}
	}

	// If it fails to match the tag, finds and replaces oldest entry with new address. //
	int indexOldestEntry = 0 ;
	int oldestUpdate = test->cache[set][0].lastUpdate ;
	for (i = 1 ; i < test->assoc ; ++i) {
		if (oldestUpdate > test->cache[set][i].lastUpdate) {
			oldestUpdate = test->cache[set][i].lastUpdate ;
			indexOldestEntry = i ;
		}
	}
	test->cache[set][indexOldestEntry].loadedAddress = address ;
	test->cache[set][indexOldestEntry].lastUpdate = loadNum ;
	return false ;
}
