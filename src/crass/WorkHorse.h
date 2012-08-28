// File: WorkHorse.h
// Original Author: Michael Imelfort 2011
// --------------------------------------------------------------------
//
// OVERVIEW:
//
// This class is responsible for "running" the algorithm
// 
// --------------------------------------------------------------------
//  Copyright  2011 Michael Imelfort and Connor Skennerton
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
// --------------------------------------------------------------------
//
//                        A
//                       A B
//                      A B R
//                     A B R A
//                    A B R A C
//                   A B R A C A
//                  A B R A C A D
//                 A B R A C A D A
//                A B R A C A D A B 
//               A B R A C A D A B R  
//              A B R A C A D A B R A 
//

#ifndef WorkHorse_h
    #define WorkHorse_h

// system includes
#include <iostream>
#include <string>
#include <vector>
#include <map>

// local includes
#include "crassDefines.h"
#include "libcrispr.h"
#include "NodeManager.h"
#include "ReadHolder.h"
#include "StringCheck.h"
#include <libcrispr/writer.h>
#if SEARCH_SINGLETON
#include "SearchChecker.h"
#endif

// typedefs
typedef std::map<std::string, NodeManager *> DR_List;
typedef std::map<std::string, NodeManager *>::iterator DR_ListIterator;

// for storing clusters of DRs
// indexed using StringCheck type tokens
typedef std::vector<StringToken> DR_Cluster; 
typedef std::vector<StringToken>::iterator DR_ClusterIterator;

typedef std::map<int, DR_Cluster *>::iterator DR_Cluster_MapIterator;
typedef std::map<int, DR_Cluster *> DR_Cluster_Map;

typedef std::map<int, std::map<std::string, int> * > GroupKmerMap;

typedef std::vector<std::string> Vecstr;

bool sortLengthAssending( const std::string &a, const std::string &b);
bool sortLengthDecending( const std::string &a, const std::string &b);
bool includeSubstring(const std::string& a, const std::string& b);
bool isNotEmpty(const std::string& a);

class WorkHorse {
    public:
    WorkHorse (options * opts, std::string timestamp, std::string commandLine) 
        { 
            mOpts = opts; 
            mMaxReadLength = 0;
            mStringCheck.setName("WH");
            mTimeStamp = timestamp;
            mCommandLine = commandLine;
        }
        ~WorkHorse();
        
        // do all the work!
        int doWork(Vecstr seqFiles);

        //**************************************
        // file IO
        //**************************************
        //void write_spacerID(direct_repeat &dr_match, kseq_t *seq);
        //void write_direct_repeatID(direct_repeat &dr_match, kseq_t *seq);
        //void writeLookupToFile(string &outFileName, lookupTable &outLookup);
        int numOfReads(void);
        
    private:
        
        void clearReadList(ReadList * tmp_list);
        void clearReadMap(ReadMap * tmp_map);
        
        //**************************************
        // functions used to cluster DRs into groups and identify the "true" DR
        //**************************************
        int parseSeqFiles(Vecstr seqFiles);	// parse the raw read files
        
        int buildGraph(void);									// build the basic graph structue
        
        int cleanGraph(void);									// clean the graph structue

        void removeRedundantRepeats(Vecstr& repeatVector);
        
        Vecstr * createNonRedundantSet(GroupKmerMap& groupKmerCountsMap, 
                                                         int& nextFreeGID);

        int removeLowConfidenceNodeManagers(void);
        
        int findConsensusDRs(GroupKmerMap& groupKmerCountsMap, 
                             int& nextFreeGID);
    
        bool clusterDRReads(StringToken DRToken, 
                int * nextFreeGID, 
                std::map<std::string, int> * k2GIDMap, 
                GroupKmerMap * groupKmerCountsMap);  // cut kmers and hash
        
        bool findMasterDR(int GID, 
                Vecstr * nTopKmers, 
                StringToken * masterDRToken, 
                std::string * masterDRSequence);
        
