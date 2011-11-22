// File: CrassXML.h
// Original Author: Michael Imelfort 2011
// --------------------------------------------------------------------
//
// OVERVIEW:
// 
// Header file for the crass XML reader writer
//
// Many thanks to http://www.yolinux.com/TUTORIALS/XML-Xerces-C.html
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

#ifndef CrassXML_h
    #define CrassXML_h

// system includes
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>


#include <string>
#include <stdexcept>
#include <set>
#include <list>

#if defined(XERCES_NEW_IOSTREAMS)
#include <iostream>
#else
#include <iostream.h>
#endif
// local includes
#include "crassDefines.h"

// Error codes
enum {
   ERROR_ARGS = 1,
   ERROR_XERCES_INIT,
   ERROR_PARSE,
   ERROR_EMPTY_DOCUMENT
};

// ---------------------------------------------------------------------------
//  This is a simple class that lets us do easy (though not terribly efficient)
//  trancoding of char* data to XMLCh data.
// ---------------------------------------------------------------------------
class XStr
{
    public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    XStr(const char* const toTranscode)
    {
        // Call the private transcoding method
        fUnicodeForm = xercesc::XMLString::transcode(toTranscode);
    }
    XStr(const std::string& toTranscode)
    {
        // Call the private transcoding method
        fUnicodeForm = xercesc::XMLString::transcode(toTranscode.c_str());
    }
    ~XStr()
    {
        xercesc::XMLString::release(&fUnicodeForm);
    }
    
    
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    const XMLCh* unicodeForm() const
    {
        return fUnicodeForm;
    }
    
    private :
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fUnicodeForm
    //      This is the Unicode XMLCh format of the string.
    // -----------------------------------------------------------------------
    XMLCh*   fUnicodeForm;
};


class CrassXML 
{
    public:
        //constructor / destructor
        CrassXML(void);
        ~CrassXML(void);
        
        //
        // Generic get and set
        //
        
        //
        // Working functions
        //
        void parseCrassXMLFile(std::string XMLFile);
        void parseCrassXMLFile(std::string XMLFile, std::string& wantedGroup, std::string * directRepeat, std::set<std::string>& wantedContigs, std::list<std::string>& spacersForAssembly);
        //std::string XMLCH_2_STR(const XMLCh* xmlch);
        
        inline char * XMLCH_2_STR( const XMLCh* toTranscode ) 
        {  
            return xercesc::XMLString::transcode(toTranscode); 
        }
        
        inline XMLCh * STR_2_XMLCH( const std::string& toTranscode ) 
        {  
            return xercesc::XMLString::transcode(toTranscode.c_str()); 
        }
        xercesc::DOMElement * getWantedGroupFromRoot(xercesc::DOMElement * currentElement, std::string& wantedGroup, std::string * directRepeat);
        xercesc::DOMElement * parseGroupForAssembly(xercesc::DOMElement* currentElement);
        void parseAssemblyForContigIds(xercesc::DOMElement* currentElement, std::set<std::string>& wantedContigs, std::list<std::string>& spacersForAssembly);
        void getSpacerIdForAssembly(xercesc::DOMElement* currentElement, std::list<std::string>& spacersForAssembly);
        
        //DOMDocument Creation
        xercesc::DOMElement * createDOMDocument(std::string& rootElement, std::string& versionNumber, int& errorNumber );
        
        // add a <metadata> tag to <group> with notes
        xercesc::DOMElement * addMetaData(std::string& notes, xercesc::DOMElement * parentNode);
        
        // add a <group> to <crass_assem>
        xercesc::DOMElement * addGroup(std::string& gID, std::string& drConsensus, xercesc::DOMElement * parentNode);
        
        // add the <data> to <group>
        xercesc::DOMElement * addData(xercesc::DOMElement * parentNode);
        
        // add the <assembly> to <group>
        xercesc::DOMElement * addAssembly(xercesc::DOMElement * parentNode);
        
        // add a direct repeat to the <data> (<dr>)
        void addDirectRepeat(std::string& drid, std::string& seq, xercesc::DOMElement * dataNode);
        
