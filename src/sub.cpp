/*******************************************************************************
 * subtool project
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream> 
#include <cctype>
#include <boost/date_time.hpp>
#include "sub.h"

namespace bt = boost::posix_time;

Sub::Sub():
  _timeStamps( new timeStamps_t )
{
}

Sub::~Sub()
{

}

/**
 * @brief Parse a subrip subtitles file and store it in sub, any remaining data is first cleared.
 * @param filename file to parse
 * @return true is successful, false otherwise
 */
bool Sub::Parse( const std::string& filename )
{

  bool containTimeStep = true;
  bool firstText = true;
  bool parsingError = false;
  readingMode mode = NONE;

  // Opening File
  std::ifstream subFile;
  std::string line;
  subFile.open (filename, std::ios::in);
  if(subFile.is_open() )
    {
      // Clearing members
      _timeStamps->clear();
      _texts.clear();

      while( std::getline( subFile, line ) && !parsingError )
        {
          switch(mode)
            {
            case NONE:
                {
                  // Search first non empty line 
                  if( line.empty() )
                    {
                      continue;
                    }

                  // Check it is not a time stamp
                  if( !isdigit(line[0]) )
                    {
                      std::cout<<"Error in Sub::Parse, parsing :"<< line << std::endl << "First non empty line should contain a digit." <<std::endl;
                      parsingError = true;
                      continue;
                    }

                  if( line.find( " --> " ) != std::string::npos )
                    {
                      containTimeStep = false;
                      mode = TIME_STAMP;
                    }
                  else
                    {
                      mode = TIME_STEP;
                    }
                  subFile.seekg(0);
                  continue;
                  break;
                }
            case TIME_STEP:
                {
                  // Search first non empty line 
                  if( line.empty() )
                    {
                      continue;
                    }
                  mode = TIME_STAMP;
                  continue;
                  break;
                }
            case TIME_STAMP:
                {
                  // Search first non empty line 
                  if( line.empty() )
                    {
                      continue;
                    }
                  std::size_t pos =  line.find( " --> " );
                  if( pos == std::string::npos )
                    {
                      std::cout<<"Error in Sub::Parse, parsing :"<< line << std::endl << "Cannot find \" --> \" in line when parsing a time stamp." <<std::endl;
                      parsingError = true;
                      continue;
                    }
                  std::string tStart = line.substr( 0, pos );
                  std::string tEnd   = line.substr( pos+5 );
                  subTime_t start;
                  subTime_t end;
                  if( !SubTimeFromString(tStart,start) || !SubTimeFromString(tEnd,end) )
                    {
                      parsingError = true;
                      continue;
                    }
                  _timeStamps->push_back( std::make_pair( start, end ) );
                  mode = TEXT;
                  continue;
                  break;
                }
            case TEXT:
                {
                  //  empty line -> finished parsing texts
                  if( line.empty() )
                    {
                      if( containTimeStep )
                        {
                          mode = TIME_STEP;
                        }
                      else
                        {
                          mode = TIME_STAMP;
                        }
                      firstText = true;
                      continue;
                    }

                  if( firstText )
                    {
                      _texts.push_back(line);
                      firstText = false;
                    }
                  else
                    {
                      _texts.back().append("\n");
                      _texts.back().append(line);
                    }
                  continue;
                  break;
                }
            default:
                {
                  break;
                }
            }
        }
      subFile.close();

      // Checking sizes
      if( _texts.size() != _timeStamps->size() )
        {
          std::cout<< "Error in Sub::Parse, texts and timeStamps are not the same size."<<std::endl;
          return false;
        }

      // Checking parsing
      if( parsingError )
        {
          return false;
        }
    }
  else
    {
      std::cout<< "Error in Sub::Parse, " << filename << " cannot be opened."<<std::endl;
      return false;
    }
  if( _texts.size() == 0 )
    {
      std::cout<< "Warning in Sub::Parse, Parsing " << filename << " produced a empty sub."<<std::endl;
    }
  return true;
}

/**
 * @brief Write the sub to a file
 * @param filename file to (over) write
 * @return true is successful, false otherwise
 */
bool Sub::Write( const std::string& filename )
{
  if( _texts.size() == 0 )
    {
      std::cout<< "Warning in Sub::Write, Writing an empty sub."<<std::endl;
    }

  std::ofstream subFile;
  subFile.open (filename, std::ios::out | std::ios::trunc );
  for(unsigned int i = 0; i < _timeStamps->size(); i++ )
    {
      subFile << i+1 <<"\n";
      subFile << SubTimeToString(_timeStamps->at(i).first) << " --> " << SubTimeToString(_timeStamps->at(i).second) <<"\n";
      subFile << _texts[i] <<"\n";
      subFile << "\n";
    }
  subFile.close();
  return true;
}

/**
 * @brief Resync this sub with timing of another sub, using Levenshtein Distance to guess which sub texts goes where
 * @param timeSourceSub
 * @return true is successful, false otherwise
 */
bool Sub::Resync( const Sub& timeSourceSub )
{
  // Find three sub texts wich a good distance
  similarIdx_t similars;
  unsigned int tmp;
  for( unsigned int i = 0; i < _texts.size(); i++)
    {
      if( timeSourceSub.FindSimilar( _texts[i], i*100/_texts.size(), tmp ) )
        {
          similars.push_back(std::make_pair(i,tmp));
        }
    }
  if( similars.size() < SUB_MIN_SIMILARS )
    {
      std::cout<<"Error in Sub::iResync : Cannot find enough similar texts"<<std::endl;
      return false;
    }
  return true;
}

