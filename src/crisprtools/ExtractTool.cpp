// ExtractTool.cpp
//
// Copyright (C) 2011, 2012 - Connor Skennerton
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
#include "ExtractTool.h"
#include "Utils.h"
#include "config.h"
#include "Exception.h"

#include "StlExt.h"
#include <getopt.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>


ExtractTool::ExtractTool (void)
{
    // set the 'print coverage' bit by defult
    ET_BitMask.set(6);
    
    ET_OutputPrefix = "./";
    ET_OutputNamePrefix = "";
    ET_OutputHeaderPrefix = "";
}

ExtractTool::~ExtractTool (void)
{}

void ExtractTool::generateGroupsFromString ( std::string str)
{
	std::set<std::string> vec;
	split ( str, vec, ",");
	ET_Group = vec;
	// set the 'do subset' bit
    ET_BitMask.set(0);
}

int ExtractTool::processOptions (int argc, char ** argv)
{
	int c;
    int index;
    static struct option long_options [] = {       
        {"help", no_argument, NULL, 'h'}
    };
	while((c = getopt_long(argc, argv, "hH:g:Csdfxyo:O:", long_options, &index)) != -1)
	{
        switch(c)
		{
			case 'C':
            {
                ET_BitMask.reset(6);
                break;
            }
            case 'h':
			{
				extractUsage ();
				exit(1);
				break;
			}
            case 'H':
            {
                ET_OutputHeaderPrefix = optarg;
                break;
            }
			case 'g':
			{
				generateGroupsFromString (optarg);
				break;
			}
			case 's':
			{
				//ET_Spacer = true;
                ET_BitMask.set(4);
                break;
			}
			case 'd':
			{
				//ET_DirectRepeat = true;
                ET_BitMask.set(5);
                break;
			}
			case 'f':
			{
				//ET_Flanker = true;
                ET_BitMask.set(3);
                break;
			}
			case 'x':
			{
				//ET_SplitGroup = true;
                ET_BitMask.set(2);
                break;
			}
			case 'y':
			{
				//ET_SplitType = true;
                ET_BitMask.set(1);
                break;
			}
            case 'o':
            {
                ET_OutputPrefix = optarg;
                // just in case the user put '.' or '..' or '~' as the output directory
                if (ET_OutputPrefix[ET_OutputPrefix.length() - 1] != '/')
                {
                    ET_OutputPrefix += '/';
                }
                
                // check if our output folder exists
                struct stat file_stats;
                if (0 != stat(ET_OutputPrefix.c_str(),&file_stats)) 
                {
                    recursiveMkdir(ET_OutputPrefix);
                }
                break;
            }
            case 'O':
            {
                ET_OutputNamePrefix = optarg;
                break;
            }
            default:
            {
                extractUsage();
                exit(1);
                break;
            }
		}
	}
    if (!(ET_BitMask[3] | ET_BitMask[4] | ET_BitMask[5])) {
        throw crispr::input_exception("Please specify at least one of -s -d -f");
    }
	return optind;
}

int ExtractTool::processInputFile(const char * inputFile)
{
    // open the file
    crispr::XML xml_obj;
    try {
        xercesc::DOMDocument * xml_doc = xml_obj.setFileParser(inputFile);
        
        xercesc::DOMElement * root_elem = xml_doc->getDocumentElement();
        
        if( !root_elem ) throw(crispr::xml_exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, "empty XML document" ));
        
        // neither -x or -y
        if ((!ET_BitMask[2]) && (!ET_BitMask[1])) {
            // just open a single file handle
            ET_OneStream.open((ET_OutputPrefix + ET_OutputNamePrefix + "extracted_data.fa").c_str());
        
        } else if(!ET_BitMask[2]) {
            // -y but not -x
            // we are only spliting on type make three generic files
            if (ET_BitMask[5]) {
                ET_RepeatStream.open((ET_OutputPrefix + ET_OutputNamePrefix +"direct_repeats.fa").c_str());
            }
            if (ET_BitMask[4]) {
                ET_SpacerStream.open((ET_OutputPrefix + ET_OutputNamePrefix +"spacers.fa").c_str());
            }
            if (ET_BitMask[3]) {
                ET_FlankerStream.open((ET_OutputPrefix + ET_OutputNamePrefix + "flankers.fa").c_str());
            }
        }
        
        parseWantedGroups(xml_obj, root_elem);
        
    } catch( xercesc::XMLException& e ) {
        char* message = xercesc::XMLString::transcode( e.getMessage() );
        std::ostringstream errBuf;
        errBuf << "Error parsing file: " << message << std::flush;
        throw (crispr::xml_exception(__FILE__, __LINE__,__PRETTY_FUNCTION__,(errBuf.str()).c_str()));
        xercesc::XMLString::release( &message );
        
    } catch (crispr::xml_exception& xe) {
        std::cerr<< xe.what()<<std::endl;
        return 1;
    }
    if ((!ET_BitMask[2]) && (!ET_BitMask[1])) {
        // just open a single file handle
        ET_OneStream.close();
        
    } else if(!ET_BitMask[2]) {
        // we are only spliting on type make three generic files
        if (ET_BitMask[4]) {
            ET_SpacerStream.close();
        }
        if (ET_BitMask[3]) {
            ET_RepeatStream.close();
        }
        if (ET_BitMask[5]) {
            ET_FlankerStream.close();
        }
    }

    return 0;
}