        bool populateCoverageArray(int numMers4Mode, 
                int GID, 
                std::string master_DR_sequence, 
                StringToken master_DR_token, 
                std::map<StringToken, int> * DR_offset_map, 
                int * dr_zone_start, 
                int * dr_zone_end, 
                Vecstr * nTopKmers, 
                int ** coverage_array, 
                int * kmer_positions_DR, 
                bool * kmer_rcs_DR, 
                int * kmer_positions_DR_master, 
                bool * kmer_rcs_DR_master, 
                int * kmer_positions_ARRAY);
        
        std::string calculateDRConsensus(int GID, 
                std::map<StringToken, int> * DR_offset_map, 
                int * collapsed_pos, 
                std::map<char, int> * collapsed_options, 
                std::map<int, bool> * refined_DR_ends, 
                int * dr_zone_start, 
                int * dr_zone_end, 
                int ** coverage_array, 
                char * consensus_array, 
                float * conservation_array, 
                int * nextFreeGID);
        
        bool parseGroupedDRs(int numMers4Mode, 
                int GID, 
                Vecstr * nTopKmers, 
                int * nextFreeGID);
        
        int numberOfReadsInGroup(DR_Cluster * currentGroup);
        
        bool isKmerPresent(bool * didRevComp, 
                int * startPosition, 
                const std::string kmer, 
                const std::string * DR);
        
        int getNMostAbundantKmers(Vecstr& mostAbundantKmers, 
                int num2Get, 
                std::map<std::string, int> * kmer_CountMap);

        int getNMostAbundantKmers(int maxAmount, 
                Vecstr& mostAbundantKmers, 
                int num2Get, 
                std::map<std::string, int> * kmer_CountMap);

        int getOffsetAgainstMaster(std::string& masterDR, std::string& slaveDR);
        
        void removeDRAndCleanMemory(int ** coverageArray, 
                char * consensusArray, 
                float * conservationArray, 
                int GID);
        
        void cleanGroup(int GID);
        
        void cleanArrays(int ** coverageArray, char * consensusArray, float * conservationArray);
        //**************************************
        // spacer graphs
        //**************************************
        int makeSpacerGraphs(void);
        int cleanSpacerGraphs(void);
        int generateFlankers(void);
        //**************************************
        // contig making
        //**************************************
        int splitIntoContigs(void);
        
        //**************************************
        // file IO
        //**************************************
        int dumpReads( bool split);		// dump the reads for this group to file
        //int dumpSpacers(void);										// Dump the spacers for this group to file
        
        int renderDebugGraphs(void);							// render debug graphs
        
        int renderDebugGraphs(std::string namePrefix);
        
        int renderSpacerGraphs(void);							// render debug graphs
        
        int renderSpacerGraphs(std::string namePrefix);
        
        bool checkFileOrError(const char * fileName);
    
        bool printXML(void) { return printXML(mOpts->output_fastq + "crass"); } // print all the assembly gossip to XML
        
        bool printXML(std::string namePrefix);

        bool addDataToDOM(crispr::xml::writer * xmlDoc, xercesc::DOMElement * groupElement, int groupNumber);
        
        bool addMetadataToDOM(crispr::xml::writer * xmlDoc, xercesc::DOMElement * groupElement, int groupNumber);

        
    // members
        DR_List mDRs;                               // list of nodemanagers, cannonical DRs, one nodemanager per direct repeat
        ReadMap mReads;                             // reads containing possible double DRs
        options * mOpts;                      // search options
        std::string mOutFileDir;                    // where to spew text to
        int mMaxReadLength;                       // the average seen read length
        StringCheck mStringCheck;                   // Place to swap strings for tokens
        std::string mTimeStamp;						// hold the timestmp so we can make filenames
        std::string mCommandLine;                   // holds the exact command line string for logging purposes
        // global variables used to cluster and munge DRs
        std::map<int, bool> mGroupMap;				// list of valid group IDs
        DR_Cluster_Map mDR2GIDMap;					// map a DR (StringToken) to a GID
        std::map<int, std::string> mTrueDRs;		// map GId to true DR strings
};

#endif //WorkHorse_h
