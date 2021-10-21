// read the config file
// setup the clients and server accordingly
#include "server.hpp"
#include <iostream>

int main()
{
    // WRITE PARSER
    
    // sample received string
    string s, key, method;

    Server sr;
    sr.handle_put("5", "500");
    sr.handle_put("4", "400");
    sr.handle_put("3", "300");
    sr.handle_put("2", "200");

    sr.handle_delete("3");
    sr.handle_put("2", "100");
    sr.handle_delete("2");
    sr.handle_put("2", "200");

    sr.handle_delete("4");
    sr.handle_get("4");
    sr.handle_get("5");
    sr.handle_get("5");
    sr.handle_delete("5");
    sr.handle_put("5", "0"); 
    sr.handle_get("5");
    sr.handle_get("5");
    sr.handle_delete("13");

    // for(auto i=sr.table.begin(); i!=sr.table.end(); i++)
    //     std::cout<<i->first<<" "<<i->second<<endl;

    return 0;
}