void ExtractTool::parseWantedGroups(crispr::XML& xmlObj, xercesc::DOMElement * rootElement)
{
    
    try {
        int num_groups_to_process = static_cast<int>(ET_Group.size());
        for (xercesc::DOMElement * currentElement = rootElement->getFirstElementChild(); currentElement != NULL; currentElement = currentElement->getNextElementSibling()) {
            // break if we have processed all of the wanted groups
            if(ET_BitMask[0] && num_groups_to_process == 0) {
                break;
            }
            
            // is this a group element
            if (xercesc::XMLString::equals(currentElement->getTagName(), xmlObj.getGroup())) {
                // new group
                char * c_group_id = tc(currentElement->getAttribute(xmlObj.getGid()));
                std::string group_id = c_group_id;
                if (ET_BitMask[0]) {
                    
                    // we only want some of the groups look at ET_Groups
                    if (ET_Group.find(group_id.substr(1)) != ET_Group.end() ) {
                        if (ET_BitMask[2]) {
                            if (!ET_BitMask[1]) {
                                ET_GroupStream.open((ET_OutputPrefix +ET_OutputNamePrefix+ group_id + "_extracted_data.fa").c_str());
                            } else {
                                if (ET_BitMask[4]) {
                                    ET_SpacerStream.open((ET_OutputPrefix +ET_OutputNamePrefix+ group_id + "_spacers.fa").c_str());
                                }
                                if (ET_BitMask[5]) {
                                    ET_RepeatStream.open((ET_OutputPrefix +ET_OutputNamePrefix+ group_id + "_direct_repeats.fa").c_str());
                                }
                                if (ET_BitMask[3]) {
                                    ET_FlankerStream.open((ET_OutputPrefix +ET_OutputNamePrefix+ group_id + "_flankers.fa").c_str());
                                }
                            }
                        }
                        
                        // matches to one of our wanted groups
                        extractDataFromGroup(xmlObj, currentElement);
                        if(ET_BitMask[0]) num_groups_to_process--;
                    } 
                    xr(&c_group_id);
                } else {
                    if (ET_BitMask[2]) {
                        if (!ET_BitMask[1]) {
                            ET_GroupStream.open((ET_OutputPrefix +ET_OutputNamePrefix+ group_id + "_extracted_data.fa").c_str());
                        } else {
                            if (ET_BitMask[4]) {
                                ET_SpacerStream.open((ET_OutputPrefix +ET_OutputNamePrefix+ group_id + "_spacers.fa").c_str());
                            }
                            if (ET_BitMask[5]) {
                                ET_RepeatStream.open((ET_OutputPrefix +ET_OutputNamePrefix+ group_id + "_direct_repeats.fa").c_str());
                            }
                            if (ET_BitMask[3]) {
                                ET_FlankerStream.open((ET_OutputPrefix +ET_OutputNamePrefix+ group_id + "_flankers.fa").c_str());
                            }
                        }
                    }
                    extractDataFromGroup(xmlObj, currentElement);
                    if(ET_BitMask[0]) num_groups_to_process--;
                    
                    if(ET_BitMask[2]) {
                        // we are only spliting on type make three generic files
                        if (ET_BitMask[1]) {
                            if (ET_BitMask[4]) {
                                ET_SpacerStream.close();
                            }
                            if (ET_BitMask[5]) {
                                ET_RepeatStream.close();
                            }
                            if (ET_BitMask[3]) {
                                ET_FlankerStream.close();
                            }
                        } else {
                            ET_GroupStream.close();
                        }

                    }
                }
            }
            // reduse the number of groups to look for 
            // if the subset option is set
        }
    } catch( xercesc::XMLException& e ) {
        char* message = xercesc::XMLString::transcode( e.getMessage() );
        std::stringstream errBuf;
        errBuf << "Error parsing file: " << message << std::flush;
        throw (crispr::xml_exception(__FILE__, __LINE__,__PRETTY_FUNCTION__,(errBuf.str()).c_str()));
        xercesc::XMLString::release( &message );
    } catch (crispr::xml_exception& xe) {
        std::cerr<< xe.what()<<std::endl;
        return;
    }
}