/**
 * @brief try to find a text similar to the one provided in param
 * @param text text to look for similar
 * @param aroundPercent index in percent of total sub to look around for text
 * @param[out] index of similar sub text
 * @return true if successfull, false if cannot find similar text
 */
bool Sub::FindSimilar( const std::string& text, unsigned int aroundPercent, unsigned int & similarIndex ) const
{
  unsigned int aroundBase = _texts.size()*aroundPercent/100;
  if( aroundBase > SUB_AROUND_DELTA )
    {
      aroundBase -= SUB_AROUND_DELTA;
    }
  else
    {
      aroundBase = 0;
    }

  for( unsigned int i = 0; ( i < SUB_AROUND_DELTA*2 ) && ( i+aroundBase < _texts.size() ); i++ )
    {
      std::cout<< LevenshteinDistance( text, _texts.at(i+aroundBase) ) << std::endl;
      if( LevenshteinDistance( text, _texts.at(i+aroundBase) ) < text.size() * SUB_DIST_RATE )
        {
          similarIndex = i+aroundBase;
          std::cout<<"---------------"<<std::endl;
          std::cout<< text<<std::endl;
          std::cout<<"============"<<std::endl;
          std::cout<< _texts.at(i+aroundBase) <<std::endl;
          std::cout<<"---------------"<<std::endl;
          return true;
        }
    }
  std::cout<<"Error in Sub::FindSimilar : Cannot find similar text to :\""<< text <<"\""<<std::endl;
  return false;
}

/**
 * @brief Parse a string containing a sub time and  convert it to subTime_t
 * @param stringTime string containing a sub time
 * @param[out] subTime sub time output
 * @return true is successful, false otherwise
 */
bool Sub::SubTimeFromString( const std::string & stringTime, subTime_t & subTime )
{
  std::size_t pos1 = std::string::npos;
  std::size_t pos2 = std::string::npos;
  std::size_t pos3 = std::string::npos;
  for( std::size_t i = 0; i < stringTime.length(); i++ )
    {
      if( pos1 == std::string::npos )
        {
          if( stringTime[i] == ':' )
            {
              pos1 = i;
            }
        }
      else if( pos2 == std::string::npos )
        {
          if( stringTime[i] == ':' )
            {
              pos2 = i;
            }
        }     
      else if( pos3 == std::string::npos )
        {
          if( stringTime[i] == ',' )
            {
              pos3 = i;
              break;
            }
        }    
    }
  if( pos3 == std::string::npos )
    {
      std::cout<<"Error in Sub::SubTimeFromString, parsing :"<< stringTime << std::endl;
      return false;
    }

  unsigned long hours = strtoul( stringTime.substr(0, pos1).c_str(), NULL, 10 );
  unsigned long minutes = strtoul( stringTime.substr(pos1+1, pos2-pos1-1).c_str(), NULL, 10 );
  unsigned long seconds = strtoul( stringTime.substr(pos2+1, pos3-pos2-1).c_str(), NULL, 10 );
  unsigned long ms = strtoul( stringTime.substr(pos3+1, std::string::npos).c_str(), NULL, 10 );
  subTime = ms + 1000 * ( seconds + 60 * ( minutes + 60 * hours ) );
  return true;
}

/**
 * @brief Convert a subTime_t to a string
 * @param stringTime string containing a sub time
 * @param[out] subTime sub time output
 * @return true is successful, false otherwise
 */
std::string Sub::SubTimeToString( const subTime_t & subTime )
{
  unsigned long ms = subTime % 1000;
  unsigned long seconds = (subTime/1000);
  unsigned long minutes = (subTime/(1000*60));
  unsigned long hours = (subTime/(60*60*1000));
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(2) << hours << ":" << std::setw(2) << minutes << ":" << std::setw(2) <<seconds << ","<< std::setw(3) << ms;
  return ss.str();
}


/**
 * Compute non-case sentisitve lenvenshtein distance computation between two strings
 * @param s1 first string to compare
 * @param s2 second string to compare
 * @return lenvenshtein distance
 */
size_t Sub::LevenshteinDistance(const std::string &s1, const std::string &s2)
{
  const size_t m(s1.size());
  const size_t n(s2.size());

  if( m==0 ) return n;
  if( n==0 ) return m;

  size_t *costs = new size_t[n + 1];

  for( size_t k=0; k<=n; k++ ) costs[k] = k;

  size_t i = 0;
  for ( std::string::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1, ++i )
    {
      costs[0] = i+1;
      size_t corner = i;

      size_t j = 0;
      for ( std::string::const_iterator it2 = s2.begin(); it2 != s2.end(); ++it2, ++j )
        {
          size_t upper = costs[j+1];
          // non-case sensitive UTF8 string equality test
          if( *it1 == *it2 || toupper(*it1) == toupper(*it2) )
            {
              costs[j+1] = corner;
            }
          else
            {
              size_t t(upper<corner?upper:corner);
              costs[j+1] = (costs[j]<t?costs[j]:t)+1;
            }

          corner = upper;
        }
    }

  size_t result = costs[n];
  delete [] costs;

  return result;
}
