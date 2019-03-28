#ifndef OGRAPHBIN
#define OGRAPHBIN

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <cstdlib>
#include "binSeq.h"
#include "ograph.h" // just for kmerIndice




class graph4{
	public:
		uint k,indiceUnitigs,nbElement,minimizer,minsize;
		binSeq* unitigs;
		std::vector<kmerIndice> left;
		std::vector<kmerIndice> right;
		void addvertex(string& str);
        void addtuple(tuple<binSeq,uint,uint>& tuple);
		void addleftmin(unsigned int mini);
		void addrightmin(unsigned int mini);
		void debruijn();
        void debruijn2();
        void compaction2(uint iL, uint iR);
		void compress();
        kmer rcb(kmer min);
		void compaction(uint iR, uint iL);
		uint size();
        bool output(uint i);
        void clear();

		graph4(uint ka, uint min,uint size, uint nb){
            indiceUnitigs=0;
			minsize=size;
			k=ka;
			minimizer=min;
            nbElement=nb;
            unitigs=new binSeq [nbElement];
            left.reserve(nbElement);
    	    right.reserve(nbElement);
		}
};


void compareUnitigs(const std::string& fileFa,const std::string& fileDot);
void compareKmers(const std::string& fileFa,const std::string& fileDot);



#endif