void ExtractTool::extractDataFromGroup(crispr::XML& xmlDoc, xercesc::DOMElement * currentGroup)
{
    // get the first child - the data element
    try {
        // the data element is the first child of the group
        xercesc::DOMElement * dataElement = currentGroup->getFirstElementChild();
        // go through all the children of data
        for (xercesc::DOMElement * currentElement = dataElement->getFirstElementChild(); currentElement != NULL; currentElement = currentElement->getNextElementSibling()) {
            char * c_gid = tc(currentGroup->getAttribute(xmlDoc.getGid()));
            if (xercesc::XMLString::equals(currentElement->getTagName(), xmlDoc.getDrs())) {
                if (ET_BitMask[5]) {
                    // get direct repeats
                    if (ET_BitMask[1]) {
                        processData(xmlDoc, currentElement, REPEAT, c_gid, ET_RepeatStream);
                    } else if (ET_BitMask[2]) {
                        processData(xmlDoc, currentElement, REPEAT, c_gid, ET_GroupStream);   
                    } else {
                        processData(xmlDoc, currentElement, REPEAT, c_gid, ET_OneStream);   
                    }
                }
            } else if (xercesc::XMLString::equals(currentElement->getTagName(), xmlDoc.getSpacers())) {
                if (ET_BitMask[4]) {
                    // get spacers
                    if (ET_BitMask[1]) {
                        processData(xmlDoc, currentElement, SPACER, c_gid, ET_SpacerStream);
                    } else if (ET_BitMask[2]) {
                        processData(xmlDoc, currentElement, SPACER, c_gid, ET_GroupStream);   
                    } else {
                        processData(xmlDoc, currentElement, SPACER, c_gid, ET_OneStream);
                    }
                }
            } else if (xercesc::XMLString::equals(currentElement->getTagName(), xmlDoc.getFflankers())) {
                if (ET_BitMask[3]) {
                    // get flankers
                    if (ET_BitMask[1]) {
                        processData(xmlDoc, currentElement, FLANKER, c_gid, ET_FlankerStream);
                    } else if (ET_BitMask[2]) {
                        processData(xmlDoc, currentElement, FLANKER, c_gid, ET_GroupStream);
                    } else {
                        processData(xmlDoc, currentElement, FLANKER, c_gid, ET_OneStream);

                    }
                }
            }
            xr(&c_gid);
        }
    } catch( xercesc::XMLException& e ) {
        char* message = xercesc::XMLString::transcode( e.getMessage() );
        std::ostringstream errBuf;
        errBuf << "Error parsing file: " << message << std::flush;
        xercesc::XMLString::release( &message );
    } catch (crispr::xml_exception& xe) {
        std::cerr<< xe.what()<<std::endl;
        return;
    }
}