        // add a spcaer to the <data> (<spacer>)
        void addSpacer(std::string& seq, std::string& spid, xercesc::DOMElement * dataNode);
        
        // create a <flankers> tag in <data>
        xercesc::DOMElement * createFlankers(xercesc::DOMElement * parentNode);
        
        // add a <flanker> to the <data> section (<flanker>)
        void addFlanker(std::string& seq, std::string& flid, xercesc::DOMElement * dataNode);
        
        // add a <contig> to an <assembly>
        xercesc::DOMElement * addContig(std::string& cid, xercesc::DOMElement * parentNode);
        
        // add the concensus sequence to the contig (creates <concensus>)
        void createConsensus(std::string& concensus, xercesc::DOMElement * parentNode);
        
        // add a spacer to a contig (<cspacer> tag)
        xercesc::DOMElement * addSpacerToContig(std::string& spid, xercesc::DOMElement * parentNode);
        
        // use to create a backward spacers (<bspacers>) or forward spacers (<fspacers>) element by changing the value of tag
        xercesc::DOMElement * createSpacers(std::string& tag, xercesc::DOMElement * parentNode);
        
        // use to create a backward flankers (<bflankers>) or forward flankers (<fflankers>) element by changing the value of tag
        xercesc::DOMElement * createFlankers(std::string& tag, xercesc::DOMElement * parentNode);

        // use to add either a backward spacer (<bs>) or forward spacer (<fs>) by changing the value of tag
        void addSpacer(std::string& tag, std::string& spid, std::string& drid, std::string& drconf, xercesc::DOMElement * parentNode);
        
        // use to add either a backward flanker (<bf>) or forward flanker (<ff>) by changing the value of tag
        void addFlanker(std::string& tag, std::string& flid, std::string& drconf, std::string& directjoin, xercesc::DOMElement * parentNode);
        
    
        //
        // File IO / printing
        //

    
    private:
        xercesc::XercesDOMParser * mConfigFileParser;			// parsing object
        xercesc::DOMDocument * CX_DocElem;
        // grep ATTLIST crass.dtd | sed -e "s%[^ ]* [^ ]* \([^ ]*\) .*%XMLCh\* ATTR_\1;%" | sort | uniq
        // grep ELEMENT crass.dtd | sed -e "s%[^ ]* \([^ ]*\) .*%XMLCh\* TAG_\1;%" | sort | uniq
        XMLCh* ATTR_cid;
        XMLCh* ATTR_confcnt;
        XMLCh* ATTR_directjoin;
        XMLCh* ATTR_drconf;
        XMLCh* ATTR_drid;
        XMLCh* ATTR_drseq;
        XMLCh* ATTR_flid;
        XMLCh* ATTR_gid;
        XMLCh* ATTR_seq;
        XMLCh* ATTR_spid;
        XMLCh* ATTR_cov;
        XMLCh* ATTR_totcnt;
        XMLCh* ATTR_type;
        XMLCh* ATTR_url;
        XMLCh* ATTR_version;
        
        XMLCh* TAG_assembly;
        XMLCh* TAG_bf;
        XMLCh* TAG_bflankers;
        XMLCh* TAG_bs;
        XMLCh* TAG_bspacers;
        XMLCh* TAG_consensus;
        XMLCh* TAG_contig;
        XMLCh* TAG_crass_assem;
        XMLCh* TAG_cspacer;
        XMLCh* TAG_data;
        XMLCh* TAG_dr;
        XMLCh* TAG_drs;
        XMLCh* TAG_ff;
        XMLCh* TAG_fflankers;
        XMLCh* TAG_file;
        XMLCh* TAG_flanker;
        XMLCh* TAG_flankers;
        XMLCh* TAG_fs;
        XMLCh* TAG_fspacers;
        XMLCh* TAG_group;
        XMLCh* TAG_log;
        XMLCh* TAG_metadata;
        XMLCh* TAG_notes;
        XMLCh* TAG_spacer;
        XMLCh* TAG_spacers;

};

#endif //CrassXML_h
