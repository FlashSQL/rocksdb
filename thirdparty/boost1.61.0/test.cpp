
  /* The following is a simple example that shows conversion of dates 
   * to and from a std::string.
   * 
   * Expected output:
   * 2001-Oct-09
   * 2001-10-09
   * Tuesday October 9, 2001
   * An expected exception is next: 
   * Exception: Month number is out of range 1..12
   */

  #include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/tokenizer.hpp"
  #include <iostream>
  #include <string>

  int
  main() 
  {

    using namespace boost::gregorian;
	using namespace boost::posix_time;
	using namespace boost;

	ptime now = second_clock::local_time();
    //Get the date part out of the time
	ptime t(now.date());
	char_separator <char> sep(",T");
	tokenizer<char_separator<char> > tokens(to_iso_string(t), sep);

	std::string gg;
	int ii = 0 ;
	for (const std::string &t: tokens) {
		if (ii++ > 1 ) break;
		gg += t;
	}
	std::cout << "djdj: " << gg << std::endl;
	for(const std::string& t: tokens)
		std::cout<< "to: " << t << std::endl;

    try {
      // The following date is in ISO 8601 extended format (CCYY-MM-DD)
      std::string s("2001-10-9"); //2001-October-09
      date d(from_simple_string(s));
      std::cout << to_simple_string(d) << std::endl;
      
      //Read ISO Standard(CCYYMMDD) and output ISO Extended
      std::string ud("20011009"); //2001-Oct-09
      date d1(from_undelimited_string(ud));
      std::cout << to_iso_extended_string(d1) << std::endl;
      
      //Output the parts of the date - Tuesday October 9, 2001
      date::ymd_type ymd = d1.year_month_day();
      greg_weekday wd = d1.day_of_week();
      std::cout << wd.as_long_string() << " "
                << ymd.month.as_long_string() << " "
                << ymd.day << ", " << ymd.year
                << std::endl;

      //Let's send in month 25 by accident and create an exception
      std::string bad_date("20012509"); //2001-??-09
      std::cout << "An expected exception is next: " << std::endl;
      date wont_construct(from_undelimited_string(bad_date));
      //use wont_construct so compiler doesn't complain, but you wont get here!
      std::cout << "oh oh, you shouldn't reach this line: " 
                << to_iso_string(wont_construct) << std::endl;
    }
    catch(std::exception& e) {
      std::cout << "  Exception: " <<  e.what() << std::endl;
    }


    return 0;
  }
