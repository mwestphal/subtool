/*******************************************************************************
 * subtool project
 ******************************************************************************/

#ifndef SUB_H
#define SUB_H

#include <vector>
#include <utility> 
#include <ctime>
#include <string>
#include <memory>
/**
 * @class sub
 */
class Sub {
  // CONSTANTS
  //---------------------------------------------------------------------------------
  const unsigned int SUB_MIN_SIMILARS = 3; 
  const double SUB_DIST_RATE = 0.6; 
  const unsigned int SUB_AROUND_DELTA = 10; 

  // NESTED ENUMS & TYPES
  //---------------------------------------------------------------------------------
  typedef uint32_t subTime_t;
  typedef std::vector < std::pair< subTime_t, subTime_t > > timeStamps_t;
  typedef std::vector < std::pair< unsigned int, unsigned int> > similarIdx_t;
  typedef std::vector< std::string > texts_t;
  
  typedef enum readingMode {
    NONE,
    TIME_STEP,
    TIME_STAMP,
    TEXT
  } readingMode;

  // CONSTRUCTORS
  //---------------------------------------------------------------------------------

public:
  Sub();
  ~Sub();

private:
  /** @brief disabled copy constructor
  */
  Sub(const Sub&);

  // METHODS
  //---------------------------------------------------------------------------------
public:

  virtual bool Parse(const std::string & filename);
  virtual bool Write(const std::string & filename);
  virtual bool Resync( const Sub & timeSourceSub );

protected:

  bool FindSimilar( const std::string& text, unsigned int aroundPercent, unsigned int & similarIndex ) const;

  static size_t LevenshteinDistance(const std::string &s1, const std::string &s2);
  static bool SubTimeFromString( const std::string & stringTime, subTime_t & subTime );
  static std::string SubTimeToString( const subTime_t & subTime );

  // OPERATORS
  //---------------------------------------------------------------------------------

  // ACCESSORS
  //---------------------------------------------------------------------------------

  // ATTRIBUTES
  //---------------------------------------------------------------------------------
protected:
  std::unique_ptr< timeStamps_t > _timeStamps;
  texts_t _texts;
};

#endif //Sub_H