void ExtractTool::processData(crispr::XML& xmlDoc, xercesc::DOMElement * currentType, ELEMENT_TYPE wantedType, std::string gid, std::ostream& outStream)
{
    try {
        for (xercesc::DOMElement * currentElement = currentType->getFirstElementChild(); currentElement != NULL; currentElement = currentElement->getNextElementSibling()) {
            char * c_seq = tc(currentElement->getAttribute(xmlDoc.getSeq()));
            std::string id;
            switch (wantedType) {
                case REPEAT:
                {
                    char * c_id = tc(currentElement->getAttribute(xmlDoc.getDrid()));
                    id = c_id;
                    xr(&c_id);
                    break;
                }
                case SPACER:
                {
                    char * c_id = tc(currentElement->getAttribute(xmlDoc.getSpid()));
                    id = c_id;
                    if (ET_BitMask[6] && currentElement->hasAttribute(xmlDoc.getCov())) {
                        char * c_cov = tc(currentElement->getAttribute(xmlDoc.getCov()));
                        id += "_Cov_"; 
                        id += c_cov;
                        xr(&c_cov);
                    }
                    xr(&c_id);
                    break;
                }
                case FLANKER:
                {
                    char * c_id = tc(currentElement->getAttribute(xmlDoc.getFlid()));
                    id = c_id;
                    xr(&c_id);
                    break;
                }
                case CONSENSUS:
                {
                    break;
                }
                default:
                {
                    throw (crispr::runtime_exception(__FILE__, __LINE__, __PRETTY_FUNCTION__,"Input element enum unknown"));
                    break;
                }
            }
            outStream<<'>'<<ET_OutputHeaderPrefix<<gid<<id<<std::endl<<c_seq<<std::endl;
            xr(&c_seq);
        }
    } catch( xercesc::XMLException& e ) {
        char* message = xercesc::XMLString::transcode( e.getMessage() );
        std::stringstream errBuf;
        errBuf << "Error parsing file: " << message << std::flush;
        throw (crispr::xml_exception(__FILE__, __LINE__, __PRETTY_FUNCTION__, (errBuf.str()).c_str()));
        xercesc::XMLString::release( &message );
    } catch (crispr::xml_exception& xe) {
        std::cerr<< xe.what()<<std::endl;
        return;
    } catch (crispr::runtime_exception& re) {
        std::cerr<<re.what()<<std::endl;
        return;
    }
}

int extractMain (int argc, char ** argv)
{
    try {
		ExtractTool et;
		int opt_index = et.processOptions (argc, argv);
		if (opt_index >= argc) {
			throw crispr::input_exception("No input file provided" );
		
        } else {
			// get cracking and process that file
			return et.processInputFile(argv[opt_index]);
		}
	} catch(crispr::input_exception& re) {
        std::cerr<<re.what()<<std::endl;
        extractUsage();
        return 1;
    } catch(crispr::exception& ce ) {
		std::cerr<<ce.what()<<std::endl;
		return 1;
	}

}


void extractUsage (void)
{
	std::cout<<CRISPRTOOLS_PACKAGE_NAME<<" extract [-ghyxsdfCoO] file.crispr"<<std::endl;
	std::cout<<"Options:"<<std::endl;
	std::cout<<"-h					print this handy help message"<<std::endl;
    std::cout<<"-o DIR              output file directory  [default: .]" <<std::endl; 
    std::cout<<"-O STRING           Give a custom prefix to each of the outputed files [default: ""]"<<std::endl;
    std::cout<<"-g INT[,n]          a comma separated list of group IDs that you would like to extract data from."<<std::endl;
	std::cout<<"					Note that only the group number is needed, do not use prefixes like 'Group' or 'G', which"<<std::endl;
	std::cout<<"					are sometimes used in file names or in a .crispr file"<<std::endl;
	std::cout<<"-s                  Extract the spacers of the listed group"<<std::endl;
	std::cout<<"-d					Extract the direct repeats of the listed group"<<std::endl;
	std::cout<<"-f					Extract the flanking sequences of the listed group"<<std::endl;
    std::cout<<"-C                  Supress coverage information when printing spacers"<<std::endl;
    std::cout<<"-x					Split the results into different files for each group.  If multiple types are set i.e. -sd"<<std::endl;
	std::cout<<"					then both the spacers and direct repeats from each group will be in the one file"<<std::endl;
	std::cout<<"-y					Split the results into different files for each type of sequence from all selected groups."<<std::endl;
	std::cout<<"					Only has an effect if multiple types are set."<<std::endl;

}
				
				